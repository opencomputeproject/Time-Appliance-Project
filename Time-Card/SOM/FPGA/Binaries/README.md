# SOM Module Binaries
- **200T** TimeCard FPGA binaries for the SOM with the Artix7 200T FPGA (AC7200)
- **AX7103_Baseboard** TimeCard FPGA binaries for the AX7103 base board (not maintained anymore)
- **Debug** TimeCard FPGA binaries for special debug use cases (not maintained anymore)
- **FirmwareUpdateTest** TimeCard FPGA binaries for firmware update tests

## In this folder are the bitstreams for the SOM Module. ## 
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
