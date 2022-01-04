# System on Module

![Time Card FPGA](TimeCard_FPGA.png)

The FPGA bitstream can be configured to various options. Currently it is based on the [NetTimeLogic clock module](https://www.nettimelogic.com/clock-products.php).  <br />

Check the [Readme.pdf](https://github.com/opencomputeproject/Time-Appliance-Project/blob/master/Time-Card/SOM/FPGA/Readme.pdf) for more details about the implementation.  <br />

In the [Doc folder](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card/SOM/FPGA/Doc) are all relevant documents about the integrated IP Cores (e.g. register description). <br />

## The FPGA starts with a static configuration with following settings: ##
* PPS (including TOD) is used as correction input for the clock  
* PPS Slave Pulse detection on rising edge
* PPS Slave cable delay 0
* TOD Slave UART Baudrate is 115200
* TOD Slave UART polarity default
* TOD Slave in UBX Mode, all GNSS and no messages disabled
* PPS Master polarity rising edge
* PPS Master cable delay 0
* PPS Master pulse width 100 ms
* Clock, PPS Slave, TOD Slave and PPS Master are enabled
* All Timestampers are disabled
* IRIG Slave/Master are disabled
* DCF Slave/Master are disabled
* TOD/NMEA Master is disabled

## In the Binaries folder are the bitstreams for the SOM Module. ## 
* TimeCard.bit to flash the FPGA (volatile)
* Factory_TimeCard.bin to load the SPI flash via JTAG (Golden Image + Update Image)
* TimeCard.bin to updated the SPI flash via SPI starting @ 0x00400000 (Update Image only)
    
The version for AX7103 will be not longer maintained. 

## Firmware upgrade
```
$ dmesg | grep ptp_ocp | head -1
[   21.527678] ptp_ocp 0000:11:00.0: enabling device (0140 -> 0142)
$ cp TimeCard.bin /lib/firmware
$ devlink dev flash pci/0000:11:00.0 file TimeCard.bin
```
