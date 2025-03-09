cmake_minimum_required(VERSION 3.20)

# Set vcpkg toolchain file before project()
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# Set Qt path from vcpkg
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")

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
  QT REQUIRED 
  NAMES Qt6 Qt5
)
find_package(
  Qt${QT_VERSION_MAJOR} REQUIRED
  COMPONENTS Core Gui Test Widgets Charts Network
)

# Find FFTW
find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)

# Add FFTW include directories to all targets
include_directories(${FFTW3_INCLUDE_DIRS})

# Find GTest
find_package(GTest REQUIRED)

# Find Open-C37.118
find_package(open-c37118 CONFIG REQUIRED)

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
