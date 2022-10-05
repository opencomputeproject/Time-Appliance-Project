---
sidebar_position: 1
---

# Introduction

#### Spec revision 1.0

Time Card is the heart of the [Open Time Server](http://www.opentimeserver.com) Project.  
This spec can be accessed using http://www.timingcard.com

![Docusaurus Plushie](/img/timecard/timecard.png)

## General

Time Master is a critical part of a PTP enabled network. It provides accurate time via GNSS while maintaining accuracy in case of GNSS failure via a high stability (and holdover) oscillator such as an atomic clock. Existing products in the market are often closed sourced and are far from having sufficient features. The Time Card project presents an open source solution via a PCIe card.

## Form Factor

- Standard PCIe Stand-up Card
- Single Slot - Passive Cooling Solution

## GNSS

### Receiver

The GNSS receiver can be a product from ublock or any other vendor as long as it provides PPS output and the TOD using any suitable format.

This is the recommended module: **u-blox RCB-F9T GNSS time module**

![GNSS Receiver](https://content.u-blox.com/sites/default/files/products/RCB-F9T_PCBVer-C.png)

<p align="center">GNSS Receiver</p>

### Security

There are 2 main attack vectors on GNSS receiver

### Jamming

Jamming is the simplest form of attack. In order to keep operations while under attack the most reliable approach is to perform a long run holdover.  
See more about holdover in the [clock](#Clock) section.

### Spoofing

GNSS authenticity is relevant today. A mechanism to protect against over-the-air spoofing incidents is desirable. With special equipment it is possible to simulate a GNSS constellation and spoof the receiver. Basic principals to protect against such attack:

- Use high-quality GNSS receivers which verify packet signature
- Disciplining implementations see more in [bridge](#bridge) section should protect against sudden jumps in time and space. For the datacenter use cases jump in space could be completely forbidden.

## Clock

GNSS requires "clear sky" to function properly. Moreover there were several historical events of a short term time jumps by some GNSS constallations. Because of reliability and in combination with the security concerns an additional holdover should be performed by [high quality](https://www.meinbergglobal.com/english/specs/gpsopt.htm) XO. An example could be AC, OCXO, TCXO etc. In order to perform sustainable operation we recommend to use an AC with a holdover ± 1us or HQ OCXO with a holdover ± 22 µs.

Atomic clock examples:

- [SA5X](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/5570-miniature-atomic-clock-mac-sa5x)
- [mRO-50](https://www.orolia.com/products/atomic-clocks-oscillators/mro-50)
- [SA.45s](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/5207-space-csac)

<!-- <p align="center">
<img width="666" alt="microchip-mac-sa5x" src="https://user-images.githubusercontent.com/1751211/132626512-3fded23e-dcad-4325-ba4a-0825a9191dce.png">
<img width="666" alt="microchip-mac-sa5x" src="https://user-images.githubusercontent.com/1751211/132625857-12b92625-4d08-4ae7-be18-f8ff6b79b49d.png">
<img width="555" alt="microchip-mac-sa5x" src="https://user-images.githubusercontent.com/1751211/132625863-4a053483-61bf-4617-9bbe-95c464014563.png">
</p>

<p align="center">Figure 2. Atomic Clock Examples</p> -->

OCXO examples:

- [SiT5711](https://www.sitime.com/products/stratum-3e-ocxos/sit5711)

TCXO examples:

- [SiT5356](https://www.sitime.com/products/super-tcxo/sit5356)

## Bridge

The bridge between the GNSS receiver and the Atomic clock can be implemented using software or hardware solutions. The hardware implementation is preferred and is our end goal.

### Hardware Implementation

Here is one of the examples of hardware implementations.

- FPGA is responsible for most of the functionality
- Exposed /dev/phc and /dev/pps are read by open source software such as ptp4l and chronyd

![Time Card - Block Diagram](https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Time-Card/images/idea.png) ![Bridge Block Diagram 1](https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Time-Card/images/block1.png) ![Bridge Block Diagram 2](https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Time-Card/images/block2.png) ![Bridge Block Diagram 3](https://raw.githubusercontent.com/opencomputeproject/Time-Appliance-Project/master/Time-Card/images/block3.png)

### Software Implementation

Software implementation still requires most of the components, however the communication between components is done with user space software:

- GPSd exposing `/dev/ppsY` and provides `TOD` via `SHM`
- FPGA board reads 1 PPS from different sources
- Host daemon monitors the offset and steers oscillator
- `phc2sys` can copy data between clocks, including between GPSd and Atomic and then Atomic to PHC on the NIC

## Interfaces

- PCIe
  - PCIe x1 (18 pins) generation 1.0 or above on a x4 form-factor
  - Generic, supporting multiple OS versions
  - Exposes PHC device in Linux (`/dev/ptpX`) as well as PPS (`/dev/ppsY`)
  - Exposes leap second indicator to a shared memory segment read by `chrony/ptp4l`
- 1PPS / 10MHz SMA output
- 1PPS / 10MHz SMA input
- IRIG-B input output
- DCF77 input output
- GNSS Antenna SMA input

### LED

An LED should be used to provide externally visible status information of the time card.

LED status signals:

- Off - card is not powered or not properly fitted
- Solid green - card is powered, GNSS ok, 1PPS/10MHz output ok
- Flashing green - card is in warm-up, acquiring satellites
- Solid red - alarm / malfunction

## Precision

Time card has 4 SMA connectors that can be configured as outputs and outputs for various things such as 10Mhz, PHC, MAC, GNSS, GNSS2, IRIG, DCF for output and 10Mhz, PPS1, PPS2, TS1, TS2, IRIG, DCF for input.  
Using a Calnex Sentinel device are comparing various things. Here we are comparing the 1PPS output (Channel A) and the 10Mhz output (Channel B) from the MAC (SA.53).

![MACvs10MHz](/img/timecard/MACvs10MHz.png)

## Repository content

- Bill of Materials (parts from Digikey)
- Schematic and PCB of the time card
- Driver (Kernel Module) CentOS 8
- CAD files for the custom PCIe bracket

## Where can I get one?

You have all necessary source code, BOM, Gerber files and binaries to build it yourself. However, we are currently working with several suppliers and will have their contact info soon available to allow you to purchase an out-of-the-box ready Time Card.

If you don't want to build your own then they are commercially available via Timebeat. For ordering go to https://store.timebeat.app/products/ocp-tap-timecard