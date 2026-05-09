#include <gtest/gtest.h>

#include "core/event/api/EventBus.hpp"
#include "modules/media/api/events/PlayMediaRequestedEvent.hpp"

TEST(EventBusTest, BasicDispatch) {
    omc::event::EventBus bus;

    bool called = false;

    bus.subscribe<omc::event::PlayMediaRequestedEvent>(
        [&](const omc::event::PlayMediaRequestedEvent&) {
            called = true;
        }
    );

    bus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(1, false, 800.0f, 600.0f, 800.0f, 600.0f));
    bus.processQueue();

    EXPECT_TRUE(called);
}