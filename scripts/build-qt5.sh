#!/bin/bash
# Build script for QPMU project with Qt 5.12+

set -e

# Set Qt 5 paths
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@5"
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"
export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"
export PKG_CONFIG_PATH="/opt/homebrew/opt/qt@5/lib/pkgconfig"

# Navigate to project root
cd "$(dirname "$0")"

# Create build directory
BUILD_DIR="../build"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure entire QPMU project with Qt 5
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DQT_DEFAULT_MAJOR_VERSION=5 \
    -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@5"

# Build
cmake --build . -j"$(sysctl -n hw.ncpu)"
