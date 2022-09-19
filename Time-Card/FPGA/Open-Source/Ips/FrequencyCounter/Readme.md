# Frequency Counter Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Frequency Counter is a full hardware (FPGA) only implementation that measures the frequency of an input signal. 
The counter calculates non-fractional frequencies of range [0 Hz - 10'000'000 Hz] and it is aligned to the local clock's new second. 
The core can be configured by an AXI4Lite-Slave Register interface.
## 2. Interface Description
### 2.1 Frequency Counter IP
The interface of the Frequency Counter  is:
- System Reset and System Clock as inputs
- An input signal, for which the frequency will be measured 
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- An AXI4L slave interface, via which the CPU reads and writes the core's registers
 
![Frequency Counter IP](Additional%20Files/FrequencyCounterIP.PNG) 

The core provides a configuration option regarding the polarity of the input signal.

![Frequency Counter GUI](Additional%20Files/FrequencyCounterConfig.PNG) 

## 3. Register Set
This is the register set of the Frequency Counter. It is accessible via AXI4 Light Memory Mapped. 
All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. 
Register address space is not contiguous. 
Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. 
Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/RegsetOverview.png)
### 3.2 Register Decription
The tables below describe the registers of the Frequency Counter.     
![Control](Additional%20Files/Regset1_Control.png)
![Frequency](Additional%20Files/Regset2_Frequency.png)
![Polatiry](Additional%20Files/Regset3_Polarity.png)
![Version](Additional%20Files/Regset4_Version.png)

## 4 Design Description
The Frequency Counter receives a (synchronized) time input and a signal/frequency input and measures the frequency of the signal over a specified number of seconds. 
The core contains an AXI4Lite slave for configuration and status supervision from a CPU. 
The component consists of 2 main operations:
- Measure the frequency of the input signal    
- Interface with the CPU (AXI master) via the AXI slave
### 4.1 Measure the frequency of the input signal
The core starts measuring the signal frequency when its configuration has been provided. 
The configuration includes (i) the enable flag and (ii) the measurement period. 
The measurement period is the number of seconds over which the frequency will be measured and its range is 1-255. 
A measurement period of value zero is considered a wrong configuration. 
After the core is enabled, at the beginning of the next second (of the input time), the frequency counter starts to count the rising edges of the input signal for the configured measurement period. 
After the measurement period has passed, the counted value is divided by this measurement period and the integral result is written to a dedicated register. 
Afterwards, a new measurement starts again. 
If the calculated result is more than 10 MHz, then an error flag is raised.      
### 4.2 AXI slave of the Frequency Counter 
The Frequency Counter includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the core. 
An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. 
It also provides the frequency register which allows to supervise the status of the frequency count. [Chapter 3](#3-register-set) has a complete description of the register set.
