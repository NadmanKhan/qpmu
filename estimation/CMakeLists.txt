add_library(${PROJECT_NAME}-estimation STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp)

target_include_directories(${PROJECT_NAME}-estimation
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(
  ${PROJECT_NAME}-estimation
  PUBLIC fftw3
  PUBLIC fftw3f)
