# Core List Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](4-design-description)

## 1. Context Overview
The Core List is an FPGA core that provides to the CPU the list of the FPGA cores which are currently instantiated in the FPGA version. The intention is that the CPU accesses the Core List during startup, to either verify or to set properly the address range for each FPGA core instance, the interrupt masks, etc.
The process of providing the core list to the CPU is:
- A Core List text file is created manually, before compiling the FPGA design. It includes the FPGA core's information at a predefined format. 
- The text file is loaded to the FPGA during compilation time. The FPGA stores the file as a ROM.
- The CPU can read the Core List information by accessing the AXI registers of the CoreList core.   

## 2. Interface Description
### 2.1 Core List IP
The interface of the Core List is:
- System Reset and System Clock as inputs
- An AXI4L slave interface, via which the CPU reads the Core List information (the base address shall be always at a fixed address so software knows where to look for it)
- A sticky flag output that the list has been read by the CPU
 
![Core List IP](Additional%20Files/CoreList%20IP.png) 

The configuration options of the core are:
- The System Clock period in nanoseconds 
- The local path of the text file that includes the Core List 

![Core List Gui](Additional%20Files/Core%20List%20Customization%20options.png)
### 2.2 Core List Text file
The Core List text file is a configuration input to the Core List core. The file's path, name and contents are defined before the compilation of the FPGA.
The format of the Core List file is predefined: 
- Each line of the text file describes a core instantiation of the FPGA
- Each line should have 8 fields, as shown in the example below.   

|Core Type Nr|Core Instance Nr|Version Nr|Address Range Low|Address Range High|Interrupt Nr|Interrupt Sensitivity|Magic word (max 36 ASCII characters, optional field)|
|--------|--------|--------|--------|--------|--------|--------|------------------------------------|
|00000001|00000002|00000003|10000000|1000FFFF|0000001A|00000001|Core Type Example 1|

