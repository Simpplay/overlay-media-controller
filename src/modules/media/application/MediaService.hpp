#pragma once

#include <memory>

#include "IMediaService.hpp"
#include "modules/media/domain/IMediaRepository.hpp"


namespace omc::media
{
	class MediaService : public IMediaService
	{
	public:
		MediaService(std::shared_ptr<IMediaRepository> repository);
		~MediaService();

		std::vector<MediaSourceDto> getAllMediaSources() override;

		MediaSourceDto addMediaSource(
			std::span<const std::byte> data,
			const std::string& filename,
			const std::string& contentType
		) override;

		std::optional<MediaSourceDto> getMediaSourceById(int id) override;
		std::optional<MediaFileDto> getMediaFileById(int id) override;

		bool deleteMediaSourceById(int id) override;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
		std::shared_ptr<IMediaRepository> repository_;
	};
}
