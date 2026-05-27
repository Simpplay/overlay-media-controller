add_library(server
    "runtime/ApiServer.hpp"
    "runtime/ApiServer.cpp"

    "application/HttpUtils.hpp"
    "application/MediaApiModule.hpp"
    "application/MediaApiModule.cpp"
    "application/CategoriesApiModule.hpp"
    "application/CategoriesApiModule.cpp"
    "application/OverlaysApiModule.hpp"
    "application/OverlaysApiModule.cpp"
    "application/SoundboardApiModule.hpp"
    "application/SoundboardApiModule.cpp"

    "api/IApiModule.hpp"
  )

target_include_directories(server 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

get_target_property(HTTPLIB_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(server SYSTEM PRIVATE ${HTTPLIB_INCLUDE_DIRS})

target_link_libraries(server
    PUBLIC 
        core 
        httplib::httplib
        nlohmann_json::nlohmann_json
)