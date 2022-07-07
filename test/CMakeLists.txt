if (BUILD_TESTING)
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(TestSlotMap TestSlotMap.cpp)
target_link_libraries(TestSlotMap GTest::gtest_main Attractadore::SlotMap)

gtest_discover_tests(TestSlotMap)
endif()
