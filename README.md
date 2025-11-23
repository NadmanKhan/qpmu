# QPMU

QPMU is a software tool to control and monitor Phasor Measurement Unit (PMU) device. It is a part of the PMU project at the Cyber-Physical Systems Lab, North South University, Bangladesh.

## Features

- **Real-time signal processing**: QPMU can process the incoming streaming data in real-time to calculate the phasor values and other power system parameters.
- **Data visualization**: QPMU can visualize the processed data in real-time.
- **IEEE C37.118-2011 compliant**: QPMU is compliant with the IEEE C37.118-2011 standard, which defines the communication protocol for PMUs with other devices.

## Installation

### Prerequisites

**Qt 5 or Qt 6**: Install Qt on your platform
- **macOS**: `brew install qt@6` or `brew install qt@5`
- **BeagleBone Black/Debian**: `sudo apt install qt6-base-dev qt6-charts-dev` or `sudo apt install qtbase5-dev libqt5charts5-dev`
- See [docs/qt-setup.md](docs/qt-setup.md) for detailed instructions

Other dependencies (FFTW3, Open-C37.118, GTest) will be automatically downloaded and built by CMake during configuration.

### Building QPMU

Clone the repository:

```bash
git clone https://github.com/NadmanKhan/qpmu.git
cd qpmu
```

Build the project using CMake (version 3.20 or higher required):

```bash
# Configure (CMake will download and build FFTW3 and GTest automatically)
cmake --preset debug   # or --preset release

# Build
cmake --build build-debug   # or build-release
```

The built application will be in `build-debug/app/` (or `build-release/app/`).
