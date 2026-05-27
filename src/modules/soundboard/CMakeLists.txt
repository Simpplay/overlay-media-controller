add_library(soundboard
    "api/ISoundboardService.hpp"
    "api/SoundboardDto.hpp"

    "application/SoundboardService.hpp"
    "application/SoundboardService.cpp"

    "domain/ISoundboardPlayer.hpp"
    "domain/AudioDevice.hpp"

    "infrastructure/WasapiPlayer.hpp"
    "infrastructure/WasapiPlayer.cpp"
)

target_include_directories(soundboard 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/application
        ${CMAKE_CURRENT_SOURCE_DIR}/domain
        ${CMAKE_CURRENT_SOURCE_DIR}/infraestructure
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)

target_link_libraries(soundboard
    PUBLIC core
)