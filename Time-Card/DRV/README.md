# Driver

Driver is based on a kernel module for CentOS 8

## Instruction

Run the remake followed by modprobe ptp_ocp

## Outcome

After successfully loading the driver, you will see a new PTP POSIX clock, linking to the physical hardware clock (PHC) on the Time Card (/dev/ptp*).
Now,you can use standard PTP4L tools such as phc2sys or ts2phc to copy, sync, tune, etc...
