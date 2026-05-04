add_library(infra
    "window/SDLRenderer.cpp"
 "window/SDLRenderer.hpp")

target_include_directories(infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(infra 
    PUBLIC SDL3::SDL3
)