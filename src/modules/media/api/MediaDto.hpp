#pragma once

#include <string>

#include "core/types/JsonSerializer.hpp"

namespace omc::media
{
	struct MediaSourceDto
	{
		int id;
		std::string title;
		std::string filename;
		std::string contentType;
	};

	struct MediaFileDto
	{
		int id;
		std::string filename;
		std::string filepath;
		std::string contentType;
		uint64_t size;
	};

    inline std::string to_json(const MediaSourceDto& dto)
    {
        using omc::json::JsonSerializer;

        return omc::json::JsonObject{}
            .field("id", dto.id)
            .field("title", dto.title)
            .field("filename", dto.filename)
            .field("contentType", dto.contentType)
            .build();
    }

    inline std::string to_json(const MediaFileDto& dto)
    {
        using omc::json::JsonSerializer;

        return omc::json::JsonObject{}
            .field("id", dto.id)
            .field("filename", dto.filename)
            .field("filepath", dto.filepath)
            .field("contentType", dto.contentType)
            .field("size", dto.size)
            .build();
    }
}