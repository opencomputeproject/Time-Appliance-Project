# Adjustable Clock Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Adjustable Clock consists of a timer clock in the Second and Nanosecond format that can be frequency and phase adjusted.
It adds the clock period it runs on, in nanoseconds, every clock cycle.The nanosecond counter has an overflow at 1000000000 nanoseconds. At each overflow of the nano-second counter it will add an additional second to the seconds counter.
For adjustments, additional nanoseconds can be added or subtracted from the standard period to adjust frequency and phase. When small offset adjustments are not suitable, the time can be overwritten to hard set the clock to a new time e.g. for the startup phase where a jump from the 1.1.1970 to the present is required.
An adjustment process takes the offset, drift and time adjustments as inputs and converts them into a combined adjustment which is then applied to the timer clock. The offset and drift is converted into evenly spread adjustments which allows smooth corrections on the clock without time jumps. E.g. a drift of 1ppm is adjusted as 1 ns every 5000000 clock cycles at 50MHz. In parallel to the correction a quality computation done to mark the in-sync state of the clock if corrections are below a certain threshold for at least 4 corrections.
The drift and offset adjustments inputs are normaly outputs of PI servo loops (not required though), which are not part of the Adjustable clock. The PI servo loop parameters however are provided by the configuration of the Adjustable Clock to  output signals. The drift and offset adjustments of the servos can be chosen individually, since frequency changes might happen quite slowly where offset adjustments probably shall be done much faster. 
Since the Adjustable Clock is the heart of a synchronization solution it can take several (5) adjustment inputs from different cores as input. Only one adjustment input is taken as source for corrections at the time. From the Registerset, the multiplexer gets the source selection of the adjustments. The adjustments can be taken by one of the 5 source inputs or by the CPU via the AXI registers (REG mode). In this case, the adjustment calculations (e.g. also via PI servo loops) are executed on a CPU and their outputs are provided to the core as time, offset and drift adjustements. 
Additionally, the Registerset allows reading the in-sync state and taking a snapshot of the time.  

## 2. Interface Description
### 2.1 ADjustable Clock IP
The interfave of the Adjustable Clock is:
- System Reset and System Clock as inputs
- An AXI4L slave interface, via which the CPU can read and write the Adjustable Clock registers (including the register Adjustments, if in REG mode)
- The 5 input adjustemnts of time, offset and drift
- The adjustable clock output, which is provided to other cores as reference time
- The PI servo coefficients of offset and drift as outputs, as received by the Registerset by the CPU. If a synchronization source other than REG is selected (e.g. input source 1 (maybe a PPS slave)), these coefficients should be used by the corresponding PI servo loops.  
- The adjustable clock's status flag outputs (InSync and InHoldover)
 
![Adjustable Clock IP](Additional%20Files/Adjustable%20Clock%20IP.png) 

The configuration options of the core are:
- The System Clock period in nanoseconds. 
- The InSync threshold in nanoseconds. At least 4 consecutive offset adjustment should be less than this thershold, in order for the adjustbale clock to be considered synchronized. 
- The InHoldover timeout in seconds. If the adjustable clock is already synchronized (i.e. InSync is active) and the no offset adjustment has been received for the given timeout, then the clock is considered InHoldover.  

