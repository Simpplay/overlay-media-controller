add_subdirectory(core)
add_subdirectory(modules)

add_executable(app
    app/main.cpp
)

target_link_libraries(app
    PRIVATE
        core
        modules
)