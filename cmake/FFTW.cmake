# Find FFTW3 installed on the system
# FFTW must be installed manually using your system's package manager:
# - macOS: brew install fftw
# - Debian/Ubuntu: sudo apt install libfftw3-dev
# - Fedora/RHEL: sudo dnf install fftw-devel
# - Arch: sudo pacman -S fftw


# Function to find FFTW with fallback to pkg-config
function(find_fftw_variant VARIANT PKG_NAME TARGET_NAME)
    # Try find_package first
    find_package(${VARIANT} QUIET)
    
    # If that fails, try pkg-config
    if(NOT ${VARIANT}_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(${VARIANT} IMPORTED_TARGET GLOBAL ${PKG_NAME})
        endif()
    endif()
    
    if(NOT ${VARIANT}_FOUND)
        message(FATAL_ERROR "${VARIANT} not found")
    endif()
    
    # Create a unified alias target
    if(TARGET ${VARIANT}::${PKG_NAME})
        add_library(${TARGET_NAME} ALIAS ${VARIANT}::${PKG_NAME})
    elseif(TARGET PkgConfig::${VARIANT})
        add_library(${TARGET_NAME} ALIAS PkgConfig::${VARIANT})
    elseif(TARGET PkgConfig::${PKG_NAME})
        add_library(${TARGET_NAME} ALIAS PkgConfig::${PKG_NAME})
    else()
        # Fallback: create target manually
        add_library(${TARGET_NAME} INTERFACE IMPORTED)
        target_include_directories(${TARGET_NAME} INTERFACE ${${VARIANT}_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} INTERFACE ${${VARIANT}_LINK_LIBRARIES})
    endif()
endfunction()

# Find both variants
find_fftw_variant(FFTW3 fftw3 FFTW::Double)
find_fftw_variant(FFTW3f fftw3f FFTW::Float)
