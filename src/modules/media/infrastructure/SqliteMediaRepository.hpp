#pragma once

#include <sqlite3.h>
#include <filesystem>
#include <memory>

#include "modules/media/domain/IMediaRepository.hpp"
#include "core/threading/api/ThreadPool.hpp"

namespace omc::media
{
	class SqliteMediaRepository : public IMediaRepository
	{
	public:
		SqliteMediaRepository(const std::filesystem::path& mediaRoot, const std::filesystem::path& thumbnailRoot);
		~SqliteMediaRepository();

		bool setupDatabase(const std::string& dbPath, std::string& out);
		void closeDatabase();
		bool initializeDatabase(std::string& out);

		std::optional<Media> getMediaById(int id) override;
		std::vector<Media> getAllMedia(const std::string& query, const std::string& category) override;
		Media addMedia(const std::vector<std::byte>& data, const std::string& filename, const std::string& contentType) override;
		bool deleteMediaById(int id) override;
		bool updateMedia(int id, const Media& updatedMedia);

		// Category management
		std::vector<Category> getAllCategories() override;
		std::optional<Category> getCategoryById(int id) override;
		bool createCategory(const std::string& name) override;
		bool deleteCategoryById(int id) override;
		bool addMediaToCategory(int mediaId, int categoryId) override;
		bool removeMediaFromCategory(int mediaId, int categoryId) override;
		std::vector<Media> getMediaByCategoryId(int categoryId) override;

		// Helpers
		bool generateThumbnail(const std::filesystem::path& inputPath, const std::filesystem::path& outputPath, int size = 256);
		void ensureThumbnailsForStoredMedia();

	private:
		sqlite3* db_ = nullptr;
		std::string dbPath_;
		std::unique_ptr<omc::shared::ThreadPool> thumbnailThreadPool_;

		std::filesystem::path mediaRoot_;
		std::filesystem::path thumbnailRoot_;
	};
}
