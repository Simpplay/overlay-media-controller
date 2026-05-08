#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "core/event/infraestructure/EventBus.hpp"
#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

namespace omc::media {
    class MediaManager {
    public:
        MediaManager(omc::event::EventBus& eventBus);

    private:
        void handlePlayMediaRequestedEvent(const omc::event::PlayMediaRequestedEvent& event);

        omc::event::EventBus& eventBus;
        std::unordered_map<int, std::string> mediaCatalog;
    };
}
