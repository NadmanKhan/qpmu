#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BUILD_DIR=$SCRIPT_DIR/../build-qpmu-Desktop-Debug
QPMU_BUILD_PATH=$BUILD_DIR/qpmu
export QPMU_CONFIG_PATH=$SCRIPT_DIR/qpmu.ini
ADC_PROGRAM_CMD=$1
ADC_PROGRAM_ARGS=${@:2}

echo $SCRIPT_DIR
echo $BUILD_DIR
echo $QPMU_BUILD_PATH
echo $QPMU_CONFIG_PATH
echo $ADC_PROGRAM_CMD
echo $ADC_PROGRAM_ARGS

# 1
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi
cmake --build $BUILD_DIR --target all

# 2
python3 $SCRIPT_DIR/adc_server.py $ADC_PROGRAM_CMD $ADC_PROGRAM_ARGS &

# 3
sleep 2

# 4
$QPMU_BUILD_PATH &&

# 5
fg

# 1. Build the QPMU GUI program.
# 2. Run the ADC server with path to the ADC program, and send it to the background.
# 3. Sleep for a few seconds to give the ADC server time to start.
# 4. Run the QPMU GUI program, and wait for it to finish.
# 5. Bring the ADC server to the foreground, and wait for it to finish.
