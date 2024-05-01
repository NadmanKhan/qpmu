add_executable(${PROJECT_NAME}-estimator)

target_sources(${PROJECT_NAME}-estimator
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp)

target_link_libraries(
  ${PROJECT_NAME}-estimator
  PRIVATE fftw3
  PRIVATE fftw3f
  PRIVATE sdft-cpp)
