include(FetchContent)
include(GNUInstallDirs)

# ---- 1. Global Settings & Source Fetching ----
set(OPENC37118_INSTALL_DIR "${CMAKE_BINARY_DIR}/openc37118_install")

# Save current BUILD_SHARED_LIBS value
if(DEFINED BUILD_SHARED_LIBS)
    set(_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(_HAD_BUILD_SHARED_LIBS TRUE)
else()
    set(_HAD_BUILD_SHARED_LIBS FALSE)
endif()

# Force static library build for Open-C37.118
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Fetch Open-C37.118 from your fork
FetchContent_Declare(openc37118_source_fetch
    GIT_REPOSITORY              https://github.com/NadmanKhan/Open-C37.118.git
    GIT_TAG                     master
    DOWNLOAD_EXTRACT_TIMESTAMP  TRUE
)
FetchContent_MakeAvailable(openc37118_source_fetch)

# Restore BUILD_SHARED_LIBS to its previous value
if(_HAD_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${_SAVED_BUILD_SHARED_LIBS} CACHE BOOL "Build shared libraries" FORCE)
else()
    unset(BUILD_SHARED_LIBS CACHE)
endif()

# The library is now available via the subdirectory added by FetchContent
# Create an alias to match the expected target name
if(TARGET open-c37118)
    add_library(OpenC37118::openc37118 ALIAS open-c37118)
    message(STATUS "Open-C37.118: Successfully fetched and built from GitHub")
elseif(TARGET openc37118)
    add_library(OpenC37118::openc37118 ALIAS openc37118)
    message(STATUS "Open-C37.118: Successfully fetched and built from GitHub")
else()
    message(FATAL_ERROR "Open-C37.118: Expected target 'open-c37118' or 'openc37118' not found after FetchContent")
endif()
