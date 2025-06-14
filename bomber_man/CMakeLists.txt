cmake_minimum_required(VERSION 3.5)

project(bomber_man LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bomber_man main.cpp
    bfs_bomber_man.hpp
    bfs_bomber_man.cpp
    abstract_bomber_man.hpp 
    abstract_bomber_man.cpp
    dfs_bomber_man.cpp
    dfs_bomber_man.hpp
)
