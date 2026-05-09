#pragma once

#include <memory>

#include "core/event/api/Event.hpp"
#include "modules/ui/domain/UiWindow.hpp"

namespace omc::event {
    class WindowOpenRequestedEvent : public Event {
    public:
        explicit WindowOpenRequestedEvent(std::unique_ptr<omc::ui::window::UiWindow> w)
            : window(std::move(w)) {
        }

        std::unique_ptr<omc::ui::window::UiWindow> window;
    };
}
