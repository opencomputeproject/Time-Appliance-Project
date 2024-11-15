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
sudo insmod ax_usb_nic.ko
```


Known Issues with the driver

Since the exisiting ax88179_178a driver does not support the hardware timestamping and shows up by default by the kernel, you need to remove it or blacklist it.
You should also remove the ax_usb_nic module (if loaded) and loaded it again in order to get the right functionality from the module.
```
sudo rmmod ax88179_178a
sudo rmmod ax_usb_nic
sudo modprobe ax_usb_nic
```
This is how it should look like when the right module is installed
```
$ ethtool -T eth1
Time stamping parameters for eth1:
Capabilities:
	hardware-transmit
	software-transmit
	hardware-receive
	software-receive
	software-system-clock
	hardware-raw-clock
PTP Hardware Clock: 1
Hardware Transmit Timestamp Modes:
	off
	on
	onestep-sync
	onestep-p2p
Hardware Receive Filter Modes:
	none
	ptpv1-l4-event
	ptpv1-l4-sync
	ptpv1-l4-delay-req
	ptpv2-l4-event
	ptpv2-l4-sync
	ptpv2-l4-delay-req
	ptpv2-l2-event
	ptpv2-l2-sync
	ptpv2-l2-delay-req
```

