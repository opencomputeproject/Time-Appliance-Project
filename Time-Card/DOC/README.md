# Time Card (The Heart of Open Source Grandmaster) 

![GitHub Logo](Artwork/background.jpg)

# TAP Project

## Table of Contents
1. [Abbreviations](#Abbreviations)
1. [General](#General)
1. [High-Level Architecture](#High-Level-Architecture)
   1. [Responsibilities and Requirements](#Responsibilities-and-Requirements)
      1. [COTS Server](#COTS-Server)
      1. [Network Interface Card](#Network-Interface-Card)
      1. [Time Card](#Time-Card)
1. [Detailed Architecture](#Detailed-Architecture)
   1. [COTS Server](#COTS-Server)
      1. [Hardware](#Hardware)
      1. [Software](#Software)
   1. [NIC](#NIC)
      1. [Form-Factor](#Form-Factor)
      1. [Pcie Interface](#Pcie-Interface)
      1. [Network Ports](#Network-Ports)
      1. [PPS out](#PPS-out)
      1. [PPS In](#PPS-In)
      1. [Hardware timestamps](#Hardware-timestamps)
   1. [Time Card](#Time-Card-1)
      1. [Form Factor](#Form-Factor)
      1. [GNSS](#GNSS)
         1. [Receiver](#Receiver)
         1. [Security](#Security)
      1. [Clock](#Clock)
      1. [Bridge](#example)
         1. [Hardware Implementation](#Hardware-Implementation)
         1. [Software Implementation](#Software-Implementation)
      1. [Interfaces](#Interfaces)
         1. [LED](#LED)

## List of images
List Of Images | Description
------------ | -------------
[Figure 1](#figure-1) | OGM Block Diagram
[Figure 2](#figure-2) | GNSS Receiver
[Figure 3](#figure-3) | Atomic Clock Example
[Figure 4](#figure-4) | Time Card Block Diagram
[Figure 5](#figure-5) | Bridge Block Diagram


## Abbreviations
Abbreviation | Description
------------ | -------------
AC | Atomic Clock
COTS | Commodity off-the-shelf
DC | Datacenter
GMC | GrandMaster Clock
GM | GrandMaster
GNSS | Global Navigation Satellite System
GTM | Go-To-Market
HW | Hardware
NIC | Network Interface Card
NTP | Network Time Protocol
OCP | Open Compute Platform
OCXO | Oven-Controlled Oscillator
OGM | Open GrandMaster
PHC | PTP Hardware Clock
PTM | Precision Time Measurements
PTP | Precision Time Protocol
SW | Software
TAP | Time Appliance Project
TCXO | Temperature-compensated Oscillator
ToD | Time of Day
TS | Timestamp
XO | Oscillator

Table 1. Abbreviations

# General 

[OCP TAP](https://www.opencompute.org/wiki/Time_Appliances_Project) is targeting to ease the addition of Time-sync as a service to the datacenter. The Project targets are to define the service requirements, deployment, and design of an open reference design. 

The time-sync service is relying on a synchronization technology, for now, we are adopting PTP (IEEE 1588) with some addition to that and NTP. 

PTP architecture is scalable and defines the time source from an entity called the **Grandmaster clock** (or stratum 1 in NTP terms).  The GM is distributing time to the entire network and usually gets its timing from an external source, (GNSS signal). 

The current state-of-the-art grandmaster implementations suffer from a few drawbacks that we wish to accommodate:

* They are HW appliances that usually target different GTM than a DC 
* They expose none standard and inconsistent Interfaces and SW feature-sets
* Development cycles and the effort needed to add new features are long and expensive
* It doesn’t rely on open-source software
* The accuracy/stability grades aren’t in line with DC requirements 

This document describes an open architecture of a Grandmaster, that could eventually be deployed either in a DC or in an edge environment. 


# High-Level Architecture

In general, the OGM is divided into 3 HW components:

1. COTS server 
2. Commodity NIC 
3. Time Card 

The philosophy behind this fragmentation is very clear, and each decision, modification that will be made, must look-out to this philosophy:

* COTS servers keep their “value for money” due to huge market requirements. They are usually updated with the latest OS version, security patches, and newer technology, faster than HW appliances. 
* Modern Commodity NICs already support HW timestamp, lead the market with Ethernet and PCIe latest Speeds and Feeds. Modern NIC also supports a wide range of OS versions and comes with a great software ecosystem. NIC + COTS server will allow the OGM to run a full software (and even open source one) PTP and NTP stack. 
* Timecard will be the smallest (conceptually) possible HW board, which will provide the GNSS signal input and stable frequency input. Isolating these functions in a timecard will allow OGM to choose the proper timecard for their needs (accuracy, stability, cost, etc) and remain with the same SW, interface, and architecture.
## Responsibilities and Requirements 
### COTS Server
* Run commodity OS
* PCIe as an interconnect
* PTM Support
#### Network Interface Card
* PPS in/out
* PTM Support
* Hardware timestamps
* [optional] Time of day tunnel from timecard to SW
#### Time Card
* Holdover
* GNSS in
* PPS in/out
* Leap second awareness
* Time of day
* [optional] PTM Support

# Detailed Architecture 
## COTS Server
### Hardware
* 2x PCIe Gen3.0/Gen4.0 slots
### Software
* Linux/*nix operating system
* ptp4l serves PTP on NIC
* Chrony/NTPd reading `/dev/ptpX` of a NIC
## NIC
### Form-Factor
* Standard PCIe Stand-up Card
* Half-Height, Half-Length, Tall Bracket
* Single Slot - Passive Cooling Solution
* Support for Standard PCIe Tall and Short brackets

### PCIe Interface
* PCIe Gen3.0/Gen4.0 X n lanes on Gold-fingers, where n = at least 8

### Network Ports
* Single or Dual-port Ethernet

### PPS out
* PPS Out Rise/Fall Time < 5 nano Sec 
* PPS Out Delay < 400 pico Sec
* PPS Out Jitter < 250 fento Sec
* PPS Out Impedance	= 50 Ohm
* PPS Out frequency	1Hz - 10MHz

### PPS In
* PPS In Delay < 400 pico Sec
* PPS In Jitter < 250 fento Sec
* PPS In Impedance 	= 50 Ohm
* PPS In frequency 1Hz - 10MHz


### Hardware timestamps 

NIC should timestamp all ingress packets.  
Non PTP packets can be batch and have a common TS in the SW descriptor, as long as they are not distant more than TBD nanosecond.  
NIC should timestamp all PPP egress packets.  

* PHC
* PTM 
* 1PPS input
* [optional] 10MHz input which can be used as frequency input to the TSU unit 
* [optional] Multi-host support

Examples:
* [NVIDIA ConnectX-6 Dx](https://www.mellanox.com/products/ethernet-adapters/connectx-6-dx)
        

## Time Card
<a id="Figure-1">![OGM Block Diagram](https://user-images.githubusercontent.com/4749052/94845761-0c7a4d00-0418-11eb-86f6-6c93f649b8de.png)</a>

<p align="center">Figure 1. OGM Block Diagram</p>

General Idea is this card will be connected via PCIe to the server and provide Time Of Day (TOD) via /dev/ptpX interface. Using this interface ptp4l will continuously synchronize PHC on the network card from the atomic clock on the Time Card. This provides precision &lt; 1us.

For the extremely high precision 1PPS output of the Time Card will be connected to the 1PPS input of the NIC, providing &lt;100ns precision. 

### Form Factor
* Standard PCIe Stand-up Card
* Single Slot - Passive Cooling Solution

### GNSS
#### Receiver
The GNSS receiver can be a product from ublock or any other vendor as long as it provides PPS output and the TOD using any suitable format.

This is the recommended module:  **u-blox LEA-M8T-0-10 concurrent GNSS timing module**


<a id="Figure-2">![GNSS Receiver](https://user-images.githubusercontent.com/4749052/94846436-0b95eb00-0419-11eb-9d20-f543b65a0ea8.png)</a>

<p align="center">Figure 2. GNSS Receiver</p>

#### Security
There are 2 main attack vectors on GNSS receiver
#### Jamming
Jamming is the simplest form of attack. In order to keep operations while under attack the most reliable approach is to perform a long run holdover.  
See more about holdover in the [clock](#Clock) section.

#### Spoofing
GNSS authenticity is relevant today. A mechanism to protect against over-the-air spoofing incidents is desirable.
With a special equipment it is possible to simulate GNSS constellation and spoof the receiver. Basic principals to protect against such attack:
* Use high-quality GNSS receivers which verify packet signature
* Disciplining implementations see more in [bridge](#bridge) section should protect against sudden jumps in time and space. For the datacenter use cases jump in space could be completely forbidden.

### Clock
GNSS requires "clear sky" to function properly. Moreover there were several historical events of a short term time jumps by some GNSS constallations.
Because of reliability and in combination with the security concenrns an additional holdover should be performed by [high quality](https://www.meinbergglobal.com/english/specs/gpsopt.htm) XO. An example could be AC, OCXO, etc.
In oreder to perform sustainable operation we recommend to use an AC with a holdover ± 1us or HQ OCXO with a holdover ± 22 µs.

One approach is to use rubidium atomic clocks. Examples:
* [SA.3Xm](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/3825-miniature-atomic-clock-mac?fbclid=IwAR26trWBnHtV6ydBpKiViv3qS4jUpHAtQXJumUusIMB_RnCGclg2Qbd6lSc)
* [mRO-50](https://www.orolia.com/products/atomic-clocks-oscillators/mro-50)
* [SA.45s](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/5207-space-csac)

<a id="Figure-3">![Atomic Clock Example](https://user-images.githubusercontent.com/4749052/98942099-6bd27f00-24e5-11eb-868a-a813986e5e94.png)</a>

<p align="center">Figure 3. Atomic Clock Example</p>

### Bridge

Bridge between GNSS receiver and Atomic clock can be implemented using software or hardware solutions. The hardware implementation is preferred and is our end goal.

#### Hardware Implementation
Here is one of the examples of hardware implementations.
* FPGA is responsible for most of the functionality
* Exposed /dev/phc and /dev/pps are read by open source software such as ptp4l and chronyd

<a id="Figure-4">![Time Card - Block Diagram](https://user-images.githubusercontent.com/4749052/95886452-8266a880-0d76-11eb-82b1-950b4a30d60b.png)</a>

<p align="center">Figure 4. Time Card Block Diagram</p>

<a id="Figure-5">![Bridge Block Diagram](https://user-images.githubusercontent.com/4749052/94845452-9aa20380-0417-11eb-9901-ac25b5109ec7.png)</a>

<p align="center">Figure 5. Bridge Block Diagram</p>

#### Software Implementation
Software implementation still requires most of the components, however the communication between components is done with user space software:
* GPSd exposing /dev/ppsY and provides TOD via SHM
* FPGA board reads 1 PPS from different sources
* Host daemon monitors the offset and steers oscillator
* phc2sys can copy data between clocks, including between GPSd and Atomic and then Atomic to PHC on the NIC

### Interfaces
* PCIe
    * PCIe x1 (18 pins) generation 3.0 or above
    * Generic, supporting multiple OS versions
    * Exposes PHC device in Linux (/dev/ptpX) as well as PPS (/dev/ppsY)
    * Exposes leap second indicator to a shared memory segment read by chrony/ptp4l
* 1PPS / 10MHz SMA output
* 1PPS / 10MHz SMA input
* GNSS Antenna SMA input

#### LED

LED should be used to provide externally visible status information of the timing card. 

For example:
* Off - card is not powered or not properly fitted
* Solid green - card is powered, GNSS ok, 1PPS/10MHz output ok
* Flashing green - card is in warm-up, acquiring satellites
* Solid red - alarm / malfunction
