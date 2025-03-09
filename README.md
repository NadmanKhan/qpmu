# QPMU

QPMU is a software tool to control and monitor Phasor Measurement Unit (PMU) device. It is a part of the PMU project at the Cyber-Physical Systems Lab, North South University, Bangladesh.

## Features

- **Real-time signal processing**: QPMU can process the incoming streaming data in real-time to calculate the phasor values and other power system parameters.
- **Data visualization**: QPMU can visualize the processed data in real-time.
- **IEEE C37.118-2011 compliant**: QPMU is compliant with the IEEE C37.118-2011 standard, which defines the communication protocol for PMUs with other devices.

## Installation

### Install dependencies

The project uses vcpkg to manage dependencies. You need to get a specific version of vcpkg and set its location as the environment variable `VCPKG_ROOT`:

```bash
git clone https://github.com/NadmanKhan/vcpkg.git
export VCPKG_ROOT=$PWD/vcpkg
```

### Building QPMU

Clone the repository:

```bash
git clone qpmu
```

Build the project using CMake (version 3.20 or higher is required):

```bash
cd qpmu
cmake -B build -S . --preset=release # or --preset=debug for debug mode
```
