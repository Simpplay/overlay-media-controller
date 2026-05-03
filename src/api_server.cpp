#include "api_server.hpp"

#include "database.hpp"
#include "media_controller.hpp"

#include <httplib.h>

#include <sstream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// Minimal JSON helpers (no extra dependency required)
// ---------------------------------------------------------------------------

namespace json {

static std::string escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

static std::string overlay_to_json(const Overlay& o)
{
    std::ostringstream ss;
    ss << "{"
       << "\"id\":"          << o.id                      << ","
       << "\"name\":\""      << escape(o.name)            << "\","
       << "\"source_path\":\"" << escape(o.source_path)   << "\","
       << "\"x\":"           << o.x                       << ","
       << "\"y\":"           << o.y                       << ","
       << "\"width\":"       << o.width                   << ","
       << "\"height\":"      << o.height                  << ","
       << "\"active\":"      << (o.active ? "true" : "false") << ","
       << "\"created_at\":\"" << escape(o.created_at)    << "\""
       << "}";
    return ss.str();
}

static std::string overlays_to_json(const std::vector<Overlay>& overlays)
{
    std::string out = "[";
    for (std::size_t i = 0; i < overlays.size(); ++i) {
        if (i) out += ",";
        out += overlay_to_json(overlays[i]);
    }
    out += "]";
    return out;
}

static std::string mediainfo_to_json(const MediaInfo& m)
{
    std::ostringstream ss;
    ss << "{"
       << "\"width\":"       << m.width                   << ","
       << "\"height\":"      << m.height                  << ","
       << "\"duration\":"    << m.duration                << ","
       << "\"codec_name\":\"" << escape(m.codec_name)     << "\","
       << "\"format_name\":\"" << escape(m.format_name)   << "\""
       << "}";
    return ss.str();
}

/// Extract a string value for @p key from a flat JSON object string.
/// Returns empty string if not found.
static std::string get_string(const std::string& body, const std::string& key)
{
    auto pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return {};
    pos = body.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = body.find('"', pos);
    if (pos == std::string::npos) return {};
    ++pos;
    auto end = body.find('"', pos);
    if (end == std::string::npos) return {};
    return body.substr(pos, end - pos);
}

/// Extract an integer value for @p key from a flat JSON object string.
static int get_int(const std::string& body, const std::string& key, int def = 0)
{
    auto pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return def;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return def;
    ++pos;
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) ++pos;
    if (pos >= body.size()) return def;
    try { return std::stoi(body.substr(pos)); } catch (...) { return def; }
}

/// Extract a boolean value for @p key from a flat JSON object string.
static bool get_bool(const std::string& body, const std::string& key, bool def = false)
{
    auto pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return def;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return def;
    ++pos;
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) ++pos;
    if (pos >= body.size()) return def;
    return body.substr(pos, 4) == "true";
}

} // namespace json

