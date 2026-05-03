add_library(core
    event/Event.cpp
    event/EventBus.cpp
    application/OverlayMediaController.cpp
 "event/events/PlayMediaRequestedEvent.cpp")

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)