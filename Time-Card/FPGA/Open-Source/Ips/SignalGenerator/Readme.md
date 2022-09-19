# Signal Generator Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Signal Generator is a full hardware (FPGA) only implementation that allows to generate pulse width modulated (PWM) signals of configurable polarity aligned with the local clock. The Signal Generator takes a start time, a pulse width and period as well as a repeat count as input and generates the signal accordingly. The settings are configurable by an AXI4Lite-Slave Register interface.
## 2. Interface Description
### 2.1 Signal Generator IP
The interface of the Signal Generator  is:
- System Reset and System Clock as inputs
- A high resolution Clock input, for high precision timestamps (an integer multiple of the System Clock, with a fix relationship to the System clock e.g. sourced by same PLL)
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- An AXI4L slave interface, via which the CPU reads and writes the generator's registers
- An optional interrupt output, when the signal generation is deactivated due to a time error
 
![Signal Generator IP](Additional%20Files/SigGeneratorIP.png) 

The configuration options of the core are:
- The system clock period in nanoseconds.
- The high resolution frequency multiplier, that indicates how many times faster than the system clock is the high resolution clock is  
- The option to enable/disable the cable delay support. If enabled, then the generator compensates for the cable delay which is provided in nanoseconds via the AXI configuration. 
- The output delay in nanoseconds which will be compensated by the generator
- The polarity of the generated signal. If '1', then the generated signal is active-high. Else, it is active-low. 

![Signal Generator Gui](Additional%20Files/SigGeneratorConfiguration.png)
## 3. Register Set
This is the register set of the Signal Generator. It is accessible via AXI4 Light Memory Mapped. All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/RegsetOverview.png)
### 3.2 Register Decription
The tables below describes the registers of the Signal Generator.     
![Control](Additional%20Files/Regset1_Control.png)
![Status](Additional%20Files/Regset2_Status.png)
![Polarity](Additional%20Files/Regset3_Polarity.png)
![Version](Additional%20Files/Regset4_Version.png)
![Cable](Additional%20Files/Regset5_Cable.png)
![IRQ](Additional%20Files/Regset6_Irq.png)
![MSK](Additional%20Files/Regset7_Msk.png)
![Start_L](Additional%20Files/Regset8_StartL.png)
![Start_H](Additional%20Files/Regset9_StartH.png)
![PulseL](Additional%20Files/Regset10_PulseL.png)
![PulseH](Additional%20Files/Regset11_PulseH.png)
![PeriodL](Additional%20Files/Regset12_PeriodL.png)
![PeriodH](Additional%20Files/Regset13_PeriodH.png)
![Repeat](Additional%20Files/Regset14_Repeat.png)
## 4 Design Description
The Signal Generator takes a (synchronized) time input as reference and generates the PWM signal aligned with this clock (start time, pulse width and period) compensating the output delay. The Signal Generator contains an AXI4Lite slave for configuration and status supervision from a CPU. The component consists of 3 main operations:
- Periodically generate the signal, aligned to the local time
- Fine-tune the rising and falling edge of the signal with a high resolution clock   
- Interface with the CPU (AXI master) via the AXI slave
### 4.1 Periodically generate the signal
The signal generation requires several configuration inputs: a start time which defines the first edge of the PWM signal, a pulse width which defines the duty cycle of the signal, a period which defines the period (in case of a repeating signal) and a pulse count which defines the number of pulses to be generated (or continuous, when set to '0').
When the Signal Generator is enabled and the new configuration inputs are set, it calculates the start time (beginning of pulse) and stop time (end of pulse). The start time is calculated the following way: input start time minus the output delay. The stop time is calculated the following way: calculated start time plus the pulse width. When the start time is reached (equal or bigger) the pulse is asserted to the configured polarity and a new start time is calculated by adding to the current start time the signal period. When the stop time is reached (also equal or bigger) the pulse is asserted to the inverse of the configured polarity, the new stop time is calculated by adding the period and a pulse counter gets incremented. 
This start/stop procedure is repeated until either the pulse count is reached or continuously repeated when the pulse count is set to zero.
When a time jump happens, the pulse generation is immediately disabled and the output signal set to the idle level of the polarity. This is required because of the
start/stop time calculations. This also means that a start time must be always in the future when the Signal Generator is configured.     
### 4.2 Compute the exact time of the generation
In order to compute the exact time of the generation, apart from compensating the output and cable delays at the start and stop times, the signal is also generated eventually by a high resolution clock, whose frequency is an integer multiple of the system clock's frequency. At the system clock domain, when the start or stop time is reached (equal or bigger), the time difference of the start/stop time to the current time is taken. The number of high resolution periods that 'fit' into this time difference, indicate when the high resolution clock should generate the rising and falling edges of the signal.    
### 4.3 AXI slave of the Signal Timestamper 
The Signal Generator includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the Signal Generator. An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. It also provides a status interface which allows to supervise the status of the generator. [Chapter 3](#3-register-set) has a complete description of the register set.
