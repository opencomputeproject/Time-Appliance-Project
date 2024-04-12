# USB to Ethernet with Hardware Timestamping and PPS outoput
A USB 3.2 Gen1 to Gigabit Ethernet dongle based on the AX88179B. 
- The AX88179B supports Precision Time Protocol (PTP) (IEEE 1588v2 and 802.1AS)
- This hardware implementation features a female SMA to break out the 1PPS signal from the AX88179B.

![PHOTO-2024-03-19-19-13-36](https://github.com/opencomputeproject/Time-Appliance-Project/assets/1751211/bad0fec2-05ab-4fcc-91c1-67a0ec44fbe9)

# Installation

You need to remove cdc_ncm before installing the AX88179A driver.
Please follow the steps below to remove cdc_ncm and install the AX88179A driver.

```
sudo rmmod cdc_mbim
sudo rmmod cdc_ncm
```

and then
```
sudo insmod ax88179a.ko
```
