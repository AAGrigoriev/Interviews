cmake_minimum_required(VERSION 3.5)

project(simple_hash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

add_executable(simple_hash main.cpp)

target_link_libraries(simple_hash OpenSSL::SSL)
