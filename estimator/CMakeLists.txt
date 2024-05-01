# qpmu-common
if(NOT TARGET ${COMMON_LIB})
  add_subdirectory(${COMMON_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${COMMON_LIB})
endif()

add_executable(${PROJECT_NAME}-estimator)

target_sources(${PROJECT_NAME}-estimator
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/estimator.cpp)

target_link_libraries(
  ${PROJECT_NAME}-estimator
  PRIVATE ${COMMON_LIB}
  PRIVATE fftw3
  PRIVATE fftw3f
  PRIVATE sdft-cpp)
