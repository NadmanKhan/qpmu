# QPMU

QPMU is a software tool to control and monitor Phasor Measurement Unit (PMU) device. It is a part of the PMU project at the Cyber-Physical Systems Lab, North South University, Bangladesh.

## Features

- **Real-time signal processing**: QPMU can process the incoming streaming data in real-time to calculate the phasor values and other power system parameters.
- **Data visualization**: QPMU can visualize the processed data in real-time.
- **IEEE C37.118-2011 compliant**: QPMU is compliant with the IEEE C37.118-2011 standard, which defines the communication protocol for PMUs with other devices.

## Installation

### Install dependencies

The project uses vcpkg to manage dependencies. Pull vcpkg from the official repository, bootstrap it, and set the environment variable `VCPKG_ROOT`:

```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh
export VCPKG_ROOT=$PWD
```

### Building QPMU

Clone the repository:

```bash
git clone https://github.com/NadmanKhan/qpmu.git
```

Build the project using CMake (version 3.20 or higher is required):

```bash
cd qpmu
cmake --preset=release # or --preset=debug for debug mode
```
