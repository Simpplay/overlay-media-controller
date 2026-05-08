#pragma once

#include <memory>

#include "core/event/api/Event.hpp"
#include "modules/ui/domain/UiWindow.hpp"

namespace omc::event {
    class WindowOpenRequestedEvent : public Event {
    public:
        WindowOpenRequestedEvent(const omc::ui::window::UiWindow& window)
            : window(window) {
        }

        const omc::ui::window::UiWindow& window;
    };
}
