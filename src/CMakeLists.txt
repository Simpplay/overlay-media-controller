add_library(omc INTERFACE)

add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(modules)

target_link_libraries(omc
    INTERFACE
        app
        core
        modules
)