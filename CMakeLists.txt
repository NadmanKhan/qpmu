cmake_minimum_required(VERSION 3.16)

set(QPMU_FLOAT_TYPE float)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(QPMU_PROJECT_NAME qpmu)

if(${QPMU_FLOAT_TYPE} STREQUAL "double")
  message(STATUS "Using double as the floating point type")
elseif(${QPMU_FLOAT_TYPE} STREQUAL "long double")
  message(STATUS "Using long double as the floating point type")
else()
  message(STATUS "Using float as the floating point type")
endif()

project(
  ${QPMU_PROJECT_NAME}
  VERSION 0.1
  LANGUAGES CXX)

# Enable compiler warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

enable_testing()

# Find Qt
find_package(
  QT NAMES Qt6 Qt5
  COMPONENTS Core Test Widgets Charts Network
  REQUIRED)
find_package(
  Qt${QT_VERSION_MAJOR}
  COMPONENTS Core Test Widgets Charts Network
  REQUIRED)

# Find FFTW
find_library(FFTW3_LIB fftw3)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# Add the "common" library and link it to the project
set(COMMON_LIB ${PROJECT_NAME}-common)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
target_compile_definitions(
  ${COMMON_LIB}
  PUBLIC FLOAT_TYPE=${QPMU_FLOAT_TYPE}
  PUBLIC PROJECT_ORG_NAME="cps-lab-nsu"
  PUBLIC PROJECT_APP_NAME="${PROJECT_NAME}"
  PUBLIC PROJECT_APP_DISPLAY_NAME="QPMU"
  PUBLIC PROJECT_ADC_BITS=12)
link_libraries(${COMMON_LIB})

# Add the remaining subdirectories
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/estimation)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/app)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/estimation-new)
