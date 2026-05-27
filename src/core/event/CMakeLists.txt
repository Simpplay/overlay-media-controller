add_library(event
    "api/Event.hpp"
    "api/EventBus.hpp"
    "api/events/ExitApplicationRequestedEvent.hpp"
    "api/EventBus.cpp"

    "api/events/ApplicationHideRequestedEvent.hpp"
    "api/events/ApplicationShowRequestedEvent.hpp"
)

target_include_directories(event 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
)

