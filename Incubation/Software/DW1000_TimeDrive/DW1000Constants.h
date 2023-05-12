/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file DW1000Constants.h
 * Arduino driver library (header file) for the Decawave DW1000 UWB transceiver IC.
 */

#ifndef _DW1000CONSTANTS_H_INCLUDED
#define _DW1000CONSTANTS_H_INCLUDED

// time stamp byte length
#define LEN_STAMP 5

// enum to determine RX or TX mode of device
#define IDLE_MODE 0x00
#define RX_MODE 0x01
#define TX_MODE 0x02

// used for SPI ready w/o actual writes
#define JUNK 0x00

// no sub-address for register write
#define NO_SUB 0xFF

// device id register
#define DEV_ID 0x00
#define LEN_DEV_ID 4

// extended unique identifier register
#define EUI 0x01
#define LEN_EUI 8

// PAN identifier, short address register
#define PANADR 0x03
#define LEN_PANADR 4

// device configuration register
#define SYS_CFG 0x04
#define LEN_SYS_CFG 4
#define FFEN_BIT 0
#define FFBC_BIT 1
#define FFAB_BIT 2
#define FFAD_BIT 3
#define FFAA_BIT 4
#define FFAM_BIT 5
#define FFAR_BIT 6
#define DIS_DRXB_BIT 12
#define DIS_STXP_BIT 18
#define HIRQ_POL_BIT 9
#define RXAUTR_BIT 29
#define PHR_MODE_SUB 16
#define LEN_PHR_MODE_SUB 2
#define RXM110K_BIT 22

// device control register
#define SYS_CTRL 0x0D
#define LEN_SYS_CTRL 4
#define SFCST_BIT 0
#define TXSTRT_BIT 1
#define TXDLYS_BIT 2
#define TRXOFF_BIT 6
#define WAIT4RESP_BIT 7
#define RXENAB_BIT 8
#define RXDLYS_BIT 9

// system event status register
#define SYS_STATUS 0x0F
#define LEN_SYS_STATUS 5
#define CPLOCK_BIT 1
#define AAT_BIT 3
#define TXFRB_BIT 4
#define TXPRS_BIT 5
#define TXPHS_BIT 6
#define TXFRS_BIT 7
#define LDEDONE_BIT 10
#define RXPHE_BIT 12
#define RXDFR_BIT 13
#define RXFCG_BIT 14
#define RXFCE_BIT 15
#define RXRFSL_BIT 16
#define RXRFTO_BIT 17
#define RXPTO_BIT 21
#define RXSFDTO_BIT 26
#define LDEERR_BIT 18
#define RFPLL_LL_BIT 24
#define CLKPLL_LL_BIT 25

// system event mask register
// NOTE: uses the bit definitions of SYS_STATUS (below 32)
#define SYS_MASK 0x0E
#define LEN_SYS_MASK 4

// system time counter
#define SYS_TIME 0x06
#define LEN_SYS_TIME LEN_STAMP

// RX timestamp register
#define RX_TIME 0x15
#define LEN_RX_TIME 14
#define RX_STAMP_SUB 0x00
#define FP_AMPL1_SUB 0x07
#define LEN_RX_STAMP LEN_STAMP
#define LEN_FP_AMPL1 2

// RX frame quality
#define RX_FQUAL 0x12
#define LEN_RX_FQUAL 8
#define STD_NOISE_SUB 0x00
#define FP_AMPL2_SUB 0x02
#define FP_AMPL3_SUB 0x04
#define CIR_PWR_SUB 0x06
#define LEN_STD_NOISE 2
#define LEN_FP_AMPL2 2
#define LEN_FP_AMPL3 2
#define LEN_CIR_PWR 2

// TX timestamp register
#define TX_TIME 0x17
#define LEN_TX_TIME 10
#define TX_STAMP_SUB 0
#define LEN_TX_STAMP LEN_STAMP

// timing register (for delayed RX/TX)
#define DX_TIME 0x0A
#define LEN_DX_TIME LEN_STAMP

// transmit data buffer
#define TX_BUFFER 0x09
#define LEN_TX_BUFFER 1024
#define LEN_UWB_FRAMES 127
#define LEN_EXT_UWB_FRAMES 1023

// RX frame info
#define RX_FINFO 0x10
#define LEN_RX_FINFO 4

// receive data buffer
#define RX_BUFFER 0x11
#define LEN_RX_BUFFER 1024

// transmit control
#define TX_FCTRL 0x08
#define LEN_TX_FCTRL 5

// channel control
#define CHAN_CTRL 0x1F
#define LEN_CHAN_CTRL 4
#define DWSFD_BIT 17
#define TNSSFD_BIT 20
#define RNSSFD_BIT 21

