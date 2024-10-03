# PCIe RF-over-Fiber Test Platform

This repository contains the hardware design and documentation for a PCIe form factor RF-over-Fiber test platform. The board features three SFP ports designed for specialized RF-over-Fiber (RoF) applications, utilizing linear SFP transceivers provided by Precision Optical Technologies. The platform enables testing of various frequency bands, including GNSS, WiFi, and IoT over fiber, making it ideal for RF communications over long distances using optical fiber.

## Key Features

- **I226 NIC**: Acts as a PCIe to Ethernet Network Interface Controller (NIC), with added functionality as a bit-bang I²C controller using two SDP pins.
- **Three SFP Ports**: Supports linear RF-over-Fiber SFP transceivers, designed for specific frequency bands:
  - **Port 1**: L1/L5 GNSS over fiber
  - **Port 2**: 2.4GHz WiFi over fiber
  - **Port 3**: Sub-GHz IoT over fiber
- **RF Signal Path**: Each SFP port integrates an RF signal path with the following components:
  - **Attenuators**: To manage signal levels.
  - **Bandpass Filters**: Designed for specific frequency bands to filter the RF signal.
  - **Baluns**: Convert differential signals from the SFP to single-ended signals compatible with SMA connectors.

## Background: Why RF Over Fiber?

RF over Fiber (RoF) is a technology that enables radio frequency (RF) signals to be transmitted over optical fiber, leveraging the low-loss and high bandwidth characteristics of fiber optic cables. Common applications include:
- **Long-Distance Communication**: RoF allows RF signals to travel over long distances with minimal loss, making it ideal for remote antenna installations.
- **Reduced Interference**: Optical fibers are immune to electromagnetic interference (EMI), making them more reliable for transmitting RF signals in noisy environments.
- **Centralized Radio Networks**: RoF enables the deployment of centralized radio processing, where RF signals are transported over fiber from remote radio heads to central processing units.

### Optical SFP Transceivers for RoF Applications

Small Form-factor Pluggable (SFP) transceivers are commonly used in data communications to convert electrical signals to optical signals (and vice versa) for transmission over fiber. For RF over Fiber applications, specially designed linear SFPs are used to pass RF signals from Electrical to Optical (E->O) and Optical to Electrical (O->E) with minimal distortion. 

The SFPs used in this design, provided by Precision Optical Technologies, are customized for RF over fiber applications such as:
- **WiFi over Fiber**: Transmitting WiFi signals from RF to optical, suitable for extending coverage in large areas.
- **GNSS over Fiber**: Transmitting GNSS signals, enabling precise location tracking in distributed systems.
- **IoT over Fiber**: Supporting Sub-GHz IoT communications over long distances, useful for large-scale IoT deployments.

## Design Overview

This platform provides a versatile hardware solution for testing RF-over-Fiber transceivers in various frequency bands. It includes:

### I226 NIC
The Intel I226 NIC provides both Ethernet connectivity and a bit-bang I²C controller using two SDP pins. This functionality is leveraged to control peripheral components on the board, including the SFP modules.

### SFP Ports
Three SFP ports are integrated into the design, each connected to coaxial interfaces with the following features:
- **Differential to Single-Ended Conversion**: Baluns convert the differential RF signal from the SFP to single-ended RF signals compatible with standard SMA connectors.
- **Frequency-Specific Filtering**: Each SFP port is designed with bandpass filters and attenuators to support a specific frequency band:
  - **Port 1**: GNSS over fiber (L1/L5 band)
  - **Port 2**: WiFi over fiber (2.4GHz band)
  - **Port 3**: Sub-GHz IoT over fiber

### RF Signal Path
The design incorporates filters and attenuators to ensure the signal integrity of each band:
- **Bandpass Filters**: Tailored to the specific frequency band of each SFP port.
- **Attenuators**: Included to manage signal strength before transmitting it via SMA connectors.

## Use Cases

- **GNSS Over Fiber**: Extend GNSS signals over long distances using optical fiber to maintain precise timing in distributed systems.
- **WiFi Over Fiber**: Deploy WiFi coverage over extended areas without significant signal degradation by transmitting the RF signal over fiber.
- **IoT Over Fiber**: Enable IoT networks across large geographical areas by transmitting sub-GHz RF signals over optical fiber.

## Contributors
- Julian St. James
- Ahmad Byagowi
- Dan Sobel
- Howard Trinh

