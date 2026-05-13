#pragma once

#include <vector>
#include <string>
#include <optional>

#include "Media.hpp"
#include "Category.hpp"

namespace omc::media
{
	class IMediaRepository
	{
	public:
		virtual ~IMediaRepository() = default;

		virtual std::optional<Media> getMediaById(int id) = 0;
		virtual std::vector<Media> getAllMedia(const std::string& query, const std::string& category) = 0;

		virtual Media addMedia(const std::vector<std::byte>& data, const std::string& filename, const std::string& contentType) = 0;
		virtual bool deleteMediaById(int id) = 0;
		virtual bool updateMedia(int id, const Media& updatedMedia) = 0;


		// Category management
		virtual std::vector<Category> getAllCategories() = 0;
		virtual std::optional<Category> getCategoryById(int id) = 0;
		virtual bool createCategory(const std::string& name) = 0;
		virtual bool deleteCategoryById(int id) = 0;
		virtual bool addMediaToCategory(int mediaId, int categoryId) = 0;
		virtual bool removeMediaFromCategory(int mediaId, int categoryId) = 0;
		virtual std::vector<Media> getMediaByCategoryId(int categoryId) = 0;
	};
}