// user-defined SFD
#define USR_SFD 0x21
#define LEN_USR_SFD 41
#define SFD_LENGTH_SUB 0x00
#define LEN_SFD_LENGTH 1

// OTP control (for LDE micro code loading only)
#define OTP_IF 0x2D
#define OTP_ADDR_SUB 0x04
#define OTP_CTRL_SUB 0x06
#define OTP_RDAT_SUB 0x0A
#define LEN_OTP_ADDR 2
#define LEN_OTP_CTRL 2
#define LEN_OTP_RDAT 4

// AGC_TUNE1/2 (for re-tuning only)
#define AGC_TUNE 0x23
#define AGC_TUNE1_SUB 0x04
#define AGC_TUNE2_SUB 0x0C
#define AGC_TUNE3_SUB 0x12
#define LEN_AGC_TUNE1 2
#define LEN_AGC_TUNE2 4
#define LEN_AGC_TUNE3 2

// DRX_TUNE2 (for re-tuning only)
#define DRX_TUNE 0x27
#define DRX_TUNE0b_SUB 0x02
#define DRX_TUNE1a_SUB 0x04
#define DRX_TUNE1b_SUB 0x06
#define DRX_TUNE2_SUB 0x08
#define DRX_TUNE4H_SUB 0x26
#define LEN_DRX_TUNE0b 2
#define LEN_DRX_TUNE1a 2
#define LEN_DRX_TUNE1b 2
#define LEN_DRX_TUNE2 4
#define LEN_DRX_TUNE4H 2

// LDE_CFG1 (for re-tuning only)
#define LDE_IF 0x2E
#define LDE_CFG1_SUB 0x0806
#define LDE_RXANTD_SUB 0x1804
#define LDE_CFG2_SUB 0x1806
#define LDE_REPC_SUB 0x2804
#define LEN_LDE_CFG1 1
#define LEN_LDE_CFG2 2
#define LEN_LDE_REPC 2
#define LEN_LDE_RXANTD 2

// TX_POWER (for re-tuning only)
#define TX_POWER 0x1E
#define LEN_TX_POWER 4

// RF_CONF (for re-tuning only)
#define RF_CONF 0x28
#define RF_RXCTRLH_SUB 0x0B
#define RF_TXCTRL_SUB 0x0C
#define LEN_RF_RXCTRLH 1
#define LEN_RF_TXCTRL 4

// TX_CAL (for re-tuning only)
#define TX_CAL 0x2A
#define TC_PGDELAY_SUB 0x0B
#define LEN_TC_PGDELAY 1
#define TC_SARC 0x00
#define TC_SARL 0x03

// FS_CTRL (for re-tuning only)
#define FS_CTRL 0x2B
#define FS_PLLCFG_SUB 0x07
#define FS_PLLTUNE_SUB 0x0B
#define FS_XTALT_SUB 0x0E
#define LEN_FS_PLLCFG 4
#define LEN_FS_PLLTUNE 1
#define LEN_FS_XTALT 1

// AON
#define AON 0x2C
#define AON_WCFG_SUB 0x00
#define LEN_AON_WCFG 2
#define ONW_LDC_BIT 6
#define ONW_LDD0_BIT 12
#define AON_CTRL_SUB 0x02
#define LEN_AON_CTRL 1
#define RESTORE_BIT 0
#define SAVE_BIT 1
#define UPL_CFG_BIT 2

#define AON_CFG0_SUB 0x06
#define LEN_AON_CFG0 4
#define SLEEP_EN_BIT 0
#define WAKE_PIN_BIT 1
#define WAKE_SPI_BIT 2
#define WAKE_CNT_BIT 3

// PMSC
#define PMSC 0x36
#define PMSC_CTRL0_SUB 0x00
#define PMSC_CTRL1_SUB 0x04
#define PMSC_LEDC_SUB 0x28
#define LEN_PMSC_CTRL0 4
#define LEN_PMSC_CTRL1 4
#define LEN_PMSC_LEDC 4
#define GPDCE_BIT 18
#define KHZCLKEN_BIT 23
#define BLNKEN 8

#define ATXSLP_BIT 11
#define ARXSLP_BIT 12

// TX_ANTD Antenna delays
#define TX_ANTD 0x18
#define LEN_TX_ANTD 2

// GPIO
#define GPIO_CTRL 0x26
#define GPIO_MODE_SUB 0x00
#define LEN_GPIO_MODE 4

#define MSGP0 6
#define MSGP1 8
#define MSGP2 10
#define MSGP3 12
#define MSGP4 14
#define MSGP5 16
#define MSGP6 18
#define MSGP7 20
#define MSGP8 22

#define GPIO_MODE 0
#define LED_MODE 1

#endif
