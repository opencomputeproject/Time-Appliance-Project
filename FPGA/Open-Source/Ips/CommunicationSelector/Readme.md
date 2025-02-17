# Communication Selector Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Register Set](#3-register-set)

[4. Design Description](#4-design-description)

## 1. Context Overview
The Communication Selector is a full hardware (FPGA) implementation that selects which communication interface will be used between the FPGA and the module's clock (e.g. MAC, OCXO). The selection can be received by an AXI configuration register (e.g. by using [AXI GPIO](https://www.xilinx.com/products/intellectual-property/axi_gpio.html#documentation)) and it is set as UART (Selection:'0') or I<sup>2</sup>C (Selection:'1'). The default configuration is UART communication.

## 2. Interface Description
### 2.1 Communication Selector IP
The interface of the Communication Selector is:
- The selection of the communication from an AXI register as input
- The UART interface (inputs and outputs) 
- The UART Interrupt request as input
- The I<sup>2</sup>C interface (inputs and outputs)
- The UART Interupt request as input 

![CommunicatioSelectorIP](Additional%20Files/CommunicationSelectorIP.png)

The core providess no configuration options 

## 3. Register Set
The Communication Selector has no dedicated AXI4L interface. It receives though the Selection input which can provided via an external AXI interface. This interface is implementation specific, and outside of this document's scope.

## 4 Design Description
The core multiplexes a UART and an I<sup> 2</sup>C interface so that the same board pinout can support both interfaces. Since the I<sup> 2</sup>C interface has more pins than the UART, when the UART interfaces is selected, some of the pins will be unused.

![CommunicationMux](Additional%20Files/CommunicationMux.png)



The table below shows the port assignment of inputs to outputs when Selection is 0 (UART):

|                       |SCL In|SCL Out|SCL T|SDA In|SDA Out|SDA T|IRQ|
|-----------------------|:----:|:-----:|:---:|:----:|:-----:|:---:|:-:|
|UART Rx                ||||X|
|UART Tx                ||X|
|UART IRQ               |||||||X|
|I<sup> 2</sup>C SCL In |
|I<sup> 2</sup>C SCL Out|
|I<sup> 2</sup>C SCL T  |
|I<sup> 2</sup>C SDA In |
|I<sup> 2</sup>C SDA Out|
|I<sup> 2</sup>C SDA T  |
|I<sup> 2</sup>C IRQ    |

The table below shows the port assignment of inputs to outputs when Selection is 1 (I<sup> 2</sup>C):

|                       |SCL In|SCL Out|SCL T|SDA In|SDA Out|SDA T|IRQ|
|-----------------------|:----:|:-----:|:---:|:----:|:-----:|:---:|:-:|
|UART Rx                |
|UART Tx                |
|UART IRQ               |
|I<sup> 2</sup>C SCL In |X|
|I<sup> 2</sup>C SCL Out||X|
|I<sup> 2</sup>C SCL T  |||X|
|I<sup> 2</sup>C SDA In ||||X|
|I<sup> 2</sup>C SDA Out|||||X|
|I<sup> 2</sup>C SDA T  ||||||X|
|I<sup> 2</sup>C SDA IRQ|||||||X|
