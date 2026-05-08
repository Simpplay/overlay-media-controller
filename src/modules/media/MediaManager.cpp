#include "MediaManager.hpp"

#include <cstdio>

namespace omc::media {
    MediaManager::MediaManager(omc::event::EventBus& eventBus, std::shared_ptr<MediaPlayer> mediaPlayer)
        : eventBus(eventBus), mediaPlayer(std::move(mediaPlayer)), mediaCatalog({
            {1, "C:\\Orgullo.mp4"}
        }) {
        this->eventBus.subscribe<omc::event::PlayMediaRequestedEvent>([this](const omc::event::PlayMediaRequestedEvent& event) {
            handlePlayMediaRequestedEvent(event);
        });
    }

    void MediaManager::handlePlayMediaRequestedEvent(const omc::event::PlayMediaRequestedEvent& event) {
        if (!mediaPlayer) {
            std::printf("Error: MediaManager has no media player configured.\n");
            return;
        }

        auto mediaIt = mediaCatalog.find(event.media_id);
        if (mediaIt == mediaCatalog.end()) {
            std::printf("Error: Unknown mediaId: %d\n", event.media_id);
            return;
        }

        if (!mediaPlayer->load(mediaIt->second)) {
            std::printf("Error: Could not load media path: %s\n", mediaIt->second.c_str());
            return;
        }

        mediaPlayer->play(event.media_id);
    }
}
