add_library(threading
    "api/ConcurrentQueue.hpp"
    "api/ThreadPool.hpp"
    "application/ConcurrentQueue.cpp"
    "application/ThreadPool.cpp"
)

target_include_directories(threading 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

