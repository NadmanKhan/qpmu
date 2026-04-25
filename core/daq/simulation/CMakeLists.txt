# C++ simulation DAQ library
add_library(qpmu_daq_sim STATIC
    sim_daq_reader.cpp
)

target_include_directories(qpmu_daq_sim
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # for interface.hpp
)

target_compile_definitions(qpmu_daq_sim PUBLIC QPMU_DAQ_SIM)

target_link_libraries(qpmu_daq_sim PUBLIC qpmu)
