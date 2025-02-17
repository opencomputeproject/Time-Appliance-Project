# TOD Slave Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Time Of Day (TOD) Slave is a full hardware (FPGA) only implementation of a synchronization core able to synchronize to a Time of Day source via UBX or TSIPv1 over UART.
The whole message parsing, algorithms and calculations are implemented in the core, no CPU is required. This allows running TOD synchronization completely independent 
and standalone from the user application. The core can be configured and monitored by an AXI4Lite-Slave Register interface. 
The core is expected to receive either UBX messages from a GNSS receiver according to the 
[u-blox specifications](https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf) or TSIPv1 messages, according to the [Trimble revision 1.00 Revision C of RES720](https://timing.trimble.com/products/res-720-secure-dual-band-gnss-timing-module/).
This core adapts the second part of the [Adjustable Clock](../AdjustableClock/Readme.md), by applying a time jump, at the beginning of a new second. 
It does not apply any drift or offset correction, which is done by the [PPS Slave](../PpsSlave/Readme.md). 
Therefore, these two cores are intended to be used together, for time and frequency adjustment of the clock.

## 2. Interface Description
### 2.1 TOD Slave IP
The interface of the TOD Slave is:
- System Reset and System Clock as inputs
- The local time input, from the [Adjustable Clock](../AdjustableClock/Readme.md)
- A UART input, to receive the messages from the [GNSS receiver](https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf).
- The calculated time adjustment, as outputs, to be applied to the [Adjustable Clock](../AdjustableClock/Readme.md)
- An AXI4L slave interface, via which the CPU reads and writes the core's registers
 
![TOD Slave IP](Additional%20Files/TodSlave_IP.png) 

The configuration options of the core are:
- The system clock period in nanoseconds.
- Whether the GNSS receiver provides the current second, or the following second.
- The default baud rate of the UART. The supported rates are provided by the 
[GNSS receiver](https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf).
- The default polarity of the UART input. If '1', then the input signal is active-high, else, active-low.

![TOD Slave Gui](Additional%20Files/TodSlave_Config.png)
## 3. Register Set
This is the register set of the TOD Slave. It is accessible via AXI4 Light Memory Mapped. 
All registers are 32bit wide, no burst access, no unaligned access, no byte enables, no timeouts are supported. 
Register address space is not contiguous. Register addresses are only offsets in the memory area where the core is mapped in the AXI inter connects. 
Non existing register access in the mapped memory area is answered with a slave decoding error.
### 3.1 Register Set Overview 
The Register Set overview is shown in the table below. 

![RegisterSet](Additional%20Files/TodSlave_Regset.png)
### 3.2 Register Decription
The tables below describes the registers of the TOD Slave.     

![Control](Additional%20Files/Reg1_Control.png)
![Status](Additional%20Files/Reg2_Status.png)
![Polarity](Additional%20Files/Reg3_Polarity.png)
![Version](Additional%20Files/Reg4_Version.png)
![Correction](Additional%20Files/Reg5_Correction.png)
![Baudrate](Additional%20Files/Reg6_Baudrate.png)
![UtcStatus](Additional%20Files/Reg7_UtcStatus.png)
![TimeToLeap](Additional%20Files/Reg8_TimeToLeap.png)
![AntennaStatus](Additional%20Files/Reg9_AntennaStatus.png)
![SateliteNumber](Additional%20Files/Reg10_SatelliteNumber.png)
## 4 Design Description
The TOD Slave receives messages from the [u-blox GNSS receiver](https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf) or from the [Trimble GNSS receiver](https://infocom.haradacorp.co.jp/wp/wp-content/uploads/2020/09/2021-10-18-RES720_UserGuide_R1C_2021-10-18.pdf).
It calculates the correct second and reports the receiver's status via the AXI4L interface.
The core consists of 4 main operations:
- The UART reception.  
- The detection of the  GNSS messages. 
- The conversion of the TOD to seconds in TAI. 
- The application of the time adjustment.
- The AXI4L interface to configure and monitor the core. 

### 4.1 UART reception
This operation converts the serial UART signal to a byte stream. It handles the RS232 protocol data stream with one start, eight data (LSB first), one stop and no parity. 
Data is oversampled and center aligned sampling is done. Metastability flip-flops handle the asynchronous input. The byte stream includes the 8-bit value and also a byte-valid flag. 
The range of supported baud rates is provided by the [GNSS receiver](https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf). 
The default baud rate is provided by a generic input, but it can be adapted by the AXI4L interface (see Register 6 of [Chapter 3](#3-register-set)). 

|Configuration value|Baud rate|
|:-----------------:|---------|
|                  2|     4800|
|                  3|     9600|
|                  4|    19200|
|                  5|    38400|
|                  6|    57600|
|                  7|   115200|
|                  8|   230400|
|                  9|   460800|

The receiver includes an error detection to decide if a byte was valid or not. 
As soon as the receiver's FSM detects and verifies a byte, it pushes it to the output and goes back to the initial state waiting for the next byte. 

### 4.2 Message detection
The bytes from the UART receiver are sent to the message detection. 
Currently, 5 types of UBX or TSIPv1 messages are detected and their information is extracted either for further processing 
or for reporting via the AXI4L interface(see Registers 7-10 of [Chapter 3](#3-register-set)). 
Only one type of messages can be enabled (see Register 1 of [Chapter 3](#3-register-set)). 
If both UBX and TSIPv1 protocols are enabled, then only UBX will be accepted. 

Each UBX message has a 2-byte checksum at its end. The message format and checksum are validated, 
and if they are correct, the extracted information from the message is considered valid, otherwise it is dropped. If any unsupported message is received it will be ignored. 

Each TSIPv1 message has a 1-byte checksum followed by 2 end characters. The message format and checksum are validated, and if they are correct, the extracted information from the message is considered valid, otherwise it is dropped. If any unsupported message is received it will be ignored.  

Each message type is expected to be received once per second. If a message type has not been received for 3 seconds, the corresponding flag at the register gets invalidated, 
until a valid message is received. The Control Register (see Register 1 of [Chapter 3](#3-register-set)), can disable the detection of each message type. 

The table below summarizes the supported types of UBX and TSIPv1 messages and the extracted information from them.
 
|Message|Information|Usage|
|----------------|-------------------------------------------------|-------------------------------|
|UBX MOMN HW     | Antenna status and jam state                    | Report via the AXI4L interface|
|UBX NAV SAT     | Number of seen satellites and locked satellites | Report via the AXI4L interface|
|UBX NAV TIMELS  | UTC offset and leap second                      | Use the UTC offset to calculate the UTC time, and report the rest via the AXI4L interface|
|UBX NAV TIMEUTC | Get the ToD in format YYYYMMDDHHmmSS            | Use the TOD (and the UTC offset) to calculate the TAI in seconds|
|UBX NAV STATUS  | GPS fix and spoofing status                     | Report via the AXI4L interface|
|TSIP Timing Info| Get the ToD in format YYYYMMDDHHmmSS and the UTC offset| Use the TOD and the UTC offset to calculate the TAI in seconds and report the UTC offset via the AXI4L interface|
|TSIP Alarms| Get Jamming/Spoofing/antenna status, the Leap second and satellite alarm| Report via the AXI4L interface, and if there is a satellite alarm disable the satellite number info|
|TSIP Receiver Info| Get the GNSS fix OK info| Report via the AXI4L interface|
|TSIP Position Info| Get the GNSS fix info |Report via the AXI4L interface|
|TSIP SAtellite Info | Number of seen satellites and locked satellites. For each satellite receive one message| Report via the AXI4L interface| 

### 4.3 Time conversion
In UBX protocol, the TOD is extracted from the UBX NAV TIMEUTC message and the UTC offset is extracted from the UBX NAV TIMELS message. 

In TSIPV1 protocol, the TOD and UTC offset are extracted from the TSIP Timing message. 

The purpose of this operation is to convert the GNSS time to TAI in 32-bit seconds format,
in order to compare it with the time received by the [Adjustable Clock](../AdjustableClock/Readme.md).

NOTE: The Ublox and Trimble GNSS receivers provide the current second via the TIMEUTC and Timing message, respectively. Some receivers might provide the following second. A generic boolean input selects if the received time refers 
to the current or the next second. The default value is True. (see [Chapter 2](# 2-interface-description)).

### 4.4 Time adjustment
The calculated TAI is compared with the time received by the [Adjustable Clock](../AdjustableClock/Readme.md). If the Seconds field is the same, then the clock's time is correct (in terms of seconds), 
so, nothing happens. If the Seconds field is not the same, then the time must be corrected. In order to make the time correction as smooth as possible, a time adjustment is applied at the 
beginning of the next second. The time adjustment assigns the calculated seconds while the nanoseconds field is zero, since it is applied at the beginning of the new second. 

Note: The delay of time adjustment process is 2 clock cycles, so the new time is applied correspondingly 2 clock cycles before the new second. 

### 4.5 AXI slave of the TOD Slave 
The TOD Slave includes an AXI Light Memory Mapped Slave. It provides access to all registers and allows to configure the TOD Slave. 
An AXI Master has to configure the Datasets with AXI writes to the registers, which is typically done by a CPU. 
It also provides a status interface which allows to supervise the status of the GNSS receiver. [Chapter 3](#3-register-set) has a complete description of the register set. 
