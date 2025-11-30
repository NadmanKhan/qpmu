include(ExternalProject)
include(GNUInstallDirs)

# ---- 1. Global Settings & Source Fetching ----
set(FFTW_INSTALL_DIR "${CMAKE_BINARY_DIR}/fftw_install")

ExternalProject_Add(fftw_source_fetch
    URL https://www.fftw.org/fftw-3.3.10.tar.gz
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
    DOWNLOAD_DIR      "${CMAKE_BINARY_DIR}/_deps"
    SOURCE_DIR        "${CMAKE_BINARY_DIR}/_deps/fftw-src"
)

ExternalProject_Get_Property(fftw_source_fetch SOURCE_DIR)
set(FFTW_SRC_DIR "${SOURCE_DIR}")

# ---- 2. Architecture & Optimization Detection ----
set(FFTW_COMMON_FLAGS
    --prefix=${FFTW_INSTALL_DIR}
    --disable-shared
    --enable-static
    --with-pic
    --enable-threads
)

find_package(OpenMP QUIET)

set(FFTW_CONFIGURE_ENV "")
set(FFTW_USE_OPENMP FALSE)

if(OpenMP_FOUND OR OpenMP_C_FOUND)
    message(STATUS "FFTW: OpenMP detected, C Compiler: ${CMAKE_C_COMPILER_ID}")
    message(STATUS "FFTW: OpenMP C flags: ${OpenMP_C_FLAGS}")

    if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
        set(FFTW_CONFIGURE_ENV
            CC=${CMAKE_C_COMPILER}
            "CFLAGS=${OpenMP_C_FLAGS}"
            "LDFLAGS=${OpenMP_C_FLAGS}"
        )
        set(FFTW_USE_OPENMP TRUE)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(FFTW_CONFIGURE_ENV
            CC=${CMAKE_C_COMPILER}
        )
        set(FFTW_USE_OPENMP TRUE)
    else()
        set(FFTW_USE_OPENMP TRUE)
    endif()
endif()

if(FFTW_USE_OPENMP)
    list(APPEND FFTW_COMMON_FLAGS --enable-openmp)
else()
    message(WARNING "FFTW: Building without OpenMP")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    set(FFTW_DOUBLE_SIMD "")
    set(FFTW_FLOAT_SIMD  "--enable-neon")

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64")
    list(APPEND FFTW_COMMON_FLAGS --enable-avx --enable-avx2 --enable-fma)
    set(FFTW_DOUBLE_SIMD --enable-sse2)
    set(FFTW_FLOAT_SIMD  --enable-sse --enable-sse2)

else()
    set(FFTW_DOUBLE_SIMD "")
    set(FFTW_FLOAT_SIMD  "")
endif()

# ---- 3. Builder Function ----
function(add_fftw_variant SUFFIX CONFIGURE_PRECISION_FLAG SIMD_FLAGS)

    set(TARGET_NAME "fftw3${SUFFIX}")
    set(BUILD_DIR   "${CMAKE_BINARY_DIR}/fftw_build_${TARGET_NAME}")

    set(LIB_FILENAME
        "${CMAKE_STATIC_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )

    set(LIB_PATH
        "${FFTW_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${LIB_FILENAME}"
    )

    if(EXISTS "${LIB_PATH}")
        message(STATUS "FFTW: ${TARGET_NAME} already built, skipping...")
    else()
        file(MAKE_DIRECTORY "${BUILD_DIR}")

        set(CURRENT_FLAGS ${FFTW_COMMON_FLAGS} ${SIMD_FLAGS})
        if(NOT "${CONFIGURE_PRECISION_FLAG}" STREQUAL "")
            list(APPEND CURRENT_FLAGS "--enable-${CONFIGURE_PRECISION_FLAG}")
        endif()

        message(STATUS "FFTW: Building ${TARGET_NAME} (Static)")

        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env
                    ${FFTW_CONFIGURE_ENV}
                    "${FFTW_SRC_DIR}/configure"
                    ${CURRENT_FLAGS}
            WORKING_DIRECTORY "${BUILD_DIR}"
            RESULT_VARIABLE configure_result
        )

        if(configure_result)
            message(FATAL_ERROR "FFTW ${TARGET_NAME} configure failed")
        endif()

        execute_process(
            COMMAND make -j8
            WORKING_DIRECTORY "${BUILD_DIR}"
            RESULT_VARIABLE build_result
        )

        if(build_result)
            message(FATAL_ERROR "FFTW ${TARGET_NAME} build failed")
        endif()

        execute_process(
            COMMAND make install
            WORKING_DIRECTORY "${BUILD_DIR}"
            RESULT_VARIABLE install_result
        )

        if(install_result)
            message(FATAL_ERROR "FFTW ${TARGET_NAME} install failed")
        endif()
    endif()

    set(INC_PATH
        "${FFTW_INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR}"
    )

    if(NOT TARGET FFTW::${TARGET_NAME})
        add_library(FFTW::${TARGET_NAME} STATIC IMPORTED GLOBAL)

        set_target_properties(FFTW::${TARGET_NAME} PROPERTIES
            IMPORTED_LOCATION "${LIB_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${INC_PATH}"
        )
    endif()

endfunction()

# ---- 4. Execute Builds ----
add_fftw_variant("" "" "${FFTW_DOUBLE_SIMD}")       # Double
add_fftw_variant("f" "float" "${FFTW_FLOAT_SIMD}") # Float

# Ensure download happens before any build attempt
add_dependencies(FFTW::fftw3 fftw_source_fetch)
add_dependencies(FFTW::fftw3f fftw_source_fetch)
