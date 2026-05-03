#include "database.hpp"

#include <sqlite3.h>
#include <stdexcept>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// RAII wrapper that finalises a prepared statement on scope exit.
struct StmtGuard {
    sqlite3_stmt* stmt{nullptr};
    ~StmtGuard() { if (stmt) sqlite3_finalize(stmt); }
};

Overlay row_to_overlay(sqlite3_stmt* stmt)
{
    Overlay o;
    o.id          = sqlite3_column_int (stmt, 0);
    o.name        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    o.source_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    o.x           = sqlite3_column_int (stmt, 3);
    o.y           = sqlite3_column_int (stmt, 4);
    o.width       = sqlite3_column_int (stmt, 5);
    o.height      = sqlite3_column_int (stmt, 6);
    o.active      = sqlite3_column_int (stmt, 7) != 0;
    const unsigned char* ts = sqlite3_column_text(stmt, 8);
    if (ts) o.created_at = reinterpret_cast<const char*>(ts);
    return o;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Database
// ---------------------------------------------------------------------------

Database::Database(const std::string& db_path)
{
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = "Cannot open database '";
        msg += db_path;
        msg += "': ";
        msg += sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error(msg);
    }
    init_schema();
}

Database::~Database()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::init_schema()
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS overlays ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name        TEXT    NOT NULL,"
        "  source_path TEXT    NOT NULL,"
        "  x           INTEGER NOT NULL DEFAULT 0,"
        "  y           INTEGER NOT NULL DEFAULT 0,"
        "  width       INTEGER NOT NULL DEFAULT 0,"
        "  height      INTEGER NOT NULL DEFAULT 0,"
        "  active      INTEGER NOT NULL DEFAULT 0,"
        "  created_at  TEXT    NOT NULL DEFAULT (datetime('now'))"
        ");";

    char* errmsg = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg) != SQLITE_OK) {
        std::string msg = "Failed to initialise schema: ";
        msg += errmsg;
        sqlite3_free(errmsg);
        throw std::runtime_error(msg);
    }
}

bool Database::create_overlay(const std::string& name,
                               const std::string& source_path,
                               int x, int y, int width, int height)
{
    const char* sql =
        "INSERT INTO overlays (name, source_path, x, y, width, height) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    StmtGuard g;
    if (sqlite3_prepare_v2(db_, sql, -1, &g.stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(g.stmt, 1, name.c_str(),        -1, SQLITE_STATIC);
    sqlite3_bind_text(g.stmt, 2, source_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int (g.stmt, 3, x);
    sqlite3_bind_int (g.stmt, 4, y);
    sqlite3_bind_int (g.stmt, 5, width);
    sqlite3_bind_int (g.stmt, 6, height);

    return sqlite3_step(g.stmt) == SQLITE_DONE;
}

std::vector<Overlay> Database::list_overlays() const
{
    const char* sql =
        "SELECT id, name, source_path, x, y, width, height, active, created_at "
        "FROM overlays ORDER BY id;";

    StmtGuard g;
    std::vector<Overlay> result;
    if (sqlite3_prepare_v2(db_, sql, -1, &g.stmt, nullptr) != SQLITE_OK)
        return result;

    while (sqlite3_step(g.stmt) == SQLITE_ROW)
        result.push_back(row_to_overlay(g.stmt));

    return result;
}

std::optional<Overlay> Database::get_overlay(int id) const
{
    const char* sql =
        "SELECT id, name, source_path, x, y, width, height, active, created_at "
        "FROM overlays WHERE id = ?;";

    StmtGuard g;
    if (sqlite3_prepare_v2(db_, sql, -1, &g.stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    sqlite3_bind_int(g.stmt, 1, id);

    if (sqlite3_step(g.stmt) == SQLITE_ROW)
        return row_to_overlay(g.stmt);

    return std::nullopt;
}

bool Database::update_overlay_active(int id, bool active)
{
    const char* sql = "UPDATE overlays SET active = ? WHERE id = ?;";

    StmtGuard g;
    if (sqlite3_prepare_v2(db_, sql, -1, &g.stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(g.stmt, 1, active ? 1 : 0);
    sqlite3_bind_int(g.stmt, 2, id);

    return sqlite3_step(g.stmt) == SQLITE_DONE;
}

bool Database::delete_overlay(int id)
{
    const char* sql = "DELETE FROM overlays WHERE id = ?;";

    StmtGuard g;
    if (sqlite3_prepare_v2(db_, sql, -1, &g.stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(g.stmt, 1, id);

    return sqlite3_step(g.stmt) == SQLITE_DONE;
}
