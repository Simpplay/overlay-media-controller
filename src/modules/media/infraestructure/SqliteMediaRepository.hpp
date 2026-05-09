#pragma once

#include <sqlite3.h>

#include "modules/media/domain/IMediaRepository.hpp"

namespace omc::media
{
	class SqliteMediaRepository : public IMediaRepository
	{
	public:
		SqliteMediaRepository() = default;
		~SqliteMediaRepository();

		bool setupDatabase(const std::string& dbPath, std::string& out);
		void closeDatabase();
		bool initializeDatabase(std::string& out);

		Media getMediaById(int id) override;
		std::vector<Media> getAllMedia() override;
		Media addMedia(const std::vector<std::byte>& data, const std::string& filename, const std::string& contentType) override;
		bool deleteMediaById(int id) override;

	private:
		sqlite3* db_ = nullptr;
		std::string dbPath_;
	};
}