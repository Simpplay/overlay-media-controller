#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "core/event/EventBus.hpp"
#include "core/event/events/PlayMediaRequestedEvent.hpp"
#include "modules/media/MediaPlayer.hpp"

namespace omc::media {
    class MediaManager {
    public:
        MediaManager(omc::event::EventBus& eventBus, std::shared_ptr<MediaPlayer> mediaPlayer);

    private:
        void handlePlayMediaRequestedEvent(const omc::event::PlayMediaRequestedEvent& event);

        omc::event::EventBus& eventBus;
        std::shared_ptr<MediaPlayer> mediaPlayer;
        std::unordered_map<int, std::string> mediaCatalog;
    };
}
