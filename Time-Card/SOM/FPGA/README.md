# System on Module

The FPGA bitstream can be configured to various options. Currently it is based on the [NetTimeLogic clock module](https://www.nettimelogic.com/clock-products.php). 

Check the [Readme.pdf](https://github.com/opencomputeproject/Time-Appliance-Project/blob/master/Time-Card/SOM/FPGA/Readme.pdf) for more details about the implementation.

In the Doc folder are all relevant documents about the integrated IP Core (e.g. register description).

In the Binaries folder are the bitstreams for the SOM Module. 
    - .bit to flash the FPGA (volatile)
    - .bin to load the SPI flash
    
The version for AX7103 will be not longer maintained. 