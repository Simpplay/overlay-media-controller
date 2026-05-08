add_library(threading
    "api/ConcurrentQueue.hpp"
    "api/ThreadPool.hpp"
    "application/ConcurrentQueue.cpp"
    "application/ThreadPool.cpp"
)

target_include_directories(threading 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/api
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/application
)

target_link_libraries(threading
    PUBLIC core
)