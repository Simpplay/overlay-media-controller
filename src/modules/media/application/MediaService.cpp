#include "MediaService.hpp"

#include <stdio.h>

namespace omc::media
{
	struct MediaService::Impl {
		MediaSourceDto mediaSourceToDto(const Media& media) {
			return MediaSourceDto{
				static_cast<int>(media.id),
				media.title,
				media.filename,
				media.contentType
			};
		}

		MediaFileDto mediaToFileDto(const Media& media) {
			return MediaFileDto{
				static_cast<int>(media.id),
				media.filename,
				media.filepath,
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
					media.contentType
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

	std::vector<MediaSourceDto> MediaService::getAllMediaSources() {
		auto media = repository_->getAllMedia();
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
		if (media.id == 0) {
			return std::nullopt;
		}
		return impl_->mediaSourceToDto(media);
	}

	std::optional<MediaFileDto> MediaService::getMediaFileById(int id) {
		const auto media = repository_->getMediaById(id);
		if (media.id == 0) {
			return std::nullopt;
		}
		return impl_->mediaToFileDto(media);
	}

	bool MediaService::deleteMediaSourceById(int id) {
		return repository_->deleteMediaById(id);
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