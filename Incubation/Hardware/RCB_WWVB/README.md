# SDR Board

This repository contains the design files, firmware, and documentation for a Software-Defined Radio (SDR) board that integrates several key components for LoRa and sub-GHz SDR applications. The board is designed to support a wide range of functionalities, including LoRa communication, IQ sample capture, high-precision clocking, and low-frequency signal detection.

## Key Components

- SX1276: Standard LoRa transceiver for LoRaWAN and custom LoRa applications.
- SX1257: Sub-GHz SDR used to capture IQ samples from the SX1276 transceiver.
- iCE40UP5K FPGA: Acts as a coprocessor to parse the SX1257 datastream and push the data to the microcontroller.
- STM32H747 Microcontroller: Dual-core microcontroller with SPI interfaces to the FPGA, handling the IQ data stream and additional processing.
- Si5341 PLL: Provides clocking for all components except the STM32, ensuring synchronized operation across the board.
- SiT5501: High stability (10ppb) TCXO, providing a stable reference clock.
- RCB Header: Interface to the timecard, with UART, 1PPS in/out, and 10MHz in/out support.
- Low-Frequency ADC Path: Connected to the STM32H747, with optional amplifiers for picking up low-frequency signals such as WWVB.
