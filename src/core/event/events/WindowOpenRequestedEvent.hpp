#pragma once

#include <memory>

#include "core/event/Event.hpp"
#include "modules/ui/UiWindow.hpp"

namespace omc::event {
    class WindowOpenRequestedEvent : public Event {
    public:
        WindowOpenRequestedEvent(const omc::ui::window::UiWindow& window)
            : window(window) {
        }

        const omc::ui::window::UiWindow& window;
    };
}
