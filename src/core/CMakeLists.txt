add_library(core
    event/Event.cpp
    event/EventBus.cpp
 "event/events/PlayMediaRequestedEvent.hpp" "event/events/WindowOpenRequestedEvent.hpp" "event/events/ExitApplicationRequestedEvent.hpp" "event/events/MediaStartedEvent.hpp" "types/VideoFrame.hpp")

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)