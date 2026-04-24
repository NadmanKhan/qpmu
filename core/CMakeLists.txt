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

add_executable(qpmu_service
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pmu.cpp
)

target_include_directories(qpmu_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/daq)

find_package(Threads REQUIRED)

target_link_libraries(qpmu_service PRIVATE qpmu open-c37118 Threads::Threads)

if(QPMU_DAQ_BBB)
    target_link_libraries(qpmu_service PRIVATE qpmu_daq_bbb)
    target_compile_definitions(qpmu_service PRIVATE QPMU_DAQ_BBB)
endif()

if(QPMU_DAQ_SIM)
    target_link_libraries(qpmu_service PRIVATE qpmu_daq_sim)
    target_compile_definitions(qpmu_service PRIVATE QPMU_DAQ_SIM)
endif()
