---
sidebar_position: 3
---

# SLES

In order to build the latest `ocp_tap` driver and use all the bleeding edge features of the Time Card, we recommend building the kernel from scratch.

### Clone TAP GitHub repository

```
cd ~
git clone https://github.com/opencomputeproject/Time-Appliance-Project
```

### Install prerequisite packages

```
export KVER=$(uname -r)
export kver_only=${KVER%-default}
zypper in -y bc bison dwarves flex gcc git grub2 kernel-default-devel-"${kver_only}".1 kernel-devel-"${kver_only}".1 kernel-source-"${kver_only}".1 kernel-syms-"${kver_only}".1 libelf-devel make ncurses-devel openssl-devel
```

### Change kernel build parameters in .config

```
CONFIG_I2C_XILINX=m
CONFIG_SPI_XILINX=m
```

Then run these make targets:

```
make oldconfig
make modules_prepare 
make prepare
```

### Allow Unsupported Modules

```
cp /lib/modprobe.d/10-unsupported-modules.conf /etc/modprobe.d/10-unsupported-modules.conf
sed -i 's/allow_unsupported_modules 0/allow_unsupported_modules 1/' /etc/modprobe.d/10-unsupported-modules.conf
```

### Install drivers from Time Appliance Project repository

```
cd ~/Time-Appliance-Project/Time-Card/DRV/
./remake
modprobe ptp_ocp
```

### Confirming installation

View the output of `dmesg` to confirm that the Time Card device has been installed and the kernel driver has sucessefully enumerated the device.

Example output confirming the `ptp_ocp` kernel driver enumering the Time Card device.

```
[ 1491.447310] pps pps1: new PPS source ptp2
[ 1491.449374] ptp_ocp 0000:02:00.0: Version 1.2.0, clock PPS, device ptp2
[ 1491.449388] ptp_ocp 0000:02:00.0: Time: 1647548640.775286647, in-sync
[ 1491.449390] ptp_ocp 0000:02:00.0: version 8005
[ 1491.449392] ptp_ocp 0000:02:00.0: regular image, version 32773
[ 1491.449393] ptp_ocp 0000:02:00.0:  GNSS: /dev/ttyS5  @ 115200
[ 1491.449394] ptp_ocp 0000:02:00.0: GNSS2: /dev/ttyS6  @ 115200
[ 1491.449396] ptp_ocp 0000:02:00.0:   MAC: /dev/ttyS7  @  57600
[ 1491.449399] ptp_ocp 0000:02:00.0:  NMEA: /dev/ttyS0  @ 115200
```
