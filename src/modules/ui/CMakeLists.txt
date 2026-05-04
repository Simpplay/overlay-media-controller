add_library(ui
    UiManager.cpp
 "UiWindow.cpp" )

target_include_directories(ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ui
    PUBLIC core infra
)