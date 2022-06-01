# FPGA Version Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)


## 1. Context Overview
The FPGA Version core is a 32-bit register accessible by an AXI4-Lite interface.
The register consists of 2 parts:
- the FPGA version, which occupies the 2 last bytes (LSB) of the register
- the FPGA Golden version, which occupies the 2 last bytes (LSB) of the register
An input selects which of the 2 versions is provided to the AXI interface. 

## 2. Interface Description

The interface of the Core List is:
- System Reset and System Clock as inputs
- An AXI4L slave interface, via which the CPU reads the FPGA version information
- An input that selects which of the 2 versions is provide to the AXI interface
 
![FPGA Version IP](Additional%20Files/FpgaVersion_IP.png) 

The configuration options of the core are the FPGA and GOlden FPGA versions

![FPGA Version Config](Additional%20Files/FpgaVersion_Config.png)

## 3. Register Set
The CPU receives the versions by AXI accessing the memory-mapped register of the FPGA Version core.
![FPGA Version Regset](Additional%20Files/FpgaVersion_Regset.png)

