#pragma once

#include <vector>
#include <optional>

#include "MediaDto.hpp"

namespace omc::media
{
	struct MediaSource;
	class IMediaService
	{
	public:
		virtual ~IMediaService() = default;

		virtual std::vector<omc::media::MediaSourceDto> getAllMediaSources() = 0;

		virtual MediaSourceDto addMediaSource(
			std::span<const std::byte> data,
			const std::string& filename,
			const std::string& contentType
		) = 0;

		virtual std::optional<MediaSourceDto> getMediaSourceById(int id) = 0;

		virtual bool deleteMediaSourceById(int id) = 0;
	};
}