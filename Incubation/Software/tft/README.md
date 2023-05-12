# The Firmware Tool
This is a simple tool to add a header to a raw firmware binary. The header is used by a driver to check if the hardware is compatible with the image. The header is only 16 bytes long and contains a PCI Vendor ID, a PCI Device ID and a PCI Hardware Revision ID values to check. It also contains the CRC value of the raw image. The reason to add a header is to be sure that incompatible firware will not be written to the device via devlink unintentionally.

## Header format
| Magic Bytes | PCI Vendor ID | PCI Device ID | Image size | HW Rev ID | CRC16 |
| :----: | :----: | :----: | :----: | :----: | :----: |
| 4 bytes | 2 bytes | 2 bytes | 4 bytes | 2 bytes | 2 bytes |

The firmware header consists 6 fields, all values are network order to be consistent across architectures:

1. Magic header (4 bytes, 32 bits) - constant value, ‘OCTC’ means Open Compute Time Card
2. PCI Vendor ID (2 bytes, 16 bits) - PCI device vendor ID compatible with this image
3. PCI Device ID (2 bytes, 16 bits) - PCI device ID compatible with this image
4. Image size (4 bytes, 32 bits, unsigned) - size of firmware itself without header (and footer should we have one)
5. HW Revision (2 bytes, 16 bits) - Information provided by HW register to differentiate revisions of the same board
6. CRC16 (2 bytes, 16 bits) - check value of CRC16 implementation with default polynom implemented in kernel


## Usage
The tool has several options:
* `-input <filename>` - Mandatory option, provides a file name of a raw binary.
* `-output <filename>` - Mandatory option, provides a file name to write a new firmware file with header. If file already exists it will be overwritten.
* `-vendor <int>` - Mandatory option, provides a PCI Vendor ID to add to header.
* `-device <int>` - Mandatory option, provides a PCI Device ID to add to header.
* `-hw <int>` - Optional, used to provide a PCI Hardware Rev ID. Default is 0.
* `-apply` - Optional. This is used to actually create a new (or overwrite) output file with the header in the beginning.

## Examples
```
./tft -input Time-Card/FPGA/Binary/Production/Binaries/TimeCardProduction.bin -output TimeCardProduction_Celestica.bin -vendor 0x18d4 -device 0x1008 -apply
```
This call will create `TimeCardProduction_Celestica.bin` with header for timecard produced by Celestica.
