#!/bin/bash

# This script is intended to be run on the device

make &&
    ~/1_cpptest/pmu-adc/host_rpmsg_mcp3208 |
    ./estimator/qpmu-estimator \
        --window 24 \
        --phasor-est fft \
        --freq-est tbzc \
        --vscale 0.3205000 \
        --voffset 0 \
        --iscale 0.0073663 \
        --ioffset 0 \
        --infmt s |
        ./gui/qpmu-gui
