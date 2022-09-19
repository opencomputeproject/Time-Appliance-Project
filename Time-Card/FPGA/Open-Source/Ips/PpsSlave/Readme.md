# PPS Slave Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The PPS Slave is a full hardware (FPGA) only implementation that calculates the offset and drift corrections to be applied to the local clock, in order to synchronize to a PPS input. 
On the event of a PPS reception, a timestamp is taken and the pulse gets evaluated. 
If the pulse's width and period are valid, then the offset and the drift corrections of the local clock are calculated, in reference to the PPS input. 
To smooth the corrections to the local clock, PI servo loops are used. The PPS Slave is configured and monitored by an AXI4L slave interface.

## 2. Interface Description
### 2.1 PPS Slave IP
The interface of the PPS Slave  is:
- System Reset and System Clock as inputs
- A high resolution Clock input, for high precision timestamps (an integer multiple of the System Clock, with a fix relationship to the System clock)
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- An event input, for which the timestamp will be taken
- Input Values of the PI coefficients, to optionally update the default values
- The calculated offset and drift adjustments, as outputs, to be applied to the [Adjustable Clock](../AdjustableClock/Readme.md)
- An AXI4L slave interface, via which the CPU reads and writes the core's registers
 
![PPS Slave IP](Additional%20Files/PpsSlave_IP.png) 

The configuration options of the core are:
- The system clock period in nanoseconds.
- The high resolution frequency multiplier, that indicates how many times faster is the high resolution clock than the system clock.
- The option to enable/disable the cable delay support. If enabled, then the PPS timestamp will be compensated for the cable delay (provided in nanoseconds via the AXI4L interface). 
- The input delay in nanoseconds which will be compensated by the PPS timestamp (e.g. for buffers, always positive).
- The polarity of the PPS input. If '1', then the timestamp will be taken at the rising edge of the pulse. Else, in the falling edge of the pulse.
- The default values of the coefficient factors for the Offset and Drift PI Servos, where:
    - Offset Coefficient I, is *K<sub>I_Offset</sub> = (OffsetMultiplyFactorI/OffsetDivideFactorI)\*2<sup>16</sup>*
    - Offset Coefficient P, is *K<sub>P_Offset</sub> = (OffsetMultiplyFactorP/OffsetDivideFactorP)\*2<sup>16</sup>*  
    - Drift Coefficient I, is *K<sub>I_Drift</sub> = (DriftMultiplyFactorI/DriftDivideFactorI)\*2<sup>16</sup>*
    - Drift Coefficient P, is *K<sub>P_Drift</sub>= (DriftMultiplyFactorP/DriftDivideFactorP)\*2<sup>16</sup>*    

![PPS Slave Gui](Additional%20Files/PpsSlave_Config.png)
## 3. Register Set
This is the register set of the PPS Slave. It is accessible via AXI4 Light Memory Mapped. All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. 
Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. 
Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/PpsSlave_Regset.png)
### 3.2 Register Decription
The tables below describes the registers of the PPS Slave.     
![Control](Additional%20Files/Reg1_Control.png)
![Status](Additional%20Files/Reg2_Status.png)
![Polarity](Additional%20Files/Reg3_Polarity.png)
![Version](Additional%20Files/Reg4_Version.png)
![PulseWidth](Additional%20Files/Reg5_PulseWidth.png)
![CableDelay](Additional%20Files/Reg6_CableDelay.png)
## 4 Design Description
The PPS Slave takes a PPS input as reference to the local time, and, it calculates the offset and drift correction to be applied to the local time.
The core consists of 4 main operations:
- The timestamping of the PPS input on the configurable edge (polarity) with a high resolution clock, including compensation for potential cable and input delays.  
- The validation of the PPS input. If the format of the pulse is not as expected, the timestamp is ignored.
- The calculation of the Offset and Drift correction, including PI servo loops.
- The AXI4L interface to configure and monitor the core. 

