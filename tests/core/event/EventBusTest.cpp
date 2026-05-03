#include <gtest/gtest.h>

#include "core/event/EventBus.hpp"
#include "core/event/events/PlayMediaRequestedEvent.cpp"

TEST(EventBusTest, BasicDispatch) {
    omc::event::EventBus bus;

    bool called = false;

    bus.subscribe<omc::event::PlayMediaRequestedEvent>(
        [&](const omc::event::PlayMediaRequestedEvent&) {
            called = true;
        }
    );

    bus.post(std::make_unique<omc::event::PlayMediaRequestedEvent>(1));
    bus.processQueue();

    EXPECT_TRUE(called);
}