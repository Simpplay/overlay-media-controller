add_executable(app main.cpp resources.rc)

if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(app PROPERTIES WIN32_EXECUTABLE ON)
    target_link_options(app PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

target_sources(app PRIVATE
    "resources.h"
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