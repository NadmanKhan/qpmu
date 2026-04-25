# C++ host library
add_library(qpmu_daq_bbb STATIC
    host/bbb_daq_reader.cpp
)

target_include_directories(qpmu_daq_bbb
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # for interface.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}      # for shared/buffer.h
)

target_compile_definitions(qpmu_daq_bbb PUBLIC QPMU_DAQ_BBB)

target_link_libraries(qpmu_daq_bbb PUBLIC qpmu)

# --- PRU0 firmware (custom target, only builds when explicitly requested) ---

set(PRU_CGT "/usr/share/ti/cgt-pru" CACHE PATH "TI PRU Code Generation Tools")
set(PRU_SSP "${CMAKE_SOURCE_DIR}/external/pru-software-support-package" CACHE PATH "TI PRU Software Support Package")

set(PRU0_SRC ${CMAKE_CURRENT_SOURCE_DIR}/pru0/pru0_main.c)
set(PRU0_OBJ ${CMAKE_CURRENT_BINARY_DIR}/pru0/pru0_main.object)
set(PRU0_OUT ${CMAKE_CURRENT_BINARY_DIR}/pru0/pru0.out)

add_custom_command(OUTPUT ${PRU0_OBJ}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/pru0
    COMMAND ${PRU_CGT}/bin/clpru
        --include_path=${PRU_CGT}/include
        --include_path=${PRU_SSP}/include
        --include_path=${PRU_SSP}/include/am335x
        --include_path=${CMAKE_CURRENT_SOURCE_DIR}/shared
        -v3 -O2 --display_error_number --endian=little --hardware_mac=on
        --obj_directory=${CMAKE_CURRENT_BINARY_DIR}/pru0
        --pp_directory=${CMAKE_CURRENT_BINARY_DIR}/pru0
        --asm_directory=${CMAKE_CURRENT_BINARY_DIR}/pru0
        -fe ${PRU0_OBJ} ${PRU0_SRC}
    DEPENDS ${PRU0_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/shared/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pru0/resource_table_empty.h
    COMMENT "Compiling PRU0 firmware"
)

add_custom_command(OUTPUT ${PRU0_OUT}
    COMMAND ${PRU_CGT}/bin/clpru -v3 -z
        -i${PRU_CGT}/lib -i${PRU_CGT}/include
        --reread_libs --warn_sections
        --stack_size=0x100 --heap_size=0x100
        -o ${PRU0_OUT} ${PRU0_OBJ}
        -m${CMAKE_CURRENT_BINARY_DIR}/pru0/pru0.map
        ${CMAKE_CURRENT_SOURCE_DIR}/pru0/pru0.cmd
        --library=libc.a
    DEPENDS ${PRU0_OBJ}
    COMMENT "Linking PRU0 firmware"
)

add_custom_target(pru0_firmware DEPENDS ${PRU0_OUT})

# --- Device tree overlay (custom target) ---

set(DT_INCLUDE "/opt/source/dtb-6.18.x/include" CACHE PATH "Device tree include path")

set(OVERLAY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/overlay/BB-PRU-MCP3208-00A0.dtso)
set(OVERLAY_PP  ${CMAKE_CURRENT_BINARY_DIR}/overlay/BB-PRU-MCP3208-00A0.pp.dts)
set(OVERLAY_BIN ${CMAKE_CURRENT_BINARY_DIR}/overlay/BB-PRU-MCP3208-00A0.dtbo)

add_custom_command(OUTPUT ${OVERLAY_BIN}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/overlay
    COMMAND cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp
        -I${DT_INCLUDE} ${OVERLAY_SRC} -o ${OVERLAY_PP}
    COMMAND dtc -I dts -O dtb -@ -o ${OVERLAY_BIN} ${OVERLAY_PP}
    DEPENDS ${OVERLAY_SRC}
    COMMENT "Compiling device tree overlay"
)

add_custom_target(bbb_overlay DEPENDS ${OVERLAY_BIN})

# --- Deploy / install targets ---

set(RPROC0 "/sys/class/remoteproc/remoteproc1")

add_custom_target(deploy_pru0
    COMMAND cp ${PRU0_OUT} /lib/firmware/pru0_mcp3208
    COMMAND bash -c "echo stop > ${RPROC0}/state 2>/dev/null || true"
    COMMAND bash -c "echo pru0_mcp3208 > ${RPROC0}/firmware"
    COMMAND bash -c "echo start > ${RPROC0}/state"
    DEPENDS pru0_firmware
    COMMENT "Deploying PRU0 firmware"
)

add_custom_target(install_overlay
    COMMAND cp ${OVERLAY_BIN} /boot/dtbs/$\(uname\ -r\)/overlays/
    DEPENDS bbb_overlay
    COMMENT "Installing device tree overlay to /boot"
)
