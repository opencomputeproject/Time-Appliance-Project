#CLOCK DCXO 1 and 2
set_property PACKAGE_PIN W21 [get_ports Mhz10ClkDcxo1_ClkIn]
set_property IOSTANDARD LVCMOS33 [get_ports Mhz10ClkDcxo1_ClkIn]
set_property PULLDOWN true [get_ports Mhz10ClkDcxo1_ClkIn]

set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets Mhz10ClkDcxo1_ClkIn_IBUF]

#MAC RF Out
set_property PACKAGE_PIN V13 [get_ports Mhz10Clk0_ClkIn]
set_property IOSTANDARD LVCMOS33 [get_ports Mhz10Clk0_ClkIn]
set_property PACKAGE_PIN V14 [get_ports Mhz10Clk1_ClkIn]
set_property IOSTANDARD LVCMOS33 [get_ports Mhz10Clk1_ClkIn]

#Reset
set_property PACKAGE_PIN T6 [get_ports RstN_RstIn]
set_property IOSTANDARD LVCMOS15 [get_ports RstN_RstIn]

#200Hz
set_property IOSTANDARD DIFF_SSTL15 [get_ports Mhz200ClkP_ClkIn]
set_property PACKAGE_PIN R4 [get_ports Mhz200ClkP_ClkIn]
set_property PACKAGE_PIN T4 [get_ports Mhz200ClkN_ClkIn]
set_property IOSTANDARD DIFF_SSTL15 [get_ports Mhz200ClkN_ClkIn]

#125MHz
#set_property IOSTANDARD LVDS_25 [get_ports Mhz125ClkP_ClkIn]
set_property PACKAGE_PIN F6 [get_ports Mhz125ClkP_ClkIn]
set_property PACKAGE_PIN E6 [get_ports Mhz125ClkN_ClkIn]
#set_property IOSTANDARD LVDS_25 [get_ports Mhz125ClkN_ClkIn]

