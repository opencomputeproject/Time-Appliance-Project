# FPGA Open Source Time Card IP Cores

The folder contains the files for the custom IP cores which are used at the Time Card project. 

```bash
   │
   │   Readme.md
   │
   │   TC_ClockAdjustment.xml
   │   TC_ClockAdjustment_rtl.xml
   │   TC_Servo.xml
   │   TC_Servo_rtl.xml
   │   TC_Time.xml
   │   TC_Time_rtl.xml
   │   
   ├───AdjustableClock
   │               
   ├───ClockDetector
   │               
   ├───CommunicationSelecor
   │               
   ├───ConfMaster
   │               
   ├───CoreList
   │               
   ├───DummyAxiSlave
   │               
   ├───FPGA Version
   │               
   ├───FrequencyCounter
   │               
   ├───MsiIrq
   │               
   ├───PpsGenerator
   │               
   ├───PpsSlave
   │               
   ├───PpsSourceSelector
   │               
   ├───SignalGenerator
   │               
   ├───SignalTimestamper
   │               
   ├───SmaSelector
   │               
   └───TodSlave
```

## Readme.md
The current file.

## User-defined interfaces
The .xml files in the folder define user-defined interfaces which are used by the IP cores of the project. 
Currently, there are 3 user-defined interfaces.

| Interface Name | Interface Description |
|----------------|-----------------------|
| [Time](TC_Time_rtl.xml) | It contains the actual time counter in seconds and nanoseconds and also a validity bit and a flag that notifies a hard time-set. |
| [ClockAdjustment](TC_ClockAdjustment_rtl.xml) | It contains the time adjustment in terms of seconds, nanoseconds, direction, interval over which the adjustment should be applied and validity |
| [Servo](TC_Servo_rtl.xml) | It contains the coefficients of a PI servo loop |

## IP cores
Each user-defined open source IP cores is defined in a seperate folder. Each IP folder has similar structure 
- The **Readme.md** file which describes the FPGA core (context overview, register set description, design description, etc.)     
- The **.vhd** files of the core
- The **Xilinx** folder contains the vendor specific IPI files to integrate the cores in the Vivado tool
- The **Additional Files** folder contains files (e.g. images) for the documentation of the core and -for some cores- file templates

