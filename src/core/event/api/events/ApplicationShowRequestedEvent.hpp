#pragma once

#include <memory>

#include "core/event/api/Event.hpp"

namespace omc::event {
    class ApplicationShowRequestedEvent : public Event {
    public:
        explicit ApplicationShowRequestedEvent() 
        {
        }
    };
}