#Buttons
set_property PACKAGE_PIN J21 [get_ports {Key_DatIn[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Key_DatIn[0]}]
set_property PACKAGE_PIN E13 [get_ports {Key_DatIn[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Key_DatIn[1]}]

#LEDs
set_property PACKAGE_PIN B13 [get_ports {Led_DatOut[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[0]}]
set_property PACKAGE_PIN C13 [get_ports {Led_DatOut[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[1]}]
set_property PACKAGE_PIN D14 [get_ports {Led_DatOut[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[2]}]
set_property PACKAGE_PIN D15 [get_ports {Led_DatOut[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[3]}]

#IO LEDs
#set_property PACKAGE_PIN E21 [get_ports {IoLed_DatOut[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {IoLed_DatOut[0]}]
#set_property PACKAGE_PIN D21 [get_ports {IoLed_DatOut[1]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {IoLed_DatOut[1]}]
#set_property PACKAGE_PIN E22 [get_ports {IoLed_DatOut[2]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {IoLed_DatOut[2]}]
#set_property PACKAGE_PIN D22 [get_ports {IoLed_DatOut[3]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {IoLed_DatOut[3]}]

#PMOD
#set_property PACKAGE_PIN M22 [get_ports {Pmod_DatOut[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[0]}]
#set_property PACKAGE_PIN N22 [get_ports {Pmod_DatOut[1]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[1]}]
#set_property PACKAGE_PIN H18 [get_ports {Pmod_DatOut[2]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[2]}]
#set_property PACKAGE_PIN H17 [get_ports {Pmod_DatOut[3]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[3]}]
#set_property PACKAGE_PIN H22 [get_ports {Pmod_DatOut[4]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[4]}]
#set_property PACKAGE_PIN J22 [get_ports {Pmod_DatOut[5]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[5]}]
#set_property PACKAGE_PIN K21 [get_ports {Pmod_DatOut[6]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[6]}]
#set_property PACKAGE_PIN K22 [get_ports {Pmod_DatOut[7]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {Pmod_DatOut[7]}]

#EEPROM
set_property PACKAGE_PIN V9 [get_ports EepromWp_DatOut]
set_property IOSTANDARD LVCMOS15 [get_ports EepromWp_DatOut]

#QSPI Flash
set_property PACKAGE_PIN P22 [get_ports SpiFlashDq0_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports SpiFlashDq0_DatInOut]
set_property PACKAGE_PIN R22 [get_ports SpiFlashDq1_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports SpiFlashDq1_DatInOut]
set_property PACKAGE_PIN P21 [get_ports SpiFlashDq2_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports SpiFlashDq2_DatInOut]
set_property PACKAGE_PIN R21 [get_ports SpiFlashDq3_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports SpiFlashDq3_DatInOut]
set_property PACKAGE_PIN T19 [get_ports SpiFlashCsN_EnaOut]
set_property IOSTANDARD LVCMOS33 [get_ports SpiFlashCsN_EnaOut]

#I2C
set_property PACKAGE_PIN N17 [get_ports I2cScl_ClkInOut]
set_property IOSTANDARD LVCMOS33 [get_ports I2cScl_ClkInOut]
set_property PULLUP true [get_ports I2cScl_ClkInOut]
set_property PACKAGE_PIN T16 [get_ports I2cSda_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports I2cSda_DatInOut]
set_property PULLUP true [get_ports I2cSda_DatInOut]

#SMA Inputs/Outputs
# INPUTS
# ANT1 Y11
# ANT2 Y12
# ANT3 AA21
# ANT4 AA20
# ANT1_IN_EN H15
# ANT2_IN_EN J14
# ANT3_IN_EN K14
# ANT4_IN_EN L13
# OUTPUTS
# ANT1 W11
# ANT2 W12
# ANT3 V10
# ANT4 W10
# ANT1_OUT_EN J15
# ANT2_OUT_EN H14
# ANT3_OUT_EN K13
# ANT4_OUT_EN M13

set_property PACKAGE_PIN Y11 [get_ports SmaIn1_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn1_DatIn]
set_property PULLDOWN true [get_ports SmaIn1_DatIn]
set_property PACKAGE_PIN Y12 [get_ports SmaIn2_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn2_DatIn]
set_property PULLDOWN true [get_ports SmaIn2_DatIn]
set_property PACKAGE_PIN AA21 [get_ports SmaIn3_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn3_DatIn]
set_property PULLDOWN true [get_ports SmaIn3_DatIn]
set_property PACKAGE_PIN AA20 [get_ports SmaIn4_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn4_DatIn]
set_property PULLDOWN true [get_ports SmaIn4_DatIn]

set_property PACKAGE_PIN H15 [get_ports Sma1InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1InBufEnableN_EnOut]
set_property PACKAGE_PIN J14 [get_ports Sma2InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2InBufEnableN_EnOut]
set_property PACKAGE_PIN K14 [get_ports Sma3InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3InBufEnableN_EnOut]
set_property PACKAGE_PIN L13 [get_ports Sma4InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4InBufEnableN_EnOut]

set_property PACKAGE_PIN W11 [get_ports SmaOut1_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut1_DatOut]
set_property DRIVE 16 [get_ports SmaOut1_DatOut]
set_property SLEW FAST [get_ports SmaOut1_DatOut]
set_property PACKAGE_PIN W12 [get_ports SmaOut2_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut2_DatOut]
set_property DRIVE 16 [get_ports SmaOut2_DatOut]
set_property SLEW FAST [get_ports SmaOut2_DatOut]
set_property PACKAGE_PIN V10 [get_ports SmaOut3_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut3_DatOut]
set_property DRIVE 16 [get_ports SmaOut3_DatOut]
set_property SLEW FAST [get_ports SmaOut3_DatOut]
set_property PACKAGE_PIN W10 [get_ports SmaOut4_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut4_DatOut]
set_property DRIVE 16 [get_ports SmaOut4_DatOut]
set_property SLEW FAST [get_ports SmaOut4_DatOut]

set_property PACKAGE_PIN J15 [get_ports Sma1OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1OutBufEnableN_EnOut]
set_property PACKAGE_PIN H14 [get_ports Sma2OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2OutBufEnableN_EnOut]
set_property PACKAGE_PIN K13 [get_ports Sma3OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3OutBufEnableN_EnOut]
set_property PACKAGE_PIN M13 [get_ports Sma4OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4OutBufEnableN_EnOut]

#UART1
set_property PACKAGE_PIN P15 [get_ports Uart1TxDat_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Uart1TxDat_DatOut]
set_property PACKAGE_PIN P16 [get_ports Uart1RxDat_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports Uart1RxDat_DatIn]

#GNSS1
set_property PACKAGE_PIN P20 [get_ports UartGnss1TxDat_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports UartGnss1TxDat_DatOut]
set_property PACKAGE_PIN N15 [get_ports UartGnss1RxDat_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports UartGnss1RxDat_DatIn]
set_property PACKAGE_PIN W14 [get_ports {Gnss1Tp_DatIn[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Gnss1Tp_DatIn[0]}]
set_property PACKAGE_PIN Y14 [get_ports {Gnss1Tp_DatIn[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Gnss1Tp_DatIn[1]}]
set_property PACKAGE_PIN Y16 [get_ports Gnss1RstN_RstOut]
set_property IOSTANDARD LVCMOS33 [get_ports Gnss1RstN_RstOut]

#GNSS2
set_property PACKAGE_PIN M17 [get_ports UartGnss2TxDat_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports UartGnss2TxDat_DatOut]
set_property PACKAGE_PIN J16 [get_ports UartGnss2RxDat_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports UartGnss2RxDat_DatIn]
set_property PACKAGE_PIN G17 [get_ports {Gnss2Tp_DatIn[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Gnss2Tp_DatIn[0]}]
set_property PACKAGE_PIN G18 [get_ports {Gnss2Tp_DatIn[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Gnss2Tp_DatIn[1]}]
set_property PACKAGE_PIN G15 [get_ports Gnss2RstN_RstOut]
set_property IOSTANDARD LVCMOS33 [get_ports Gnss2RstN_RstOut]

#MAC
set_property PACKAGE_PIN AA18 [get_ports MacTxDat_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacTxDat_DatInOut]
set_property PACKAGE_PIN AB18 [get_ports MacRxDat_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacRxDat_DatInOut]

set_property PACKAGE_PIN AA10 [get_ports MacFreqControl_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacFreqControl_DatOut]
set_property PACKAGE_PIN AB10 [get_ports MacAlarm_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacAlarm_DatIn]
set_property PACKAGE_PIN AA9 [get_ports MacBite_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacBite_DatIn]

set_property PACKAGE_PIN V15 [get_ports MacUsbPower_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbPower_EnOut]
set_property PACKAGE_PIN R14 [get_ports MacUsbP_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbP_DatOut]
set_property PACKAGE_PIN P14 [get_ports MacUsbN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbN_DatOut]

set_property PACKAGE_PIN U20 [get_ports MacPps_EvtIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacPps_EvtIn]
set_property PACKAGE_PIN V18 [get_ports MacPps0_EvtOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacPps0_EvtOut]
set_property PACKAGE_PIN P19 [get_ports MacPps1_EvtOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacPps1_EvtOut]

#PCIe
set_property PACKAGE_PIN J20 [get_ports PciePerst_RstIn]
set_property IOSTANDARD LVCMOS33 [get_ports PciePerst_RstIn]
set_property PULLUP true [get_ports PciePerst_RstIn]

set_property PACKAGE_PIN F10 [get_ports PcieRefClkP_ClkIn]
set_property LOC GTPE2_CHANNEL_X0Y5 [get_cells {Bd_Inst/TimeCard_i/axi_pcie_0/inst/comp_axi_enhanced_pcie/comp_enhanced_core_top_wrap/axi_pcie_enhanced_core_top_i/pcie_7x_v2_0_2_inst/pcie_top_with_gt_top.gt_ges.gt_top_i/pipe_wrapper_i/pipe_lane[0].gt_wrapper_i/gtp_channel.gtpe2_channel_i}]
set_property PACKAGE_PIN D11 [get_ports {pcie_7x_mgt_0_rxp[0]}]
set_property PACKAGE_PIN D5 [get_ports {pcie_7x_mgt_0_txp[0]}]