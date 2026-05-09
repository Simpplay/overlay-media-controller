#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace omc::media
{
	struct Media 
	{
		uint64_t id;
		std::string title;
		std::string filename;
		std::string contentType;
		std::vector<std::byte> data;
	};
}