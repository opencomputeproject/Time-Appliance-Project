# Dummy Axi Slave Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Dummy Axi Slave is an FPGA core that consists of an AXI4L slave interface. This interface provides valid responses to AXI4L accesses of the CPU. 
The core is used as a "placeholder" of a specific address range. 

For example, if the CPU's driver tries to access via AXI4L an address which does not belong in a specified register of any instantiated FPGA core, then the bus will return access fault.  On the other hand, if the CPU's driver accesses an address 
in the DummyAxiSlave address range, then the core will send an empty but valid AXI4L response. 

The intention of this core is to be used if the same CPU driver version is used for multiple FPGA versions of the Time card, which might have specified different address ranges (e.g. due to different core instantiations).  

## 2. Interface Description
### 2.1 Core List IP
The interface of the Core List is:
- System Reset and System Clock as inputs
- An AXI4L slave interface, via which the CPU reads and writes data to the specified address space
 
![DummyAxiSlave IP](Additional%20Files/DummyAxiSlave%20IP.png) 

The configuration option of the core is:
- The System Clock period in nanoseconds 

![DummyAxiSlave Gui](Additional%20Files/DummyAxiSlave%20Customization%20options.png)

## 3. Register Set
All addresses of the specified address range will send a valid AXI4L response when accessed. However, the actual memory size might be much smaller than the address range, so the data read from the DummyAxiSlave should not be considered valid. 

## 4 Design Description
The Core List core consists only of an AXI4L slave that reads and writes the data from/to a RAM. The actual size of the RAM is 1 kB. If the specified address range is larger than 1 kB, then the access of address *"A"* is actually accessing the RAM address 
*"A % 1024"*.

