enable_testing()

add_executable(EventBusTests
    core/event/EventBusTest.cpp
)

target_link_libraries(EventBusTests
    PRIVATE
        core
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(EventBusTests)