add_library(storage
    StorageManager.cpp
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(storage
    PUBLIC core
)