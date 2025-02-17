# PPS Generator Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The PPS Generator is a full hardware (FPGA) only implementation that generates a Pulse Per Second (PPS) aligned to the local clock's new second. The core can be configured by an AXI4Lite-Slave Register interface.
## 2. Interface Description
### 2.1 PPS Generator IP
The interface of the PPS Generator  is:
- System Reset and System Clock as inputs
- A high resolution Clock input, for high precision timestamps (an integer multiple of the System Clock, with a fix relationship to the System clock e.g. sourced by same PLL)
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- An AXI4L slave interface, via which the CPU reads and writes the PPS' registers
- The PPS output signal
 
![PPS Generator IP](Additional%20Files/PpsGenIp.png) 

The configuration options of the core are:
- The system clock period in nanoseconds.
- The high resolution frequency multiplier, that indicates how many times faster than the system clock is the high resolution clock  
- The option to enable/disable the cable delay support. If enabled, then the generator compensates for the cable delay which is provided in nanoseconds via the AXI configuration. 
- The output delay in nanoseconds which will be compenstated by the generator. It can be only a positive value.
- The polarity of the generated signal. If '1', then the generated signal is active-high. Else, it is active-low. 

![Pps Generator Gui](Additional%20Files/PpsGenGui.png)
## 3. Register Set
This is the register set of the PPS Generator. It is accessible via AXI4 Light Memory Mapped. All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/RegsetOverview.png)
### 3.2 Register Decription
The tables below describes the registers of the PPS Generator.     
![Control](Additional%20Files/Regset1_Control.png)
![Status](Additional%20Files/Regset2_Status.png)
![Polarity](Additional%20Files/Regset3_Polarity.png)
![Version](Additional%20Files/Regset4_Version.png)
![Width](Additional%20Files/Regset5_Width.png)
![Cable](Additional%20Files/Regset6_Cable.png)
## 4 Design Description
The PPS Generator takes a (synchronized) time input as reference and generates the Pulse Per Second aligned with this clock (start time and period) compensating the output delay. The PPS Generator contains an AXI4Lite slave for configuration and status supervision from a CPU. The component consists of 3 main operations:
- Periodically generate the pulse, aligned to the local time
- Fine-tune the activation of the puse with a high resolution clock   
- Interface with the CPU (AXI master) via the AXI slave
### 4.1 Periodically generate the PPS
When the PPS generation is enabled, the generation starts at the beginning of the local time's new second. This is done by comparing the current local time with the max nanosecond value (1000000000) minus the accumulated output delays. For high resolution pulse generation, it is marked how many high resolution periods 'fit' between these two compared values, and this is stored in a shift register.  
The duty cycle of the pulse is currently fixed to 500ms. Note that the duty cycle is counted by a free-running counter, so the deactivation of the pulse is not aligned to the local time. 
If the local time gets disabled, or a time jump occurs, the PPS generator reports an error. The pulse will continue its generation, if the error condition is removed and the pulse is deactivated.  
### 4.2 Compute the exact time of the generation
In order to compute the exact time of the generation, apart from compensating the output delays, the pulse is also generated eventually by a high resolution clock, whose frequency is a multiple of the system clock's frequency. The shift register marked at the system clock domain (see [Chapter 4.1](#4-1-periodically-generate-the-pps)) is transferred to the high resolution clock domain. The number of high resolution periods that 'fit' into this time difference, indicate when the high resolution clock should generate the PPS.    
### 4.3 AXI slave of the PPS Generator 
The PPS Generator includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the PPS Generator. An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. It also provides a status interface which allows to supervise the status of the generator. [Chapter 3](#3-register-set) has a complete description of the register set.
