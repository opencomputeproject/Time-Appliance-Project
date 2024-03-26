===============================================================================
ASIX USB3.0/2.0 Gigabit Ethernet Network Adapter
Driver Compilation & Configuration on Linux
===============================================================================

================
Prerequisites
================

Prepare to build the driver, you need the Linux kernel sources installed on the
build machine, and make sure that the version of the running kernel must match
the installed kernel sources. If you don't have the kernel sources, you can get
it from www.kernel.org or contact to your Linux distributor. If you don't know 
how to do, please refer to KERNEL-HOWTO.

Note: Please make sure the kernel is built with one of the "Support for 
      Host-side, XHCI, EHCI, OHCI, or UHCI" option support.

================
Getting Start
================

1. Extract the compressed driver source file to your temporary directory by the
following command:
	$tar -xf DRIVER_SOURCE_PACKAGE.tar.bz2


2. Now, the driver source files should be extracted under the current directory.
Issue the following command to compile the driver:
	$make


3. If the compilation is done, the ax_usb_nic.ko will be created under the
current directory.

================
Usage
================

=== Install driver to your system
1. If you want to use modprobe command to mount the driver, issue the
following command to install the driver into your system:
	$sudo make install
Note: This command will backup the built-in ax88179_178a driver if it exists.

2. If you want to load the driver by modprobe command, issue the following 
commands:
	$sudo modprobe ax_usb_nic

3. If you want to unload the driver by modprobe command, issue the following 
commands:
	$sudo modprobe -r ax_usb_nic

4. If you want to check the information of driver, issue the following commands:
	$modinfo ax_usb_nic 

=== Install driver manually
1. If you want to load the driver manually, go to the driver directory and
execute the following commands:
	$sudo modprobe mii
	$sudo insmod ax_usb_nic.ko

2. If you want to unload the driver, just executing the following command:
	$sudo rmmod ax_usb_nic

3. If you want to check the information of driver, issue the following commands:
	$modinfo ax_usb_nic.ko


================
Programmer & IEEE TEST
================
ASIX USB Ethernet Linux Command Line Programming Tool

[Note]: Enable DEBUG message
* Modify ENABLE_IOCTL_DEBUG to y in Makefile.
	ENABLE_IOCTL_DEBUG = n
	->
	ENABLE_IOCTL_DEBUG = y

1. Extract the compressed driver source file to your temporary directory by the
following command:
	$tar -xf DRIVER_SOURCE_PACKAGE.tar.bz2

2. Now, the driver source files should be extracted under the current directory.
Executing the following command to compile the driver:
	$make

3. Load the driver manually, go to the driver directory and execute the 
following commands:
	$sudo modprobe mii
	$sudo insmod ax_usb_nic.ko

4. If the compilation is well, the ioctl will be created under the current 
directory.

Note: The default way to find the interface is to scan the ASIX device using
 the ethx (x: 0~255).It is defined in the file, command.h.
	(As follows)
	...
	// DEFAULT_SCAN   : scan "eth0" - "eth255"
	// INTERFACE_SCAN : scan all available network interfaces
	#define NET_INTERFACE	DEFAULT_SCAN
	#define	DEFAULT_SCAN	0x00
	#define	INTERFACE_SCAN	0x01
	...
Adjust the contents of #define NET_INTERFACE to select the method you want.

================
Enable PTP(Precision Time Protocol)
================
1. If you want to enable the PTP function, please modify ENABLE_PTP_FUNC to 'y' in the makefile, 
	and then recompile it.
	
	ENABLE_PTP_FUNC = n
	->
	ENABLE_PTP_FUNC = y

=== AX88179/178A EEPROM/eFuse Programmer
1. If you want to read out values of the EEPROM/EFUSE to a file, go to the 
driver directory and execute the following command:
	$sudo ./ax88179_programmer reeprom 0 eeprom 512
	$sudo ./ax88179_programmer reeprom 1 efuse 64

2. If you want to write values of a file to the EEPROM/EFUSE,  go to the driver
directory and execute the following command:
	$sudo ./ax88179_programmer weeprom 0 eeprom 512
	$sudo ./ax88179_programmer weeprom 1 efuse 64

3. If you want to change the MAC address of a dongle, go to the driver directory
 and execute the following command:
	$sudo ./ax88179_programmer chgmac 0 mac_addr 512
	$sudo ./ax88179_programmer chgmac 1 mac_addr 64

4. If you need more information about the instructions, go to the driver 
directory and execute the following commands:
	$sudo ./ax88179_programmer reeprom help
	$sudo ./ax88179_programmer weeprom help
	$sudo ./ax88179_programmer chgmac help

=== AX88179B/179A/772E/772D Flash/eFuse Programmer
1. If you want to get help message for specific command, go to the driver 
directory and execute the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer help [command]

2. If you want to get the version of firmware, go to the driver directory 
and execute the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer rversion

3. If you want to get the MAC address, go to the driver directory and execute 
the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer rmacaddr