### 4.1 PPS timestamp with the high resolution clock
The frequency of the high resolution clock is an integer multiple of the system clock's frequency (provided as a configuration). The high resolution clock is used to reduce the jitter of the timestamp. 
The timestamp will be taken when the PPS edge is detected. The polarity of the edge is provided via configuration.
When the configured PPS edge is detected, the high resolution clock marks the event in a shift register. 
The shift register is copied at the system clock's domain, and so, the high resolution time "window" can be identified and can be correspondingly compensated.
In order to compute more accurately the time of the PPS event, the following delays are compensated:
- **the high resolution delay**. As described above, timestamping with the high resolution clock reduces the timestamp jitter, by computing a high resolution delay. 
This delay includes the register offset for switching between clock domains and the difference between the rising edge of the system clock and the high resolution clock, when the event occurred.  
- **the input delay**. It is provided as a configuration option to the core, during compilation time. It can be only a positive value.
- **the cable delay**. It is provided as a configuration option via a AXI register, given that the cable delay compensation is enabled. It can be only a positive value.

### 4.2 Validate the PPS input
The core validates the format of the PPS input, and raises error signals when a wrong format has been detected. 
The PPS input is checked towards:
- its Period
- its Pulse Width (Duty Cycle)

When a new PPS input is detected, a free-running timer starts counting the milliseconds until the next PPS input is detected (or until the max allowed value is reached). 
The value of this timer indicates the period of the input pulse. 
The period of the pulse is expected to be ~1 second. I.e., there is an acceptance time window around 1 second, currently set to 100ms. 
If the period timer is out of the expected time window (i.e. the new PPS input has been received too early or too late, in reference to the previously received PPS), then:
- the pulse's timestamp is ignored 
- a sticky period error is reported to the status AXI4L register (see Register 2 of [Chapter 3](#3-register-set))

Additionally, when a new PPS input is detected, a free-running timer starts counting the milliseconds, until the pulse is not active (or until the max allowed value is reached). 
The value of this timer indicates the pulse width of the input pulse. 
The width of the pulse is expected to be in certain limits, currently set to 1ms as minimum and 999ms as maximum value. 
If the pulse width is out of bounds then a sticky pulse-width error is reported to the status AXI4L register (see Register 2 of [Chapter 3](#3-register-set))

*Note*: In order to avoid initialization errors, when the PPS core is enabled (see Register 1 of [Chapter 3](#3-register-set)), the PPS input will be ignored until 2 active edges of the PPS input are detected.

### 4.3 Offset and Drift correction calculation
The calculation of the local clock offset and drift to the PPS input is done in 2 steps. 

Initially the offset and drift are calculated in reference to the PPS timestamp. 
The offset of the PPS timestamp is the difference of the timestamp to the closest change of second (i.e. positive offset if the timestamp is taken after a new second, negative timestamp if it is taken before a new second).
Additionally, the final calculation of the current drift is subtracted. The drift is the delta of 1 second to the difference of two consecutive timestamps, subtracting the previous offset, and normalizing over the interval of the 2 timestamps.

The calculated offset and drift corrections are sent to 2 PI servos, in order to introduce smooth adjustments to the local clock. 
The control function of the offset and drift PI Servos is 

![PiServo](Additional%20Files/PiServo.png)

where 
- ***u(t)*** is the control variable, the offset and drift adjustment to be applied to the local clock 
- ***e(t)*** is the error, i.e. the calculated, from the previous step, offset and drift, respectively
- ***K<sub>p</sub>*** is the proportional coefficient of the servo
- ***K<sub>i</sub>*** is the integral coefficient of the servo

The outputs of the PI servos are sent to the [Adjustable Clock](../AdjustableClock/Readme.md).
### 4.4 AXI slave of the PPS Slave 
The PPS Slave includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the PPS Slave. An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. 
It also provides a status interface which allows to supervise the status of the timestamper. [Chapter 3](#3-register-set) has a complete description of the register set. 
   
