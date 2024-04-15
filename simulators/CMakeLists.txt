cmake_minimum_required(VERSION 3.14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(qpmu-simulator VERSION 0.1 LANGUAGES CXX)

add_subdirectory(../common CMAKE_CURRENT_BINARY_DIR/common)

set(ADC_SIMULATOR ${PROJECT_NAME}-adc)

set(ADC_SIMULATOR_SOURCES
    src/adc_simulator.cpp
)

add_executable(${ADC_SIMULATOR} ${ADC_SIMULATOR_SOURCES})

target_link_libraries(${ADC_SIMULATOR} PRIVATE qpmu-common)


