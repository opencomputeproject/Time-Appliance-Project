#MAC RF Out
set_property PACKAGE_PIN V13 [get_ports Mhz10Clk0_ClkIn]
set_property IOSTANDARD LVCMOS33 [get_ports Mhz10Clk0_ClkIn]

#Reset
set_property PACKAGE_PIN T6 [get_ports RstN_RstIn]
set_property IOSTANDARD LVCMOS25 [get_ports RstN_RstIn]

#200Hz
set_property PACKAGE_PIN R4 [get_ports Mhz200ClkP_ClkIn]
set_property IOSTANDARD LVDS_25 [get_ports Mhz200ClkP_ClkIn]
set_property PACKAGE_PIN T4 [get_ports Mhz200ClkN_ClkIn]
set_property IOSTANDARD LVDS_25 [get_ports Mhz200ClkN_ClkIn]

#125MHz
#set_property IOSTANDARD LVDS_25 [get_ports Mhz125ClkP_ClkIn]
set_property PACKAGE_PIN F6 [get_ports Mhz125ClkP_ClkIn]
set_property PACKAGE_PIN E6 [get_ports Mhz125ClkN_ClkIn]
#set_property IOSTANDARD LVDS_25 [get_ports Mhz125ClkN_ClkIn]

#Board Revision
set_property PACKAGE_PIN N20 [get_ports BoardRev0_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports BoardRev0_DatIn]
set_property PACKAGE_PIN N19 [get_ports BoardRev1_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports BoardRev1_DatIn]
set_property PACKAGE_PIN N18 [get_ports BoardRev2_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports BoardRev2_DatIn]

