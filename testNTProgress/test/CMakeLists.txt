cmake_minimum_required(VERSION 3.10)

project(googltest)

find_package(GTest REQUIRED)

add_executable(gtest_ntProgress
    main_test.cpp  
    ${CMAKE_CURRENT_SOURCE_DIR}/../calculator/Calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../converter/Converter.cpp
)

target_link_libraries(gtest_ntProgress PRIVATE ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)

target_include_directories(gtest_ntProgress PRIVATE 
${GTEST_INCLUDE_DIRS}
${CMAKE_CURRENT_SOURCE_DIR}/../calculator
${CMAKE_CURRENT_SOURCE_DIR}/../converter
)

set_target_properties(gtest_ntProgress PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(gtest_ntProgress gtest_ntProgress) 