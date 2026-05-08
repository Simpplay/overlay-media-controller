add_library(types
    "Constants.hpp"
)

target_include_directories(types PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(types
    PUBLIC core
)