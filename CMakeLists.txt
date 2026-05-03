cmake_minimum_required(VERSION 3.20)

project(overlay-media-controller
    VERSION 0.1.0
    DESCRIPTION "C++ overlay media controller with FFmpeg, SQLite, and HTTP API"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Dependencies (vcpkg style)
# ---------------------------------------------------------------------------

find_package(httplib CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------

add_executable(overlay-media-controller
    src/main.cpp
    src/database.cpp
    src/media_controller.cpp
    src/api_server.cpp
)

target_include_directories(overlay-media-controller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(overlay-media-controller PRIVATE
    httplib::httplib
    unofficial::sqlite3::sqlite3
    ${FFMPEG_LIBRARIES}
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

install(TARGETS overlay-media-controller
    RUNTIME DESTINATION bin
)