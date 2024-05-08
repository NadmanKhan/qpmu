#!/bin/bash

make && \
./simulator/qpmu-simulator \
--v "$1" \
--p "$2" \
--outfmt s \
| ./estimator/qpmu-estimator \
--window 24 \
--vscale 0.3205000 \
--voffset 0 \
--iscale 0.0073663 \
--ioffset 0.0357669 \
--infmt s \
| ./gui/qpmu-gui