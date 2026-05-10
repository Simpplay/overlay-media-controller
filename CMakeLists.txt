cmake_minimum_required(VERSION 3.20)

project(overlay-media-controller LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional external dependencies.
find_package(unofficial-webview2 CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG QUIET)
find_package(GTest CONFIG QUIET)

# módulos
add_subdirectory(src)

if (GTest_FOUND)
    add_subdirectory(tests)
else ()
    message(STATUS "GTest not found: skipping tests target configuration.")
endif ()
