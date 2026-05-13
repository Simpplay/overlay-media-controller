#include "SqliteMediaRepository.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <optional>

#include <iostream>

// Thumbnail
#include <Windows.h>
#include <shobjidl.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "Ole32.lib")

namespace omc::media
{
	std::string SqliteMediaRepository::toStoredRelativePath(
		const std::filesystem::path& absolutePath,
		const std::filesystem::path& root) const
	{
		const auto normalizedRoot = std::filesystem::weakly_canonical(root);
		const auto normalizedPath = std::filesystem::weakly_canonical(absolutePath);
		auto relativePath = std::filesystem::relative(normalizedPath, normalizedRoot);
		return relativePath.generic_string();
	}

	std::filesystem::path SqliteMediaRepository::resolveStoredPath(
		const std::string& storedPath,
		const std::filesystem::path& root) const
	{
		if (storedPath.empty()) {
			return {};
		}

		std::filesystem::path stored(storedPath);
		if (stored.is_absolute()) {
			return stored;
		}

		return root / stored;
	}

	bool SqliteMediaRepository::migrateStoredPathsToRelative(std::string& out)
	{
		if (!db_) {
			return false;
		}

		const char* sql = "SELECT id, filepath, thumbnailPath FROM media;";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			out = "Failed to prepare migration query";
			return false;
		}

		const char* updateSql = "UPDATE media SET filepath = ?, thumbnailPath = ? WHERE id = ?;";
		sqlite3_stmt* updateStmt = nullptr;
		if (sqlite3_prepare_v2(db_, updateSql, -1, &updateStmt, nullptr) != SQLITE_OK) {
			sqlite3_finalize(stmt);
			out = "Failed to prepare migration update query";
			return false;
		}

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const int id = sqlite3_column_int(stmt, 0);
			const unsigned char* filepathText = sqlite3_column_text(stmt, 1);
			const unsigned char* thumbnailPathText = sqlite3_column_text(stmt, 2);

			if (!filepathText) {
				continue;
			}

			const std::string filepathRaw = reinterpret_cast<const char*>(filepathText);
			const std::string thumbnailRaw = thumbnailPathText
				? reinterpret_cast<const char*>(thumbnailPathText)
				: "";

			const auto filepathAbsolute = resolveStoredPath(filepathRaw, mediaRoot_);
			const auto thumbnailAbsolute = resolveStoredPath(thumbnailRaw, thumbnailRoot_);

			const auto filepathRelative = toStoredRelativePath(filepathAbsolute, mediaRoot_);
			const auto thumbnailRelative = thumbnailRaw.empty()
				? ""
				: toStoredRelativePath(thumbnailAbsolute, thumbnailRoot_);

			sqlite3_reset(updateStmt);
			sqlite3_clear_bindings(updateStmt);
			sqlite3_bind_text(updateStmt, 1, filepathRelative.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_text(updateStmt, 2, thumbnailRelative.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(updateStmt, 3, id);

			if (sqlite3_step(updateStmt) != SQLITE_DONE) {
				sqlite3_finalize(updateStmt);
				sqlite3_finalize(stmt);
				out = "Failed updating migrated media paths";
				return false;
			}
		}

		sqlite3_finalize(updateStmt);
		sqlite3_finalize(stmt);
		return true;
	}

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
			m.contentType,
			m.thumbnailPath
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

			const auto thumbnailPathText = sqlite3_column_text(stmt, 4);
			if (thumbnailPathText) {
				media.thumbnailPath = reinterpret_cast<const char*>(thumbnailPathText);
			}

