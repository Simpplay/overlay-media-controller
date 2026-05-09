add_library(media
    "api/IMediaService.hpp"
    "api/MediaDto.hpp"
    "api/events/MediaStartedEvent.hpp"
    "api/events/PlayMediaRequestedEvent.hpp"

    "application/MediaService.hpp"
    "application/MediaService.cpp"

    "domain/Media.hpp"
    "domain/Media.cpp"
    "domain/Category.hpp"
    "domain/Category.cpp"
    "domain/IMediaRepository.hpp"

    "infrastructure/SqliteMediaRepository.hpp"
    "infrastructure/SqliteMediaRepository.cpp"

    "runtime/MediaManager.hpp"
    "runtime/MediaManager.cpp"
)

target_include_directories(media 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

target_link_libraries(media
    PUBLIC core unofficial::sqlite3::sqlite3
)