#pragma once

#include "core/event/Event.hpp"
#include "modules/ui/UiWindow.hpp"
#include <string>

namespace omc::event {
    class WindowOpenRequestedEvent : public Event {
    public:
        WindowOpenRequestedEvent(const std::type_info& type)
            : type(type) {
        }

        const std::type_info& type;
    };
}
