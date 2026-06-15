# C++ host library — CSV file replay (simulation / testing)
add_library(qpmu_daq STATIC
    reader.cpp
)

target_include_directories(qpmu_daq
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # for interface.hpp
)

target_compile_definitions(qpmu_daq PUBLIC QPMU_DAQ_HOST_CSV_FILE)

target_link_libraries(qpmu_daq PUBLIC qpmu)
