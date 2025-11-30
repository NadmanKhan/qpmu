cmake_minimum_required(VERSION 3.16)
project(
  "qpmu"
  VERSION 0.1
  LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(USE_DOUBLE "Whether to use double precision floating point" OFF)

# Include custom CMake modules
include(cmake/FFTW.cmake)
include(cmake/OpenC37118.cmake)
include(cmake/GoogleTest.cmake)
include(cmake/Qt.cmake)

# Enable compiler warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add the "common" library and link it to the project
set(COMMON_LIB ${PROJECT_NAME}-common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(
  ${COMMON_LIB}
  PUBLIC PROJECT_ORG_NAME="cps-lab-nsu"
  PUBLIC PROJECT_APP_NAME="${PROJECT_NAME}"
  PUBLIC PROJECT_APP_DISPLAY_NAME="QPMU"
)
if (USE_DOUBLE)
  target_compile_definitions(${COMMON_LIB} PUBLIC USE_DOUBLE)
endif()
link_libraries(${COMMON_LIB})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/estimation)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/app)
