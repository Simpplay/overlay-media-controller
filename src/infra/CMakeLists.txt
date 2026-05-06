set(INFRA_SOURCES
    "InfraStub.cpp"
    "window/Win32Renderer.hpp"
)

if (WIN32)
    list(APPEND INFRA_SOURCES "window/Win32Renderer.cpp")
endif ()

if (FFMPEG_FOUND)
    list(APPEND INFRA_SOURCES "media/FfmpegMediaPlayer.cpp")
endif ()

add_library(infra ${INFRA_SOURCES})

target_include_directories(infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if (WIN32)
    target_link_libraries(infra
        PUBLIC
            d3d11
            dxgi
    )
endif ()

if (FFMPEG_FOUND)
    target_include_directories(infra PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(infra PRIVATE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(infra PRIVATE ${FFMPEG_LIBRARIES})
endif ()
