# Open Source Timecard Production Design Description
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

The original Open Source TimeCard design includes open-source IP cores from [NetTimeLogic](https://www.nettimelogic.com/) and free-to-use IP cores from [Xilinx](https://www.xilinx.com/).
The following cores are used in the Open Source TimeCard design.

|Core|Vendor|Description|
|----|:----:|-----------|
|[AXI Memory Mapped to PCI Express](https://www.xilinx.com/products/intellectual-property/axi_pcie.html) |Xilinx|Interface between the AXI4 interface and the Gen2 PCI Express (PCIe) silicon hard core|
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
|[TC MsiIrq](../../../Ips/MsiIrq)|NetTimeLogic|Forwarding single interrupts as Message-Signaled Interrupts the [AXI-PCIe bridge](https://www.xilinx.com/products/intellectual-property/axi_pcie.html)|
|[TC Clock Detector](../../../Ips/ClockDetector)|NetTimeLogic|Detection of the available clock sources and selection of the clocks to be used, according to a priority scheme and a configuration|
|[TC SMA Selector](../../../Ips/SmaSelector)|NetTimeLogic|Select the mapping of the inputs and the outputs of the 4 SMA connectors of the [TimeCard](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card)|
|[TC PPS Selector](../../../Ips/PpsSourceSelector)|NetTimeLogic|Detection of the available PPS sources and selection of the PPS source to be used, according to a priority scheme and a configuration|
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
- [TC ConfMaster](../../../Ips/ConfMaster) provides a default configuration to the cores after a reset. The default configuration can be changed at compilation time.
- [AXI PCIe](https://www.xilinx.com/products/intellectual-property/axi_pcie.html) provides full bridge functionality between the AXI4 architecture and the PCIe network. 
Typically, a CPU is connected to the TimeCard via this PCIe interface. 

The AXI Slave interfaces have the following addresses:

|Slave|AXI Slave interface|Offset Address|High Address|
|-----|-------------------|--------------|------------|
|AXI PCIe Control|S_AXI_CTL|0x0001_0000|0x0001_0FFF|
|TC FPGA Version|axi4l_slave|0x0002_0000|0x0002_0FFF|
|AXI GPIO Ext|S_AXI|0x0010_0000|0x0010_0FFF|
|AXI GPIO GNSS/MAC|S_AXI|0x0011_0000|0x0011_0FFF|
|TC Clock Detector|axi4l_slave|0x0013_0000|0x0013_0FFF|
|TC SMA Selector|axi4l_slave1|0x0014_0000|0x0014_3FFF|
|AXI I2C PCA9546|S_AXI|0x0015_0000|0x0015_FFFF|
|AXI UART 16550 GNSS1|S_AXI|0x0016_0000|0x0016_FFFF|
|AXI UART 16550 GNSS2|S_AXI|0x0017_0000|0x0017_FFFF|
|AXI UART 16550 MAC|S_AXI|0x0018_0000|0x0018_FFFF|
|AXI UART 16550 EXT|S_AXI|0x001A_0000|0x001A_FFFF|
|AXI I2C EEPROM|S_AXI|0x0020_0000|0x0020_0FFF|
|AXI I2C RGB|S_AXI|0x0021_0000|0x0021_0FFF|
|TC SMA Selector|axi4l_slave2|0x0022_0000|0x0022_3FFF|
|AXI GPIO RGB|S_AXI|0x0023_0000|0x0023_0FFF|
|AXI HWICAP|S_AXI_LITE|0x0030_0000|0x0030_FFFF|
|AXI Quad SPI Flash|AXI_LITE|0x0031_0000|0x0031_FFFF|
|TC Adj. Clock|axi4l_slave|0x0100_0000|0x0100_FFFF|
|TC Signal TS GNSS1 PPS|axi4l_slave|0x0101_0000|0x0101_FFFF|
|TC Signal TS1|axi4l_slave|0x0102_0000|0x0102_FFFF|
|TC PPS Generator|axi4l_slave|0x0103_0000|0x0103_FFFF|
|TC PPS Slave|axi4l_slave|0x0104_0000|0x0104_FFFF|
|TC ToD Slave|axi4l_slave|0x0105_0000|0x0105_FFFF|
|TC Signal TS2|axi4l_slave|0x0106_0000|0x0106_FFFF|
|TC Dummy Axi Slave1|axi4l_slave|0x0107_0000|0x0107_FFFF|
|TC Dummy Axi Slave2|axi4l_slave|0x0108_0000|0x0108_FFFF|
|TC Dummy Axi Slave3|axi4l_slave|0x0109_0000|0x0109_FFFF|
|TC Dummy Axi Slave4|axi4l_slave|0x010A_0000|0x010A_FFFF|
|TC Dummy Axi Slave5|axi4l_slave|0x010B_0000|0x010B_FFFF|
|TC Signal TS FPGA PPS|axi4l_slave|0x010C_0000|0x010C_FFFF|
|TC Signal Generator1|axi4l_slave|0x010D_0000|0x010D_FFFF|
|TC Signal Generator2|axi4l_slave|0x010E_0000|0x010E_FFFF|
|TC Signal Generator3|axi4l_slave|0x010F_0000|0x010F_FFFF|
|TC Signal Generator4|axi4l_slave|0x0110_0000|0x0110_FFFF|
|TC Signal TS3|axi4l_slave|0x0111_0000|0x0111_FFFF|
|TC Signal TS4|axi4l_slave|0x0112_0000|0x0112_FFFF|
|TC Frequency Counter 1|axi4l_slave|0x0120_0000|0x0120_FFFF|
|TC Frequency Counter 2|axi4l_slave|0x0121_0000|0x0121_FFFF|
|TC Frequency Counter 3|axi4l_slave|0x0122_0000|0x0122_FFFF|
|TC Frequency Counter 4|axi4l_slave|0x0123_0000|0x0123_FFFF|
|TC CoreList|axi4l_slave|0x0130_0000|0x0130_FFFF|

The detailed register description of each instance is available at the corresponding core description document (see links at [Chapter 1](#1-design-overview)). 

### 2.1 FPGA Version Register

The Version Slave has one single 32-Bit Register. 
The upper 16 Bits show the version number of the golden image and the lower 16 Bits the version number of the regular image.
E.g.:

- Register 0x0200_0000 of the Golden image shows: 0x8001_0000
- Register 0x0200_0000 of the Regular image shows: 0x0000_8001

If the lower 16 Bits are 0x0000 the Golden image has booted. Additionally, the SMA LEDs are blinking red.

### 2.2 AXI GPIO Registers

The implementation uses three instantiations of the [AXI GPIO](https://www.xilinx.com/products/intellectual-property/axi_gpio.html) IP. 
Inputs or Outputs might be high or low active for the external signal level. 
The inversion is done in the FPGA. On the AXI GPIO always active high is used. The inverted signals are underlined in the tables below.

The mapping of the AXI GPIO Ext is as below

![AXI_GPIO_Ext](Additional%20Files/AXI_GPIO_Ext.png)

The mapping of the AXI GPIO GNSS/MAC is as below

![AXI_GPIO_GNSS_MAC](Additional%20Files/AXI_GPIO_GNSS_MAC.png)

The mapping of the AXI GPIO RGB is as below

![AXI_GPIO_RGB](Additional%20Files/AXI_GPIO_RGB.png)

## 3. Interrupt Mapping
The interrupts in the design are connected to the MSI Vector of the AXI Memory Mapped to PCI Express Core via a MSI controller. 
The PCI Express Core needs to set the MSI_enable to ‘1’. 
The MSI controller sends INTX_MSI Request with the MSI_Vector_Num to the PCI Express Core and with the INTX_MSI_Grant the interrupt is acknowledged. 
If there are several interrupts pending, the messages are sent with the round-robin principle. 
Level interrupts (e.g. AXI UART 16550) are taking at least one round for the next interrupt.

|MSI Number|Interrupt Source|
|----------|----------------|
|0|TC Signal TS FPGA PPS|
|1|TC Signal TS GNSS1 PPS|
|2|TC Signal TS1|
|3|AXI UART 16550 GNSS1|
|4|AXI UART 16550 GNSS2|
|5|AXI UART 16550 MAC|
|6|TC Signal TS2|
|7|AXI I2C PCA9546|
|8|AXI HWICAP|
|9|AXI Quad SPI Flash|
|10|Reserved|
|11|TC Signal Generator1|
|12|TC Signal Generator2|
|13|TC Signal Generator3|
|14|TC Signal Generator4|
|15|TC Signal TS3|
|16|TC Signal TS4|
|17|AXI I2C EEPROM|
|18|AXI I2C RGB|
|19|AXI UART 16550 Ext|

## 4. SMA Connectors
The [TimeCard](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card) has currently 4 SMA Connectors of configurable input/output and an additional GNSS Antenna SMA input.
The default configuration of the SMA connectors is shown below.

<p align="left"> <img src="Additional%20Files/SmaConnectors.png" alt="Sublime's custom image"/> </p>

This default mapping and the direction can be changed via the 2 AXI slaves of the [TC SMA Selector](../../../Ips/SmaSelector) IP core. 

## 5. Status LEDs
At the current design, the Status LEDs are not connected to the AXI GPIO Ext and they are used directly by the FPGA.

- LED1: Alive LED of the FPGA internal Clock (50MHz)
- LED2: Alive LED of the PCIe clocking part (62.5MHz)
- LED3: PPS of the FPGA (Time of the Local Clock via PPS Master)
- LED4: PPS of the MAC (differential inputs from the MAC via diff-buffer)

The RGB LEDs (SMA1-4 and GNSS1) can be controlled via I2C or GPIO (depending on assembly option).
In case the Fall-Back image is loaded all four SMA LEDs are blinking red.

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

*source /[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/CreateProject.tcl*

(Alternatively, it can be started via the Tool=>Run Tcl Script… menu in the Vivado GUI)

The script will add all necessary files to the project as well as the constraints so everything is ready to generate a bitstream for the FPGA.
The project will be generated in the following folder:

*/[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/TimeCard*
### 8.2 Synthesis, Implementation and Bitstream generation
A bitstream generation script runs synthesis and implementation and generates the bitstreams for the specified design runs:
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/CreateBinaries.tcl* runs the synthesis/implementation of the TimeCardOS_Production design, it generates the  bitstreams and it updates correspondingly the Factory_TimeCardOS_Production.bin
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/CreateBinariesGolden.tcl* runs the synthesis/implementation of the Golden_TimeCardOS_Production design, it generates the bitstreams and it updates correspondingly the Factory_TimeCardOS_Production.bin
- The script */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/CreateBinariesAll.tcl* runs the synthesis/implementation of both designs, it generates the corresponding bitstreams and it updates correspondingly the Factory_TimeCardOS_Production.bin. It also create the file TimeCardOS_Production_Gotham.bin which is based on the TimeCardOS_Production.bin and it has an additional 16-byte header with the PCIe ID. 

The binaries are copied to the folder */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/*. 
The existing bitstreams in the Binaries folder are overwritten and also a copy of the files is created in a subfolder of the Binaries folder with a timestamp. This way, the latest implementation run is always found at the same position, but backups of the previous (and current) runs are still available.

The latest binaries can be found here:
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/Factory_TimeCardOS_Production.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/Golden_TimeCardOS_Production.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/TimeCardOS_Production.bin*
- */[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/TimeCardOS_Production_Gotham.bin*

The timestamped backups are found in a folder in this format:

*/[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/YYYY_MM_DD hh_mm_ss/*, where YYYY: Year, MM: Month, DD: Day, hh: Hour, mm Minute, ss: Second

E.g. for 30th of January 2022 at 13:05:00: 
/[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/Binaries/2022_01_30 13_05_00/

The Script can be run from the Vivado TCL console (when project is open) by the command:

*source /[YOUR_PATH]/Implementation/Xilinx/TimeCard_Production/CreateBinariesAll.tcl*

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
- The first image is the Golden/Fallback image *Golden_TimeCardOS_Production.bin*. It contains only a limited functionality which provides access to the SPI Flash. 
- The second image is the latest version of the regular image *TimeCardOS_Production.bin*. It is used for normal operation and it is the one which is replaced in a field update.

The FPGA configuration starts always at Addr0 where the Golden image is located. The Golden image has the start address of the Update image TimeCardOS_Production. 
The configuration jumps directly to this address and tries to load the Update image. If this load fails it falls back to the Golden image.

Details about this Multiboot/Fallback approach can be found in the Xilinx Application Note 
[MultiBoot with 7 Series FPGAs and BPI](https://www.xilinx.com/support/documentation/application_notes/xapp1246-multiboot-bpi.pdf).

The *Factory_TimeCardOS_Production.bin* image contains the two bitstreams and it shall be used to program the SPI flash for the first time, as example, during the productization. 

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
|0x00000000    |0x002C8B2F    |Golden_TimeCardOS_Production.bit|
|0x00400000    |0x006CA157    |TimeCardOS_Production.bit       |

The image *TimeCardOS_Production.bin* is the update/regular image and it shall be used for the field update via SPI.
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
**Factory_TimeCardOS_Production.bin** 

IMPORTANT NOTE: 
If the TimeCardOS_Production.bin is loaded in this step, the field update will not work, as described in [Chapter 9.1](#91-bitstreams-with-fallback-configuration)!

11. Press Ok and wait for completion
12. Disconnect the JTAG interface from the board
13. Power cycle or Reset the board / Cold start of the PC
14. The RUN LED will blink
