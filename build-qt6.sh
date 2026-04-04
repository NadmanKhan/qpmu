#!/bin/bash
# Build script for QPMU project with Qt 6.x

set -e

# Qt 6 is the default homebrew installation
export CMAKE_PREFIX_PATH="/opt/homebrew"
export PATH="/opt/homebrew/bin:$PATH"

# Navigate to project root
cd "$(dirname "$0")"

# Create build directory
BUILD_DIR="build"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure entire QPMU project with Qt 6
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DQT_DEFAULT_MAJOR_VERSION=6

# Build
cmake --build . -j"$(sysctl -n hw.ncpu)"

echo ""
echo "✅ Build complete! Qt 6.10 binaries:"
echo "   Core:    $BUILD_DIR/app/qpmu"
echo "   GUI:     $BUILD_DIR/gui/appqpmu"
echo ""
echo "Run core: ./$BUILD_DIR/app/qpmu"
echo "Run GUI:  ./$BUILD_DIR/gui/appqpmu"
