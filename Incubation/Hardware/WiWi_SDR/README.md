# Hardware Design Overview

This hardware design integrates multiple high-performance components for precise time and frequency control, real-time IQ data processing, and versatile radio communication. The goal is to create a flexible system that can handle complex radio communication tasks, such as channel estimation and two-way phase transfer, while maintaining strict timing and frequency synchronization across all components.

## Components

### 1. STM32H747 Microcontroller (Board Controller)
- **Role**: Acts as the primary controller for the entire board. 
- **Features**: Dual-core ARM Cortex-M7 and Cortex-M4 for handling control tasks and communication with external components.
- **Interfaces**: Communicates with the SDR, LoRa transceiver, and other modules to manage configuration and control.
  - Exposes 100M Ethernet over RJ45 to run PTP for time synchronization, or Data Transfer to Network nodes. 1PPS from PTP goes to the DPLL.
  - Exposes 480Mb/s High speed USB over USB-A connector, to connect to a host PC and run as a normal SDR 

### 2. 8A34001 DPLL (Source of Time and Frequency Signals)
- **Role**: Provides precise time and frequency signals, ensuring synchronization across the entire system.
- **Features**: Digitally controlled phase-locked loop (DPLL) for stable clock generation.
- **Purpose**: Controls and synchronizes the time and frequency of all components, including the STM32, FPGA, oscillators, and SDR radios.

### 3. LFE5U-25F-6BG256I ECP5 FPGA (SDR Interface and Parallel Processor)
- **Role**: Acts as the bridge between the STM32 microcontroller and the SDR interface, while also performing real-time IQ data processing.
- **Features**:
  - Real-time signal processing for IQ data captured from the AT86RF215IQ dual-channel SDR.
  - Handles complex DSP tasks required for channel estimation and two-way phase transfer.
- **Interfaces**: Manages data flow between the STM32, the SDR radios, and other components for efficient processing.

### 4. SiT5501 Oscillator (High Stability Oscillator)
- **Role**: Provides a stable reference clock for the entire system.
- **Features**: High stability and precision, used as the reference frequency for the 8A34001 DPLL and other time-critical components.

### 5. SX1276 LoRa Transceiver (Standard LoRa Transceiver)
- **Role**: Enables standard LoRa communication for low-power, long-range wireless communication.
- **Features**: Supports sub-GHz communication with LoRa modulation, typically used for IoT applications.
- **Purpose**: Provides the system with standard LoRa capabilities for basic communication tasks.

### 6. ESP32-S3 (WiFi/BLE/2.4GHz Transceiver)
- **Role**: Handles WiFi, Bluetooth, and 2.4GHz wireless communication.
- **Features**:
  - Dual-core processor with integrated 2.4GHz radio.
  - WiFi and BLE capabilities for IoT and general-purpose wireless connectivity.
- **Purpose**: Allows for wireless communication with external devices or cloud services, adding versatile connectivity options.

### 7. AT86RF215IQ Dual-Channel SDR (Sub-GHz and 2.4GHz Radios)
- **Role**: The primary SDR (Software Defined Radio) component with dual radios for sub-GHz and 2.4GHz frequency bands.
- **Features**:
  - Supports simultaneous operation on both sub-GHz and 2.4GHz bands.
  - Captures IQ samples for deep radio communication processing, including channel estimation and two-way phase transfer.
- **Purpose**: Provides flexible, programmable radio capabilities for advanced radio processing tasks.

## System Architecture

The system is designed for **precise control of time and frequency** using the **8A34001 DPLL** to synchronize the operations of the entire board. The **STM32H747** serves as the main controller, managing the various communication tasks and interacting with the **LIFCL-17-7BG256 FPGA**, which acts as the interface between the SDR radios and the microcontroller.

### Key Features:
- **Real-Time IQ Processing**: The FPGA processes IQ data captured from the **AT86RF215IQ** SDR's sub-GHz and 2.4GHz radios, performing tasks such as channel estimation and two-way phase transfer.
- **Time and Frequency Synchronization**: The **8A34001 DPLL** ensures all components are tightly synchronized, providing a stable and unified time and frequency reference across the system.
- **Multi-Protocol Communication**: The system supports LoRa, WiFi, Bluetooth, and custom SDR communication protocols, making it highly versatile for various wireless communication tasks.

### Use Cases:
- **Complex Channel Estimation**: The system can perform advanced channel estimation across multiple frequency bands (sub-GHz and 2.4GHz), leveraging IQ samples from the **AT86RF215IQ** SDR.
- **Two-Way Phase Transfer**: The precise timing and synchronization allow for accurate two-way phase transfer and other timing-sensitive applications.
- **Flexible Wireless Communication**: With the combination of LoRa, WiFi, and BLE transceivers, the system is capable of handling a wide range of communication protocols, from low-power IoT applications to high-speed data transfer.

## Conclusion

This hardware design is a powerful platform for advanced wireless communication and real-time signal processing. By integrating SDR capabilities, precise time synchronization, and versatile transceivers, the system is capable of performing complex radio tasks while maintaining tight control over time and frequency.
