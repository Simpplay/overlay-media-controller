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
}