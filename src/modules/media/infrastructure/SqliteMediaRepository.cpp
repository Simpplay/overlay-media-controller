#include "SqliteMediaRepository.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>

#include <iostream>

namespace omc::media
{
	// ---------------------------------------------------------------------------
	// Helpers
	// ---------------------------------------------------------------------------

	static std::vector<CategoryPreview> fetchCategoriesForMedia(sqlite3* db, int mediaId)
	{
		std::vector<CategoryPreview> cats;

		const char* sql = R"(
        SELECT c.id, c.name
        FROM categories c
        INNER JOIN media_categories mc ON mc.category_id = c.id
        WHERE mc.media_id = ?;
    )";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return cats;
		}

		sqlite3_bind_int(stmt, 1, mediaId);

		while (sqlite3_step(stmt) == SQLITE_ROW) {

			CategoryPreview cat;

			cat.id = sqlite3_column_int(stmt, 0);

			const auto name = sqlite3_column_text(stmt, 1);

			if (name) {
				cat.name = reinterpret_cast<const char*>(name);
			}

			cats.push_back(std::move(cat));
		}

		sqlite3_finalize(stmt);

		return cats;
	}

	static std::vector<MediaPreview> fetchMediaForCategory(sqlite3* db, int categoryId)
	{
		std::vector<MediaPreview> mediaList;

		const char* sql = R"(
        SELECT
            m.id,
            m.title,
            m.filename,
            m.contentType
        FROM media m
        INNER JOIN media_categories mc
            ON mc.media_id = m.id
        WHERE mc.category_id = ?;
    )";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return mediaList;
		}

		sqlite3_bind_int(stmt, 1, categoryId);

		while (sqlite3_step(stmt) == SQLITE_ROW) {

			MediaPreview media;

			media.id = sqlite3_column_int(stmt, 0);

			const auto title = sqlite3_column_text(stmt, 1);
			if (title) {
				media.title = reinterpret_cast<const char*>(title);
			}

			const auto filename = sqlite3_column_text(stmt, 2);
			if (filename) {
				media.filename = reinterpret_cast<const char*>(filename);
			}

			const auto contentType = sqlite3_column_text(stmt, 3);
			if (contentType) {
				media.contentType = reinterpret_cast<const char*>(contentType);
			}

			mediaList.push_back(std::move(media));
		}

		sqlite3_finalize(stmt);

		return mediaList;
	}

	// ---------------------------------------------------------------------------
	// Media
	// ---------------------------------------------------------------------------

	Media SqliteMediaRepository::getMediaById(int id)
	{
		if (!db_) {
			return Media();
		}

		const char* sql = R"(
			SELECT
				id,
				title,
				filename,
				filepath,
				contentType
			FROM media
			WHERE id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return Media();
		}

		sqlite3_bind_int(stmt, 1, id);

		Media media;

		if (sqlite3_step(stmt) == SQLITE_ROW) {

			media.id = sqlite3_column_int(stmt, 0);

			const auto title = sqlite3_column_text(stmt, 1);
			if (title) {
				media.title = reinterpret_cast<const char*>(title);
			}

			const auto filename = sqlite3_column_text(stmt, 2);
			if (filename) {
				media.filename = reinterpret_cast<const char*>(filename);
			}

			const auto filepath = sqlite3_column_text(stmt, 3);
			if (filepath) {
				media.filepath = reinterpret_cast<const char*>(filepath);
			}

			const auto contentType = sqlite3_column_text(stmt, 4);
			if (contentType) {
				media.contentType = reinterpret_cast<const char*>(contentType);
			}

			if (std::filesystem::exists(media.filepath)) {
				media.size = std::filesystem::file_size(media.filepath);
			}

			media.categories = fetchCategoriesForMedia(db_, media.id);
		}

		sqlite3_finalize(stmt);

		return media;
	}

	std::vector<Media> SqliteMediaRepository::getAllMedia()
	{
		std::vector<Media> result;

		if (!db_) {
			return result;
		}

		const char* sql = R"(
			SELECT
				id,
				title,
				filename,
				filepath,
				contentType
			FROM media;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return result;
		}

		while (sqlite3_step(stmt) == SQLITE_ROW) {

			Media media;

			media.id = sqlite3_column_int(stmt, 0);

			const unsigned char* titleText = sqlite3_column_text(stmt, 1);
			if (titleText) {
				media.title = reinterpret_cast<const char*>(titleText);
			}

			const unsigned char* filenameText = sqlite3_column_text(stmt, 2);
			if (filenameText) {
				media.filename = reinterpret_cast<const char*>(filenameText);
			}

			const unsigned char* filepathText = sqlite3_column_text(stmt, 3);
			if (filepathText) {
				media.filepath = reinterpret_cast<const char*>(filepathText);
			}

			const unsigned char* contentTypeText = sqlite3_column_text(stmt, 4);
			if (contentTypeText) {
				media.contentType = reinterpret_cast<const char*>(contentTypeText);
			}

			if (std::filesystem::exists(media.filepath)) {
				media.size = std::filesystem::file_size(media.filepath);
			}

			media.categories = fetchCategoriesForMedia(db_, media.id);

			result.push_back(std::move(media));
		}

		sqlite3_finalize(stmt);

		return result;
	}

	Media SqliteMediaRepository::addMedia(
		const std::vector<std::byte>& data,
		const std::string& filename,
		const std::string& contentType)
	{
		if (!db_) {
			return Media();
		}

		auto timestamp =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch())
			.count();

		std::string storedFilename =
			std::to_string(timestamp) + "_" + filename;

		auto fullPath = mediaRoot_ / storedFilename;

		std::cout << "Storing media file: " << fullPath << "\n";

		{
			std::ofstream file(fullPath, std::ios::binary);

			if (!file.is_open()) {
				std::cout << "Failed to open file for writing: " << fullPath << "\n";
				return Media();
			}

			file.write(
				reinterpret_cast<const char*>(data.data()),
				static_cast<std::streamsize>(data.size()));

			if (!file.good()) {
				std::cout << "Failed to write data to file: " << fullPath << "\n";
				return Media();
			}
		}

		const char* sql = R"(
			INSERT INTO media (
				filename,
				filepath,
				contentType
			)
			VALUES (?, ?, ?);
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			std::filesystem::remove(fullPath);
			std::cout << "Failed to prepare SQL statement: " << sqlite3_errmsg(db_) << "\n";
			return Media();
		}

		sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, fullPath.string().c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, contentType.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			std::filesystem::remove(fullPath);
			return Media();
		}

		sqlite3_finalize(stmt);

		Media media;
		media.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
		media.filename = filename;
		media.filepath = fullPath.string();
		media.contentType = contentType;
		media.size = data.size();

		return media;
	}

	bool SqliteMediaRepository::deleteMediaById(int id)
	{
		auto media = getMediaById(id);

		if (media.id == 0) {
			return false;
		}

		if (!db_) {
			return false;
		}

		const char* sql = R"(
			DELETE FROM media
			WHERE id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return false;
		}

		sqlite3_bind_int(stmt, 1, id);

		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);
		const int changes = sqlite3_changes(db_);
		sqlite3_finalize(stmt);

		if (!success || changes == 0) {
			return false;
		}

		if (std::filesystem::exists(media.filepath)) {
			std::filesystem::remove(media.filepath);
		}

		return true;
	}

	std::vector<Media> SqliteMediaRepository::getMediaByCategoryId(int categoryId)
	{
		std::vector<Media> result;

		if (!db_) {
			return result;
		}

		const char* sql = R"(
			SELECT
				m.id,
				m.title,
				m.filename,
				m.filepath,
				m.contentType
			FROM media m
			INNER JOIN media_categories mc ON mc.media_id = m.id
			WHERE mc.category_id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return result;
		}

		sqlite3_bind_int(stmt, 1, categoryId);

		while (sqlite3_step(stmt) == SQLITE_ROW) {

			Media media;

			media.id = sqlite3_column_int(stmt, 0);

			const unsigned char* titleText = sqlite3_column_text(stmt, 1);
			if (titleText) {
				media.title = reinterpret_cast<const char*>(titleText);
			}

			const unsigned char* filenameText = sqlite3_column_text(stmt, 2);
			if (filenameText) {
				media.filename = reinterpret_cast<const char*>(filenameText);
			}

			const unsigned char* filepathText = sqlite3_column_text(stmt, 3);
			if (filepathText) {
				media.filepath = reinterpret_cast<const char*>(filepathText);
			}

			const unsigned char* contentTypeText = sqlite3_column_text(stmt, 4);
			if (contentTypeText) {
				media.contentType = reinterpret_cast<const char*>(contentTypeText);
			}

			if (std::filesystem::exists(media.filepath)) {
				media.size = std::filesystem::file_size(media.filepath);
			}

			media.categories = fetchCategoriesForMedia(db_, media.id);

			result.push_back(std::move(media));
		}

		sqlite3_finalize(stmt);

		return result;
	}

	// ---------------------------------------------------------------------------
	// Categories
	// ---------------------------------------------------------------------------

	std::vector<Category> SqliteMediaRepository::getAllCategories()
	{
		std::vector<Category> result;

		if (!db_) {
			return result;
		}

		const char* sql = R"(
			SELECT id, name FROM categories;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return result;
		}

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			Category cat;
			cat.id = sqlite3_column_int(stmt, 0);
			const auto name = sqlite3_column_text(stmt, 1);
			if (name) {
				cat.name = reinterpret_cast<const char*>(name);
			}
			result.push_back(std::move(cat));
		}

		sqlite3_finalize(stmt);

		return result;
	}

	std::optional<Category> SqliteMediaRepository::getCategoryById(int id)
	{
		if (!db_) {
			return std::nullopt;
		}

		const char* sql = R"(
        SELECT id, name
        FROM categories
        WHERE id = ?;
    )";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return std::nullopt;
		}

		sqlite3_bind_int(stmt, 1, id);

		if (sqlite3_step(stmt) != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			return std::nullopt;
		}

		Category cat;

		cat.id = sqlite3_column_int(stmt, 0);

		const auto name = sqlite3_column_text(stmt, 1);
		if (name) {
			cat.name = reinterpret_cast<const char*>(name);
		}

		sqlite3_finalize(stmt);

		cat.media = fetchMediaForCategory(db_, cat.id);

		return cat;
	}

	bool SqliteMediaRepository::createCategory(const std::string& name)
	{
		if (!db_) {
			return false;
		}

		const char* sql = R"(
			INSERT INTO categories (name) VALUES (?);
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return false;
		}

		sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);
		sqlite3_finalize(stmt);

		return success;
	}

	bool SqliteMediaRepository::deleteCategoryById(int id)
	{
		if (!db_) {
			return false;
		}

		// ON DELETE CASCADE removes rows from media_categories automatically.
		const char* sql = R"(
			DELETE FROM categories WHERE id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return false;
		}

		sqlite3_bind_int(stmt, 1, id);

		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);
		const int changes = sqlite3_changes(db_);
		sqlite3_finalize(stmt);

		return success && changes > 0;
	}

	bool SqliteMediaRepository::addMediaToCategory(int mediaId, int categoryId)
	{
		if (!db_) {
			return false;
		}

		if (getMediaById(mediaId).id == 0) {
			return false;
		}

		auto category = getCategoryById(categoryId);
		if (!category.has_value()) {
			return false;
		}

		sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		const char* sql = R"(
        INSERT OR IGNORE INTO media_categories (
            media_id,
            category_id
        )
        VALUES (?, ?);
    )";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
			return false;
		}

		sqlite3_bind_int(stmt, 1, mediaId);
		sqlite3_bind_int(stmt, 2, categoryId);

		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);

		sqlite3_finalize(stmt);

		if (!success) {
			sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
			return false;
		}

		sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);

		return true;
	}

	bool SqliteMediaRepository::removeMediaFromCategory(int mediaId, int categoryId)
	{
		if (!db_) {
			return false;
		}

		sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

		const char* sql = R"(
        DELETE FROM media_categories
        WHERE media_id = ?
          AND category_id = ?;
    )";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
			return false;
		}

		sqlite3_bind_int(stmt, 1, mediaId);
		sqlite3_bind_int(stmt, 2, categoryId);

		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);
		const int changes = sqlite3_changes(db_);

		sqlite3_finalize(stmt);

		if (!success || changes == 0) {
			sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
			return false;
		}

		sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);

		return true;
	}

	// ---------------------------------------------------------------------------
	// Database lifecycle
	// ---------------------------------------------------------------------------

	bool SqliteMediaRepository::setupDatabase(const std::string& dbPath, std::string& out)
	{
		int rc = sqlite3_open(dbPath.c_str(), &db_);
		if (rc != SQLITE_OK) {
			out = sqlite3_errmsg(db_);
			sqlite3_close(db_);
			db_ = nullptr;
			return false;
		}

		if (!initializeDatabase(out)) {
			closeDatabase();
			return false;
		}

		dbPath_ = dbPath;
		return true;
	}

	void SqliteMediaRepository::closeDatabase()
	{
		if (db_) {
			sqlite3_close(db_);
			db_ = nullptr;
		}
	}

	bool SqliteMediaRepository::initializeDatabase(std::string& out)
	{
		// Enable foreign-key enforcement for this connection.
		if (sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
			out = "Failed to enable foreign keys";
			return false;
		}

		const char* ddl = R"(
			CREATE TABLE IF NOT EXISTS media (
				id          INTEGER PRIMARY KEY AUTOINCREMENT,
				title       TEXT,
				filename    TEXT,
				filepath    TEXT NOT NULL,
				contentType TEXT
			);

			CREATE TABLE IF NOT EXISTS categories (
				id   INTEGER PRIMARY KEY AUTOINCREMENT,
				name TEXT NOT NULL UNIQUE
			);

			CREATE TABLE IF NOT EXISTS media_categories (
				media_id    INTEGER NOT NULL REFERENCES media(id)      ON DELETE CASCADE,
				category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE CASCADE,
				PRIMARY KEY (media_id, category_id)
			);
		)";

		char* errmsg = nullptr;
		if (sqlite3_exec(db_, ddl, nullptr, nullptr, &errmsg) != SQLITE_OK) {
			out = "Failed to initialise database schema: ";
			out += errmsg;
			sqlite3_free(errmsg);
			return false;
		}

		return true;
	}

	SqliteMediaRepository::SqliteMediaRepository(const std::filesystem::path& mediaRoot)
		: mediaRoot_(std::filesystem::absolute(mediaRoot))
	{
		std::filesystem::create_directories(mediaRoot_);
	}

	SqliteMediaRepository::~SqliteMediaRepository()
	{
		closeDatabase();
	}
}