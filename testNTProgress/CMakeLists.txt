cmake_minimum_required(VERSION 3.10)

project(main)

SET(CALC calculator)
SET(CONV converter)
SET(OPT  option)

SET(SRC 
main.cpp 
${CONV}/Converter.cpp 
${CALC}/Calculator.cpp)

add_executable(testNTProgress ${SRC})

target_include_directories(testNTProgress PRIVATE 
                                            ${CMAKE_CURRENT_SOURCE_DIR}/calculator
                                            ${CMAKE_CURRENT_SOURCE_DIR}/converter
                                            ${CMAKE_CURRENT_SOURCE_DIR}/opt
                                            )

add_subdirectory(test)

enable_testing()