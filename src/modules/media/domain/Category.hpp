#pragma once

#include <string>
#include <vector>

namespace omc::media
{
    struct MediaPreview
    {
        int id = 0;
        std::string title;
        std::string filename;
        std::string contentType;
    };

    struct Category
    {
        int id = 0;
        std::string name;

        std::vector<MediaPreview> media;
    };
}