#include "SqliteMediaRepository.hpp"

namespace omc::media
{
	Media SqliteMediaRepository::getMediaById(int id)
	{
		if (!db_) {
			return Media();
		}

		const char* sql = R"(
			SELECT id, filename, contentType, data
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

			const unsigned char* filenameText = sqlite3_column_text(stmt, 1);
			const unsigned char* contentTypeText = sqlite3_column_text(stmt, 2);

			if (filenameText) {
				media.filename = reinterpret_cast<const char*>(filenameText);
			}

			if (contentTypeText) {
				media.contentType = reinterpret_cast<const char*>(contentTypeText);
			}

			const std::byte* blobData =
				reinterpret_cast<const std::byte*>(sqlite3_column_blob(stmt, 3));

			int blobSize = sqlite3_column_bytes(stmt, 3);

			if (blobData && blobSize > 0) {
				media.data.assign(blobData, blobData + blobSize);
			}
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
			SELECT id, filename, contentType, data
			FROM media;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return result;
		}

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			Media media;

			media.id = sqlite3_column_int(stmt, 0);

			const unsigned char* filenameText = sqlite3_column_text(stmt, 1);
			const unsigned char* contentTypeText = sqlite3_column_text(stmt, 2);

			if (filenameText) {
				media.filename = reinterpret_cast<const char*>(filenameText);
			}

			if (contentTypeText) {
				media.contentType = reinterpret_cast<const char*>(contentTypeText);
			}

			const std::byte* blobData =
				reinterpret_cast<const std::byte*>(sqlite3_column_blob(stmt, 3));

			int blobSize = sqlite3_column_bytes(stmt, 3);

			if (blobData && blobSize > 0) {
				media.data.assign(blobData, blobData + blobSize);
			}

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

		const char* sql = R"(
			INSERT INTO media (filename, contentType, data)
			VALUES (?, ?, ?);
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return Media();
		}

		sqlite3_bind_text(
			stmt,
			1,
			filename.c_str(),
			static_cast<int>(filename.size()),
			SQLITE_TRANSIENT);

		sqlite3_bind_text(
			stmt,
			2,
			contentType.c_str(),
			static_cast<int>(contentType.size()),
			SQLITE_TRANSIENT);

		sqlite3_bind_blob(
			stmt,
			3,
			data.data(),
			static_cast<int>(data.size()),
			SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			return Media();
		}

		sqlite3_finalize(stmt);

		Media media;
		media.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
		media.filename = filename;
		media.contentType = contentType;
		media.data = data;

		return media;
	}

	bool SqliteMediaRepository::deleteMediaById(int id)
	{
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

		bool success = (sqlite3_step(stmt) == SQLITE_DONE);

		sqlite3_finalize(stmt);

		return success && sqlite3_changes(db_) > 0;
	}

	bool SqliteMediaRepository::setupDatabase(const std::string& dbPath, std::string& out)
	{
		int rc = sqlite3_open(dbPath.c_str(), &db_);
		if (rc != SQLITE_OK) {
			db_ = nullptr;
			out = sqlite3_errmsg(db_);
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
		const char* createTableSql = R"(
			CREATE TABLE IF NOT EXISTS media (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				title TEXT,
				filename TEXT,
				contentType TEXT,
				data BLOB
			);
		)";

		char* errmsg = nullptr;
		if (sqlite3_exec(db_, createTableSql, nullptr, nullptr, &errmsg) != SQLITE_OK) {
			out = "Failed to create media table: ";
			out += errmsg;
			sqlite3_free(errmsg);
			return false;
		}

		return true;
	}

	SqliteMediaRepository::~SqliteMediaRepository()
	{
		closeDatabase();
	}
}