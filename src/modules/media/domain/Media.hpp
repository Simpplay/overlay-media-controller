#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace omc::media
{
    struct CategoryPreview
    {
        int id = 0;
        std::string name;
    };

    struct Media
    {
        int id = 0;

        std::string title;
        std::string filename;
        std::string filepath;
        std::string contentType;

        std::vector<CategoryPreview> categories;

        uint64_t size = 0;
    };
}