# USB to Ethernet with Hardware Timestamping and PPS outoput
A USB 3.2 Gen1 to Gigabit Ethernet dongle based on the AX88179B. 
- The AX88179B supports Precision Time Protocol (PTP) (IEEE 1588v2 and 802.1AS)
- This hardware implementation features a female SMA to break out the 1PPS signal from the AX88179B.

![PHOTO-2024-03-19-19-13-36](https://github.com/opencomputeproject/Time-Appliance-Project/assets/1751211/bad0fec2-05ab-4fcc-91c1-67a0ec44fbe9)

# Installation

Step 1: First you need to compile the Time Stick module (driver). Either clone the entire repo and take it from there (skip to step 2) so use the following commands:
```
cd
mkdir TimeStick
cd TimeStick
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/Makefile
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/Readme
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179_178a.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179_178a.h
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179a_772d.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179a_772d.h
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179a_ieee.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179_programmer.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88279_programmer.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax88179a_programmer.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax_ioctl.h
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax_main.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax_main.h
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax_ptp.c
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/ax_ptp.h
wget https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/refs/heads/master/Incubation/Hardware/TimeStick/DRV/axcmd.c
make
sudo make install
```
Step 2: You need to remove cdc_ncm before installing the AX88179A module.
Please follow the steps below to remove cdc_ncm and install the AX88179A module.

```
sudo rmmod cdc_mbim
sudo rmmod cdc_ncm
sudo rmmod ax88179_178a
```

Step 3: Insert the new module
```
sudo modprobe ax_usb_nic
```
