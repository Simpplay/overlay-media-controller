add_library(updates
    "api/events/RequestUpdateEvent.hpp"
    "api/events/UpdateAvailableEvent.hpp"
    "application/Updater.cpp"
)

target_include_directories(updates 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(updates 
    PUBLIC 
        httplib::httplib
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
)