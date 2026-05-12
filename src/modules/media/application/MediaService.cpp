#include "MediaService.hpp"

#include <filesystem>
#include <iostream>

namespace omc::media
{
	struct MediaService::Impl {
		MediaSourceDto mediaSourceToDto(const Media& media) {
			std::vector<int> categoryIds;
			for (const auto& category : media.categories) {
				categoryIds.push_back(category.id);
			}

			return MediaSourceDto{
				static_cast<int>(media.id),
				media.title,
				media.filename,
				media.contentType,
				categoryIds
			};
		}

		MediaFileDto mediaToFileDto(const Media& media) {
			return MediaFileDto{
				static_cast<int>(media.id),
				media.filename,
				media.filepath,
				media.thumbnailPath,
				media.contentType,
				media.size
			};
		}

		MediaCategoryDto categoryToDto(const Category& category)
		{
			MediaCategoryDto dto;

			dto.id = category.id;
			dto.name = category.name;

			for (const auto& media : category.media) {
				dto.media.push_back(MediaSourceDto{
					media.id,
					media.title,
					media.filename,
					media.contentType,
					std::vector<int>{ category.id }
					});
			}

			return dto;
		}
	};

	MediaService::MediaService(std::shared_ptr<IMediaRepository> repository)
		: repository_(std::move(repository)), impl_(std::make_unique<Impl>())
	{
	}

	MediaService::~MediaService() = default;

	// ---------------------------------------------------------------------------
	// Media
	// ---------------------------------------------------------------------------

	std::vector<MediaSourceDto> MediaService::getAllMediaSources(const SearchMediaDto& searchDto) {
		auto query = searchDto.query.value_or("");
		auto category = searchDto.category.value_or("");

		auto media = repository_->getAllMedia(query, category);
		std::vector<MediaSourceDto> dtos;
		dtos.reserve(media.size());

		for (const auto& m : media) {
			dtos.push_back(impl_->mediaSourceToDto(m));
		}

		return dtos;
	}

	MediaSourceDto MediaService::addMediaSource(
		std::span<const std::byte> data,
		const std::string& filename,
		const std::string& contentType)
	{
		return impl_->mediaSourceToDto(repository_->addMedia(
			std::vector<std::byte>(data.begin(), data.end()),
			filename,
			contentType
		));
	}

	std::optional<MediaSourceDto> MediaService::getMediaSourceById(int id) {
		const auto media = repository_->getMediaById(id);
		if (!media.has_value()) {
			return std::nullopt;
		}
		return impl_->mediaSourceToDto(media.value());
	}

	std::optional<MediaFileDto> MediaService::getMediaFileById(int id) {
		const auto media = repository_->getMediaById(id);
		if (!media.has_value()) {
			return std::nullopt;
		}
		return impl_->mediaToFileDto(media.value());
	}

	bool MediaService::deleteMediaSourceById(int id) {
		return repository_->deleteMediaById(id);
	}

	bool MediaService::updateMediaSource(UpdateMediaDto& newMedia) {
		auto media = repository_->getMediaById(newMedia.id);
		if (!media) {
			return false;
		}

		if (newMedia.title) {
			media->title = newMedia.title.value();
		}

		return repository_->updateMedia(newMedia.id, media.value());
	}

	// ---------------------------------------------------------------------------
	// Categories
	// ---------------------------------------------------------------------------

	std::vector<MediaCategoryDto> MediaService::getAllCategories() {
		const auto categories = repository_->getAllCategories();
		std::vector<MediaCategoryDto> dtos;
		dtos.reserve(categories.size());

		for (const auto& cat : categories) {
			dtos.push_back(impl_->categoryToDto(cat));
		}

		return dtos;
	}

	bool MediaService::createCategory(const std::string& name) {
		return repository_->createCategory(name);
	}

	std::optional<MediaCategoryDto> MediaService::getCategoryById(int id) {
		const auto category = repository_->getCategoryById(id);
		if (!category.has_value()) {
			return std::nullopt;
		}
		return impl_->categoryToDto(category.value());
	}

	bool MediaService::deleteCategoryById(int id) {
		return repository_->deleteCategoryById(id);
	}

	bool MediaService::addMediaToCategory(int mediaId, int categoryId) {
		return repository_->addMediaToCategory(mediaId, categoryId);
	}

	bool MediaService::removeMediaFromCategory(int mediaId, int categoryId) {
		return repository_->removeMediaFromCategory(mediaId, categoryId);
	}

	std::vector<MediaSourceDto> MediaService::getMediaByCategoryId(int categoryId) {
		const auto media = repository_->getMediaByCategoryId(categoryId);
		std::vector<MediaSourceDto> dtos;
		dtos.reserve(media.size());

		for (const auto& m : media) {
			dtos.push_back(impl_->mediaSourceToDto(m));
		}

		return dtos;
	}
}