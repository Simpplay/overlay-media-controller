add_subdirectory(core)
add_subdirectory(infra)
add_subdirectory(modules)
add_subdirectory(shared)

add_executable(app
    app/main.cpp
    app/OverlayMediaController.cpp
    app/OverlayMediaController.hpp
)

target_link_libraries(app
    PUBLIC
        modules
        core
        infra
        shared

)
if (FFMPEG_FOUND)
    target_compile_definitions(app PRIVATE OMC_HAS_FFMPEG=1)
endif ()
