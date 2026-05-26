#pragma once

#include <string>

#include "core/types/JsonSerializer.hpp"

namespace omc::soundboard
{
    // Response DTOs
    struct SoundboardDeviceDto
    {
        int device_id;
        std::string name;
        bool selected;
    };

    // Request DTOs
    struct SetInputDeviceDto {
        int device_id;
    };

    inline std::string to_json(const SoundboardDeviceDto& dto)
    {
        using omc::json::JsonSerializer;
        return omc::json::JsonObject{}
            .field("device_id", dto.device_id)
            .field("name", dto.name)
            .field("selected", dto.selected)
            .build();
    }

    inline std::string to_json(const SetInputDeviceDto& dto)
    {
        using omc::json::JsonSerializer;
        return omc::json::JsonObject{}
            .field("device_id", dto.device_id)
            .build();
    }
}