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
# Dependencies
# ---------------------------------------------------------------------------

find_package(PkgConfig REQUIRED)

# FFmpeg (libavcodec, libavformat, libavutil, libswscale)
pkg_check_modules(AVCODEC  REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL   REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(SWSCALE  REQUIRED IMPORTED_TARGET libswscale)

# SQLite3
find_package(SQLite3 REQUIRED)

# cpp-httplib
pkg_check_modules(HTTPLIB REQUIRED IMPORTED_TARGET cpp-httplib)

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
)

target_link_libraries(overlay-media-controller PRIVATE
    PkgConfig::AVCODEC
    PkgConfig::AVFORMAT
    PkgConfig::AVUTIL
    PkgConfig::SWSCALE
    SQLite::SQLite3
    PkgConfig::HTTPLIB
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------

install(TARGETS overlay-media-controller
    RUNTIME DESTINATION bin
)
