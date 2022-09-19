# MSI IRQ Design Description
## Contents

[1. Context Overview](#1-context-overview)

[2. Interface Description](#2-interface-description)

[3. Design Description](#3-design-description)

## 1. Context Overview
The MSI IRQ receives single interrupts of the FPGA cores and puts them into a message for the[AXI-PCIe bridge](https://www.xilinx.com/products/intellectual-property/axi_pcie.html).
Once a message is ready a request is set and it waits until the grant signal from the Xilinx Core. If there are several interrupts pending the messages are sent with the round-robin principle. 
It supports up to 32 Interrupt Requests. 

## 2. Interface Description
### 2.1 MSI IRQ IP
The interface of the MSI IRQ is:
- System Reset and System Clock as inputs
- The list of interrupt requests, as inputs
- The Enable of MSI Irq, as input from the MSI controller 
- The Grant access of MSI, as input from the MSI controller
- The request valid flag of the MSI IRQ, as output to the MSI controller
- The message number of the MSI IRQ, as output to the MSI controller
 
![MSI IRQ IP](Additional%20Files/MsiIrqIP.PNG) 

The core's configuration options are shown below

![MSI IRQ Config](Additional%20Files/MsiIrqConfig.PNG) 


## 3 Design Description
The core supports up to 32 interrupt requests. A generic input specifies the max number of the supported requests of the current design. 
Another generic input, the "Level Interrupt" specifies if the external interrupt is a level interrupt.
A level interrupt is level sensitive while the others expect a rising edge to generate a message. 
The core sends the interrupt requests one by one (round-robin principle) to the output (AXI-PCIe bridge). 
It waits until the request is granted and then moves on to the next request input.
