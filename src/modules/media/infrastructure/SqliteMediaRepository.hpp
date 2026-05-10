#pragma once

#include <sqlite3.h>
#include <filesystem>

#include "modules/media/domain/IMediaRepository.hpp"

namespace omc::media
{
	class SqliteMediaRepository : public IMediaRepository
	{
	public:
		SqliteMediaRepository(const std::filesystem::path& mediaRoot);
		~SqliteMediaRepository();

		bool setupDatabase(const std::string& dbPath, std::string& out);
		void closeDatabase();
		bool initializeDatabase(std::string& out);

		Media getMediaById(int id) override;
		std::vector<Media> getAllMedia() override;
		Media addMedia(const std::vector<std::byte>& data, const std::string& filename, const std::string& contentType) override;
		bool deleteMediaById(int id) override;

		// Category management
		std::vector<Category> getAllCategories() override;
		std::optional<Category> getCategoryById(int id) override;
		bool createCategory(const std::string& name) override;
		bool deleteCategoryById(int id) override;
		bool addMediaToCategory(int mediaId, int categoryId) override;
		bool removeMediaFromCategory(int mediaId, int categoryId) override;
		std::vector<Media> getMediaByCategoryId(int categoryId) override;

	private:
		sqlite3* db_ = nullptr;
		std::string dbPath_;

		std::filesystem::path mediaRoot_;
	};
}