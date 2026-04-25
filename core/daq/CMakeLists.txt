
set(QPMU_DAQ "sim" CACHE STRING "Data acquisition backend to build (options: 'bbb', 'sim')")
string(TOLOWER "${QPMU_DAQ}" QPMU_DAQ_NORMALIZED)

if(QPMU_DAQ_NORMALIZED STREQUAL "bbb")
    add_subdirectory(bbb-debian)
elseif(QPMU_DAQ_NORMALIZED STREQUAL "sim")
    add_subdirectory(simulation)
endif()
