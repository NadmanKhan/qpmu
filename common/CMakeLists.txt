cmake_minimum_required(VERSION 3.14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/empty.cpp
)

add_library(qpmu-common STATIC
    ${SOURCES}
)

target_include_directories(qpmu-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)