/// Parse an integer ID from a regex match string.
/// Returns -1 and sets a 400 Bad Request response on failure.
static int parse_id(const std::string& s, httplib::Response& res)
{
    try {
        return std::stoi(s);
    } catch (...) {
        res.status = 400;
        res.set_content("{\"error\":\"Invalid ID\"}", "application/json");
        return -1;
    }
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct ApiServer::Impl {
    Database&        db;
    MediaController& media;
    int              port;
    httplib::Server  svr;

    Impl(Database& db_, MediaController& media_, int port_)
        : db(db_), media(media_), port(port_)
    {
        register_routes();
    }

    void register_routes()
    {
        // Health check
        svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("{\"status\":\"ok\"}", "application/json");
        });

        // List all overlays
        svr.Get("/api/overlays", [this](const httplib::Request&,
                                        httplib::Response& res) {
            auto overlays = db.list_overlays();
            res.set_content(json::overlays_to_json(overlays), "application/json");
        });

        // Create overlay
        // Body: { "name": "...", "source_path": "...", "x": 0, "y": 0,
        //         "width": 1920, "height": 1080 }
        svr.Post("/api/overlays", [this](const httplib::Request& req,
                                          httplib::Response& res) {
            const std::string& body = req.body;
            std::string name    = json::get_string(body, "name");
            std::string source  = json::get_string(body, "source_path");
            int x      = json::get_int(body, "x");
            int y      = json::get_int(body, "y");
            int width  = json::get_int(body, "width");
            int height = json::get_int(body, "height");

            if (name.empty() || source.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"'name' and 'source_path' are required\"}",
                                "application/json");
                return;
            }

            if (!db.create_overlay(name, source, x, y, width, height)) {
                res.status = 500;
                res.set_content("{\"error\":\"Failed to create overlay\"}", "application/json");
                return;
            }

            res.status = 201;
            res.set_content("{\"message\":\"Overlay created\"}", "application/json");
        });

        // Get single overlay
        svr.Get(R"(/api/overlays/(\d+))", [this](const httplib::Request& req,
                                                   httplib::Response& res) {
            int id = parse_id(req.matches[1], res);
            if (id < 0) return;
            auto overlay = db.get_overlay(id);
            if (!overlay) {
                res.status = 404;
                res.set_content("{\"error\":\"Overlay not found\"}", "application/json");
                return;
            }
            res.set_content(json::overlay_to_json(*overlay), "application/json");
        });

        // Delete overlay
        svr.Delete(R"(/api/overlays/(\d+))", [this](const httplib::Request& req,
                                                      httplib::Response& res) {
            int id = parse_id(req.matches[1], res);
            if (id < 0) return;
            if (!db.delete_overlay(id)) {
                res.status = 404;
                res.set_content("{\"error\":\"Overlay not found or already deleted\"}",
                                "application/json");
                return;
            }
            res.set_content("{\"message\":\"Overlay deleted\"}", "application/json");
        });

        // Set active flag
        // Body: { "active": true }
        svr.Put(R"(/api/overlays/(\d+)/active)", [this](const httplib::Request& req,
                                                          httplib::Response& res) {
            int id = parse_id(req.matches[1], res);
            if (id < 0) return;
            bool active = json::get_bool(req.body, "active");

            if (!db.get_overlay(id)) {
                res.status = 404;
                res.set_content("{\"error\":\"Overlay not found\"}", "application/json");
                return;
            }

            if (!db.update_overlay_active(id, active)) {
                res.status = 500;
                res.set_content("{\"error\":\"Failed to update overlay\"}", "application/json");
                return;
            }

            res.set_content("{\"message\":\"Overlay updated\"}", "application/json");
        });

        // Probe media source of an overlay
        svr.Get(R"(/api/overlays/(\d+)/probe)", [this](const httplib::Request& req,
                                                         httplib::Response& res) {
            int id = parse_id(req.matches[1], res);
            if (id < 0) return;
            auto overlay = db.get_overlay(id);
            if (!overlay) {
                res.status = 404;
                res.set_content("{\"error\":\"Overlay not found\"}", "application/json");
                return;
            }

            try {
                MediaInfo info = media.probe(overlay->source_path);
                res.set_content(json::mediainfo_to_json(info), "application/json");
            } catch (const std::exception& e) {
                res.status = 422;
                std::string err = "{\"error\":\"";
                err += json::escape(e.what());
                err += "\"}";
                res.set_content(err, "application/json");
            }
        });
    }
};

// ---------------------------------------------------------------------------
// ApiServer
// ---------------------------------------------------------------------------

ApiServer::ApiServer(Database& db, MediaController& media, int port)
    : impl_(std::make_unique<Impl>(db, media, port))
{}

ApiServer::~ApiServer() = default;

void ApiServer::run()
{
    impl_->svr.listen("0.0.0.0", impl_->port);
}

void ApiServer::stop()
{
    impl_->svr.stop();
}