#LEDs
set_property PACKAGE_PIN B13 [get_ports {Led_DatOut[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[0]}]
set_property PACKAGE_PIN C13 [get_ports {Led_DatOut[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[1]}]
set_property PACKAGE_PIN D14 [get_ports {Led_DatOut[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[2]}]
set_property PACKAGE_PIN D15 [get_ports {Led_DatOut[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {Led_DatOut[3]}]

set_property PACKAGE_PIN A14 [get_ports {FpgaLed_DatOut[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {FpgaLed_DatOut[0]}]
set_property PACKAGE_PIN A13 [get_ports {FpgaLed_DatOut[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {FpgaLed_DatOut[1]}]

#EEPROM I2C
set_property PACKAGE_PIN AB16 [get_ports EepromI2cScl_ClkInOut]
set_property IOSTANDARD LVCMOS33 [get_ports EepromI2cScl_ClkInOut]
set_property PACKAGE_PIN AB17 [get_ports EepromI2cSda_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports EepromI2cSda_DatInOut]
set_property PACKAGE_PIN AA13 [get_ports EepromWp_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports EepromWp_DatOut]

#PCA9546 & I2C
set_property PACKAGE_PIN N17 [get_ports Pca9546I2cScl_ClkInOut]
set_property IOSTANDARD LVCMOS33 [get_ports Pca9546I2cScl_ClkInOut]
set_property PACKAGE_PIN T16 [get_ports Pca9546I2cSda_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports Pca9546I2cSda_DatInOut]
set_property PACKAGE_PIN U16 [get_ports Pca9546RstN_RstOut]
set_property IOSTANDARD LVCMOS33 [get_ports Pca9546RstN_RstOut]

#RGB & I2C
set_property PACKAGE_PIN W20 [get_ports RgbI2cScl_ClkInOut]
set_property IOSTANDARD LVCMOS33 [get_ports RgbI2cScl_ClkInOut]
set_property PACKAGE_PIN W19 [get_ports RgbI2cSda_DatInOut]
set_property IOSTANDARD LVCMOS33 [get_ports RgbI2cSda_DatInOut]
set_property PACKAGE_PIN Y18 [get_ports RgbShutDownN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports RgbShutDownN_EnOut]

#SHT3X
set_property PACKAGE_PIN J19 [get_ports Sht3xAlertN_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports Sht3xAlertN_DatIn]
set_property PACKAGE_PIN H19 [get_ports Sht3xRstN_RstOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sht3xRstN_RstOut]

#LM75B
set_property PACKAGE_PIN N22 [get_ports Lm75BInt1N_EvtIn]
set_property IOSTANDARD LVCMOS33 [get_ports Lm75BInt1N_EvtIn]
set_property PACKAGE_PIN M22 [get_ports Lm75BInt2N_EvtIn]
set_property IOSTANDARD LVCMOS33 [get_ports Lm75BInt2N_EvtIn]
set_property PACKAGE_PIN M18 [get_ports Lm75BInt3N_EvtIn]
set_property IOSTANDARD LVCMOS33 [get_ports Lm75BInt3N_EvtIn]

#BNO
set_property PACKAGE_PIN J17 [get_ports BnoRstN_RstOut]
set_property IOSTANDARD LVCMOS33 [get_ports BnoRstN_RstOut]
set_property PACKAGE_PIN L14 [get_ports BnoIntN_EvtIn]
set_property IOSTANDARD LVCMOS33 [get_ports BnoIntN_EvtIn]
set_property PACKAGE_PIN L15 [get_ports BnoBootN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports BnoBootN_DatOut]

#SMBUS
set_property PACKAGE_PIN K16 [get_ports SmI2CBufEn_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmI2CBufEn_EnOut]

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

#SMA Inputs/Outputs
# INPUTS
# ANT1 Y11
# ANT2 Y12
# ANT3 AA21
# ANT4 AA20
# OUTPUTS
# ANT1 W11
# ANT2 W12
# ANT3 V10
# ANT4 W10
#############################################################
#   SMA1 <-> SMA3 swapped due to wrong labelling/placement on poduction timecard
#   SMA2 <-> SMA4 swapped due to wrong labelling/placement on poduction timecard
#
#############################################################


set_property PACKAGE_PIN V10 [get_ports SmaOut1_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut1_DatOut]
set_property DRIVE 16 [get_ports SmaOut1_DatOut]
set_property SLEW FAST [get_ports SmaOut1_DatOut]
set_property PACKAGE_PIN W10 [get_ports SmaOut2_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut2_DatOut]
set_property DRIVE 16 [get_ports SmaOut2_DatOut]
set_property SLEW FAST [get_ports SmaOut2_DatOut]
set_property PACKAGE_PIN W11 [get_ports SmaOut3_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut3_DatOut]
set_property DRIVE 16 [get_ports SmaOut3_DatOut]
set_property SLEW FAST [get_ports SmaOut3_DatOut]
set_property PACKAGE_PIN W12 [get_ports SmaOut4_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports SmaOut4_DatOut]
set_property DRIVE 16 [get_ports SmaOut4_DatOut]
set_property SLEW FAST [get_ports SmaOut4_DatOut]

set_property PACKAGE_PIN AA21 [get_ports SmaIn1_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn1_DatIn]
set_property PULLDOWN true [get_ports SmaIn1_DatIn]
set_property PACKAGE_PIN AA20 [get_ports SmaIn2_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn2_DatIn]
set_property PULLDOWN true [get_ports SmaIn2_DatIn]
set_property PACKAGE_PIN Y11 [get_ports SmaIn3_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn3_DatIn]
set_property PULLDOWN true [get_ports SmaIn3_DatIn]
set_property PACKAGE_PIN Y12 [get_ports SmaIn4_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports SmaIn4_DatIn]
set_property PULLDOWN true [get_ports SmaIn4_DatIn]

set_property PACKAGE_PIN K14 [get_ports Sma1InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1InBufEnableN_EnOut]
set_property PACKAGE_PIN K13 [get_ports Sma1OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1OutBufEnableN_EnOut]

set_property PACKAGE_PIN L13 [get_ports Sma2InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2InBufEnableN_EnOut]
set_property PACKAGE_PIN M13 [get_ports Sma2OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2OutBufEnableN_EnOut]

set_property PACKAGE_PIN H15 [get_ports Sma3InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3InBufEnableN_EnOut]
set_property PACKAGE_PIN J15 [get_ports Sma3OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3OutBufEnableN_EnOut]

set_property PACKAGE_PIN J14 [get_ports Sma4InBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4InBufEnableN_EnOut]
set_property PACKAGE_PIN H14 [get_ports Sma4OutBufEnableN_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4OutBufEnableN_EnOut]

#SMA LEDs
set_property PACKAGE_PIN D17 [get_ports Sma1LedBlueN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1LedBlueN_DatOut]
set_property PACKAGE_PIN C17 [get_ports Sma1LedGreenN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1LedGreenN_DatOut]
set_property PACKAGE_PIN C18 [get_ports Sma1LedRedN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma1LedRedN_DatOut]

set_property PACKAGE_PIN E19  [get_ports Sma2LedBlueN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2LedBlueN_DatOut]
set_property PACKAGE_PIN D19  [get_ports Sma2LedGreenN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2LedGreenN_DatOut]
set_property PACKAGE_PIN F18 [get_ports Sma2LedRedN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma2LedRedN_DatOut]

set_property PACKAGE_PIN F13 [get_ports Sma3LedBlueN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3LedBlueN_DatOut]
set_property PACKAGE_PIN F14 [get_ports Sma3LedGreenN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3LedGreenN_DatOut]
set_property PACKAGE_PIN F16 [get_ports Sma3LedRedN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma3LedRedN_DatOut]

set_property PACKAGE_PIN C14 [get_ports Sma4LedBlueN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4LedBlueN_DatOut]
set_property PACKAGE_PIN C15 [get_ports Sma4LedGreenN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4LedGreenN_DatOut]
set_property PACKAGE_PIN E13 [get_ports Sma4LedRedN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Sma4LedRedN_DatOut]

#USB Mux
set_property PACKAGE_PIN M16 [get_ports DebugUsbMuxSel_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports DebugUsbMuxSel_DatOut]

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

#GNSS1 LEDs
set_property PACKAGE_PIN B20 [get_ports Gnss1LedBlueN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Gnss1LedBlueN_DatOut]
set_property PACKAGE_PIN A20 [get_ports Gnss1LedGreenN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Gnss1LedGreenN_DatOut]
set_property PACKAGE_PIN A18 [get_ports Gnss1LedRedN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports Gnss1LedRedN_DatOut]

#MAC
set_property PACKAGE_PIN AA18 [get_ports MacTxDat_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacTxDat_DatOut]
set_property PACKAGE_PIN AB18 [get_ports MacRxDat_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacRxDat_DatIn]

set_property PACKAGE_PIN AA10 [get_ports MacFreqControl_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacFreqControl_DatOut]
set_property PACKAGE_PIN AB10 [get_ports MacAlarm_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacAlarm_DatIn]
set_property PACKAGE_PIN AA9 [get_ports MacBite_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacBite_DatIn]

set_property PACKAGE_PIN V15 [get_ports MacUsbPower_EnOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbPower_EnOut]
set_property PACKAGE_PIN P14 [get_ports MacUsbP_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbP_DatOut]
set_property PACKAGE_PIN R14 [get_ports MacUsbN_DatOut]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbN_DatOut]

set_property PACKAGE_PIN M15 [get_ports MacUsbOcN_DatIn]
set_property IOSTANDARD LVCMOS33 [get_ports MacUsbOcN_DatIn]

set_property PACKAGE_PIN AB3 [get_ports MacPps0P_EvtOut]
set_property IOSTANDARD LVDS_25 [get_ports MacPps0P_EvtOut]
set_property PACKAGE_PIN AB2 [get_ports MacPps0N_EvtOut]
set_property IOSTANDARD LVDS_25 [get_ports MacPps0N_EvtOut]
set_property PACKAGE_PIN AA5 [get_ports MacPps1P_EvtOut]
set_property IOSTANDARD LVDS_25 [get_ports MacPps1P_EvtOut]
set_property PACKAGE_PIN AB5 [get_ports MacPps1N_EvtOut]
set_property IOSTANDARD LVDS_25 [get_ports MacPps1N_EvtOut]
set_property PACKAGE_PIN W1 [get_ports MacPpsP_EvtIn]
set_property IOSTANDARD LVDS_25 [get_ports MacPpsP_EvtIn]
set_property PACKAGE_PIN Y1 [get_ports MacPpsN_EvtIn]
set_property IOSTANDARD LVDS_25 [get_ports MacPpsN_EvtIn]

#PCIe
set_property PACKAGE_PIN J20 [get_ports PciePerstN_RstIn]
set_property IOSTANDARD LVCMOS33 [get_ports PciePerstN_RstIn]
#set_property PULLUP true [get_ports PciePerstN_RstIn]

set_property PACKAGE_PIN F10 [get_ports PcieRefClkP_ClkIn]
#set_property LOC GTPE2_CHANNEL_X0Y5 [get_cells {Bd_Inst/TimeCard_i/axi_pcie_0/inst/comp_axi_enhanced_pcie/comp_enhanced_core_top_wrap/axi_pcie_enhanced_core_top_i/pcie_7x_v2_0_2_inst/pcie_top_with_gt_top.gt_ges.gt_top_i/pipe_wrapper_i/pipe_lane[0].gt_wrapper_i/gtp_channel.gtpe2_channel_i}]
set_property PACKAGE_PIN B8 [get_ports {pcie_7x_mgt_0_rxp[0]}]
set_property PACKAGE_PIN B4 [get_ports {pcie_7x_mgt_0_txp[0]}]