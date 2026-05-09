#pragma once

#include <memory>

#include "core/event/api/EventBus.hpp"

namespace omc::media {
    class MediaManager {
    public:
        MediaManager();
		~MediaManager();

        void init(omc::event::EventBus& eventBus);

    private:
        struct Impl;
		std::unique_ptr<Impl> impl_;
    };
}
