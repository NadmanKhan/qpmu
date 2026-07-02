# C++ host library — AM335x + MCP3208 ADC via PRU shared RAM
add_library(qpmu_daq STATIC
    host/reader.cpp
)

target_include_directories(qpmu_daq
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..   # for interface.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}      # for shared/buffer.h
)

target_compile_definitions(qpmu_daq PUBLIC QPMU_DAQ_AM335X_MCP3208_PRU_SHRAM)

target_link_libraries(qpmu_daq PUBLIC qpmu)

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

# --- Device tree overlays (custom targets) ---

set(DT_INCLUDE "/opt/source/dtb-6.18.x/include" CACHE PATH "Device tree include path")

function(add_dtso_overlay NAME)
    set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/overlay/${NAME}.dtso)
    set(PP  ${CMAKE_CURRENT_BINARY_DIR}/overlay/${NAME}.pp.dts)
    set(BIN ${CMAKE_CURRENT_BINARY_DIR}/overlay/${NAME}.dtbo)

    add_custom_command(OUTPUT ${BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/overlay
        COMMAND cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp
            -I${DT_INCLUDE} ${SRC} -o ${PP}
        COMMAND dtc -I dts -O dtb -@ -o ${BIN} ${PP}
        DEPENDS ${SRC}
        COMMENT "Compiling overlay ${NAME}"
    )
    add_custom_target(${NAME} DEPENDS ${BIN})
endfunction()

add_dtso_overlay(BB-PRU-MCP3208-00A0)
add_dtso_overlay(BB-UART1-00A0)
add_dtso_overlay(BB-PPS-00A0)

add_custom_target(bbb_overlays DEPENDS BB-PRU-MCP3208-00A0 BB-UART1-00A0 BB-PPS-00A0)

# --- Deploy / install targets ---

add_custom_target(deploy_pru0
    COMMAND cp ${PRU0_OUT} /lib/firmware/pru0_mcp3208
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/start-pru.sh
    DEPENDS pru0_firmware
    COMMENT "Deploying PRU0 firmware"
)

add_custom_target(install_overlays
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/overlay/BB-PRU-MCP3208-00A0.dtbo /boot/dtbs/$\(uname\ -r\)/overlays/
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/overlay/BB-UART1-00A0.dtbo      /boot/dtbs/$\(uname\ -r\)/overlays/
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/overlay/BB-PPS-00A0.dtbo        /boot/dtbs/$\(uname\ -r\)/overlays/
    DEPENDS bbb_overlays
    COMMENT "Installing device tree overlays to /boot"
)

# --- Shared RAM test tool (only builds on ARM / BBB) ---

add_executable(test_shram EXCLUDE_FROM_ALL tools/test_shram.c)
target_include_directories(test_shram PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(config_pru_pins EXCLUDE_FROM_ALL tools/config_pru_pins.c)
