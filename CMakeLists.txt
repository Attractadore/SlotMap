cmake_minimum_required(VERSION 3.12)
project(SlotMap LANGUAGES CXX)

add_library(SlotMap INTERFACE include/SlotMap/SlotMap.hpp)
target_include_directories(SlotMap INTERFACE include)
target_compile_features(SlotMap INTERFACE cxx_std_20)

add_library(SlotMap::SlotMap ALIAS SlotMap)

enable_testing()
add_subdirectory(test)
