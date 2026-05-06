add_library(shared
    threading/ConcurrentQueue.hpp
 "threading/ConcurrentQueue.cpp" "threading/ThreadPool.cpp")

target_include_directories(shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(shared
    PUBLIC core
)