#pragma once

#include <httplib.h>

namespace omc::server {
    class IApiModule {
    public:
        virtual ~IApiModule() = default;

        virtual void registerRoutes(httplib::Server& server) = 0;
    };
}