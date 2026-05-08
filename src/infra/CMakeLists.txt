set(INFRA_SOURCES
    "InfraStub.cpp"
    "window/Win32Renderer.hpp"
    "window/WebViewRenderer.hpp"
)

if (WIN32)
    list(APPEND INFRA_SOURCES
        "window/Win32Renderer.cpp"
        "window/WebViewRenderer.cpp"
    )
endif ()

add_library(infra ${INFRA_SOURCES})

target_include_directories(infra PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(infra
    PUBLIC
        core
)

if (WIN32)
    target_link_libraries(infra
        PUBLIC
            d3d11
            dxgi
            dcomp
            unofficial::webview2::webview2
    )
endif ()
