# 200T FPGA version for the Artix 7 AC7200 SOM Module

* TimeCard200T.bit to flash the FPGA (volatile)
* Factory_TimeCard200T.bin to load the SPI flash via JTAG (Golden Image + Update Image)
* **IMPORTANT**: TimeCard200T.bin to updated the SPI flash via SPI starting @ **0x00800000** (Update Image only)
