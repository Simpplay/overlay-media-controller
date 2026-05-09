#pragma once

#include <vector>
#include <string>

#include "Media.hpp"

namespace omc::media
{
	class IMediaRepository
	{
	public:
		virtual ~IMediaRepository() = default;

		virtual Media getMediaById(int id) = 0;
		virtual std::vector<Media> getAllMedia() = 0;

		virtual Media addMedia(const std::vector<std::byte>& data, const std::string& filename, const std::string& contentType) = 0;
		virtual bool deleteMediaById(int id) = 0;
	};
}