# PPS Source Selector Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Design Description](#3-design-description)

## 1. Context Overview
The PPS Source Selector is a full hardware (FPGA) only implementation that detects the available PPS sources and selects the PPS source according to a priority scheme and a configuration.  
The configuration and monitoring of the core is done via an optional external AXI4L slave interface (see the register set of the [Clock Detector](../ClockDetector)). 
If a configuration input is not provided, then the PPS selection is done based on the available PPS inputs and a default priority scheme.  
## 2. Interface Description
### 2.1 PPS Source Selector IP
The interface of the PPS Source Selector is:
- System Reset and System Clock as inputs
- The configuration to select a PPS source, as input (typically provided by an external AXI4L slave interface)
- The available PPS sources, as output (typically provided to an external AXI4L slave interface)
- The PPS signals, as inputs (SMA, MAC, GPS)
- The selected PPS signals, as outputs (PPS Slave, MAC) 

![PPS Source Selector IP](Additional%20Files/PpsSourceSelectorIP.png) 

The system clock period and the validation threshold of an input PPS are provided as configuration options:

![PPS Source Selector Conf](Additional%20Files/PpsSourceSelector_Conf.png) 

## 3 Design Description
The PPS Source Selector detects and evaluates the available PPS inputs and selects the ones to be used, based on a configuration input and a priority scheme. 
The component consists of 2 main operations:
- Evaluate the available PPS inputs
- Select the PPS sources
### 3.1 Evaluate the available clocks
A PPS input is considered available, if it passes 2 evaluation criteria:
- The pulse period is considered valid, if the period of the pulse is 1 second, with a ±10% error window
- The pulse period is valid for more than "Pps Available Threshold" seconds in a row 
### 3.2 Select the PPS sources
There are 4 possible configuration options for the selection of the Slave PPS source.
|Configuration|Source Select Value|Description|
|-------------|:-----------------:|-----------|
|Auto Select|0|The default selection priority is (1) SMA PPS (2) MAC PPS (3) GPS PPS|
|SMA|1|Force the SMA PPS input to the Slave PPS output. If the SMA PPS is unavailable, the output is 0.|
|MAC|2|Force the MAC PPS input to the Slave PPS output. If the MAC PPS is unavailable, the output is 0.|
|GPS|3|Force the GPS PPS input to the Slave PPS output. If the GPS PPS is unavailable, the output is 0.|

There are 3 possible configuration options for the selection of the MAC PPS source.
|Configuration|Source Select Value|Description|
|-------------|:-----------------:|-----------|
|Auto Select|0|The default selection priority is (1) SMA PPS (2) GPS PPS 
|SMA|1|Force the SMA PPS input to the MAC PPS output. If the SMA PPS is unavailable, the output is 0.|
|GPS|2/3|Force the GPS PPS input to the MAC PPS output. If the GPS PPS is unavailable, the output is 0.|

