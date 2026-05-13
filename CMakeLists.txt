cmake_minimum_required(VERSION 3.20)

project(
    overlay-media-controller
    VERSION 1.0.0
    DESCRIPTION "Overlay Media Controller"
    LANGUAGES CXX
)

# =========================================================
# Global configuration
# =========================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (MINGW)
    message(FATAL_ERROR
        "MinGW/GCC is not supported. Use MSVC.")
endif()

# =========================================================
# Frontend configuration
# =========================================================

set(FRONTEND_DIR ${CMAKE_SOURCE_DIR}/frontend)
set(FRONTEND_DIST_DIR ${FRONTEND_DIR}/dist)

# Runtime folder:
# bin/
#   app.exe
#   *.dll
#   web/
#
set(WEB_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/web)

# =========================================================
# Dependencies
# =========================================================

find_package(unofficial-webview2 CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

find_package(unofficial-sqlite3 CONFIG QUIET)
find_package(GTest CONFIG QUIET)

# =========================================================
# Frontend build target
# =========================================================

find_program(NPM_EXECUTABLE npm REQUIRED)

add_custom_target(frontend-build ALL
    COMMAND ${NPM_EXECUTABLE} install
    COMMAND ${NPM_EXECUTABLE} run build
    WORKING_DIRECTORY ${FRONTEND_DIR}
    COMMENT "Building frontend with Vite"
)

# =========================================================
# Backend modules
# =========================================================

add_subdirectory(src)

# =========================================================
# Copy frontend to runtime folder
# =========================================================

add_custom_command(
    TARGET frontend-build POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WEB_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${FRONTEND_DIST_DIR}
        ${WEB_OUTPUT_DIR}
    COMMENT "Copying frontend build to runtime web folder"
)

# =========================================================
# Tests
# =========================================================

if (GTest_FOUND)
    enable_testing()
    add_subdirectory(tests)
else ()
    message(STATUS "GTest not found: skipping tests target configuration.")
endif ()

# =========================================================
# Packaging helper
# =========================================================

add_custom_target(package-portable
    COMMAND ${CMAKE_COMMAND} -E tar "cfv"
        ${CMAKE_BINARY_DIR}/overlay-media-controller-${PROJECT_VERSION}-win64.zip
        --format=zip
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Creating portable ZIP package"
)