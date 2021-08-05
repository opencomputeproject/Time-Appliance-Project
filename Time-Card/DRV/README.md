# Driver

Driver is based on a kernel module for CentOS and Ubunutu. 
Kernel 5.12+ is recommended

## Instruction
Make sure vt-d option is enabled in BIOS.   
Run the remake followed by modprobe ptp_ocp

## Outcome

* After successfully loading the driver, you will see a new PTP POSIX clock, linking to the physical hardware clock (PHC) on the Time Card (`/dev/ptp*`). 
* In addition there will be two uarts (`/dev/ttyS*`) and two i2c (`/dev/i2c-*`) devices added.
* Now,you can use standard PTP4L tools such as phc2sys or ts2phc to copy, sync, tune, etc...

## Drive is included in the mainstream Linux Kernel

https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net-next.git/commit/?id=a7e1abad13f3f0366ee625831fecda2b603cdc17
