add_executable(app main.cpp)

target_sources(app PRIVATE
    "runtime/OverlayMediaController.hpp"
    "runtime/OverlayMediaController.cpp"
    "api/Event.cpp")

target_link_libraries(app PRIVATE omc CLI11::CLI11)

target_include_directories(
    app
    PRIVATE
    ${CMAKE_BINARY_DIR}/generated
)

add_dependencies(
    app
    frontend-build
)