4. If you want to write values of a file to the flash, go to the driver 
directory and execute the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer wflash [file]

5. If you want to write the eFuse, go to the driver directory and execute 
the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer wefuse -m [MAC] -s [SN] -f [File]
	--led0 [value] --led1 [value] -p [device]
		-m [MAC]    - MAC address (XX:XX:XX:XX:XX:XX)
		-s [SN]     - Serial number
		-f [File]   - eFuse file path
		--led0 [value]   - value: control_blink (XXXX_XXXX)
		--led1 [value]   - value: control_blink (XXXX_XXXX)
		-p [device] - device: "AX88179B" or "AX88179A" or "AX88772E" or "AX88772D" 
	example: 
		$sudo ./ax88179b_179a_772e_772d_programmer wefuse -m 00:0E:C6:81:79:01 
		-s 00000000000001 -f eFuse_Dump.txt

		$sudo ./ax88179b_179a_772e_772d_programmer wefuse -m 00:0E:C6:81:79:01 
		-s 0000000000179A --led0 8007_0000 --led1 8000_003F -p AX88179A

		$sudo ./ax88179b_179a_772e_772d_programmer wefuse -m 00:0E:C6:87:72:D1
		-s 0000000000772D -p AX88772D

6. If you want to read the eFuse, go to the driver directory and execute 
the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer refuse -f [File]

7. If you want to reload the flash or eFuse to check version or MAC address 
and so, go to the driver directory and execute the following command:
	$sudo ./ax88179b_179a_772e_772d_programmer reload

=== AX88179B/179A/772E/772D IEEE Test Tool
1. If you want to execute IEEE test, go to the driver directory and execute the following command:

	$sudo ./ax88179b_179a_772e_772d_ieee ieeetest speed option
	    -- AX88179B_179A_772E_772D IEEE Test Tool
		[speed]    - 1000: 1000Mbps,  100: 100Mbps,  10: 10Mbps
		[option]   - For 1000Mbps
				M1: Mode 1
				M2: Mode 2
				M3: Mode 3
				M4: Mode 4

			   - For 100Mbps
				CA: Channel A
				CB: Channel B

			   - For 10Mbps
				RP: Random Pattern
				FF: Fixed Pattern(FF)
				MDI: MDI

=== AX88279 Linux Flash Programming Tool
1. If you want to get help message for specific command, go to the driver 
directory and execute the following command:
	$sudo ./ax88279_programmer help [command]

2. If you want to get the version of firmware, go to the driver directory 
and execute the following command:
	$sudo ./ax88279_programmer rversion

3. If you want to get the MAC address, go to the driver directory and execute 
the following command:
	$sudo ./ax88279_programmer rmacaddr

4. If you want to write values of a file to the flash, go to the driver 
directory and execute the following command:
	$sudo ./ax88279_programmer wflash [file]

5. If you want to write the parameter in flash, go to the driver directory and execute 
the following command:
	$sudo ./ax88279_programmer wpara -m [MAC] -s [SN] -p [PID] -v [VID] -P [PS] -M [MN] -D [dump]
	-S [SS] -H [HS] -w [wol] -l [led0 value] -e [led1 value] -d [led2 value]
       -m [MAC]   	 - MAC address (XX:XX:XX:XX:XX:XX) X:'0'-'F'
       -s [SN]    	 - Serial Number (Characters must be less than 19 bytes) X:'0'-'F'
       -p [PID]   	 - Product ID (XX:XX) X:'0'-'F'
       -v [VID]   	 - Vendor ID (XX:XX) X:'0'-'F'
       -P [PS]    	 - Product String (Characters must be less than 19 bytes)
       -M [MN]    	 - Manufacture Name (Characters must be less than 19 bytes)
       -D [dump]	 - The parameter content currently in flash (dump)
       -S [SS]    	 - SS bus power (XX) X:0-896
       -H [HS]    	 - HS bus power (XX) X:0-500
       -w [wol]    	 - wake on LAN (XXXXXXXX) X:digit
       -l [led0 value]	 - value: control_blink (XXXX_XXXX)
       -e [led1 value]	 - value: control_blink (XXXX_XXXX)
	example: 
		$sudo ./ax88279_programmer wpara -m 00:0e:c6:81:79:01 -s 00000000000001

		$sudo ./ax88279_programmer wpara -p 17:90 -v 0b:95 -P ax88279 -M asix

		$sudo ./ax88279_programmer wpara -S 400 -H 200 -w 21426543

		$sudo ./ax88279_programmer wpara -l C113_0004 -e C002_0C4F

6. If you want to view the current parameter content in flash, go to the driver directory and execute 
the following command, and you can see the file parameter.txt in driver directory:
	$sudo ./ax88279_programmer wpara -D dump

7. If you want to reload the flash to check version or MAC address 
and so, go to the driver directory and execute the following command:
	$sudo ./ax88279_programmer reload