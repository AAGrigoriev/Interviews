cmake_minimum_required(VERSION 3.10)

project(main)

find_package( OpenCV REQUIRED )

SET(CORE            controller)

SET(OPENCV_MODULE  opencv_module)

SET(SOURCE_CODE 
${CORE}/controller.cpp
${OPENCV_MODULE}/opencv_module.cpp
main.cpp
)

SET(CMAKE_CXX_STANDARD 17)


add_executable(main ${SOURCE_CODE})

target_include_directories(main PRIVATE 
                                            ${CMAKE_CURRENT_SOURCE_DIR}/opencv_module
                                            ${CMAKE_CURRENT_SOURCE_DIR}/controller
                                            )

target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(main ${OpenCV_LIBS} )