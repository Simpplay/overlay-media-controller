#pragma once

#include <memory>

#include "IMediaService.hpp"
#include "modules/media/domain/IMediaRepository.hpp"


namespace omc::media
{
	class MediaService : public IMediaService
	{
	public:
		MediaService(IMediaRepository& repository);
		~MediaService();

		std::vector<MediaSourceDto> getAllMediaSources(const SearchMediaDto& searchDto) override;

		MediaSourceDto addMediaSource(
			std::span<const std::byte> data,
			const std::string& filename,
			const std::string& contentType
		) override;

		std::optional<MediaSourceDto> getMediaSourceById(int id) override;
		std::optional<MediaFileDto> getMediaFileById(int id) override;

		bool deleteMediaSourceById(int id) override;
		bool updateMediaSource(UpdateMediaDto& newMedia) override;

		// Category management
		std::vector<MediaCategoryDto> getAllCategories() override;
		bool createCategory(const std::string& name) override;
		std::optional<MediaCategoryDto> getCategoryById(int id) override;
		bool deleteCategoryById(int id) override;

		bool addMediaToCategory(int mediaId, int categoryId) override;
		bool removeMediaFromCategory(int mediaId, int categoryId) override;
		
		std::vector<MediaSourceDto> getMediaByCategoryId(int categoryId) override;
	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
		IMediaRepository& repository_;
	};
}
