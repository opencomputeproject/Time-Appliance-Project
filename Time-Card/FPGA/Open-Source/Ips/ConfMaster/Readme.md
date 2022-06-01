# Conf Master Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Design Description](3-design-description)

## 1. Context Overview
The Conf Master is an FPGA core that provides a configuration to the other FPGA cores. The intention is that a fixed configuration can be provided to the FPGA during startup without the support of the CPU. The fixed configuration is provided via a text file, which is loaded to an FPGA ROM during compilation time.    
The process of providing the configuration to the FPGA cores is:
- A configuration text file is created manually, before compiling the FPGA design. It includes commands for accessing the FPGA cores' AXI registers at a predefined format. 
- The text file is loaded to the FPGA bitstream during compilation time. The the file is converted as load of a ROM as part of the FPGA bitstream.
- At startup (e.g. when the reset is deactivated), the Conf Master reads the ROM data and accesses the AXI registers of the FPGA cores' correspondingly.   

## 2. Interface Description
### 2.1 Conf Master IP
The interface of the Conf Master is:
- System Reset and System Clock as inputs
- An AXI4L master interface, via which the Conf Master accesses the FPGA cores' registers 
- A sticky flag output that the configuration has been completed
 
![Conf Master IP](Additional%20Files/ConfMasterIP.png) 

The configuration options of the core are:
- The System Clock period in nanoseconds 
- The local path of the configuration text file
- The AXI timeout in system clock cycles. If the register access has not completed when the timeout is reached, the specific access is skipped. If the AXI timeout is '0', then there is no timeout, and the access will wait forever until it is completed.  

![Conf Master Gui](Additional%20Files/ConfMasterConfiguration.png)
### 2.2 Configuration text file
The configuration text file is a generic input to the Conf Master core. The file's path, name and contents are defined before the compilation of the FPGA.
The format of the configuration file is predefined: 
- Each line of the text file is a command. 
- Each line should have 4 fields, as shown in the example below.   

|Command Type|Base Address|Register Address|Data|
|--------|--------|--------|--------|
|00000004|01010000|00000008|00000001|
|00000002|00000000|00000000|40000000|

- A template format of the file can be found at the [Configuration Template](Additional%20Files/ConfigurationTemplate.txt).
- The 4 fields are 8-digit HEX values seperated by a 'space'character. (no leading "0x", see also [Chapter 3.1](#31-text-file-parsing))
- Empty lines (starting with 'CR', 'LF', 'NUL', 'HT', or ' ' characters), or commented lines (starting with "--" or "//") are skipped.
- Invalid lines (e.g. too short or unexpected format) are also skipped.

### 2.3 Command structutre 
The **Command Type** field is described in the table below.

|Command Type|Enumeration|Description|
|--------|----------|---------------------------|
|Skip|0x00000001|Skip the specific command|
|Wait|0x00000002|Wait in nanoseconds before moving on to the next command|
|Read|0x00000003|Read a specific AXI register|
|Write|0x00000004|Write a specidic AXI register|

The **Base Address** field is used by the Read and Write commands. As the name indicates, it is the base address of an FPGA core's registers that will be accessed. 

The **Register Address** field is used by the Read and Write commands. As the name indicates, it is the address of the 32-bit register (offset by Base Address).

The **Data** field is used by the Write and Wait commands. The Write command, writes the Data to the indicated register. The Wait command waits "Data" nanoseconds. 

## 3 Design Description
The Conf Master core consists of 2 main parts 
- accessing the text file and loading it to a ROM (at compile time)
- AXI4L master for accessing the AXI registers according to the contents of the ROM

### 3.1 Text file parsing
The function that accesses the text file (its path is provided as a configuration during compilation time) expects a specific format. The parsing of the file is done line-by-line and completes when the EOF is reached.
The acceptable format of each text line is:
- The line starts with a CR, LF, HT, NUL, ' '. The line is skipped.
- The line starts with comment characters "--" or "//". The line is skipped
- A line is accepted as a command if:  
  - the characters 1-9 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Command Type**)
  - the characters 10-18 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Base Address**)
  - the characters 19-27 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Register Address**)
  - the characters 28-35 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') (**Data**)
  - additional characters in the line are ignored
  
Examples:
- Do a Write to address 0x01000120 with 0xDEADBEEF
  - 00000004 01000000 00000120 DEADBEEF
- Wait for a second (3B9ACA00 = 1000000000)
  - 00000002 00000000 00000000 3B9ACA00

 ### 3.2 AXI master 
The Conf Master should be able to access the AXI registers of other FPGA cores. Therefore, the AXI master can read and write 32-bit registers. A deviation from the AXI4L spec is the following: if the AXI timeout is reached and the access is not yet completed (e.g. the AXI slave has not responded yet), then the access is skipped, instead of blocking the bus forever. 