			mediaList.push_back(std::move(media));
		}

		sqlite3_finalize(stmt);

		return mediaList;
	}

	bool SqliteMediaRepository::generateThumbnail(
		const std::filesystem::path& inputPath,
		const std::filesystem::path& outputPath,
		int size)
	{
		IShellItemImageFactory* imageFactory = nullptr;
		const HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool comInitialized = SUCCEEDED(comInit) || comInit == RPC_E_CHANGED_MODE;
		HRESULT hr = SHCreateItemFromParsingName(
			inputPath.wstring().c_str(),
			nullptr,
			IID_PPV_ARGS(&imageFactory));

		if (FAILED(hr) || !imageFactory) {
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		SIZE thumbnailSize{};
		thumbnailSize.cx = size;
		thumbnailSize.cy = size;

		HBITMAP hBitmap = nullptr;

		hr = imageFactory->GetImage(
			thumbnailSize,
			SIIGBF_BIGGERSIZEOK,
			&hBitmap);

		imageFactory->Release();

		if (FAILED(hr) || !hBitmap) {
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		IWICImagingFactory* wicFactory = nullptr;

		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory));

		if (FAILED(hr)) {
			DeleteObject(hBitmap);
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		IWICBitmap* wicBitmap = nullptr;

		hr = wicFactory->CreateBitmapFromHBITMAP(
			hBitmap,
			nullptr,
			WICBitmapUseAlpha,
			&wicBitmap);

		DeleteObject(hBitmap);

		if (FAILED(hr)) {
			wicFactory->Release();
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		IWICStream* stream = nullptr;

		hr = wicFactory->CreateStream(&stream);

		if (FAILED(hr)) {
			wicBitmap->Release();
			wicFactory->Release();
			return false;
		}

		hr = stream->InitializeFromFilename(
			outputPath.wstring().c_str(),
			GENERIC_WRITE);

		if (FAILED(hr)) {
			stream->Release();
			wicBitmap->Release();
			wicFactory->Release();
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		IWICBitmapEncoder* encoder = nullptr;

		hr = wicFactory->CreateEncoder(
			GUID_ContainerFormatPng,
			nullptr,
			&encoder);

		if (FAILED(hr)) {
			stream->Release();
			wicBitmap->Release();
			wicFactory->Release();
			return false;
		}

		hr = encoder->Initialize(
			stream,
			WICBitmapEncoderNoCache);

		if (FAILED(hr)) {
			encoder->Release();
			stream->Release();
			wicBitmap->Release();
			wicFactory->Release();
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		IWICBitmapFrameEncode* frame = nullptr;
		IPropertyBag2* props = nullptr;

		hr = encoder->CreateNewFrame(&frame, &props);

		if (FAILED(hr)) {
			encoder->Release();
			stream->Release();
			wicBitmap->Release();
			wicFactory->Release();
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		hr = frame->Initialize(props);

		if (FAILED(hr)) {
			if (props) props->Release();
			frame->Release();
			encoder->Release();
			stream->Release();
			wicBitmap->Release();
			wicFactory->Release();
			if (comInitialized) {
				CoUninitialize();
			}
			return false;
		}

		UINT width = 0;
		UINT height = 0;

		wicBitmap->GetSize(&width, &height);

		frame->SetSize(width, height);

		WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;

		frame->SetPixelFormat(&format);

		hr = frame->WriteSource(wicBitmap, nullptr);

		if (SUCCEEDED(hr)) {
			frame->Commit();
			encoder->Commit();
		}

		if (props) props->Release();

		frame->Release();
		encoder->Release();
		stream->Release();
		wicBitmap->Release();
		wicFactory->Release();
		if (comInitialized) {
			CoUninitialize();
		}

		return SUCCEEDED(hr);
	}

	// ---------------------------------------------------------------------------
	// Media
	// ---------------------------------------------------------------------------

	std::optional<Media> SqliteMediaRepository::getMediaById(int id)
	{
		if (!db_) {
			return std::nullopt;
		}

		const char* sql = R"(
			SELECT
				id,
				title,
				filename,
				filepath,
				contentType,
				thumbnailPath
			FROM media
			WHERE id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return std::nullopt;
		}

		sqlite3_bind_int(stmt, 1, id);

		std::optional<Media> media;

		if (sqlite3_step(stmt) == SQLITE_ROW) {

			media.emplace();
			media->id = sqlite3_column_int(stmt, 0);

			const auto title = sqlite3_column_text(stmt, 1);
			if (title) {
				media->title = reinterpret_cast<const char*>(title);
			}

			const auto filename = sqlite3_column_text(stmt, 2);
			if (filename) {
				media->filename = reinterpret_cast<const char*>(filename);
			}

			const auto filepath = sqlite3_column_text(stmt, 3);
			if (filepath) {
				media->filepath = reinterpret_cast<const char*>(filepath);
			}

			const auto contentType = sqlite3_column_text(stmt, 4);
			if (contentType) {
				media->contentType = reinterpret_cast<const char*>(contentType);
			}

			const auto thumbnailPathText = sqlite3_column_text(stmt, 5);
			if (thumbnailPathText) {
				media->thumbnailPath = reinterpret_cast<const char*>(thumbnailPathText);
			}

			const auto resolvedFilepath = resolveStoredPath(media->filepath, mediaRoot_);
			media->filepath = resolvedFilepath.string();
			media->thumbnailPath = resolveStoredPath(media->thumbnailPath, thumbnailRoot_).string();

			if (std::filesystem::exists(resolvedFilepath)) {
				media->size = std::filesystem::file_size(resolvedFilepath);
			}

			media->categories = fetchCategoriesForMedia(db_, media->id);
		}

		sqlite3_finalize(stmt);

		return media;
	}

	std::vector<Media> SqliteMediaRepository::getAllMedia(const std::string& query, const std::string& category)
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
				contentType,
				thumbnailPath
			FROM media
			WHERE title LIKE '%' || ? || '%' OR filename LIKE '%' || ? || '%';
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return result;
		}

		sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, query.c_str(), -1, SQLITE_TRANSIENT);

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

			const auto thumbnailPathText = sqlite3_column_text(stmt, 5);
			if (thumbnailPathText) {
				media.thumbnailPath = reinterpret_cast<const char*>(thumbnailPathText);
			}

			const auto resolvedFilepath = resolveStoredPath(media.filepath, mediaRoot_);
			media.filepath = resolvedFilepath.string();
			media.thumbnailPath = resolveStoredPath(media.thumbnailPath, thumbnailRoot_).string();
			if (std::filesystem::exists(resolvedFilepath)) {
				media.size = std::filesystem::file_size(resolvedFilepath);
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

		auto thumbnailPath = thumbnailRoot_ / storedFilename;

		auto& thumbnailFilename =
			thumbnailPath.replace_extension(".png");

		thumbnailThreadPool_->enqueue([this, fullPath, thumbnailFilename]() {
			if (!generateThumbnail(fullPath, thumbnailFilename)) {
				std::cout << "Failed to generate thumbnail for: "
					<< fullPath << "\n";
			}
			});

		const char* sql = R"(
			INSERT INTO media (
				title,
				filename,
				filepath,
				contentType,
				thumbnailPath
			)
			VALUES (?, ?, ?, ?, ?);
		)";

		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			std::filesystem::remove(fullPath);
			std::cout << "Failed to prepare SQL statement: " << sqlite3_errmsg(db_) << "\n";
			return Media();
		}

		sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_TRANSIENT);
		const auto storedFilepath = toStoredRelativePath(fullPath, mediaRoot_);
		const auto storedThumbnailPath = toStoredRelativePath(thumbnailFilename, thumbnailRoot_);

		sqlite3_bind_text(stmt, 3, storedFilepath.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, contentType.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 5, storedThumbnailPath.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			sqlite3_finalize(stmt);
			std::filesystem::remove(fullPath);
			return Media();
		}

		sqlite3_finalize(stmt);

		Media media;
		media.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
		media.title = filename;
		media.filename = filename;
		media.filepath = fullPath.string();
		media.contentType = contentType;
		media.thumbnailPath = thumbnailFilename.string();
		media.size = data.size();

		return media;
	}

	bool SqliteMediaRepository::deleteMediaById(int id)
	{
		auto media = getMediaById(id);

		if (!media.has_value()) {
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

		if (std::filesystem::exists(media->filepath)) {
			std::filesystem::remove(media->filepath);
		}

		if (!media->thumbnailPath.empty() && std::filesystem::exists(media->thumbnailPath)) {
			std::filesystem::remove(media->thumbnailPath);
		}

		return true;
	}

	bool SqliteMediaRepository::updateMedia(int id, const Media& updatedMedia)
	{
		if (!db_) {
			return false;
		}

		const char* sql = R"(
			UPDATE media
			SET title = ?
			WHERE id = ?;
		)";

		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			return false;
		}

		sqlite3_bind_text(stmt, 1, updatedMedia.title.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 2, updatedMedia.id);
		const bool success = (sqlite3_step(stmt) == SQLITE_DONE);
		sqlite3_finalize(stmt);

		if (!success) {
			return false;
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
				m.contentType,
				thumbnailPath
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

			const auto thumbnailPathText = sqlite3_column_text(stmt, 5);
			if (thumbnailPathText) {
				media.thumbnailPath = reinterpret_cast<const char*>(thumbnailPathText);
			}

			const auto resolvedFilepath = resolveStoredPath(media.filepath, mediaRoot_);
			media.filepath = resolvedFilepath.string();
			media.thumbnailPath = resolveStoredPath(media.thumbnailPath, thumbnailRoot_).string();
			if (std::filesystem::exists(resolvedFilepath)) {
				media.size = std::filesystem::file_size(resolvedFilepath);
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
			cat.media = fetchMediaForCategory(db_, cat.id);
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

		if (!getMediaById(mediaId).has_value()) {
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
		ensureThumbnailsForStoredMedia();
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
					contentType TEXT,
					thumbnailPath TEXT -- NUEVA COLUMNA AQUÍ
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

		return migrateStoredPathsToRelative(out);
	}

	void SqliteMediaRepository::ensureThumbnailsForStoredMedia()
	{
		if (!db_) {
			return;
		}

		const auto mediaList = getAllMedia("", "");

		for (const auto& media : mediaList) {
			const auto resolvedFilepath = resolveStoredPath(media.filepath, mediaRoot_);
			if (resolvedFilepath.empty() || !std::filesystem::exists(resolvedFilepath)) {
				continue;
			}

			std::filesystem::path thumbnailPath = media.thumbnailPath.empty()
				? (thumbnailRoot_ / std::filesystem::path(resolvedFilepath).filename()).replace_extension(".png")
				: resolveStoredPath(media.thumbnailPath, thumbnailRoot_);

			if (std::filesystem::exists(thumbnailPath)) {
				continue;
			}

			if (!generateThumbnail(resolvedFilepath, thumbnailPath)) {
				std::cout << "Failed to generate missing thumbnail for: " << resolvedFilepath << "\n";
				continue;
			}

			if (media.thumbnailPath.empty()) {
				const char* sql = "UPDATE media SET thumbnailPath = ? WHERE id = ?;";
				sqlite3_stmt* stmt = nullptr;
				if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
					const auto storedThumbnailPath = toStoredRelativePath(thumbnailPath, thumbnailRoot_);
					sqlite3_bind_text(stmt, 1, storedThumbnailPath.c_str(), -1, SQLITE_TRANSIENT);
					sqlite3_bind_int(stmt, 2, media.id);
					sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
			}
		}
	}

	SqliteMediaRepository::SqliteMediaRepository(const std::filesystem::path& mediaRoot, const std::filesystem::path& thumbnailRoot)
		: mediaRoot_(std::filesystem::absolute(mediaRoot)),
		thumbnailRoot_(std::filesystem::absolute(thumbnailRoot)),
		thumbnailThreadPool_(std::make_unique<omc::shared::ThreadPool>(1))
	{
		std::filesystem::create_directories(mediaRoot_);
		std::filesystem::create_directories(thumbnailRoot_);
	}

	SqliteMediaRepository::~SqliteMediaRepository()
	{
		thumbnailThreadPool_.reset();
		closeDatabase();
	}
}
