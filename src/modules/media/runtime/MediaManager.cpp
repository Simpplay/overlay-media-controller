#include "MediaManager.hpp"

#include <cstdio>
#include <memory>

#include "modules/ui/api/events/WindowOpenRequestedEvent.hpp"
#include "modules/ui/application/windows/WebViewWindow.hpp"

#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

namespace omc::media {
    struct MediaManager::Impl {
        omc::event::EventBus& eventBus;

        MediaManager::Impl(omc::event::EventBus& eventBus) : eventBus(eventBus) {
            eventBus.subscribe<omc::event::PlayMediaRequestedEvent>([this](const omc::event::PlayMediaRequestedEvent& event) {
                handlePlayMediaRequestedEvent(event);
                });
        }

        void handlePlayMediaRequestedEvent(const omc::event::PlayMediaRequestedEvent& event) {
            auto window = std::make_unique<omc::ui::window::WebViewWindow>(
                "http://localhost:8080/media/" + std::to_string(event.id),
                omc::ui::Vec2{ event.posX, event.posY },
                omc::ui::Vec2{ event.width, event.height }
            );

            if (event.fullscreen)
                window->setMaximized(true);

            eventBus.post(std::make_unique<omc::event::WindowOpenRequestedEvent>(
                std::move(window)
            ));
        }
    };  

    void MediaManager::init(omc::event::EventBus& eventBus) 
    {
		impl_ = std::make_unique<Impl>(eventBus);
    }

    MediaManager::MediaManager() = default;
    MediaManager::~MediaManager() = default;
}
