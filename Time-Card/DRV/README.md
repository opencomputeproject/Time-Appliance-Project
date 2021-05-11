# Driver

Driver is based on a kernel module for CentOS and Ubunutu. 
Kernel 5.2+ is recommended

# Supported devices

* Facebook's Time Card
* Orolia's Atomic Reference Time Card (ART Card)

## Instruction

Run the remake followed by modprobe ptp_ocp

## Time Card Outcome

* After successfully loading the driver, you will see a new PTP POSIX clock, linking to the physical hardware clock (PHC) on the Time Card (`/dev/ptp*`). 
* In addition there will be two uarts (`/dev/ttyS*`) and two i2c (`/dev/i2c-*`) devices added.
* Now,you can use standard PTP4L tools such as phc2sys or ts2phc to copy, sync, tune, etc...

## ART Card Outcome

* After successfully loading the driver, you will see a new PTP POSIX clock, linking to the physical hardware clock (PHC) on the Time Card (`/dev/ptp*`)
* In addition there will be:
  * One uarts (`/dev/ttyS*`) corresponding to the GNSS serial port
  * One PPS device (`/dev/pps*`)
  * One character device (`/dev/phase_error_gnss_mRO50`) to get phase error between the GNSS and the oscillator and to apply a phase jump.
  * One MTD (`/dev/mtd*`) to perform flash firmware update of the card.

## ART Card disciplining

ART card does not time discipline itself, and needs ART Daemon running on the host to be disciplined.

## Drive is included in the mainstream Linux Kernel

https://git.kernel.org/pub/scm/linux/kernel/git/netdev/net-next.git/commit/?id=a7e1abad13f3f0366ee625831fecda2b603cdc17
