**IMPORTANT**


**This is a preliminary version of the TimeCard with the [LitePcie](https://github.com/enjoy-digital/litepcie) core as the PCIe to AXIMM bridge.**

**!!! The current driver does not support this version !!!**


The Version with LitePCIe has different Memory-Mapped Offsets (+ 0x0200_0000) as well as different MSI-Numbers (+ 32).
Check the differences in:

[2. Address Mapping](#2-address-mapping)

and 

[3. Interrupt Mapping](#3-interrupt-mapping)

Interrupts must be enabled: 0xFF to register 0x1800:
[csr.h](https://github.com/opencomputeproject/Time-Appliance-Project/blob/c1beb991008c8410d31bb13e4aadf1443c65d549/Time-Card/FPGA/Open-Source/Ips/LitePcie/software/include/generated/csr.h#L154C9-L154C34)




The LitePCIe Core supports MSI-X as well as PTM (with a correct driver).
More details about PTM and LitePCIe can be found here: https://github.com/enjoy-digital/litepcie_ptm_test

A pre-generated version of the LitePCIe version is available here: [LitePCIe](../../../Ips/LitePcie)

**IMPORTANT**

Generate the binaries only with the CreateBinariesAll.tcl, CreateBinaries.tcl, CreateBinariesGolden.tcl.

An additional PostSynth.tcl script which is called after Synthesis is required.



# Open Source Timecard Design Description
## Contents

[1. Design Overview](#1-design-overview)

[2. Address Mapping](#2-address-mapping)

[3. Interrupt Mapping](#3-interrupt-mapping)

[4. SMA Connectors](#4-sma-connectors)

[5. Status LEDs](#5-status-leds)

[6. Default Configuration](#6-default-configuration)

[7. Core List](#7-core-list)

[8. Create FPGA project and binaries](#8-create-fpga-project-and-binaries)

[9. Program FPGA and SPI Flash](#9-program-fpga-and-spi-flash)

## 1. Design Overview

The original Open Source Timecard design includes open-source IP cores from [NetTimeLogic](https://www.nettimelogic.com/) and free-to-use IP cores from [Xilinx](https://www.xilinx.com/).
Additionally it use the a pre-generated version of the [LitePCIe](https://github.com/enjoy-digital/litepcie) which supports MSI-X and PTM.
The following cores are used in the Open Source Timecard design.

|Core|Vendor|Description|
|----|:----:|-----------|
|[LitePCIe](https://github.com/enjoy-digital/litepcie) |EnjoyDigital|LitePCIe provides a small footprint and configurable PCIe core. It is the interface between the AXI4 interface and the PCI Express (PCIe)|
|[AXI GPIO](https://www.xilinx.com/products/intellectual-property/axi_gpio.html) |Xilinx|General purpose input/output interface to AXI4-Lite|
|[AXI I2C](https://www.xilinx.com/products/intellectual-property/axi_iic.html) |Xilinx| Interface between AXI4-Lite and IIC bus|
|[AXI UART 16550](https://www.xilinx.com/products/intellectual-property/axi_uart16550.html)|Xilinx|Interface between AXI4-Lite and UART interface|
|[AXI HWICAP](https://www.xilinx.com/products/intellectual-property/axi_hwicap.html) |Xilinx|AXI4-Lite interface to read and write the FPGA configuration memory through the Internal Configuration Access Port (ICAP)|
|[AXI Quad SPI Flash](https://www.xilinx.com/products/intellectual-property/axi_quadspi.html) |Xilinx|Interface between AXI4-Lite and Dual or Quad SPI||
|[AXI Interconnect](https://www.xilinx.com/products/intellectual-property/axi_interconnect.html) |Xilinx|Connection between one or more AXI4 memory-mapped Master devices to one or more memory-mapped Slave devices.|
|[Clocking Wizard](https://www.xilinx.com/products/intellectual-property/clocking_wizard.html) |Xilinx|Configuration of a clock circuit to user requirements|
|[Processor System Reset](https://www.xilinx.com/products/intellectual-property/proc_sys_reset.html) |Xilinx|Setting certain parameters to enable/disable features|
|[TC Adj. Clock](../../../Ips/AdjustableClock/)|NetTimeLogic|A timer clock in the Second and Nanosecond format that can be frequency and phase adjusted|
|[TC Signal Timestamper](../../../Ips/SignalTimestamper)|NetTimeLogic|Timestamping of an event signal of configurable polarity and generate interrupts|
|[TC PPS Generator](../../../Ips/PpsGenerator)|NetTimeLogic|Generation of a Pulse Per Second (PPS) of configurable polarity and aligned to the local clock's new second|
|[TC Signal Generator](../../../Ips/SignalGenerator)|NetTimeLogic|Generation of pulse width modulated (PWM) signals of configurable polarity and aligned to the local clock|
|[TC PPS Slave](../../../Ips/PpsSlave)|NetTimeLogic|Calculation of the offset and drift corrections to be applied to the Adjustable Clock, in order to synchronize to a PPS input|
|[TC ToD Slave](../../../Ips/TodSlave)|NetTimeLogic|Reception of GNSS receiver's messages over UART and synchronization to the Time of Day|
|[TC Frequency Counter](../../../Ips/FrequencyCounter)| NetTimeLogic|Measuring of the frequency of an input signal of range 1 - 10'000'000 Hz|
|[TC CoreList](../../../Ips/CoreList)|NetTimeLogic|A list of the current FPGA core instantiations which are accessible by an AXI4-Lite interface|
|[TC Conf Master](../../../Ips/ConfMaster)|NetTimeLogic|A default configuration which is provided to the AXI4-Lite slaves during startup, without the support of a CPU|
|[TC Clock Detector](../../../Ips/ClockDetector)|NetTimeLogic|Detection of the available clock sources and selection of the clocks to be used, according to a priority scheme and a configuration|
|[TC SMA Selector](../../../Ips/SmaSelector)|NetTimeLogic|Select the mapping of the inputs and the outputs of the 4 SMA connectors of the [Timecard](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card)|
|[TC PPS Selector](../../../Ips/PpsSourceSelector)|NetTimeLogic|Detection of the available PPS sources and selection of the PPS source to be used, according to a priority scheme and a configuration|
|[TC Communication Selector](../../../Ips/CommunicationSelector)|NetTimeLogic|Selection of the clock's communication interface (UART or I<sup>2</sup>C)|
|[TC Dummy Axi Slave](../../../Ips/DummyAxiSlave)|NetTimeLogic|AXI4L slave that is used as a placeholder of an address range|
|[TC FPGA Version](../../../Ips/FpgaVersion)|NetTimeLogic|AXI register that stores the design's version numbers|

The top-level design description is shown below.

![TimeCardTop](Additional%20Files/TimeCardTop.png) 

*NOTE:* In order to offload the drawing not all the AXI connections are shown, while the IRQ connections to the MSI Control are mentioned directly at the IPs.

The TimeCard runs partially from the 200MHz SOM oscillator. 
The NetTimeLogic cores with all the high precision parts are running based on the selected clock by the Clock Detector (10 MHz from MAC, SMA, etc.).

## 2. Address Mapping
The design's cores are accessible via AXI4 Light Memory Mapped slave interfaces for configuration and reporting. 
The AXI slave interfaces are accessed via the AXI interconnect by 2 AXI masters:
- [TC ConfMaster](../../../Ips/LitePcie) provides a default configuration to the cores after a reset. The default configuration can be changed at compilation time.
- [LitePcie](https://github.com/enjoy-digital/litepcie) provides full bridge functionality between the AXI4 architecture and the PCIe network. 
Typically, a CPU is connected to the timecard via this PCIe interface. 

The AXI Slave interfaces have the following addresses:

|Slave|AXI Slave interface|Offset Address|High Address|
|-----|-------------------|--------------|------------|
|AXI PCIe Control|S_AXI_CTL|0x0201_0000|0x0201_0FFF|
|TC FPGA Version|axi4l_slave|0x0202_0000|0x0202_0FFF|
|AXI GPIO Ext|S_AXI|0x0210_0000|0x0210_0FFF|
|AXI GPIO GNSS/MAC|S_AXI|0x0211_0000|0x0211_0FFF|
|TC Clock Detector|axi4l_slave|0x0213_0000|0x0213_0FFF|
|TC SMA Selector|axi4l_slave1|0x0214_0000|0x0214_3FFF|
|AXI I2C|S_AXI|0x0215_0000|0x0215_FFFF|
|AXI UART 16550 GNSS1|S_AXI|0x0216_0000|0x0216_FFFF|
|AXI UART 16550 GNSS2|S_AXI|0x0217_0000|0x0217_FFFF|
|AXI UART 16550 MAC|S_AXI|0x0218_0000|0x0218_FFFF|
|AXI UART 16550 ΕΧΤ|S_AXI|0x021Α_0000|0x021Α_FFFF|
|AXI I2C Clock|S_AXI|0x0220_0000|0x0220_FFFF|
|TC SMA Selector|axi4l_slave2|0x0222_0000|0x0222_3FFF|
|AXI HWICAP|S_AXI_LITE|0x0230_0000|0x0230_FFFF|
|AXI Quad SPI Flash|AXI_LITE|0x0231_0000|0x0231_FFFF|
|TC Adj. Clock|axi4l_slave|0x0300_0000|0x0300_FFFF|
|TC Signal TS GNSS1 PPS|axi4l_slave|0x0301_0000|0x0301_FFFF|
|TC Signal TS1|axi4l_slave|0x0302_0000|0x0302_FFFF|
|TC PPS Generator|axi4l_slave|0x0303_0000|0x0303_FFFF|
|TC PPS Slave|axi4l_slave|0x0304_0000|0x0304_FFFF|
|TC ToD Slave|axi4l_slave|0x0305_0000|0x0305_FFFF|
|TC Signal TS2|axi4l_slave|0x0306_0000|0x0306_FFFF|
|TC Dummy Axi Slave1|axi4l_slave|0x0307_0000|0x0307_FFFF|
|TC Dummy Axi Slave2|axi4l_slave|0x0308_0000|0x0308_FFFF|
|TC Dummy Axi Slave3|axi4l_slave|0x0309_0000|0x0309_FFFF|
|TC Dummy Axi Slave4|axi4l_slave|0x030A_0000|0x030A_FFFF|
|TC Dummy Axi Slave5|axi4l_slave|0x030B_0000|0x030B_FFFF|
|TC Signal TS FPGA PPS|axi4l_slave|0x030C_0000|0x030C_FFFF|
|TC Signal Generator1|axi4l_slave|0x030D_0000|0x030D_FFFF|
|TC Signal Generator2|axi4l_slave|0x030E_0000|0x030E_FFFF|
|TC Signal Generator3|axi4l_slave|0x030F_0000|0x030F_FFFF|
|TC Signal Generator4|axi4l_slave|0x0310_0000|0x0310_FFFF|
|TC Signal TS3|axi4l_slave|0x0311_0000|0x0311_FFFF|
|TC Signal TS4|axi4l_slave|0x0312_0000|0x0312_FFFF|
|TC Frequency Counter 1|axi4l_slave|0x0320_0000|0x0320_FFFF|
|TC Frequency Counter 2|axi4l_slave|0x0321_0000|0x0321_FFFF|
|TC Frequency Counter 3|axi4l_slave|0x0322_0000|0x0322_FFFF|
|TC Frequency Counter 4|axi4l_slave|0x0323_0000|0x0323_FFFF|
|TC CoreList|axi4l_slave|0x0330_0000|0x0330_FFFF|

The detailed register description of each instance is available at the corresponding core description document (see links at [Chapter 1](#1-design-overview)). 

### 2.1 FPGA Version Register

The Version Slave has one single 32-Bit Register. 
The upper 16 Bits show the version number of the golden image and the lower 16 Bits the version number of the regular image.
E.g.:

- Register 0x2200_0000 of the Golden image shows: 0x0001_0000
- Register 0x2200_0000 of the Regular image shows: 0x0000_0003

If the lower 16 Bits are 0x0000 the Golden image has booted.

### 2.2 AXI GPIO Registers

The implementation uses two instantiations of the [AXI GPIO](https://www.xilinx.com/products/intellectual-property/axi_gpio.html) IP. 

The mapping of the AXI GPIO Ext is as below

![AXI_GPIO_Ext](Additional%20Files/AXI_GPIO_Ext.png)

The mapping of the AXI GPIO GNSS/MAC is as below

![AXI_GPIO_GNSS_MAC](Additional%20Files/AXI_GPIO_GNSS_MAC.png)

## 3. Interrupt Mapping
The interrupts in the design are connected to the MSI Vector of the LitePCIe core.
The MSI interrupts must be enabled (register 0x1800 of the LitePCIe Core). The LitePCIe Core supports MSI-X.

|MSI Number|Interrupt Source|
|----------|----------------|
|32|TC Signal TS FPGA PPS|
|33|TC Signal TS GNSS1 PPS|
|34|TC Signal TS1|
|35|AXI UART 16550 GNSS1|
|36|AXI UART 16550 GNSS2|
|37|AXI UART 16550 MAC or AXI I<sup>2</sup>C OSC|
|38|TC Signal TS2|
|39|AXI I2C|
|40|AXI HWICAP|
|41|AXI Quad SPI Flash|
|42|Reserved|
|43|TC Signal Generator1|
|44|TC Signal Generator2|
|45|TC Signal Generator3|
|46|TC Signal Generator4|
|47|TC Signal TS3|
|48|TC Signal TS4|
|49|Reserved|
|50|Reserved|
|51|AXI UART 16550 Ext|

## 4. SMA Connectors
The [Timecard](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card) has currently 4 SMA Connectors of configurable input/output and an additional GNSS Antenna SMA input.
The default configuration of the SMA connectors is shown below.

<p align="left"> <img src="Additional%20Files/SmaConnectors.png" alt="Sublime's custom image"/> </p>

This default mapping and the direction can be changed via the 2 AXI slaves of the [TC SMA Selector](../../../Ips/SmaSelector) IP core. 

## 5. Status LEDs
At the current design, the Status LEDs are not connected to the AXI GPIO Ext and they are used directly by the FPGA.

- LED1: Alive LED of the FPGA internal Clock (50MHz)
- LED2: Alive LED of the PCIe clocking part (62.5MHz)
- LED3: PPS of the FPGA (Time of the Local Clock via PPS Master)
- LED4: PPS of the MAC (differential inputs from the MAC via diff-buffer)

## 6. Default Configuration 

The default configuration is provided by the [TC ConfMaster](../../../Ips/ConfMaster) and can be edited by updating the [DefaultConfigFile.txt](DefaultConfigFile.txt).
Currently, the following cores are configured at startup by the default configuration file. 

|Core Instance|Configuration|
|-------------|-------------|
|Adjustable Clock|Enable with synchronization source 1 (ToD+PPS)|
|PPS Generator|Enable with high polarity of the output pulse|
|PPS Slave|Enable with high polarity of the input pulse|
|ToD Slave|Enable with high polarity of the UART input|
|SMA Selector|Set the FPGA PPS and GNSS PPS as SMA outputs|


## 7. Core List 
The list of the configurable cores (via AXI) is provided by the [TC CoreList](../../../Ips/CoreList) and can be edited by updating the [CoreListFile.txt](CoreListFile.txt).
## 8. Create FPGA project and binaries
### 8.1 Create FPGA Project
The Vivado project was generated with Vivado 2019.1.
Since the Vivado project is not meant to be stored in a source control system or stored as is in general, a project script was created which will create the Vivado Project from the script.

The script has to be run once to create the project from scratch.

Run this from the Vivado TCL console:

*source /[YOUR_PATH]/Implementation/Xilinx/TimeCard/CreateProject.tcl*

(Alternatively, it can be started via the Tool=>Run Tcl Script… menu in the Vivado GUI)

The script will add all necessary files to the project as well as the constraints so everything is ready to generate a bitstream for the FPGA.
The project will be generated in the following folder:

*/[YOUR_PATH]/Implementation/Xilinx/TimeCard/TimeCard*
### 8.2 Synthesis, Implementation and Bitstream generation
A bitstream generation script runs synthesis and implementation and generates the bitstreams for the specified design runs:
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard/CreateBinaries.tcl* runs the synthesis/implementation of the TimeCardOS design, it generates the  bitstreams and it updates correspondingly the Factory_TimeCardOS.bin
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard/CreateBinariesGolden.tcl* runs the synthesis/implementation of the Golden_TimeCardOS design, it generates the bitstreams and it updates correspondingly the Factory_TimeCardOS.bin
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard/CreateBinariesAll.tcl* runs the synthesis/implementation of both designs, it generates the corresponding bitstreams and it updates correspondingly the Factory_TimeCardOS.bin. It also create the file TimeCardOS_Gotham.bin which is based on the TimeCardOS.bin and it has an additional 16-byte header with the PCIe ID. 

The binaries are copied to the folder */[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/*. 
The existing bitstreams in the Binaries folder are overwritten and also a copy of the files is created in a subfolder of the Binaries folder with a timestamp. This way, the latest implementation run is always found at the same position, but backups of the previous (and current) runs are still available.

The latest binaries can be found here:
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/Factory_TimeCardOS.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/Golden_TimeCardOS.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/TimeCardOS.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/TimeCardOS_Gotham.bin*

The timestamped backups are found in a folder in this format:

*/[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/YYYY_MM_DD hh_mm_ss/*, where YYYY: Year, MM: Month, DD: Day, hh: Hour, mm Minute, ss: Second

E.g. for 30th of January 2022 at 13:05:00: 
/[YOUR_PATH]/Implementation/Xilinx/TimeCard/Binaries/2022_01_30 13_05_00/

The Script can be run from the Vivado TCL console (when project is open) by the command:

*source /[YOUR_PATH]/Implementation/Xilinx/TimeCard/CreateBinariesAll.tcl*

(Alternatively, it can be started via the Tool=>Run Tcl Script… menu in the Vivado GUI while the project is open)

### 8.3 Resource Utilization
The design is implemented at the FPGA [Artix-7 XC7A100T-FGG484-1](https://docs.xilinx.com/v/u/en-US/ug475_7Series_Pkg_Pinout).

A resource utilization summary is shown below.
|Resource|Used|Available|Util%|
|--------|:--:|:-------:|:---:|
|LUTs|35300|63400|55.68|
|Flip Flops|29881|126800|23.57|
|BRAMs|22.5|135|22.90|
|DSPs|23|240|9.58|


## 9. Program FPGA and SPI Flash
For the initial programming of the FPGA and SPI Flash the JTAG programmer is needed and has to be connected to the USB JTAG.
After a successfully programmed FPGA, the design contains an AXI QUAD SPI Core which allows field updates.

The FPGA images are stored at the folder [Binaries](Binaries/).

### 9.1 Bitstreams with Fallback Configuration
The FPGA design is split into two different bitstreams/bin-files to allow a fail-safe field update. 
- The first image is the Golden/Fallback image *Golden_TimeCardOS.bin*. It contains only a limited functionality which provides access to the SPI Flash. 
- The second image is the latest version of the regular image *TimeCardOS.bin*. It is used for normal operation and it is the one which is replaced in a field update.

The FPGA configuration starts always at Addr0 where the Golden image is located. The Golden image has the start address of the Update image TimeCardOS. 
The configuration jumps directly to this address and tries to load the Update image. If this load fails it falls back to the Golden image.

Details about this Multiboot/Fallback approach can be found in the Xilinx Application Note 
[MultiBoot with 7 Series FPGAs and BPI](https://www.xilinx.com/support/documentation/application_notes/xapp1246-multiboot-bpi.pdf).

The *Factory_TimeCardOS.bin* image contains the two bitstreams and it shall be used to program the SPI flash for the first time, as example, during the productization. 

This combined image has following structure:

|Configuration Info|Value|
|-------------------|----------|
|File Format        |BIN       |
|Interface          |SPIX4     |
|Size               |16M       |
|Start Address      |0x00000000|
|End Address        |0x00FFFFFF|

|Addr1         |Addr2         |File(s)              |
|:------------:|:------------:|---------------------|
|0x00000000    |0x002C2A3B    |Golden_TimeCardOS.bit|
|0x00400000    |0x006C37AF    |TimeCardOS.bit       |

The image *TimeCardOS.bin* is the update/regular image and it shall be used for the field update via SPI.
For the update, this bitstream must be placed at 0x00400000 in the SPI flash.

### 9.2 SPI programming steps (non-volatile)

If the configuration memory device is already added to the project go to step 7, otherwise, go to step 1.
1. Go to the Hardware Manager menu

   ![Hardware Manager](Additional%20Files/HwManager.png) 

2. Right klick on “xc7a100t_0(1)”, a menu will pop up
3. Choose “Add Configuration Memory Device …” from the menu, the following window will pop up

   ![Add Config Memory](Additional%20Files/AddConfigMem.png)

4. Select “mt25ql128-spi-x1_x2_x4” as the SPI Flash type
5. Press Ok, a new window will pop up:

   ![Add Config Confirm](Additional%20Files/AddConfigConfirm.png)

6. Press cancel
7. Go to the Hardware Manager Menu which will have the flash attached

   ![Hardware Manager Updated](Additional%20Files/HwManagerUpdated.png) 

8. Right klick on “mt25ql128-spi-x1_x2_x4”, a menu will pop up
9. Choose “Program Configuration Memory Device …” from the menu, the following window will pop up

   ![Config Mem Program](Additional%20Files/ConfigMemProgram.png) 

10. Select the bitstream you want to program:
**Factory_TimeCardOS.bin** 

IMPORTANT NOTE: 
If the TimeCardOS.bin is loaded in this step, the field update will not work, as described in [Chapter 9.1](#91-bitstreams-with-fallback-configuration)!

11. Press Ok and wait for completion
12. Disconnect the JTAG interface from the board
13. Power cycle or Reset the board / Cold start of the PC
14. The RUN LED will blink
