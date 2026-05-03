#pragma once

#include <string>
#include <vector>
#include <optional>

// Forward-declare the opaque sqlite3 handle so callers do not need sqlite3.h.
struct sqlite3;

struct Overlay {
    int         id{0};
    std::string name;
    std::string source_path;
    int         x{0};
    int         y{0};
    int         width{0};
    int         height{0};
    bool        active{false};
    std::string created_at;
};

class Database {
public:
    /// Open (or create) the SQLite database at @p db_path.
    explicit Database(const std::string& db_path);
    ~Database();

    // Non-copyable
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    /// Insert a new overlay record.  Returns true on success.
    bool create_overlay(const std::string& name,
                        const std::string& source_path,
                        int x, int y, int width, int height);

    /// Return all stored overlays.
    std::vector<Overlay> list_overlays() const;

    /// Return a single overlay by @p id, or nullopt if not found.
    std::optional<Overlay> get_overlay(int id) const;

    /// Set the active flag of an overlay.  Returns true on success.
    bool update_overlay_active(int id, bool active);

    /// Delete an overlay by @p id.  Returns true on success.
    bool delete_overlay(int id);

private:
    sqlite3* db_{nullptr};

    void init_schema();
};
