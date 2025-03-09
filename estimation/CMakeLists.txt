add_library(${PROJECT_NAME}-estimation STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp)

find_package(FFTW3 CONFIG REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(FFTW3l CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME}-estimation
                         PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                         PRIVATE "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

target_link_libraries(
  ${PROJECT_NAME}-estimation
  PUBLIC FFTW3::fftw3
  PUBLIC FFTW3::fftw3f
  PUBLIC FFTW3::fftw3l)
