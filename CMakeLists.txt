cmake_minimum_required(VERSION 3.24)
project(
  "qpmu"
  VERSION 0.1
  LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)
    enable_testing()
endif()


# Enable compiler warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/core)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gui)
