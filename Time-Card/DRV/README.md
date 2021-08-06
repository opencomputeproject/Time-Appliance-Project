# Driver

Driver is based on a kernel module for CentOS and Ubunutu. 
Kernel 5.12+ is recommended

## Instruction
Make sure vt-d option is enabled in BIOS.   
Run the remake followed by modprobe ptp_ocp

## Outcome
```
$ ls -g /sys/class/timecard/ocp1/
total 0
-r--r--r--. 1 root 4096 Aug  3 19:49 available_clock_sources
-rw-r--r--. 1 root 4096 Aug  3 19:49 clock_source
lrwxrwxrwx. 1 root    0 Aug  3 19:49 device -> ../../../0000:04:00.0/
-r--r--r--. 1 root 4096 Aug  3 19:49 gnss_sync
lrwxrwxrwx. 1 root    0 Aug  3 19:49 i2c -> ../../xiic-i2c.1024/i2c-2/
drwxr-xr-x. 2 root    0 Aug  3 19:49 power/
lrwxrwxrwx. 1 root    0 Aug  3 19:49 pps -> ../../../../../virtual/pps/pps1/
lrwxrwxrwx. 1 root    0 Aug  3 19:49 ptp -> ../../ptp/ptp2/
-r--r--r--. 1 root 4096 Aug  3 19:49 serialnum
lrwxrwxrwx. 1 root    0 Aug  3 19:49 subsystem -> ../../../../../../class/timecard/
lrwxrwxrwx. 1 root    0 Aug  3 19:49 ttyGNSS -> ../../tty/ttyS7/
lrwxrwxrwx. 1 root    0 Aug  3 19:49 ttyMAC -> ../../tty/ttyS8/
-rw-r--r--. 1 root 4096 Aug  3 19:39 uevent
```

After successfully loading the driver, one will see:
* PTP POSIX clock, linking to the physical hardware clock (PHC) on the Time Card (`/dev/ptp2`) 
* GNSS serial `/dev/ttyS7` 
* Atomic clock serial `/dev/ttyS8`
* two i2c (`/dev/i2c-*`) devices added

Now, once can use standard `linuxptp` tools such as `phc2sys` or `ts2phc` to copy, sync, tune, etc... See more in [software](/Software) section

## Driver is included in the mainstream Linux Kernel
* Initial primitive version ([5.2](https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net-next.git/commit/?id=a7e1abad13f3f0366ee625831fecda2b603cdc17))
* Exposing all devices version ([5.15](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=773bda96492153e11d21eb63ac814669b51fc701)) 
