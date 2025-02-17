#**************************************************************
# Clock
#**************************************************************
create_clock -period 20.000 -name DummyClk -waveform {0.000 10.000}
create_clock -period 100.000 -name Mhz10Clk -waveform {0.000 50.000} [get_ports {Mhz10Clk0_ClkIn}]
create_clock -period 100.000 -name Mhz10ClkExt -waveform {0.000 50.000} [get_ports {SmaIn1_DatIn}]
set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets SmaIn1_DatIn_IBUF]

create_clock -period 5.000 -name Mhz200Clk -waveform {0.000 2.500} [get_ports {Mhz200ClkP_ClkIn}]
create_clock -period 8.000 -name Mhz125Clk -waveform {0.000 4.000} [get_ports {Mhz125ClkP_ClkIn}]
create_clock -period 10.000 -name PcieClk [get_ports {PcieRefClkP_ClkIn}]

# Clock Wiz0
create_generated_clock -name PllFixMhz50Clk -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_0/inst/mmcm_adv_inst/CLKIN1] -master_clock [get_clocks Mhz200Clk] [get_pins Bd_Inst/TimeCard_i/clk_wiz_0/inst/mmcm_adv_inst/CLKOUT0]
create_generated_clock -name InternalMhz200Clk -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_0/inst/mmcm_adv_inst/CLKIN1] -master_clock [get_clocks Mhz200Clk] [get_pins Bd_Inst/TimeCard_i/clk_wiz_0/inst/mmcm_adv_inst/CLKOUT1]

# Clock Mux1, 2 and 3
create_generated_clock -name clk1mux -divide_by 1 -add -master_clock Mhz10ClkExt -source [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_0/U0/BufgMux_Inst/I0] [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_0/U0/BufgMux_Inst/O]
create_generated_clock -name clk2mux -divide_by 1 -add -master_clock Mhz10Clk -source [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_0/U0/BufgMux_Inst/I1] [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_0/U0/BufgMux_Inst/O]
set_clock_groups -logically_exclusive -group clk1mux -group clk2mux -group PllFixMhz50Clk -group InternalMhz200Clk
 
create_generated_clock -name clk1mux_cas -divide_by 1 -add -master_clock clk1mux -source [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_2/U0/BufgMux_Inst/I0] [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_2/U0/BufgMux_Inst/O]
create_generated_clock -name clk2mux_cas -divide_by 1 -add -master_clock clk2mux -source [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_2/U0/BufgMux_Inst/I0] [get_pins Bd_Inst/TimeCard_i/BufgMux_IPI_2/U0/BufgMux_Inst/O]
set_clock_groups -logically_exclusive -group clk1mux_cas -group clk2mux_cas -group PllFixMhz50Clk -group InternalMhz200Clk

# Clock Wiz1
create_generated_clock -name InternalMhz200Clk_Src1 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_1/inst/mmcm_adv_inst/CLKIN1] -master_clock clk1mux_cas [get_pins Bd_Inst/TimeCard_i/clk_wiz_1/inst/mmcm_adv_inst/CLKOUT0]
create_generated_clock -name InternalMhz200Clk_Src2 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_1/inst/mmcm_adv_inst/CLKIN1] -master_clock clk2mux_cas [get_pins Bd_Inst/TimeCard_i/clk_wiz_1/inst/mmcm_adv_inst/CLKOUT0]
set_clock_groups -logically_exclusive -group InternalMhz200Clk_Src1 -group InternalMhz200Clk_Src2 -group PllFixMhz50Clk -group InternalMhz200Clk

# Clock Wiz2
create_generated_clock -name PllDynMhz50Clk_Src1 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN1] -master_clock InternalMhz200Clk [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT0]
create_generated_clock -name PllDynMhz50Clk_Src2 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN2] -master_clock InternalMhz200Clk_Src1 [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT0]
create_generated_clock -name PllDynMhz50Clk_Src3 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN2] -master_clock InternalMhz200Clk_Src2 [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT0]
set_clock_groups -logically_exclusive -group PllDynMhz50Clk_Src1 -group PllDynMhz50Clk_Src2 -group PllDynMhz50Clk_Src3 -group PllFixMhz50Clk -group InternalMhz200Clk

create_generated_clock -name PllMhz200Clk_Src1 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN1] -master_clock InternalMhz200Clk [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT1]
create_generated_clock -name PllMhz200Clk_Src2 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN2] -master_clock InternalMhz200Clk_Src1 [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT1]
create_generated_clock -name PllMhz200Clk_Src3 -source [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKIN2] -master_clock InternalMhz200Clk_Src2 [get_pins Bd_Inst/TimeCard_i/clk_wiz_2/inst/mmcm_adv_inst/CLKOUT1]
set_clock_groups -logically_exclusive -group {PllMhz200Clk_Src1 PllDynMhz50Clk_Src1} -group {PllMhz200Clk_Src2 PllDynMhz50Clk_Src2} -group {PllMhz200Clk_Src3 PllDynMhz50Clk_Src3} -group {PllFixMhz50Clk InternalMhz200Clk}

