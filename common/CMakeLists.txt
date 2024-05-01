add_library(${COMMON_LIB} STATIC)

target_include_directories(${COMMON_LIB}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(
  ${COMMON_LIB}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/adc_sample.cpp
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/measurement.cpp)
