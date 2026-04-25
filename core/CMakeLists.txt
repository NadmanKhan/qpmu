# Core module

add_library(qpmu INTERFACE)
target_include_directories(qpmu INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING)
add_executable(qpmu_unit_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/include/qpmu/algorithms/minmax_heap.test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/qpmu/containers/sliding_mmm.test.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/qpmu/algorithms/linear_algebra.test.cpp
    )

    target_link_libraries(qpmu_unit_tests
        PRIVATE
        qpmu
        GTest::gtest
        GTest::gtest_main
    )

    gtest_discover_tests(qpmu_unit_tests)
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/daq)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/Open-C37.118 external/Open-C37.118)

add_executable(qpmu_service
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pmu.cpp
)

target_include_directories(qpmu_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/daq)

find_package(Threads REQUIRED)

target_link_libraries(qpmu_service PRIVATE qpmu open-c37118 Threads::Threads)

string(TOLOWER "${QPMU_DAQ}" _qpmu_daq_lower)
if(_qpmu_daq_lower STREQUAL "bbb")
    target_link_libraries(qpmu_service PRIVATE qpmu_daq_bbb)
elseif(_qpmu_daq_lower STREQUAL "sim")
    target_link_libraries(qpmu_service PRIVATE qpmu_daq_sim)
endif()
