#include "MediaManager.hpp"

#include <cstdio>

namespace omc::media {
    MediaManager::MediaManager(omc::event::EventBus& eventBus)
        : eventBus(eventBus), mediaCatalog({
            {1, "C:\\Orgullo.mp4"}
        }) {

        this->eventBus.subscribe<omc::event::PlayMediaRequestedEvent>([this](const omc::event::PlayMediaRequestedEvent& event) {
            handlePlayMediaRequestedEvent(event);
        });

        
    }

    void MediaManager::handlePlayMediaRequestedEvent(const omc::event::PlayMediaRequestedEvent& event) {
        auto mediaIt = mediaCatalog.find(event.media_id);
        if (mediaIt == mediaCatalog.end()) {
            std::printf("Error: Unknown mediaId: %d\n", event.media_id);
            return;
        }
    }
}
