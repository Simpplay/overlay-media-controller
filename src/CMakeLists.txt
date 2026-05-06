add_subdirectory(core)
add_subdirectory(infra)
add_subdirectory(modules)

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

)