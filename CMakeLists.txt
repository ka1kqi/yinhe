cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME yinhe)
project(${PROJECT_NAME})

#requires c++17 or higher
set(CMAKE_CXX_STANDARD_ 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++17")

find_package(Threads REQUIRED)

# MAKE FROM BUILD DIRECTORY
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
message(STATUS)
add_subdirectory(src/common)
add_subdirectory(src/engine)

# Tests
add_executable(test_orderbook
    src/tests/test_orderbook.cpp
    src/engine/order.cpp
    src/engine/orderbook.cpp
)
target_include_directories(test_orderbook PRIVATE
    src/common
    src/engine
    src/engine/tradeUtils
)
target_link_libraries(test_orderbook PRIVATE Threads::Threads)

add_executable(test_orderbook_stress
    src/tests/test_orderbook_stress.cpp
    src/engine/order.cpp
    src/engine/orderbook.cpp
)
target_include_directories(test_orderbook_stress PRIVATE
    src/common
    src/engine
    src/engine/tradeUtils
)
target_link_libraries(test_orderbook_stress PRIVATE Threads::Threads)

add_executable(bench_orderbook
    src/tests/bench_orderbook.cpp
    src/engine/order.cpp
    src/engine/orderbook.cpp
)
target_include_directories(bench_orderbook PRIVATE
    src/common
    src/engine
    src/engine/tradeUtils
)
target_link_libraries(bench_orderbook PRIVATE Threads::Threads)