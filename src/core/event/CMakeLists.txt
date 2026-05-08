add_library(event
    "api/Event.hpp"
    "api/IEventBus.hpp"
    "api/events/ExitApplicationRequestedEvent.hpp"
    "infraestructure/EventBus.hpp"
    "infraestructure/EventBus.cpp"
)

target_include_directories(event 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api  # Solo la API es pública
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
)

