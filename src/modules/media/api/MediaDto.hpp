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
		std::vector<int> categoryIds;
	};

	struct MediaFileDto
	{
		int id;
		std::string filename;
		std::string filepath;
		std::string thumbnailPath;
		std::string contentType;
		uint64_t size;
	};

    struct MediaCategoryDto
    {
        int id;
        std::string name;

        std::vector<MediaSourceDto> media;
    };

    inline std::string to_json(const MediaSourceDto& dto)
    {
        using omc::json::JsonSerializer;

        return omc::json::JsonObject{}
            .field("id", dto.id)
            .field("title", dto.title)
            .field("filename", dto.filename)
            .field("contentType", dto.contentType)
			.field("categoryIds", dto.categoryIds)
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

    inline std::string to_json(const MediaCategoryDto& dto)
    {
        using omc::json::JsonSerializer;
        return omc::json::JsonObject{}
            .field("id", dto.id)
            .field("name", dto.name)
            .field("media", dto.media)
            .build();
	}
}