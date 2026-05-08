add_library(media
    "api/IMediaService.hpp"
    "api/MediaDto.hpp"
    "api/events/MediaStartedEvent.hpp"
    "api/events/PlayMediaRequestedEvent.hpp"

    "application/MediaService.cpp"

    "domain/Media.cpp"
    "domain/Category.cpp"
    "domain/IMediaRepository.hpp"

    "infraestructure/SqliteMediaRepository.cpp"

    "runtime/MediaManager.hpp"
    "runtime/MediaManager.cpp"
)

target_include_directories(media 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

target_link_libraries(media
    PUBLIC core
)