# Ore Cart Hardware

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

This repo contains the code to run on the embedded hardware inside the OreCart buses.

## About
The hardware is based around the RP2040 (ARM Cortex M0+).
FreeRTOS is used as the underlying real-time operating system.

## Building
To set up your environment, please refer to Chapter 2 of the [Raspberry Pi Pico datasheet](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf).
In order to build this project, you should have the `PICO_SDK_PATH` environment variable, as well as the `gcc-arm-none-eabi` cross-compiler.

To build the project:
```
mkdir build
cd build
cmake ..
make -j`nproc`
```

This should create the `orecart_hw.uf2` file that can be flashed to your RP2040 hardware.

Using the VSCode built-in tools also works for this.
