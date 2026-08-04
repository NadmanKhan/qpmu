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

**FFTW3**: Install FFTW library on your platform
- **macOS**: `brew install fftw`
- **Debian/Ubuntu**: `sudo apt install libfftw3-dev`
- **Fedora/RHEL**: `sudo dnf install fftw-devel`
- **Arch Linux**: `sudo pacman -S fftw`

Other dependencies (Open-C37.118, GTest) will be automatically downloaded and built by CMake during configuration.

### Building QPMU

Clone the repository:

```bash
git clone https://github.com/NadmanKhan/qpmu.git
cd qpmu
```

Build the project using CMake (version 3.16 or higher required):

```bash
# Configure (CMake will download and build Open-C37.118 and GTest automatically)
cmake --preset debug   # or --preset release

# Build
cmake --build build-debug   # or build-release
```

The built application will be in `build-debug/app/` (or `build-release/app/`).

## BeagleBone Black DAQ

Configure and build the service and PRU firmware with the AM335x/MCP3208
backend:

```bash
cmake -S . -B build \
  -DQPMU_DAQ=am335x.mcp3208.pru-shram \
  -DBUILD_TESTING=OFF
cmake --build build --target qpmu_service pru0_firmware
sudo install -m 0644 \
  build/core/daq/am335x.mcp3208.pru-shram/pru0/pru0.out \
  /lib/firmware/pru0_mcp3208
```

After installing the firmware, initialize PRU0 once after each reboot, then
start the service. The PRU backend samples at a fixed 1200 Hz.

```bash
sudo ./core/daq/am335x.mcp3208.pru-shram/start-pru.sh
sudo ./build/core/qpmu_service -v
```

Use repeatable `--scale NAME:FACTOR` options to apply channel calibration to
the processed phasor values sent to the GUI and logger.
