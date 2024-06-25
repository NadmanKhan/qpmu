#!/bin/bash

# This script is intended to be run on the device

make &&
    ~/1_cpptest/pmu-adc/host_rpmsg_mcp3208 |
    ./estimator/qpmu-estimator \
        --window 24 \
        --phasor-est fft \
        --freq-est tbzc \
        --ch0-scale 0.3205000 \
        --ch0-offset 0 \
        --ch1-scale 0.3205000 \
        --ch1-offset 0 \
        --ch2-scale 0.3205000 \
        --ch2-offset 0 \
        --ch3-scale 0.0073663 \
        --ch3-offset 0 \
        --ch4-scale 0.0073663 \
        --ch4-offset 0 \
        --ch5-scale 0.0073663 \
        --ch5-offset 0 \
        --infmt s |
        ./gui/qpmu-gui
