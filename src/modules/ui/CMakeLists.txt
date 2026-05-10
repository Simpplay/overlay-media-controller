add_library(ui
    "api/UiDto.hpp"
    "api/IUiService.hpp"
    "api/events/WindowOpenRequestedEvent.hpp"

    "application/UiService.hpp"
    "application/UiService.cpp"
    "application/windows/TestWindow.hpp"
    "application/windows/WebViewWindow.hpp"

    "domain/RenderTypes.hpp"
    "domain/UiWindow.hpp"
    "domain/UiRepository.hpp"

    "infrastructure/HitTest.hpp"
    "infrastructure/WebViewRenderer.hpp"
    "infrastructure/WebViewRenderer.cpp"
    "infrastructure/Win32Renderer.hpp"
    "infrastructure/Win32Renderer.cpp"

    "runtime/UiManager.hpp"
    "runtime/UiManager.cpp"
)

target_include_directories(ui 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

target_link_libraries(ui
    PUBLIC core
)

if (WIN32)
    target_link_libraries(ui
        PUBLIC
            d3d11
            dxgi
            dcomp
            unofficial::webview2::webview2
    )
endif ()
