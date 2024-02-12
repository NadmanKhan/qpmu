#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BUILD_DIR=$SCRIPT_DIR/../build-qpmu-Desktop-Debug
QPMU_EXEC_PATH=$BUILD_DIR/qpmu
export QPMU_CONFIG_PATH=$SCRIPT_DIR/qpmu.ini

echo "qpmu: script directory = $SCRIPT_DIR"
echo "qpmu: build directory  = $BUILD_DIR"
echo "qpmu: exec path        = $QPMU_EXEC_PATH"
echo "qpmu: config path      = $QPMU_CONFIG_PATH"

# 1
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory does not exist. Creating it..."
    mkdir "$BUILD_DIR"
fi
cd "$BUILD_DIR" || exit
cmake -DCMAKE_BUILD_TYPE=Debug "$SCRIPT_DIR"
cmake --build "$BUILD_DIR" --target all

# 2
$QPMU_EXEC_PATH -C $QPMU_CONFIG_PATH

# 1. Build the QPMU GUI program.
# 2. Run the QPMU GUI program with the configuration file passed as an argument.
