cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME yinhe)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD_ 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

##m MAKE FROM BUILD DIRECTORY
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} src/main.cpp)
message(STATUS )
add_subdirectory(src/common)
add_subdirectory(src/engine)