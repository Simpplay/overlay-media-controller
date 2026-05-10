#pragma once

#include <string>

#include "core/types/JsonSerializer.hpp"

namespace omc::ui
{
    struct PositionDto
    {
        float x;
        float y;
	};

    struct SizeDto
    {
        float width;
        float height;
	};

    struct OverlayDto
    {
        int id;
        int media_id;
        std::string state;
        bool fullscreen;
		PositionDto position;
		SizeDto size;
    };

    inline std::string to_json(const OverlayDto& dto)
    {
        using omc::json::JsonSerializer;

        return omc::json::JsonObject{}
            .field("id", dto.id)
            .field("media_id", dto.media_id)
            .field("state", dto.state)
            .field("fullscreen", dto.fullscreen)
            .field("position", to_json(dto.position))
            .field("size", to_json(dto.size))
            .build();
    }

    inline std::string to_json(const PositionDto& dto)
    {
        using omc::json::JsonSerializer;
        return omc::json::JsonObject{}
            .field("x", dto.x)
            .field("y", dto.y)
            .build();
	}

    inline std::string to_json(const SizeDto& dto)
    {
        using omc::json::JsonSerializer;
        return omc::json::JsonObject{}
            .field("width", dto.width)
            .field("height", dto.height)
            .build();
	}
}