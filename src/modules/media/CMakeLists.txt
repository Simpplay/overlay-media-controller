add_library(media
    MediaManager.cpp
 "MediaPlayer.hpp")

target_include_directories(media PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(media
    PUBLIC core
)