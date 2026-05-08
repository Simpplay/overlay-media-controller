add_library(server
    "runtime/ApiServer.hpp"
    "runtime/ApiServer.cpp"
)

target_include_directories(server 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

get_target_property(HTTPLIB_INCLUDE_DIRS httplib::httplib INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(server SYSTEM PRIVATE ${HTTPLIB_INCLUDE_DIRS})

target_link_libraries(server
    PUBLIC 
        core 
        httplib::httplib
)