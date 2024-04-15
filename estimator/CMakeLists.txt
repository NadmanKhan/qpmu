cmake_minimum_required(VERSION 3.14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(qpmu-estimator VERSION 0.1 LANGUAGES CXX)

add_subdirectory(../common CMAKE_CURRENT_BINARY_DIR/common) # qpmu-common
add_subdirectory(external/github.com/jurihock/sdft/cpp) # sdft-cpp

set(PROJECT_SOURCES
    src/estimator.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE qpmu-common
    PRIVATE fftw3
    PRIVATE sdft-cpp
)
