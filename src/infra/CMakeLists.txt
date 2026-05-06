add_library(infra
    "window/Win32Renderer.cpp"
 "window/Win32Renderer.hpp" "media/FfmpegMediaPlayer.cpp")

target_include_directories(infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(infra 
    PUBLIC 
        d3d11 dxgi
)

target_include_directories(infra PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(infra PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(infra PRIVATE ${FFMPEG_LIBRARIES})