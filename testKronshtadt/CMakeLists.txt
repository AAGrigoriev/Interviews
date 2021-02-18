cmake_minimum_required(VERSION 3.10)

project (testKron)

add_executable(testKron main.cpp test.cpp)

include_directories(${CMAKE_SOURCE_DIR})