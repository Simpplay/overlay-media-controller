add_library(ui
    "UiManager.cpp" "UiManager.hpp" "UiTypes.hpp" "UiWindow.hpp" "windows/MediaWindow.hpp" "windows/TestWindow.hpp" "windows/WebViewWindow.hpp")

target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ui
    PUBLIC core infra
)