#pragma once

#include <memory>

#include "core/event/api/Event.hpp"

namespace omc::event {
    class ApplicationHideRequestedEvent : public Event {
    public:
        explicit ApplicationHideRequestedEvent()
        {
        }
    };
}
