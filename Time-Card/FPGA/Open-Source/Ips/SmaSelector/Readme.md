# SMA Selector Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The SMA Selector is a full hardware (FPGA) only implementation that multiplexes the outputs and demultiplexes the inputs 
of the 4 SMA connectors of the [Timecard](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card).
Each connector can be configured as input or output, depending on the configured mapping.
The following signals can be sourced by an SMA input:
- 10MHz Clock (only from SMA 1)
- External PPS 1
- External PPS 2
- Source to Signal Timestamper 1
- Source to Signal Timestamper 2
- Source to Signal Timestamper 3
- Source to Signal Timestamper 4
- Source to Frequency Counter 1
- Source to Frequency Counter 2
- Source to Frequency Counter 3
- Source to Frequency Counter 4 
- IRIG Slave (unused)
- DCF Slave (unused)
- External UART Rx

The following signals can be mapped to an SMA outputs:
- 10MHz pulse
- PPS of the FPGA
- PPS of the MAC
- PPS of the GNSS 1
- PPS of the GNSS 2
- IRIG Master (unused)
- DCF Master (unused)
- Signal Generator 1
- Signal Generator 2
- Signal Generator 3
- Signal Generator 4
- GNSS 1 UART Messages
- GNSS 2 UART Messages
- External UART Tx

The possible mappings of the directions of the SMA data are:
|Connector|Selection 1|Selection 2|
|-----|---------|---------|
|SMA 1|Input|Output|
|SMA 2|Input|Output|
|SMA 3|Output|Input|
|SMA 4|Output|Input|

The configured mapping is done via 2 AXI4L slave interfaces, named AXI1 and AXI2. Each slave interface controls one mapping option.

## 2. Interface Description
### 2.1 SMA Selector IP
The interface of the SMA Selector  is:
- System Reset and System Clock as inputs
- The SMA output sources, as core inputs
- The SMA input sources, as core outputs 
- An SMA signal for each SMA connector, as input, in case the connector is configured as input
- An SMA signal for each SMA connector, as output, in case the connector is configured as output   
- Enabling signals of input and output for each SMA connector, as outputs
- An AXI4L slave interface, for configuration mapping 1 (AXI1)
- An AXI4L slave interface, for configuration mapping 2 (AXI2)
 
![SMA Selector IP](Additional%20Files/SmaSelectorIP.png) 

The core has the following configuration options. 

![SMA Selector CONFIG](Additional%20Files/SmaSelectorConfig.png) 
 
## 3. Register Set
The SMA Selector has two register sets, one for each mapping configuration. Each mapping configuration is accessible via AXI4 Light Memory Mapped. 
All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. 
Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. 
Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set 1
The Register set 1 configures mapping 1. In mapping 1, SMA 1 and SMA 2 are inputs, while SMA 3 and SMA 4 are outputs. 

Additionally, the Register Set 1 provides the status of the 4 SMA inputs.

#### 3.1.1 Register Set 1 Overview 
The Register Set 1 overview is shown in the table below. 
![RegisterSet1](Additional%20Files/Regset1_Overview.png)
#### 3.1.2 Register Decription
The tables below describe the registers of the SMA Selector's mapping 1. 
![InputSel1](Additional%20Files/Regset1_1_InputSel.png)
![OutputSel1](Additional%20Files/Regset1_2_OutputSel.png)
![Version1](Additional%20Files/Regset1_3_Version.png)
![InputStatus](Additional%20Files/Regset1_4_InputStatus.png)
### 3.2 Register Set 2
The Register set 2 configures mapping 2. In mapping 2, SMA 1 and SMA 2 are outputs, while SMA 3 and SMA 4 are inputs. 
#### 3.2.1 Register Set 2 Overview 
The Register Set 2 overview is shown in the table below. 
![RegisterSet2](Additional%20Files/Regset2_Overview.png)
#### 3.2.2 Register Decription
The tables below describe the registers of the SMA Selector's mapping 2. The version is identical for mapping 1 and 2 (**should i remove version for mapping2?**)     
![InputSel2](Additional%20Files/Regset2_1_InputSel.png)
![OutputSel2](Additional%20Files/Regset2_2_OutputSel.png)
![Version2](Additional%20Files/Regset2_3_Version.png)

## 4 Design Description
The SMA Selector multiplexes the input and output options, according to the configuration mappings. 
The core contains 2 AXI4Lite slaves for configuration and status supervision from a CPU. 

The component consists of 2 main operations:
- Map the SMA inputs and outputs     
- Interface with the CPU (AXI master) via the 2 AXI slaves
### 4.1 Map the SMA inputs and outputs

There are 4 possible SMA Input Source Select signals received from configuration (see [Chapter 3](#3-register-set)). Each of them defines the usage of the corresponding SMA input. **The usage of an SMA Input X, where X=1,2,3,4, is mapped in the following way:**  
|SMA Input X is source to |Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|Bit 8|Bit 9|Bit 10|Bit 11|Bit 12|Bit 13|Bit 14|Bit 15|
|-------------------------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:----:|:----:|:----:|:----:|:----:|:----:|
|Ext PPS 1|1|
|Ext PPS 2||1|
|Signal Timestamper 1|||1|
|Signal Timestamper 2||||1|
|IRIG Slave (unused)|||||1|
|DCF Slave (unused)||||||1|
|Signal Timestamper 3|||||||1|
|Signal Timestamper 4||||||||1|
|Frequency Counter 1|||||||||1|
|Frequency Counter 2||||||||||1|
|Frequency Counter 3|||||||||||1|
|Frequency Counter 4||||||||||||1|
|External UART Rx|||||||||||||1|
|10 MHz enable*|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
|Enable SMA Input X||||||||||||||||1|

*Note: The 10 MHz pulse is supported only via the SMA input 1 and it is enabled if the SMA Select Source Input 1 is not mapped to any other selection. 
*Note 2: An SMA input can be the source to multiple destinations.   
 
There are 4 possible SMA Output Source Select signals received from configuration (see [Chapter 3](#3-register-set)). Each of them defines what should be sent to the corresponding SMA output. **The connection of an SMA Output X, where X=1,2,3,4, is mapped in the following way**  

|SMA Output X is sourced from|Bit 0|Bit 1|Bit 2|Bit 3|Bit 4|Bit 5|Bit 6|Bit 7|Bit 8|Bit 9|Bit 10|Bit 11|Bit 12|Bit 13|Bit 14|Bit 15|
|-------------------------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|FPGA PPS|1|
|MAC PPS||1|
|GNSS 1 PPS|||1|
|GNSS 2 PPS||||1|
|IRIG Master (unused)|||||1|
|DCF Master (unused)||||||1|
|Signal Generator 1|||||||1|
|Signal Generator 2||||||||1|
|Signal Generator 3|||||||||1|
|Signal Generator 4||||||||||1|
|UART GNSS 1 UART Messages|||||||||||1|
|UART GNSS 2 UART Messages||||||||||||1|
|External UART Tx|||||||||||||1|
|GND||||||||||||||1|
|VCC|||||||||||||||1|
|10 MHz pulse|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|
|Enable SMA Output X||||||||||||||||1|
### 4.2 AXI slave of the SMA Selector 
The SMA Selector includes 2 AXI Light Memory Mapped Slaves. Each slave provides access to the registers of a mapping and allows to configure the core. 
An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. [Chapter 3](#3-register-set) has a complete description of the register set.
