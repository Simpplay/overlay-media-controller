#pragma once

#include "core/event/api/Event.hpp"

namespace omc::event {
    class ServerStartedEvent : public Event {
    public:
        explicit ServerStartedEvent(int p) : port(p) {}
        int port;
    };
}
