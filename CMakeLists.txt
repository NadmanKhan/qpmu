cmake_minimum_required(VERSION 3.10)

project(
  qpmu
  VERSION 0.1
  LANGUAGES CXX)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(COMMON_LIB ${PROJECT_NAME}-common)

add_subdirectory(${EXTERNAL_DIR})

add_compile_definitions(FLOAT_TYPE=float)

add_subdirectory(${COMMON_DIR})

link_libraries(${COMMON_LIB})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/simulator)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/estimator)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gui)

target_compile_definitions(${COMMON_LIB} PUBLIC FLOAT_TYPE=float)
