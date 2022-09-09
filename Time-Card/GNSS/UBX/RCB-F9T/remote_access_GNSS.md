# Accessing the GNSS TTY port remotely

Time Card driver (ptp_ocp) exposes direct access to the GNSS module using the TTY port in the Linux system. Unfortunately, U-Blox only provides its u-center software for Windows. As a result, there is no direct way to monitor or reconfigure the module "on-the-fly".

To enable u-center control, the ser2net package can be used to expose the TTY port over the ethernet. This guide will describe basic steps to enable that use case and enable rover configuration on the ZED-F9T module.

# Installing and configuring ser2net

Ser2net package is available in most of the distribution via standard package. For RH-based distribution, run

```bash
sudo dnf install ser2net
```

For debian-based distibutions

```bash
sudo apt-get ser2net
```

This will install the ser2net package and create necessary config files. 

## Finding GNSS TTY port

To find the port that's associated with the GNSS port run:

```bash
dmesg | grep ocp
```

> ```
> [    4.627215] ptp_ocp 0000:02:00.0: enabling device (0100 -> 0102)
> [    4.628143] ptp_ocp 0000:02:00.0: irq 16 out of range, skipping ts4
> [    4.631062] ptp_ocp 0000:02:00.0: Version 1.2.0, clock PPS, device ptp0
> [    4.631078] ptp_ocp 0000:02:00.0: Time: 1659170219.993081800, UNSYNCED
> [    4.631081] ptp_ocp 0000:02:00.0: version 8
> [    4.631083] ptp_ocp 0000:02:00.0: regular image, version 8
> [    4.631084] ptp_ocp 0000:02:00.0:  GNSS: /dev/ttyS5  @ 115200
> [    4.631087] ptp_ocp 0000:02:00.0: GNSS2: /dev/ttyS6  @ 115200
> [    4.631089] ptp_ocp 0000:02:00.0:   MAC: /dev/ttyS7  @  57600
> [    4.631094] ptp_ocp 0000:02:00.0:  NMEA: /dev/ttyS8  @     -1
> ```

Alternatively,

```bash
ls -ls /sys/class/timecard/ocp*/ttyGNSS
```

> ```
> 0 lrwxrwxrwx. 1 root root 0 Jul 30 11:07 /sys/class/timecard/ocp0/ttyGNSS -> ../../tty/ttyS5
> ```

In both cases `/dev/ttyS5` is our device. Now we can expose it over the ethernet.

## Configuring  ser2net

Now, we need to edit the config file and expose Time Card's GNSS serial port. The default config file for ser2net tries to expose a lot of ports by default, so we we backup it and create the new one with just the port we try to expose.

```bash
sudo mv /etc/ser2net.conf /etc/ser2net.conf.bak
sudo sh -c "echo -e 2006:raw:5:/dev/ttyS5:115200 > /etc/ser2net.conf"
```

Now we can run the ser2net

```bash
ser2net
```

And open port on the firewall

```
sudo firewall-cmd --add-port=2006/tcp
```

Now we can connect to the GNSS module directly using the u-center network connection feature.

## U-center network connection

To access the GNSS remotely use the built-in Network connection feature in the u-center, select New... option and enter the remote platform address: `tcp://192.168.1.20:2006`

<img src="C:\_src_\Time-Appliance-Project\Time-Card\GNSS\UBX\RCB-F9T\remote_access_GNSS.png" style="zoom:100%;" />

The u-center should connect to the remote module and enable you to configure it, read module status and use the RTK rover feature.