set_clock_groups -logically_exclusive -group DummyClk -group userclk1 -group Mhz200Clk -group Mhz10Clk -group Mhz10ClkExt

set_false_path -from [get_clocks PllFixMhz50Clk] -to [get_clocks Mhz10Clk]
set_false_path -from [get_clocks Mhz10Clk] -to [get_clocks PllFixMhz50Clk]
set_max_delay -from [get_clocks PllFixMhz50Clk] -to [get_clocks Mhz10Clk] 8.000
set_max_delay -from [get_clocks Mhz10Clk] -to [get_clocks PllFixMhz50Clk] 8.000

set_false_path -from [get_clocks PllFixMhz50Clk] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllFixMhz50Clk]
set_max_delay -from [get_clocks PllFixMhz50Clk] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllFixMhz50Clk] 8.000

set_false_path -from [get_clocks PllDynMhz50Clk_Src1] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src1]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src1] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src1] 8.000

set_false_path -from [get_clocks PllDynMhz50Clk_Src2] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src2]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src2] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src2] 8.000

set_false_path -from [get_clocks PllDynMhz50Clk_Src3] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src3]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src3] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllDynMhz50Clk_Src3] 8.000

set_false_path -from [get_clocks PllMhz200Clk_Src1] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src1]
set_max_delay -from [get_clocks PllMhz200Clk_Src1] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src1] 8.000

set_false_path -from [get_clocks PllMhz200Clk_Src2] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src2]
set_max_delay -from [get_clocks PllMhz200Clk_Src2] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src2] 8.000

set_false_path -from [get_clocks PllMhz200Clk_Src3] -to [get_clocks Mhz10ClkExt]
set_false_path -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src3]
set_max_delay -from [get_clocks PllMhz200Clk_Src3] -to [get_clocks Mhz10ClkExt] 8.000
set_max_delay -from [get_clocks Mhz10ClkExt] -to [get_clocks PllMhz200Clk_Src3] 8.000

