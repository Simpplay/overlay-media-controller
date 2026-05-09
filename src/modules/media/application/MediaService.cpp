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
	};

	MediaService::MediaService(std::shared_ptr<IMediaRepository> repository)
		: repository_(std::move(repository)), impl_(std::make_unique<Impl>())
	{ }

	MediaService::~MediaService() = default;

	std::vector<MediaSourceDto> MediaService::getAllMediaSources() {
		auto media = repository_->getAllMedia();
		std::vector<MediaSourceDto> dtos;

		for (size_t i = 0; i < media.size(); ++i) {
			dtos.push_back(impl_->mediaSourceToDto(media[i]));
		}

		return dtos;
	}

	MediaSourceDto MediaService::addMediaSource(std::span<const std::byte> data, const std::string& filename, const std::string& contentType) {
		return impl_->mediaSourceToDto(repository_->addMedia(
			std::vector<std::byte>(data.begin(), data.end()),
			filename,
			contentType
		));
	}

	std::optional<MediaSourceDto> MediaService::getMediaSourceById(int id) {
		return impl_->mediaSourceToDto(repository_->getMediaById(id));
	}

	bool MediaService::deleteMediaSourceById(int id) {
		return repository_->deleteMediaById(id);
	}
}