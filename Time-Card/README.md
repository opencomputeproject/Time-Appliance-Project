To save spec as PDF - select text below (including images) -> right click -> Print -> Save as PDF
# Time Card (www.timingcard.com)
#### Spec revision № 1.0

![Time Card](images/timingcard.png)
Time Card is the heart of the Open Grandmaster Project. 
You can find more info at www.ocptap.com

## Table of Contents
1. [General](#General)
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
1. [Precision](#Precision)
1. [Repository content](#Repository-content)
1. [Where can I get one](#Where-can-I-get-one)
1. [License](#License)

## List of images
List Of Images | Description
------------ | -------------
[Figure 1](#figure-1) | GNSS Receiver
[Figure 2](#figure-2) | Atomic Clock Example
[Figure 3](#figure-3) | Time Card Block Diagram
[Figure 4](#figure-4) | Bridge Block Diagram

## General

Grandmaster is a critical part of a PTP enabled network. It provides accurate time via GNSS while maintains the accuracy in case of GNSS failure via a high stability (and holdover) oscillator such as an atomic clock. Exisiting products in the market are often closed sourced an far from sufficient features. The Time Card project presents an open source solution via a PCIe card.


## Form Factor
* Standard PCIe Stand-up Card
* Single Slot - Passive Cooling Solution

## GNSS
### Receiver
The GNSS receiver can be a product from ublock or any other vendor as long as it provides PPS output and the TOD using any suitable format.

This is the recommended module:  **u-blox LEA-M8T-0-10 concurrent GNSS time module**


<a id="Figure-1">![GNSS Receiver](https://user-images.githubusercontent.com/4749052/94846436-0b95eb00-0419-11eb-9d20-f543b65a0ea8.png)</a>

<p align="center">Figure 1. GNSS Receiver</p>

### Security
There are 2 main attack vectors on GNSS receiver
### Jamming
Jamming is the simplest form of attack. In order to keep operations while under attack the most reliable approach is to perform a long run holdover.  
See more about holdover in the [clock](#Clock) section.

### Spoofing
GNSS authenticity is relevant today. A mechanism to protect against over-the-air spoofing incidents is desirable.
With a special equipment it is possible to simulate GNSS constellation and spoof the receiver. Basic principals to protect against such attack:
* Use high-quality GNSS receivers which verify packet signature
* Disciplining implementations see more in [bridge](#bridge) section should protect against sudden jumps in time and space. For the datacenter use cases jump in space could be completely forbidden.

## Clock
GNSS requires "clear sky" to function properly. Moreover there were several historical events of a short term time jumps by some GNSS constallations.
Because of reliability and in combination with the security concenrns an additional holdover should be performed by [high quality](https://www.meinbergglobal.com/english/specs/gpsopt.htm) XO. An example could be AC, OCXO, TCXO etc.
In oreder to perform sustainable operation we recommend to use an AC with a holdover ± 1us or HQ OCXO with a holdover ± 22 µs.

Atomic clock examples:
* [SA.3Xm](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/3825-miniature-atomic-clock-mac?fbclid=IwAR26trWBnHtV6ydBpKiViv3qS4jUpHAtQXJumUusIMB_RnCGclg2Qbd6lSc)
* [mRO-50](https://www.orolia.com/products/atomic-clocks-oscillators/mro-50)
* [SA.45s](https://www.microsemi.com/product-directory/embedded-clocks-frequency-references/5207-space-csac)

<a id="Figure-2">![Atomic Clock Example](https://user-images.githubusercontent.com/4749052/98942099-6bd27f00-24e5-11eb-868a-a813986e5e94.png)</a>

<p align="center">Figure 2. Atomic Clock Example</p>

OCXO examples:
* [SiT5711](https://www.sitime.com/products/stratum-3e-ocxos/sit5711)

TCXO examples:
* [SiT5356](https://www.sitime.com/products/super-tcxo/sit5356)

## Bridge

Bridge between GNSS receiver and Atomic clock can be implemented using software or hardware solutions. The hardware implementation is preferred and is our end goal.

### Hardware Implementation
Here is one of the examples of hardware implementations.
* FPGA is responsible for most of the functionality
* Exposed /dev/phc and /dev/pps are read by open source software such as ptp4l and chronyd

<a id="Figure-3">![Time Card - Block Diagram](https://user-images.githubusercontent.com/4749052/95886452-8266a880-0d76-11eb-82b1-950b4a30d60b.png)</a>

<p align="center">Figure 3. Time Card Block Diagram</p>

<a id="Figure-4">![Bridge Block Diagram](https://user-images.githubusercontent.com/4749052/94845452-9aa20380-0417-11eb-9901-ac25b5109ec7.png)</a>

<p align="center">Figure 4. Bridge Block Diagram</p>

### Software Implementation
Software implementation still requires most of the components, however the communication between components is done with user space software:
* GPSd exposing /dev/ppsY and provides TOD via SHM
* FPGA board reads 1 PPS from different sources
* Host daemon monitors the offset and steers oscillator
* phc2sys can copy data between clocks, including between GPSd and Atomic and then Atomic to PHC on the NIC

## Interfaces
* PCIe
    * PCIe x1 (18 pins) generation 3.0 or above
    * Generic, supporting multiple OS versions
    * Exposes PHC device in Linux (/dev/ptpX) as well as PPS (/dev/ppsY)
    * Exposes leap second indicator to a shared memory segment read by chrony/ptp4l
* 1PPS / 10MHz SMA output
* 1PPS / 10MHz SMA input
* GNSS Antenna SMA input

### LED

LED should be used to provide externally visible status information of the time card. 

For example:
* Off - card is not powered or not properly fitted
* Solid green - card is powered, GNSS ok, 1PPS/10MHz output ok
* Flashing green - card is in warm-up, acquiring satellites
* Solid red - alarm / malfunction

# Precision
Time card has a 1PPS output which can be compared with 1PPS of the locked GNSS receiver.  
Using Calnex Sentinel device we were able to compare 1PPS outputs with internal reference which we call a "True Time".  
![Initial design](images/precision_pps.png)
From our observation we see that PPS of the GNSS receiver is off from "True Time" between -25ns to -55ns (30ns amplitude).  
Where Time Card PPS is off compared to GNSS by approximately 30ns.  
Compared to "True Time" Time Card is actually off by just +5 to -45ns (50ns amplitude).

# Repository content

* Bill of Materials (parts from Digikey)
* Schematic and PCB of the time card
* Driver (Kernel Module) CentOS 8
* CAD files for the custom PCIe bracket 

# Where can I get one?

You have all necessary source code, BOM, Gerber files and binaries to build it youself. However, we are currently working with several suppliers and will have their contact info soon available to allow you to puchase an out-of-the-box ready Time Card.

# License
OCP encourages participants to share their proposals, specifications and designs with the community. This is to promote openness and encourage continuous and open feedback. It is important to remember that by providing feedback for any such documents, whether in written or verbal form, that the contributor or the contributor's organization grants OCP and its members irrevocable right to use this feedback for any purpose without any further obligation. 

It is acknowledged that any such documentation and any ancillary materials that are provided to OCP in connection with this document, including without limitation any white papers, articles, photographs, studies, diagrams, contact information (together, “Materials”) are made available under the Creative Commons Attribution-ShareAlike 4.0 International License found here: https://creativecommons.org/licenses/by-sa/4.0/, or any later version, and without limiting the foregoing, OCP may make the Materials available under such terms.

As a contributor to this document, all members represent that they have the authority to grant the rights and licenses herein.  They further represent and warrant that the Materials do not and will not violate the copyrights or misappropriate the trade secret rights of any third party, including without limitation rights in intellectual property.  The contributor(s) also represent that, to the extent the Materials include materials protected by copyright or trade secret rights that are owned or created by any third-party, they have obtained permission for its use consistent with the foregoing.  They will provide OCP evidence of such permission upon OCP’s request. This document and any "Materials" are published on the respective project's wiki page and are open to the public in accordance with OCP's Bylaws and IP Policy. This can be found at http://www.opencompute.org/participate/legal-documents/.  If you have any questions please contact OCP.

This work is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/).
