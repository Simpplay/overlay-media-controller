add_library(event
    "api/Event.hpp"
    "api/EventBus.hpp"
    "api/events/ExitApplicationRequestedEvent.hpp"
    "api/EventBus.cpp"
)

target_include_directories(event 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
)

