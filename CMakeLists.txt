cmake_minimum_required(VERSION 3.10)

project(
  qpmu
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compiler warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external)

set(COMMON_LIB ${PROJECT_NAME}-common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(${COMMON_LIB} PUBLIC FLOAT_TYPE=double)
link_libraries(${COMMON_LIB})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/simulator)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/estimator)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gui)
