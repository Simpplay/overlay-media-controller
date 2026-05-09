#include "SqliteMediaRepository.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>

#include <iostream>

namespace omc::media
{
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

			// col 1 → title
			const auto title = sqlite3_column_text(stmt, 1);
			if (title) {
				media.title = reinterpret_cast<const char*>(title);
			}

			// col 2 → filename
			const auto filename = sqlite3_column_text(stmt, 2);
			if (filename) {
				media.filename = reinterpret_cast<const char*>(filename);
			}

			// col 3 → filepath
			const auto filepath = sqlite3_column_text(stmt, 3);
			if (filepath) {
				media.filepath = reinterpret_cast<const char*>(filepath);
			}

			// col 4 → contentType
			const auto contentType = sqlite3_column_text(stmt, 4);
			if (contentType) {
				media.contentType = reinterpret_cast<const char*>(contentType);
			}

			// Obtener tamaño real del archivo
			if (std::filesystem::exists(media.filepath)) {
				media.size = std::filesystem::file_size(media.filepath);
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

			// col 0 → id
			media.id = sqlite3_column_int(stmt, 0);

			// col 1 → title
			const unsigned char* titleText = sqlite3_column_text(stmt, 1);
			if (titleText) {
				media.title = reinterpret_cast<const char*>(titleText);
			}

			// col 2 → filename
			const unsigned char* filenameText = sqlite3_column_text(stmt, 2);
			if (filenameText) {
				media.filename = reinterpret_cast<const char*>(filenameText);
			}

			// col 3 → filepath
			const unsigned char* filepathText = sqlite3_column_text(stmt, 3);
			if (filepathText) {
				media.filepath = reinterpret_cast<const char*>(filepathText);
			}

			// col 4 → contentType
			const unsigned char* contentTypeText = sqlite3_column_text(stmt, 4);
			if (contentTypeText) {
				media.contentType = reinterpret_cast<const char*>(contentTypeText);
			}

			// Obtener tamaño real
			if (std::filesystem::exists(media.filepath)) {
				media.size = std::filesystem::file_size(media.filepath);
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

		// Generar nombre único
		auto timestamp =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch())
			.count();

		std::string storedFilename =
			std::to_string(timestamp) + "_" + filename;

		auto fullPath = mediaRoot_ / storedFilename;

		std::cout << "Storing media file: " << fullPath << "\n";

		// Guardar archivo
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

		sqlite3_bind_text(
			stmt,
			1,
			filename.c_str(),
			-1,
			SQLITE_TRANSIENT);

		sqlite3_bind_text(
			stmt,
			2,
			fullPath.string().c_str(),
			-1,
			SQLITE_TRANSIENT);

		sqlite3_bind_text(
			stmt,
			3,
			contentType.c_str(),
			-1,
			SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			std::filesystem::remove(fullPath);
			return Media();
		}

		sqlite3_finalize(stmt);

		Media media;

		media.id = static_cast<int>(
			sqlite3_last_insert_rowid(db_));

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

		// La fila fue eliminada de la BD: borrar también el fichero en disco.
		if (std::filesystem::exists(media.filepath)) {
			std::filesystem::remove(media.filepath);
		}

		return true;
	}

	bool SqliteMediaRepository::setupDatabase(const std::string& dbPath, std::string& out)
	{
		int rc = sqlite3_open(dbPath.c_str(), &db_);
		if (rc != SQLITE_OK) {
			out = sqlite3_errmsg(db_); // leer el error con el handle aún válido
			sqlite3_close(db_);        // cerrar el handle aunque la apertura falló
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
		const char* createTableSql = R"(
			CREATE TABLE IF NOT EXISTS media (
				id INTEGER PRIMARY KEY AUTOINCREMENT,
				title TEXT,
				filename TEXT,
				filepath TEXT NOT NULL,
				contentType TEXT
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
