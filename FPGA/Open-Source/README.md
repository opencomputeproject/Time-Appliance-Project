# FPGA Open Source Time Card structure

The FPGA Open Source TimeCard repository is structured as shown below

```bash
    │
    │   Readme.md
    │   
    ├───Implementation
    │   └───Xilinx
    │       └───TimeCard
    │           │   CoreListFile.txt
    │           │   DefaultConfigFile.txt
    │           │   CreateBinaries.tcl
    │           │   CreateBinariesAll.tcl
    │           │   CreateBinariesGolden.tcl
    │           │   CreateProject.tcl
    │           │   Readme.md
    │           │   
    │           ├───Additional Files
    │           │       
    │           ├───Bd
    │           │       
    │           ├───Binaries
    │           │
    │           ├───Constraints
    │           │
    │           ├───(TimeCard)
    │           │
    │           └───Top
    │                   
    ├───Ips
    │   │   TC_ClockAdjustment.xml
    │   │   TC_ClockAdjustment_rtl.xml
    │   │   TC_Servo.xml
    │   │   TC_Servo_rtl.xml
    │   │   TC_Time.xml
    │   │   TC_Time_rtl.xml
    │   │   
    │   ├───AdjustableClock
    │   │               
    │   ├───ClockDetector
    │   │               
    │   ├───ConfMaster
    │   │               
    │   ├───CoreList
    │   │               
    │   ├───DummyAxiSlave
    │   │               
    │   ├───FPGA Version
    │   │               
    │   ├───FrequencyCounter
    │   │               
    │   ├───MsiIrq
    │   │               
    │   ├───PpsGenerator
    │   │               
    │   ├───PpsSlave
    │   │               
    │   ├───PpsSourceSelector
    │   │               
    │   ├───SignalGenerator
    │   │               
    │   ├───SignalTimestamper
    │   │               
    │   ├───SmaSelector
    │   │               
    │   └───TodSlave
    │                   
    ├───Modules
    │   └───BufgMux
    │           
    └───Package
```

## Implementation
The FPGA implementations of the TimeCard are separated depending on the vendor/version. Currently, only a Xilinx implementation of the Open Source Timecard is available. 
The top folder of the project is under the name [*/[YOUR_PATH]/Implementation/Xilinx/TimeCard*](implementation/Xilinx/TimeCard/).

Under this folder there are the vendor and implementation dependent files:
- File **Readme.md** describes the design of the implementation and instructs how to create/implement the project and generate the binaries.
- File **DefaultConfigFile.txt** is the default configuration of the FPGA cores
- File **CoreListFile.txt** lists the configurable cores of the FPGA
- 4 **.tcl** files for creating/implementing the project and generating the binaries
- Folder **Additional Files** contains files (e.g. images) for the documentation of the project 
- Folder **Bd** contains the **.tcl** files for the project's Block Design creation (called by the CreateProject.tcl)
- Folder **Binaries** contains the generated binaries of the project after each implementation run 
- Folder **Constraints** contains all the constraint files of the project 
- Folder **TimeCard**  contains the project files generated during creation, synthesis and implementation of the project. 
The folder is created when the **CreateProject.tcl** is executed. The folder is not added to the repository and it shall be deleted, in order to call successfully the CreateProject.tcl
- Folder **Top** contains the top .vhd file of the project

## Ips
The [**Ips** folder](Ips) contains all the user-defined open source IP cores. Each folder of an IP core has similar structure 
- File **Readme.md** describes the FPGA core (design overview, register set description, etc.)     
- File(s) **.vhd** of the core
- Folder **Xilinx** contains the vendor specific IPI files to integrate the cores in the Vivado tool
- Folder **Additional Files** contains files (e.g. images) for the documentation of the core

Additionally, the **Ips** folder contains .xml files of the custom type definitions for the IP core interfaces.

## Modules
The [**Modules**  folder](Modules) contains all the user-defined open source .vhd files which are not instantiated as IPI.
## Package
The [**Package** folder ](Package) contains the library file of the project which contains different constants and common used procedures.
