option(QPMU_DAQ_BBB "Build BeagleBone Black / Debian DAQ backend" OFF)
option(QPMU_DAQ_SIM "Build simulation DAQ backend (replays recorded CSV)" OFF)

if(QPMU_DAQ_BBB)
    add_subdirectory(bbb-debian)
endif()

if(QPMU_DAQ_SIM)
    add_subdirectory(simulation)
endif()
