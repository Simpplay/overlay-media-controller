add_executable(app main.cpp)

target_sources(app PRIVATE
    "runtime/OverlayMediaController.hpp"
    "runtime/OverlayMediaController.cpp"
    "api/Event.cpp"
)

target_link_libraries(app PRIVATE omc)

add_dependencies(
    app
    frontend-build
)