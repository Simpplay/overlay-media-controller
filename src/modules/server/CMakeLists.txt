add_library(server
    ServerManager.cpp
)

target_include_directories(server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(server
    PUBLIC core
)