![Adjustable CLock Gui](Additional%20Files/Adjustable%20Clock%20configuration.png)
## 3. Register Set
This is the register set of the Adjustable Clock. It is accessible via AXI4 Light Memory Mapped. All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 
![RegisterSet](Additional%20Files/Regset.png)
### 3.2 Register Decription
The tables below describe the registers of the Adjustable Clock.     
![Control](Additional%20Files/Regset1_Control.png)
![Status](Additional%20Files/Regset2_Status.png)
![Select](Additional%20Files/Regset3_Select.png)
![Version](Additional%20Files/Regset4_Version.png)
![TimeL](Additional%20Files/Regset5_TimeL.png)
![TimeH](Additional%20Files/Regset6_TimeH.png)
![AdjL](Additional%20Files/Regset7_AdjL.png)
![AdjH](Additional%20Files/Regset8_AdjH.png)
![OffsetValue](Additional%20Files/Regset9_OffsetValue.png)
![OffsetIntrv](Additional%20Files/Regset10_OffsetIntrv.png)
![DriftVal](Additional%20Files/Regset11_DriftVal.png)
![DriftIntrv](Additional%20Files/Regset12_DriftIntrv.png)
![InSyncThresh](Additional%20Files/Regset13_InSyncThresh.png)
![Offset_ServoP](Additional%20Files/Regset15_OffsetServoP.png)
![Offset_ServoI](Additional%20Files/Regset14_OffsetServoI.png)
![Drift_ServoP](Additional%20Files/Regset16_DriftServoP.png)
![Drift_ServoI](Additional%20Files/Regset17_DriftServoI.png)
![ClkOffset](Additional%20Files/Regset18_ClkOffset.png)
![ClkDrift](Additional%20Files/Regset19_ClkDrift.png)
## 4 Design Description
The Adjustable Clock supports up to 5 external adjustments, plus a register adjustment, which is provided by the CPU via the AXI slave (REG mode). Each adjustment input can provide a direct time set to the timer clock (TimeAdjustment), or a phase correction (OffsetAdjustment), or a freqeuncy correction (DriftAdjustment). The component provides to the output the timer clock ClockTime and its status flags InSync and InHoldover. Also, the PI servo coefficients which are received by the AXI registers are forwarded to the output.
The component is divided into 4 main operations:
- the spreading of the offset and drift adjustements over their interval window 
- the adjustment of the timer clock by setting the time adjustment or by applying the offset and drift adjustments
- the reporting of the adjustable clock quality by setting the InSync and InHoldover flags
- the AXI slave for interfacing the clock adjustments with the CPU  
### 4.1 Spreading of Offset and Drift Adjustments
When a time adjustment is applied, the timer clock takes over directly this time (time set).    
When an offset or drift adjustment is applied, the component takes the adjustment inputs and converts them into periodic small adjustments which are then set to the timer clock. E.g. a drift of 1ns per 1000ns is converted to a 1ns adjustment every 50 clock cycles at a frequency of 50MHz. For offset it works similar. E.g. 50ns shall be corrected in 2000ns which will be converted into 1ns every second clock cycle at 50MHz. If the offset adjustment is too big (more than 1ns per clock cycle needs to be corrected), then the a time set is applied. When the drift adjustment is too big (more than 1ns per clock cycle needs to be corrected), then the drift correction is set to the maximum value (1ns per clock cycle). Finally, the process combines the offset and drift correction to be applied to the timer clock.
The drift adjustments are always the full drift to applied, not only the delta to the last adjustment.
### 4.2 Apply corrections to the timer clock
The timer clock is a time counter that consists of Second and Nanosecond fields. At each system clock cycle, the timer clock either increases by the period of the system clock or applies a correction.

In more detail, at each system clock cycle:  
- if a time adjustment is active, time set the timer clock
- if both a positive offset and a positive drift adjustments are active, the timer clock increases by the period of the system clock plus 2ns
- if both an offset and a positive drift adjustments are active, but they have different signs, the timer clock increases by the period of the system clock ((-1) + 1 = 0; or 1 + (-1) = 0; so no adjustment)
- if both a negative offset and a negative drift adjustments are active, the timer clock increases by the period of the system clock minus 2ns
- if only a positive offset or positive drift adjustment is active, the timer clock increases by the period of the system clock plus 1ns
- if only a negative offset or negative drift adjustment is active, the timer clock increases by the period of the system clock minus 1ns
- else, the timer clock increases by the period of the system clock
### 4.3 Adjustable Clock quality flags
The quality flags of the clock's status InSync and InHoldover are provided to the output.

The InSync flag is
- activated, if for 4 consecutive offset adjustments the corrections are less than a predefined threshold.
- deactivated, if a time set is a applied (i.e. a time adjustment is applied or an offset adjustment is too big) or if the clock is disabled.

The InHoldover flag is
- activated, if the timer clock has been InSync and an offset adjustment has not been received for a predefined timeout time
- deactivated, if the timer clock goes out of sync, or if a time or if an offset adjustment is received, or if the clock is disabled
### 4.4 AXI slave of the adjustable clock 
The Adjustable CLock includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the Adjustable Clock. An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. It also provides a status interface which allows to supervise the status of the clock. [Chapter 3](#3-register-set) has a description of the register set. 
 
