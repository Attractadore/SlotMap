if (BUILD_TESTING)
find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(TestSlotMap TestSlotMap.cpp)
target_link_libraries(TestSlotMap GTest::gtest_main Attractadore::SlotMap)
set_target_properties(TestSlotMap PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED TRUE
    CXX_EXTENSIONS OFF)

gtest_discover_tests(TestSlotMap)
endif()
