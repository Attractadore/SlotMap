if (NOT BUILD_TESTING)
  return()
endif()

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(TestDenseSlotMap TestDenseSlotMap.cpp)
target_link_libraries(TestDenseSlotMap GTest::gtest_main Attractadore::SlotMap)

gtest_discover_tests(TestDenseSlotMap)
