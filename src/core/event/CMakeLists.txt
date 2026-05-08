add_library(event
    "api/Event.hpp"
    "api/IEventBus.hpp"
    "api/events/ExitApplicationRequestedEvent.hpp"
    "infraestructure/EventBus.hpp"
    "infraestructure/EventBus.cpp"
)

target_include_directories(event 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/api  # Solo la API es pública
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
)

target_link_libraries(event
    PUBLIC core
)