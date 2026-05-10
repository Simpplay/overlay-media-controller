#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Category.hpp"

namespace omc::media
{
    struct Media
    {
        int id = 0;

        std::string title;
        std::string filename;
        std::string filepath;
        std::string contentType;

		std::vector<Category> categories;

        // Opcional:
        uint64_t size = 0;
    };
}