- A template format of the file can be found at the [Core List Template](Additional%20Files/CoreListTemplate.txt).
- The first 7 fields are 8-digit HEX values seperated by a 'space'character. (no leading "0x", see also [Chapter 4.1](#41-text-file-parsing))
- The last field ('Magic Word') is an optional short description of the instance as ASCII text. It can be up to 36 characters long.
- Empty lines (starting with 'CR', 'LF', 'NUL', 'HT', or ' ' characters), or commented lines (starting with "--" or "//") are skipped.

The Core List core will add an invalid Core Type Nr ("00000000") as an indication to the CPU that all the Core List reading has been completed. 

### 2.3 Core List contents
Some **Core Types** are predefined:

|Core|Core Type Nr (hex)|
|--------------------------|:------:|
|Invalid/End                         |0x00000000|
|TC Core List Core Type              |0x00000001|
|TC Adjustable Clock Core Type       |0x00000002|
|TC Signal Generator Core Type       |0x00000003|
|TC Signal Timestamper Core Type     |0x00000004|
|TC PPS Generator Core Type          |0x00000005|
|TC Frequency Counter                |0x00000006|
|TC Clock Detector Core Type         |0x00000007|
|TC SMA Selector Core Type           |0x00000008|
|TC PPS Source Selector Core Type    |0x00000009|
|TC FPGA Version Core Type    		 |0x0000000A|
|TC PPS Slave Core Type    			 |0x0000000B|
|TC ToD Slave Core Type			     |0x0000000C|
|TC Dummy Axi Slave Core Type        |0x0000000D|
|Xilinx AXI PCIe Core Type           |0x00010000|
|Xilinx AXI GPIO Core Type           |0x00010001|
|Xilinx AXI IIC Core Type            |0x00010002|
|Xilinx AXI UART Core Type           |0x00010003|
|Xilinx AXI HWICAP Core Type         |0x00010004|
|Xilinx AXI QUAD SPI FLASH Core Type |0x00010005|

The **Core Intstance Nr** indicates the instantiation number of a core type, starting counting from "0".

The **Version Nr** is typically represented in the format 0xMMmmBBBB, where MM is the Major version (8 bits), mm is the Minor version (8 bits), BBBB is the Built version (16 bits). If only 2 bytes are used for the version of a core, then the build version is considered "0".  

**AddressRangeLow** is the offset address of the core's AXI slave, while **AddressRangeHigh** is the highest address of the core's AXI slave.

The current design can provide up to 32 interupts. The **Interrupt Nr** has range 0x0-0x1F, while the value 0xFFFFFFFF is used when the core does not send any interrupt.

The **Interrupt Sensitivity** contains information for a core's interrupt. 
- When bit0 is "0" the interrupt is Edge. Otherwise, the iInterrupt is Level
- When bit1 is "0", the interrupt is active low. Otherwise, the interrupt is active high.

The **Magic Word** is a short representation of the core instance in ascii chars (maximum 36 ascii chars, by default).

## 3. Register Set
The CPU receives the Core List by AXI accessing the memory-mapped registers of the Core List core.  
Each core instance allocates a predefined address range of 16 x 32-bit words (64 byte address space per core in the ROM). The core instances are stored in the ROM according to the order that is given in the text file. For conventional purposes, the Core List instantiation should be always the 1st instance to be specified in the file and the **CoreList's address space should be fixed to the range [0x01300000-0x0130FFFF]**.
**The CPU should start reading from the base address of the Core List (0x01300000), and stop the read accesses when it reads an invalid Core Type Nr (0x000000).** 
### 3.1 Register Set Overview
The Core Type Nr of each core instance is always stored in addresses **[0x01300000 + N*(0x40)]**, where **N** is the N-th instance, as defined in the Core List text file.     
The Register Set overview is shown in the table below. The table presents the addresses of the first 2 instances in the list as well as the N-th instance.
![RegisterSet](Additional%20Files/CoreListRegsetOverview.png)
### 3.2 Register Decription
The tables below describe the registers of the first core of the list. The registers of the other cores are identical, apart from the offset addresses, which are mentioned above.    
![CoreTypeNr](Additional%20Files/Regset1.TypeNr.png)
![CoreInstanceNr](Additional%20Files/Regset2.InstanceNr.png)
![VersionNr](Additional%20Files/Regset3.VersionNr.png)
![AddressLow](Additional%20Files/Regset4.AddressLow.png)
![AddressHigh](Additional%20Files/Regset5.AddressHigh.png)
![Interrupt](Additional%20Files/Regset6.Interrupt.png)
![Sensitivity](Additional%20Files/Regset7.Sensitivity.png)
![Word1](Additional%20Files/Regset8_Word1.png)
![Word2](Additional%20Files/Regset8_Word2.png)
![Word3](Additional%20Files/Regset8_Word3.png)
![Word4](Additional%20Files/Regset8_Word4.png)
![Word5](Additional%20Files/Regset8_Word5.png)
![Word6](Additional%20Files/Regset8_Word6.png)
![Word7](Additional%20Files/Regset8_Word7.png)
![Word8](Additional%20Files/Regset8_Word8.png)
![Word9](Additional%20Files/Regset8_Word9.png)
## 4 Design Description
The Core List core consists of 2 main parts 
- accessing the text file and loading it to a ROM (at compile time)
- AXI4L slave that provides the contents of the ROM to the CPU

### 4.1 Text file parsing
The function that accesses the text file (its path is provided as a configuration during compilation time) expects a specific format. Otherwise, a 'failure' is reported. The parsing of the file is done line-by-line and completes when the EOF is reached.
The acceptable format of each text line is:
- the line starts with a CR, LF, HT, NUL, ' '. The line is skipped
- the line starts with comment characters "--" or "//". The line is skipped
- a line with a core description should have 
  - the characters 1-9 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Core Type Nr**)
  - the characters 10-18 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Core Instance Nr**)
  - the characters 19-27 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Core Version Nr**)
  - the characters 28-36 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Address Range Low**)
  - the characters 37-45 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Address Range High**)
  - the characters 46-54 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') followed by a space character (' ') (**Interrupt Nr**)
  - the characters 55-62 are an 8-digit HEX value (characters '0'-'9' or 'A'-'F' or 'a'-'f') (**Interrupt Sensitivity **)
  - the character 63 should be either a special character NUL/LF/CR/HT (which would cause the function to end the line processing and move on to the next one), or a space character (' '), which would denote that the line character is the start of the optinal Magic Word field
  - the characters 64-99 are an optional text field which can be up to 36 characters. THe text is parsed character-by-character and the parsing is completed when a special character would indicate the end of line, or when 36 characters have been parsed. Any additional text in the text file will be ignored (truncated). (**Magic Word**)  
 
 ### 4.2 AXI slave 
The CPU is expected to read the ROM content sequentially, starting from the Based Address and complete when an invalid Core Type Nr is read ("0x00000000"). 
The CPU AXI read accesses are 32-bit wide. They should access word-aligned addresses (4-byte alignment). Otherwise, the FPGA will provide automatically the contents of the word-aligned address. For example, a read access on address [0x01300004] will provide the data of addresses [0x01300004]-[0x01300007]. A read access on addresses [0x01300005], [0x01300006] and [0x01300007] will also provide the same data. This is according to the AXI4L spec.  



 
