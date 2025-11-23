include(FetchContent)
include(GNUInstallDirs)

# ---- 1. Global Settings & Source Fetching ----
# Global paths
set(FFTW_INSTALL_DIR "${CMAKE_BINARY_DIR}/fftw_install") # Shared install for both variants

# Pre-download the source using a dedicated FetchContent target
FetchContent_Declare(fftw_source_fetch
    URL                         "https://www.fftw.org/fftw-3.3.10.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP  TRUE
)
FetchContent_MakeAvailable(fftw_source_fetch)
set(FFTW_SRC_DIR ${fftw_source_fetch_SOURCE_DIR})

# ---- 2. Architecture & Optimization Detection ----
set(FFTW_COMMON_FLAGS
    --prefix=${FFTW_INSTALL_DIR}
    --disable-shared
    --enable-static
    --with-pic # Important for static libs linked into shared libs
    --enable-threads
)

# Try to detect OpenMP support
find_package(OpenMP QUIET)

set(FFTW_CONFIGURE_ENV "")
set(FFTW_USE_OPENMP FALSE)

if(OpenMP_FOUND OR OpenMP_C_FOUND)
    message(STATUS "FFTW: OpenMP detected, C Compiler: ${CMAKE_C_COMPILER_ID}")
    message(STATUS "FFTW: OpenMP C flags: ${OpenMP_C_FLAGS}")

    # Pass OpenMP flags to configure script
    if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
        # On macOS with Clang, we need to explicitly set the flags
        set(FFTW_CONFIGURE_ENV
            CC=${CMAKE_C_COMPILER}
            "CFLAGS=${OpenMP_C_FLAGS}"
            "LDFLAGS=${OpenMP_C_FLAGS}"
        )
        set(FFTW_USE_OPENMP TRUE)
        message(STATUS "FFTW: Building with OpenMP support (libomp)")
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
        # GCC has built-in OpenMP support
        set(FFTW_CONFIGURE_ENV
            CC=${CMAKE_C_COMPILER}
        )
        set(FFTW_USE_OPENMP TRUE)
        message(STATUS "FFTW: Building with OpenMP support (GCC)")
    else()
        message(WARNING "FFTW: OpenMP found but compiler not explicitly supported, will try anyway")
        set(FFTW_USE_OPENMP TRUE)
    endif()
endif()

if(NOT FFTW_USE_OPENMP)
    message(WARNING "FFTW: Building without OpenMP (performance will be reduced)")
    message(WARNING "       On macOS, install libomp: brew install libomp")
    message(WARNING "       Or use GCC: brew install gcc and set CC/CXX environment variables")
else()
    list(APPEND FFTW_COMMON_FLAGS --enable-openmp)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    # NEON only works with single precision (float)
    set(FFTW_DOUBLE_SIMD "")
    set(FFTW_FLOAT_SIMD  "--enable-neon")

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|x86_64|AMD64")
    list(APPEND FFTW_COMMON_FLAGS --enable-avx --enable-avx2 --enable-fma)
    set(FFTW_DOUBLE_SIMD --enable-sse2)
    set(FFTW_FLOAT_SIMD  --enable-sse --enable-sse2)

else()
    # Fallback for unknown architectures - use basic optimizations
    set(FFTW_DOUBLE_SIMD "")
    set(FFTW_FLOAT_SIMD  "")
    message(STATUS "FFTW: Unknown processor architecture '${CMAKE_SYSTEM_PROCESSOR}', using generic build")
endif()

# ---- 3. The Builder Function (Using execute_process) ----
function(add_fftw_variant SUFFIX CONFIGURE_PRECISION_FLAG SIMD_FLAGS)

    set(TARGET_NAME "fftw3${SUFFIX}")
    set(BUILD_DIR   "${CMAKE_BINARY_DIR}/fftw_build_${TARGET_NAME}")

    # Check if already built to avoid unnecessary rebuilds
    set(LIB_FILENAME "${CMAKE_STATIC_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(LIB_PATH "${FFTW_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${LIB_FILENAME}")

    if(EXISTS "${LIB_PATH}")
        message(STATUS "FFTW: ${TARGET_NAME} already built, skipping...")
        # Still create the target even if build is skipped
        set(INC_PATH "${FFTW_INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR}")
        if(NOT TARGET FFTW::${TARGET_NAME})
            add_library(FFTW::${TARGET_NAME} STATIC IMPORTED GLOBAL)
            set_target_properties(FFTW::${TARGET_NAME} PROPERTIES
                IMPORTED_LOCATION "${LIB_PATH}"
                INTERFACE_INCLUDE_DIRECTORIES "${INC_PATH}"
            )
        endif()
        return()
    endif()

    file(MAKE_DIRECTORY ${BUILD_DIR})

    # Merge flags
    set(CURRENT_FLAGS ${FFTW_COMMON_FLAGS} ${SIMD_FLAGS})
    if(NOT "${CONFIGURE_PRECISION_FLAG}" STREQUAL "")
        list(APPEND CURRENT_FLAGS "--enable-${CONFIGURE_PRECISION_FLAG}")
    endif()

    message(STATUS "FFTW: Building ${TARGET_NAME} (Static) during configuration...")
    
    # --- Step A: CONFIGURE ---
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env ${FFTW_CONFIGURE_ENV} ${FFTW_SRC_DIR}/configure ${CURRENT_FLAGS}
        WORKING_DIRECTORY ${BUILD_DIR}
        RESULT_VARIABLE configure_result
        ERROR_VARIABLE configure_error
        OUTPUT_QUIET
    )

    if(configure_result)
        message(FATAL_ERROR "FFTW ${TARGET_NAME} configuration failed! Result: ${configure_result}\nError: ${configure_error}")
    endif()

    # --- Step B: BUILD ---
    execute_process(
        COMMAND make -j8
        WORKING_DIRECTORY ${BUILD_DIR}
        RESULT_VARIABLE build_result
        ERROR_VARIABLE build_error
        OUTPUT_QUIET
    )

    if(build_result)
        message(FATAL_ERROR "FFTW ${TARGET_NAME} build failed! Result: ${build_result}\nError: ${build_error}")
    endif()

    # --- Step C: INSTALL ---
    execute_process(
        COMMAND make install
        WORKING_DIRECTORY ${BUILD_DIR}
        RESULT_VARIABLE install_result
        ERROR_VARIABLE install_error
        OUTPUT_QUIET
    )

    if(install_result)
        message(FATAL_ERROR "FFTW ${TARGET_NAME} install failed! Result: ${install_result}\nError: ${install_error}")
    endif()

    # --- Step D: The "Convention" Linker ---
    # Define locations where the file NOW EXISTS
    set(INC_PATH "${FFTW_INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Create Static Target
    if(NOT TARGET FFTW::${TARGET_NAME})
        add_library(FFTW::${TARGET_NAME} STATIC IMPORTED GLOBAL)
        
        set_target_properties(FFTW::${TARGET_NAME} PROPERTIES
            IMPORTED_LOCATION "${LIB_PATH}"
            INTERFACE_INCLUDE_DIRECTORIES "${INC_PATH}"
        )
    endif()

endfunction()

# ---- 4. Execute Builds ----
# The compilation happens here, when you run cmake ..
add_fftw_variant("" "" "${FFTW_DOUBLE_SIMD}")       # Double
add_fftw_variant("f" "float" "${FFTW_FLOAT_SIMD}")  # Float
