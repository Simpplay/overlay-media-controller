cmake_minimum_required(VERSION 3.20)

project(overlay-media-controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# vcpkg deps
find_package(httplib CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# módulos
add_subdirectory(src)
add_subdirectory(tests)