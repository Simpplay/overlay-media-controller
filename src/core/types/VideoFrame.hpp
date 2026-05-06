#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace omc::media {
    struct VideoFrame {
        int width = 0;
        int height = 0;

        std::shared_ptr<std::vector<uint8_t>> data = std::make_shared<std::vector<uint8_t>>();

        int64_t pts = 0;
    };
}