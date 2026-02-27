target_include_directories(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/enums.hpp)
target_sources(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/types.hpp)
target_sources(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/math.hpp)
target_sources(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/orderLog.hpp)
target_sources(yinhe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/SPSCQueue.hpp)