cmake_minimum_required(VERSION 3.10)

project(main)

SET(CORE            controller)
SET(OPEN_CV_MODULE  openc_cv_module)

SET(SOURCE_CODE 
${CORE}/controller.cpp
${OPEN_CV_MODULE}/open_cv.cpp
main.cpp
)

add_executable(main ${SOURCE_CODE})

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
