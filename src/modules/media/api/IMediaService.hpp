#pragma once

#include <vector>
#include <optional>

#include "MediaDto.hpp"

namespace omc::media
{
	class IMediaService
	{
	public:
		virtual ~IMediaService() = default;

		virtual std::vector<omc::media::MediaSourceDto> getAllMediaSources(const SearchMediaDto& searchDto) = 0;

		virtual MediaSourceDto addMediaSource(
			std::span<const std::byte> data,
			const std::string& filename,
			const std::string& contentType
		) = 0;

		virtual std::optional<MediaSourceDto> getMediaSourceById(int id) = 0;
		virtual std::optional<MediaFileDto> getMediaFileById(int id) = 0;

		virtual bool deleteMediaSourceById(int id) = 0;
		virtual bool updateMediaSource(UpdateMediaDto& newMedia) = 0;

		// Category management
		virtual std::vector<MediaCategoryDto> getAllCategories() = 0;
		virtual bool createCategory(const std::string& name) = 0;
		virtual std::optional<MediaCategoryDto> getCategoryById(int id) = 0;
		virtual bool deleteCategoryById(int id) = 0;

		virtual bool addMediaToCategory(int mediaId, int categoryId) = 0;
		virtual bool removeMediaFromCategory(int mediaId, int categoryId) = 0;
		
		virtual std::vector<MediaSourceDto> getMediaByCategoryId(int categoryId) = 0;
	};
}
