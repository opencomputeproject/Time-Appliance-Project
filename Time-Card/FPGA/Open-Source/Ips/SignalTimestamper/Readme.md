# Signal Timestamper Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Signal Timestamper is a full hardware (FPGA) only implementation. It allows to timestamp an event signal of configurable polarity. Timestamps are taken on the configured edge of the signal and interrupts are generated. The Signal Timestamper is intended to be connected to a CPU or any other AXI master that can read out the timestamps. The settings are configured by an AXI4Lite-Slave Register interface.  

## 2. Interface Description
### 2.1 Signal Timestamper IP
The interfave of the Signal Timestamper  is:
- System Reset and System Clock as inputs
- A high resolution Clock input, for high precision timestamps (an integer multiple of the System Clock, with a fix relationship to the System clock e.g. sourced by same PLL)
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- An event input, for which the timestamp will be taken- An AXI4L slave interface, via which the CPU reads and writes the timestamper's registers
- An optional interrupt output, when a timestamp is taken
 
![Signal Timestamper IP](Additional%20Files/SigTimestamper%20IP.png) 

The configuration options of the core are:
- The system clock period in nanoseconds.
- The high resolution frequency multiplier, that indicates how many times faster than the system clock is the high resolution clock is  
- The option to enable/disable the cable delay support. If enabled, then the timestamper compensates for the cable delay which is provided in nanoseconds via the AXI configuration. 
- The input delay in nanoseconds which will be compenstated by the timestamper (e.g. for buffers, always positive)
- The polarity of the event input. If '1', then the timestamp will be taken at the rising edge of the event. Else, in the falling edge of the event. 

![Signal Timestamper Gui](Additional%20Files/SigTimestamper%20Configuration.png)
## 3. Register Set
This is the register set of the Signal Timestamper. It is accessible via AXI4 Light Memory Mapped. All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/RegsetOverview.png)
### 3.2 Register Decription
The tables below describes the registers of the Signal Timestamper.     
![Control](Additional%20Files/Regset1_Control.png)
![Status](Additional%20Files/Regset2_Status.png)
![Polarity](Additional%20Files/Regset3_Polarity.png)
![Version](Additional%20Files/Regset4_Version.png)
![Cable](Additional%20Files/Regset5_Cable.png)
![IRQ](Additional%20Files/Regset6_Irq.png)
![MSK](Additional%20Files/Regset7_Msk.png)
![EventCount](Additional%20Files/Regset8_EvtCnt.png)
![TsCount](Additional%20Files/Regset9_TsCnt.png)
![TsLow](Additional%20Files/Regset10_TsL.png)
![TsHigh](Additional%20Files/Regset11_TsH.png)
![TsDataWdth](Additional%20Files/Regset12_DataWdth.png)
![TsData](Additional%20Files/Regset13_Data.png)
## 4 Design Description
The Signal Timestamper takes a (synchronized) time input as reference, an event signal and generates reference clock timestamps on the configurable edge (polarity) of the event signal, compensating the input delay. A timestamp event will also cause an interrupt to signal to the CPU that an event occurred and a timestamp is ready to be read. Whenever an edge is detected it will increase an internal counter which allows to detect missed events, since the timestamper disables itself until the CPU has cleared the interrupt. It contains an AXI4Lite slave for configuration, status supervision and timestamp readout from a CPU. The component is divided in 3 main operations:
- Taking the timestamp of the input event with the high resolution clock
- Computing the exact time of the timestamp
- Interfacing with the CPU (AXI master) via the AXI slave
### 4.1 Timestamp with the high resolution clock
The high resolution clock's frequency is an integer multiple of the system clock's frequency (provided as a configuration). The high resolution clock is used to reduce the jitter of the timestamp. When the event is received the high resolution clock marks the event in a shift register. The shift register is copied at the system clock's domain, and so, the high resolution time "window" can be identified and can be correspondingly compensated.      
### 4.2 Compute the exact time of the timestamp
In order to compute the exact time of the timestamp the following delays are compensated:
- **the high relution delay**. As described in [Chapter 4.1](#4-1-timestamp-with-the-high-resolution-clock), timestamping with the high resolution clock reduces the timestamp jitter, by computing a high resolution delay. This delay includes the register offset for switching between clock domains and the difference between the rising edge of the system clock and the high resolution clock, when the event occured.  
- **the input delay**. It is provided as a configuration option to the core, during compilation time. It can be only a positive value.
- **the cable delay**. It is provided as a configuration option via a AXI register, given that the cable delay compensation is enabled. It can be only a positive value.
### 4.3 AXI slave of the Signal Timestamper 
The Signal Timestamper includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the Signal Timestamper. An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. It also provides a status interface which allows to supervise the status of the timestamper. [Chapter 3](#3-register-set) has a complete description of the register set. 
   
