---
sidebar_position: 1
---

# CentOS

In order to build the latest `ocp_tap` driver and use all the bleeding edge features of the Time Card, we recommend building the kernel from scratch.

### Clone TAP GitHub repository

```
cd ~
git clone https://github.com/opencomputeproject/Time-Appliance-Project
```

### Install the latest kernel keys

```
sudo dnf -y install https://www.elrepo.org/elrepo-release-8.el8.elrepo.noarch.rpm
sudo rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org
dnf --enablerepo=elrepo-kernel install kernel-ml
reboot
```

### Install prerequisite packages

```
yum install -y ncurses-devel make gcc bc bison flex elfutils-libelf-devel openssl-devel grub2 i2c-tools git
```

### Acquire the full kernel source from online.

```
cd /usr/src/kernels
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.17.4.tar.xz
tar -xvf linux-5.17.4.tar.xz
rm linux-5.17.4.tar.xz
mv linux-5.17.4/ 5.17.4/
cd 5.17.4/
```

### Copy existing kernel config to kernel source

```
cp -v /boot/config-5.17.4-1.el8.elrepo.x86_64 .config
vim .config
```

You will need to edit `.config` and specify the newly installed kernel.

### Change kernel build parameters in `.config`

```
CONFIG_I2C_XILINX=m
CONFIG_MTD=m
CONFIG_MTD_SPI_NOR=m
CONFIG_SPI=y
CONFIG_SPI_ALTERA=m
CONFIG_SPI_BITBANG=m
CONFIG_SPI_MASTER=y
CONFIG_SPI_MEM=y
CONFIG_SPI_XILINX=m
CONFIG_I2C=y
CONFIG_I2C_OCORES=m
CONFIG_IKCONFIG=y
CONFIG_EEPROM_AT24=m
```

Then do `make oldconfig` to add `CONFIG_IKCONFIG_PROC=y`. Answer `y` or `n` to all questions.

```
make oldconfig
```

**NOTE: SKIP FOR 5.17.4** Temporary debug step as of 3-17-2022, patch the kernel with these two patches to fix issues related to FPGA SPI Flash update.

```
patch -p1 < ~/Time-Appliance-Project/Time-Card/DRV/0001-spi-xilinx-Inhibit-transmitter-while-filling-TX-FIFO.patch
patch -p1 < ~/Time-Appliance-Project/Time-Card/DRV/0001-spi-nor-Send-soft-reset-before-probing-flash.patch
```

### Build and install this kernel

```
make bzImage -j4; make modules -j4; make -j4;  make INSTALL_MOD_STRIP=1 modules_install; make install;
```

### Reboot

```
reboot
```

### Enable all UART peripherals

```
grubby --update-kernel=ALL --args=8250.nr_uarts=8
reboot
```

Confirm enumeration of all enabled UART peripherals on next reboot,

```
cat /proc/cmdline
[root@localhost Binaries]# cat /proc/cmdline
BOOT_IMAGE=(hd1,gpt2)/vmlinuz-5.16.0 root=/dev/mapper/cs-root ro crashkernel=auto resume=/dev/mapper/cs-swap rd.lvm.lv=cs/root rd.lvm.lv=cs/swap rhgb quiet 8250.nr_uarts=8
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
[ 1491.440223] 0000:02:00.0: ttyS5 at MMIO 0x70161000 (irq = 180, base_baud = 3125000) is a 16550A
[ 1491.440278] 0000:02:00.0: ttyS6 at MMIO 0x70171000 (irq = 181, base_baud = 3125000) is a 16550A
[ 1491.440322] 0000:02:00.0: ttyS7 at MMIO 0x70181000 (irq = 182, base_baud = 3125000) is a 16550A
[ 1491.440362] 0000:02:00.0: ttyS0 at MMIO 0x70191000 (irq = 187, base_baud = 3125000) is a 16550A
[ 1491.441702] xilinx_spi xilinx_spi.1024: at [mem 0x70310000-0x7031ffff], irq=186
[ 1491.442678] spi-nor spi1024.0: n25q128a13 (16384 Kbytes)
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
