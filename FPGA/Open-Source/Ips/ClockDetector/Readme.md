# Clock Detector Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Clock Detector is a full hardware (FPGA) only implementation that detects the available clock sources and selects the clocks to be used. The selection is done with different clock selector and clock enable outputs.
The selection is according to a priority scheme and it can be overwritten with a manual configuration.
The configuration and monitoring of the core is done via a AXI4L slave interface.
## 2. Interface Description
### 2.1 Clock Detector IP
The interface of the Clock Detector is:
- System Reset and System Clock as inputs
- The 4 available clocks, as inputs
- The enable flags for external clock selectors, as outputs 
- A reset output, during a transition of clock selection
- The PPS source to be selected, as output (configuration interface with the [PPS Source Select](../PpsSourceSelector))
- The available PPS sources, as input (monitor interface with the [PPS Source Select](../PpsSourceSelector)) 
- An AXI4L slave interface, for configuration and monitoring of the core (and, optionally, an additional [PPS Source Select](../PpsSourceSelector) core) 
 
![Clock Detector IP](Additional%20Files/ClockDetectorIP.png) 

The core has configuration options for the clock and PPS selection.

![Clock Detector IP](Additional%20Files/ClockDetectorConfig.png)

## 3. Register Set
The register set of the Clock Detector is accessible via AXI4 Light Memory Mapped. 
All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. 
Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. 
Non existing register access in the mapped memory area is answered with a slave decoding error.
The Register set configures which source should be used as clock and monitors which is currently selected. Additionally, it provides the configuration 
of a [PPS Source Select](../PpsSourceSelector) core. 
#### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/Regset_Overview.png)
#### 3.2 Register Decription
The tables below describe the registers of the Clock Detector.
![SourceSelected](Additional%20Files/Regset1_SourceSelected.png)
![SourceSelect](Additional%20Files/Regset2_SourceSelect.png)
![Version](Additional%20Files/Regset3_Version.png)
## 4 Design Description
The Clock Detector detects the available clock inputs and selects the clocks to be used, based on a priority scheme and a configuration. 
The component consists of 3 main operations:
- Detect the available clocks
- Select the clocks
- Interface with the CPU (AXI master) via the an AXI slave
### 4.1 Detect the available clocks

For each clock input, an availability check is performed. 
A "slow" clock is created for each clock domain. At the system clock domain, the toggling of these slow clocks is checked.
If their values do not toggle within a timeout, the input clock is considered unavailable.

### 4.2 Select the clocks
A clock is selected if it satisfies 2 conditions:
1) The clock is available
2)  - If the automatic selection is configured:
     the clock has higher priority than the other available clocks
    - If the manual selection is configured: 
     the clock has higher priority than the other available and manually-selected clocks

The table below shows how the clock selection outputs are set depending on the selection. The order of the clocks is according to the selection priority scheme (SMA 10 MHz Clock -> highest, External Clock -> lowest):
|Clock|Mux1|Mux2|Mux3|Wiz2|
|-----|:--:|:--:|:--:|:--:|
|SMA 10 MHz Clock|0|x|0|0|
|MAC 10 MHz Clock|1|x|0|0|
|DCXO1 10 Mhz Clock|x|0|1|0|
|MAC 10 MHz Clock|x|1|1|0|
|External Clock|x|x|x|1|

### 4.3 AXI slave of the Clock Detector 
The Clock Detector includes an AXI Light Memory Mapped Slave which provides access to the registers of the core. 
An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. [Chapter 3](#3-register-set) has a complete description of the register set.
