cmake_minimum_required(VERSION 3.10)

project(main)

SET(Commander "Commander")
SET(Haffman "Haffman")
SET(Option "option")


SET(SOURCE 
main.cpp
${Commander}/commander.cpp
${Haffman}/haffman.cpp
${Haffman}/haffman_decode.cpp
${Haffman}/haffman_encode.cpp
${Haffman}/haffman_node.cpp
${Option}/option.cpp 
${Option}/BitReader.cpp
${Option}/BitWriter.cpp)

add_executable(main ${SOURCE})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})