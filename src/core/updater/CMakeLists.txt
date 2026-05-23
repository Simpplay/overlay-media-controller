add_library(updater
    "api/events/RequestUpdateEvent.hpp"
    "api/events/UpdateAvailableEvent.hpp"
    "application/Updater.cpp"
)

target_include_directories(updater 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)