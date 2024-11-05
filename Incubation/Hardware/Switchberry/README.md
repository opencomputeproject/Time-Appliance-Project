# Raspberry Pi CM4 Managed 1G Ethernet Switch PCB

This repository contains the design files and documentation for a custom PCB based on the Raspberry Pi Compute Module 4 (CM4) as the host CPU. The PCB integrates a managed 7-port Gigabit Ethernet switch, exposing various interfaces and peripherals for a versatile and cost-effective 1G network switch with precision time protocol (PTP) capabilities.

## Overview

The PCB is designed around the **Raspberry Pi Compute Module 4 (CM4)** and a **Microchip KSZ9567 Ethernet switch**. It leverages the CM4’s processing power and network capabilities alongside the managed 7-port switch to create a flexible, open-source network switch for PTP networks and other educational and experimental purposes. 

### Key Features

- **Raspberry Pi CM4 Host**: 
  - Connects via GPIO to manage the switch over SPI.
  - Exposes essential CM4 peripherals: 
    - **HDMI Output**
    - **1G Ethernet (RJ45)**
    - **USB-A Port**
  
- **Microchip KSZ9567 Managed 7-Port Switch**:
  - 7 Ethernet Ports:
    - **5 BASE-T Ports** (RJ45 connectors)
    - **1 SGMII Port** connected to an **SFP slot**
    - **1 RGMII Port** connected to an additional **BASE-T PHY**
  - **Transparent Clock and Boundary Clock** for PTP support:
    - Capable of Transparent Clocking, providing accurate time synchronization across the network.
    - Boundary Clock support when managed by the CM4, enabling complex PTP network configurations.
  
### Purpose of the Design

This design aims to provide a cost-effective and accessible 1G Ethernet switch with PTP capabilities, allowing users to:
- Build **Precise Time Protocol (PTP) networks**: Ideal for applications that require precise timing, such as industrial automation, telecommunications, and media synchronization.
- Learn and experiment with **managed Ethernet switches** and **PTP functionalities**.
- Seamlessly integrate with the **Raspberry Pi ecosystem**, leveraging the Compute Module 4 as a powerful, low-cost host.
- Develop and prototype **network applications** within the open Raspberry Pi ecosystem, utilizing the CM4’s flexibility and expandability.

## Hardware Components

### 1. Raspberry Pi Compute Module 4 (CM4)
- **Role**: Acts as the host CPU, interfacing with the switch and exposing HDMI, Ethernet, and USB for a complete system.
- **Interfaces Exposed**:
  - **HDMI Output** for display connection.
  - **1G Ethernet (RJ45)** for network connectivity.
  - **USB-A Port** for external peripherals.

### 2. Microchip KSZ9567 Managed Ethernet Switch
- **Description**: A managed 7-port switch that supports both transparent and boundary clock modes for PTP.
- **Configuration**:
  - **5 BASE-T Ports**: Standard RJ45 connectors for direct network connections.
  - **1 SGMII Port**: Connected to an SFP slot, supporting fiber and additional media types.
  - **1 RGMII Port**: Connected to a BASE-T PHY, providing a sixth RJ45 port.
- **PTP Capabilities**:
  - **Transparent Clock**: Enables time-aware network switching, ideal for PTP networks.
  - **Boundary Clock**: CM4 management enables boundary clock mode for more complex timing setups.

## Potential Use Cases

- **Low-Cost, Open-Source 1G Switch**: An affordable, Raspberry Pi-based solution for basic managed switching, PTP support, and network management.
  
- **Building PTP Networks**: Ideal for building PTP-enabled networks for applications that require strict timing synchronization, such as:
  - **Industrial Automation**: Synchronizing devices on factory floors.
  - **Telecommunications**: Accurate timing for distributed telecom equipment.
  - **Media Production**: Time synchronization for audio/video equipment.

- **Educational Tool for Network Management**: This board can serve as an excellent teaching platform, helping users learn about network switch management, PTP functionality, and the Raspberry Pi ecosystem.

- **Prototyping Platform**: Perfect for developing and testing applications that require network management, PTP synchronization, or extended Raspberry Pi capabilities.

## Getting Started

1. **Hardware Setup**:
   - Insert the Compute Module 4 (CM4) into the PCB.
   - Connect peripherals such as HDMI, Ethernet, and USB as needed.
   - Connect network cables to the RJ45 connectors and SFP slot for the desired network configuration.

2. **Software Setup**:
   - Flash the Raspberry Pi CM4 with a compatible OS (such as Raspberry Pi OS).
   - Set up the SPI interface to manage the KSZ9567 switch via CM4.
   - Configure PTP settings as required for your application.

3. **PTP Configuration**:
   - Configure the switch’s Transparent Clock or Boundary Clock mode to enable precise time synchronization.
   - Use PTP software tools on the CM4 to manage and monitor network timing.

## License

This project is licensed under the MIT License.

## Contributions

Contributions are welcome! Feel free to submit issues, pull requests, and feature suggestions to improve this open-source project.

---

This design combines powerful features of the Raspberry Pi CM4 with a managed switch to create an accessible, open, and versatile platform for learning, prototyping, and deploying time-synchronized networks.
