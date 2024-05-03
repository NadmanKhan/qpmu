set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
  Boost 1.74.0
  COMPONENTS program_options
  REQUIRED)

find_library(FFTW3_LIB fftw3)
find_path(FFTW3_INCLUDE_DIR fftw3.h)

add_executable(${PROJECT_NAME}-estimator)

# Add the "include/" directory to the include directories
target_include_directories(${PROJECT_NAME}-estimator
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(
  ${PROJECT_NAME}-estimator
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(
  ${PROJECT_NAME}-estimator
  PRIVATE fftw3
  PRIVATE fftw3f
  PRIVATE sdft-cpp
  PRIVATE Boost::program_options)
