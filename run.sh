#!/bin/bash

make && \
./simulator/qpmu-simulator \
--v "$1" \
--p "$2" \
--outfmt b \
| ./estimator/qpmu-estimator \
--window 24 \
--vscale 0.3205000 \
--voffset 0 \
--iscale 0.0073663 \
--ioffset 0.0357669 \
--infmt b \
| ./gui/qpmu-gui