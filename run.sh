#!/bin/bash

re='^[0-9]+$'
if ! [[ $1 =~ $re ]] ; then
   echo "error: \"$1\" is not a number" >&2; exit 1
fi
if ! [[ $2 =~ $re ]] ; then
   echo "error: \"$2\" is not a number" >&2; exit 1
fi

make &&
    ./simulator/qpmu-simulator \
        --v "$1" \
        --p "$2" \
        --outfmt b |
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
        --infmt b --outfmt b |
        ./gui/qpmu-gui
