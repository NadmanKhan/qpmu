
set(QPMU_DAQ "host.csv.file" CACHE STRING "DAQ backend: 'am335x.mcp3208.pru-shram' or 'host.csv.file'")
string(TOLOWER "${QPMU_DAQ}" QPMU_DAQ_NORMALIZED)

if(QPMU_DAQ_NORMALIZED STREQUAL "am335x.mcp3208.pru-shram")
    add_subdirectory(am335x.mcp3208.pru-shram)
elseif(QPMU_DAQ_NORMALIZED STREQUAL "host.csv.file")
    add_subdirectory(host.csv.file)
endif()
