add_library(infra
    "window/Win32Renderer.cpp"
 "window/Win32Renderer.hpp")

target_include_directories(infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(infra 
    PUBLIC d3d11 dxgi
)