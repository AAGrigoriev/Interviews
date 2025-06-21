cmake_minimum_required(VERSION 3.5)

project(bomber_man)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bomber_man main.cpp
    bfs_bomber_man.hpp
    bfs_bomber_man.cpp
    abstract_resolver.hpp
    abstract_resolver.cpp
    dfs_bomber_man.cpp
    dfs_bomber_man.hpp
    island_counter.hpp
    island_counter.cpp
    plumber_resolver.hpp
    plumber_resolver.cpp
)