#**************************************************************
# UART
#**************************************************************
set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {Uart1RxDat_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {Uart1RxDat_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Uart1TxDat_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Uart1TxDat_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {UartGnss1RxDat_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {UartGnss1RxDat_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {UartGnss1TxDat_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {UartGnss1TxDat_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {Uart1RxDat_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {Uart1RxDat_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Uart1TxDat_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Uart1TxDat_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {MacRxDat_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {MacRxDat_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {MacTxDat_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {MacTxDat_DatOut}]


#**************************************************************
# GPIO
#**************************************************************
set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {BoardRev0_DatIn BoardRev1_DatIn BoardRev2_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {BoardRev0_DatIn BoardRev1_DatIn BoardRev2_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Led_DatOut[*]}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Led_DatOut[*]}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {FpgaLed_DatOut[*]}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {FpgaLed_DatOut[*]}]

#**************************************************************
# PCIe
#**************************************************************
set_false_path -from [get_clocks userclk1] -to [get_clocks PllFixMhz50Clk]
set_false_path -from [get_clocks PllFixMhz50Clk] -to [get_clocks userclk1]
set_max_delay -from [get_clocks PllFixMhz50Clk] -to [get_clocks userclk1] 5.000
set_max_delay -from [get_clocks userclk1] -to [get_clocks PllFixMhz50Clk] 5.000

set_false_path -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src1]
set_false_path -from [get_clocks PllDynMhz50Clk_Src1] -to [get_clocks userclk1]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src1] -to [get_clocks userclk1] 5.000
set_max_delay -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src1] 5.000

set_false_path -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src2]
set_false_path -from [get_clocks PllDynMhz50Clk_Src2] -to [get_clocks userclk1]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src2] -to [get_clocks userclk1] 5.000
set_max_delay -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src2] 5.000

set_false_path -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src3]
set_false_path -from [get_clocks PllDynMhz50Clk_Src3] -to [get_clocks userclk1]
set_max_delay -from [get_clocks PllDynMhz50Clk_Src3] -to [get_clocks userclk1] 5.000
set_max_delay -from [get_clocks userclk1] -to [get_clocks PllDynMhz50Clk_Src3] 5.000

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {PciePerstN_RstIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {PciePerstN_RstIn}]

#**************************************************************
# General
#**************************************************************
set_false_path -from [get_clocks DummyClk] -to [get_clocks PllMhz200Clk_Src1]
set_false_path -from [get_clocks PllMhz200Clk_Src1] -to [get_clocks DummyClk]

set_false_path -from [get_clocks DummyClk] -to [get_clocks PllMhz200Clk_Src2]
set_false_path -from [get_clocks PllMhz200Clk_Src2] -to [get_clocks DummyClk]

set_false_path -from [get_clocks DummyClk] -to [get_clocks PllMhz200Clk_Src3]
set_false_path -from [get_clocks PllMhz200Clk_Src3] -to [get_clocks DummyClk]

set_false_path -from [get_clocks DummyClk] -to [get_clocks PllDynMhz50Clk_Src1]
set_false_path -from [get_clocks PllDynMhz50Clk_Src1] -to [get_clocks DummyClk]

set_false_path -from [get_clocks DummyClk] -to [get_clocks PllDynMhz50Clk_Src2]
set_false_path -from [get_clocks PllDynMhz50Clk_Src2] -to [get_clocks DummyClk]

set_false_path -from [get_clocks DummyClk] -to [get_clocks PllDynMhz50Clk_Src3]
set_false_path -from [get_clocks PllDynMhz50Clk_Src3] -to [get_clocks DummyClk]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {RstN_RstIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {RstN_RstIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {EepromWp_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {EepromWp_DatOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {MacPps0P_EvtOut MacPps0N_EvtOut MacPps1P_EvtOut MacPps1N_EvtOut MacUsbPower_EnOut MacUsbP_DatOut MacUsbN_DatOut MacFreqControl_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {MacPps0P_EvtOut MacPps0N_EvtOut MacPps1P_EvtOut MacPps1N_EvtOut MacUsbPower_EnOut MacUsbP_DatOut MacUsbN_DatOut MacFreqControl_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {MacPpsP_EvtIn MacPpsN_EvtIn MacAlarm_DatIn MacBite_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {MacPpsP_EvtIn MacPpsN_EvtIn MacAlarm_DatIn MacBite_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {EepromI2cScl_ClkInOut EepromI2cSda_DatInOut Pca9546I2cScl_ClkInOut Pca9546I2cSda_DatInOut Pca9546RstN_RstOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {EepromI2cScl_ClkInOut EepromI2cSda_DatInOut Pca9546I2cScl_ClkInOut Pca9546I2cSda_DatInOut Pca9546RstN_RstOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {EepromI2cScl_ClkInOut EepromI2cSda_DatInOut Pca9546I2cScl_ClkInOut Pca9546I2cSda_DatInOut }]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {EepromI2cScl_ClkInOut EepromI2cSda_DatInOut Pca9546I2cScl_ClkInOut Pca9546I2cSda_DatInOut }]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {RgbI2cScl_ClkInOut RgbI2cSda_DatInOut RgbShutDownN_EnOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {RgbI2cScl_ClkInOut RgbI2cSda_DatInOut RgbShutDownN_EnOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {RgbI2cScl_ClkInOut RgbI2cSda_DatInOut}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {RgbI2cScl_ClkInOut RgbI2cSda_DatInOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Sht3xRstN_RstOut BnoRstN_RstOut BnoBootN_DatOut SmI2CBufEn_EnOut DebugUsbMuxSel_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sht3xRstN_RstOut BnoRstN_RstOut BnoBootN_DatOut SmI2CBufEn_EnOut DebugUsbMuxSel_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {Sht3xAlertN_DatIn Lm75BInt1N_EvtIn Lm75BInt2N_EvtIn Lm75BInt3N_EvtIn BnoIntN_EvtIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {Sht3xAlertN_DatIn Lm75BInt1N_EvtIn Lm75BInt2N_EvtIn Lm75BInt3N_EvtIn BnoIntN_EvtIn}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Gnss1RstN_RstOut Gnss1LedBlueN_DatOut Gnss1LedGreenN_DatOut Gnss1LedRedN_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Gnss1RstN_RstOut Gnss1LedBlueN_DatOut Gnss1LedGreenN_DatOut Gnss1LedRedN_DatOut}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {Gnss1Tp_DatIn[*]}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {Gnss1Tp_DatIn[*]}]

set_input_delay -clock [get_clocks DummyClk] -min 0.000 [get_ports {SmaIn1_DatIn SmaIn2_DatIn SmaIn3_DatIn SmaIn4_DatIn}]
set_input_delay -clock [get_clocks DummyClk] -max 5.000 [get_ports {SmaIn1_DatIn SmaIn2_DatIn SmaIn3_DatIn SmaIn4_DatIn}]

set_output_delay -clock [get_clocks DummyClk] -min 5.000 [get_ports {SmaOut1_DatOut SmaOut2_DatOut SmaOut3_DatOut SmaOut4_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {SmaOut1_DatOut SmaOut2_DatOut SmaOut3_DatOut SmaOut4_DatOut}]

set_output_delay -clock [get_clocks DummyClk] -min 5.000 [get_ports {Sma1InBufEnableN_EnOut Sma1OutBufEnableN_EnOut Sma2InBufEnableN_EnOut Sma2OutBufEnableN_EnOut Sma3InBufEnableN_EnOut Sma3OutBufEnableN_EnOut Sma4InBufEnableN_EnOut Sma4OutBufEnableN_EnOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sma1InBufEnableN_EnOut Sma1OutBufEnableN_EnOut Sma2InBufEnableN_EnOut Sma2OutBufEnableN_EnOut Sma3InBufEnableN_EnOut Sma3OutBufEnableN_EnOut Sma4InBufEnableN_EnOut Sma4OutBufEnableN_EnOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Sma1LedBlueN_DatOut Sma1LedGreenN_DatOut Sma1LedRedN_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sma1LedBlueN_DatOut Sma1LedGreenN_DatOut Sma1LedRedN_DatOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Sma2LedBlueN_DatOut Sma2LedGreenN_DatOut Sma2LedRedN_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sma2LedBlueN_DatOut Sma2LedGreenN_DatOut Sma2LedRedN_DatOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Sma3LedBlueN_DatOut Sma3LedGreenN_DatOut Sma3LedRedN_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sma3LedBlueN_DatOut Sma3LedGreenN_DatOut Sma3LedRedN_DatOut}]

set_output_delay -clock [get_clocks DummyClk] -min 15.000 [get_ports {Sma4LedBlueN_DatOut Sma4LedGreenN_DatOut Sma4LedRedN_DatOut}]
set_output_delay -clock [get_clocks DummyClk] -max 0.000 [get_ports {Sma4LedBlueN_DatOut Sma4LedGreenN_DatOut Sma4LedRedN_DatOut}]

#**************************************************************
# SPI Flash
#**************************************************************
#https://www.xilinx.com/support/answers/63174.html

set_max_delay 2.5 -from [get_pins -hier *SCK_O_reg_reg/C] -to [get_pins -hier *USRCCLKO] -datapath_only
set_min_delay 0.1 -from [get_pins -hier *SCK_O_reg_reg/C] -to [get_pins -hier *USRCCLKO]

create_generated_clock -name Cclk -source [get_pins -hierarchical *axi_quad_spi_flash/ext_spi_clk] [get_pins -hierarchical *USRCCLKO] -edges {3 5 7} -edge_shift [list 7.5 7.5 7.5]

set_input_delay -clock [get_clocks Cclk] -max 7.450 [get_ports SpiFlashDq*_DatInOut] -clock_fall
set_input_delay -clock [get_clocks Cclk] -min 1.450 [get_ports SpiFlashDq*_DatInOut] -clock_fall
set_multicycle_path 2 -setup -from Cclk -to [get_clocks -of_objects [get_pins -hierarchical */ext_spi_clk]]
set_multicycle_path 1 -hold -end -from Cclk -to [get_clocks -of_objects [get_pins -hierarchical */ext_spi_clk]]
set_output_delay -clock [get_clocks Cclk] -max 2.050 [get_ports SpiFlashDq*_DatInOut]
set_output_delay -clock [get_clocks Cclk] -min -2.800 [get_ports SpiFlashDq*_DatInOut]
set_multicycle_path 2 -setup -start -from [get_clocks -of_objects [get_pins -hierarchical */ext_spi_clk]] -to Cclk
set_multicycle_path 1 -hold -from [get_clocks -of_objects [get_pins -hierarchical */ext_spi_clk]] -to Cclk
set_output_delay -clock [get_clocks Cclk] -max 4 [get_ports {SpiFlashCsN_EnaOut}]
set_output_delay -clock [get_clocks Cclk] -min -4 [get_ports {SpiFlashCsN_EnaOut}]

set_max_delay 5.000 -from [get_pins Bd_Inst/TimeCard_i/axi_quad_spi_flash/U0/NO_DUAL_QUAD_MODE.QSPI_NORMAL/QSPI_LEGACY_MD_GEN.QSPI_CORE_INTERFACE_I/LOGIC_FOR_MD_12_GEN.SPI_MODE_CONTROL_LOGIC_I/RATIO_NOT_EQUAL_4_GENERATE.SCK_O_NQ_4_STARTUP_USED.SCK_O_reg_reg/C] -to [get_pins Bd_Inst/TimeCard_i/axi_quad_spi_flash/U0/NO_DUAL_QUAD_MODE.QSPI_NORMAL/QSPI_LEGACY_MD_GEN.QSPI_CORE_INTERFACE_I/LOGIC_FOR_MD_12_GEN.SCK_MISO_STARTUP_USED.QSPI_STARTUP_BLOCK_I/STARTUP_7SERIES_GEN.STARTUP2_7SERIES_inst/USRCCLKO]
