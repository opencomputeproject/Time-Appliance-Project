#ifndef ATRFIQ_SDR_H
#define ATRFIQ_SDR_H


#include <Arduino.h>
#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_spi.h>




// Register summary, page 181 of datasheet
// https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/DataSheets/Atmel-42415-WIRELESS-AT86RF215_Datasheet.pdf

/*** From https://doc.riot-os.org/at86rf215__registers_8h_source.html ****/

/*
 * Copyright (C) 2019 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 
#ifndef AT86RF215_REGISTERS_H
#define AT86RF215_REGISTERS_H
 
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
 
struct at86rf215_RF_regs {
    uint16_t RG_IRQS;           
    uint16_t RG_IRQM;           
    uint16_t RG_AUXS;           
    uint16_t RG_STATE;          
    uint16_t RG_CMD;            
    uint16_t RG_CS;             
    uint16_t RG_CCF0L;          
    uint16_t RG_CCF0H;          
    uint16_t RG_CNL;            
    uint16_t RG_CNM;            
    uint16_t RG_RXBWC;          
    uint16_t RG_RXDFE;          
    uint16_t RG_AGCC;           
    uint16_t RG_AGCS;           
    uint16_t RG_RSSI;           
    uint16_t RG_EDC;            
    uint16_t RG_EDD;            
    uint16_t RG_EDV;            
    uint16_t RG_RNDV;           
    uint16_t RG_TXCUTC;         
    uint16_t RG_TXDFE;          
    uint16_t RG_PAC;            
    uint16_t RG_PADFE;          
    uint16_t RG_PLL;            
    uint16_t RG_PLLCF;          
    uint16_t RG_TXCI;           
    uint16_t RG_TXCQ;           
    uint16_t RG_TXDACI;         
    uint16_t RG_TXDACQ;         
};

static const struct at86rf215_RF_regs RF09_regs = {
    .RG_IRQS   = 0x00,
    .RG_IRQM   = 0x100,
    .RG_AUXS   = 0x101,
    .RG_STATE  = 0x102,
    .RG_CMD    = 0x103,
    .RG_CS     = 0x104,
    .RG_CCF0L  = 0x105,
    .RG_CCF0H  = 0x106,
    .RG_CNL    = 0x107,
    .RG_CNM    = 0x108,
    .RG_RXBWC  = 0x109,
    .RG_RXDFE  = 0x10A,
    .RG_AGCC   = 0x10B,
    .RG_AGCS   = 0x10C,
    .RG_RSSI   = 0x10D,
    .RG_EDC    = 0x10E,
    .RG_EDD    = 0x10F,
    .RG_EDV    = 0x110,
    .RG_RNDV   = 0x111,
    .RG_TXCUTC = 0x112,
    .RG_TXDFE  = 0x113,
    .RG_PAC    = 0x114,
    .RG_PADFE  = 0x116,
    .RG_PLL    = 0x121,
    .RG_PLLCF  = 0x122,
    .RG_TXCI   = 0x125,
    .RG_TXCQ   = 0x126,
    .RG_TXDACI = 0x127,
    .RG_TXDACQ = 0x128,
};
static const struct at86rf215_RF_regs RF24_regs = {
    .RG_IRQS   = 0x01,
    .RG_IRQM   = 0x200,
    .RG_AUXS   = 0x201,
    .RG_STATE  = 0x202,
    .RG_CMD    = 0x203,
    .RG_CS     = 0x204,
    .RG_CCF0L  = 0x205,
    .RG_CCF0H  = 0x206,
    .RG_CNL    = 0x207,
    .RG_CNM    = 0x208,
    .RG_RXBWC  = 0x209,
    .RG_RXDFE  = 0x20A,
    .RG_AGCC   = 0x20B,
    .RG_AGCS   = 0x20C,
    .RG_RSSI   = 0x20D,
    .RG_EDC    = 0x20E,
    .RG_EDD    = 0x20F,
    .RG_EDV    = 0x210,
    .RG_RNDV   = 0x211,
    .RG_TXCUTC = 0x212,
    .RG_TXDFE  = 0x213,
    .RG_PAC    = 0x214,
    .RG_PADFE  = 0x216,
    .RG_PLL    = 0x221,
    .RG_PLLCF  = 0x222,
    .RG_TXCI   = 0x225,
    .RG_TXCQ   = 0x226,
    .RG_TXDACI = 0x227,
    .RG_TXDACQ = 0x228,
};




#define AT86RF215_PN    (0x34)  /* sub-GHz & 2.4 GHz */
#define AT86RF215IQ_PN  (0x35)  /* I/Q radio only */
#define AT86RF215M_PN   (0x36)  /* sub-GHz only */
#define FLAG_WRITE        0x8000
#define FLAG_READ         0x0000
#define CMD_RF_NOP          0x0
#define CMD_RF_SLEEP        0x1
#define CMD_RF_TRXOFF       0x2
#define CMD_RF_TXPREP       0x3
#define CMD_RF_TX           0x4
#define CMD_RF_RX           0x5
#define CMD_RF_RESET        0x7     /* transceiver reset, the transceiver state
                                       will automatically end up in state TRXOFF */
#define RF_STATE_TRXOFF     0x2     /* Transceiver off, SPI active */
#define RF_STATE_TXPREP     0x3     /* Transmit preparation */
#define RF_STATE_TX         0x4     /* Transmit */
#define RF_STATE_RX         0x5     /* Receive */
#define RF_STATE_TRANSITION 0x6     /* State transition in progress */
#define RF_STATE_RESET      0x7     /* Transceiver is in state RESET or SLEEP */
#define CCF0_24G_OFFSET          1500000U
 
#define RF_SR_4000K                     0x1
#define RF_SR_2000K                     0x2
#define RF_SR_1333K                     0x3
#define RF_SR_1000K                     0x4
#define RF_SR_800K                      0x5
#define RF_SR_666K                      0x6
#define RF_SR_500K                      0x8
#define RF_SR_400K                      0xA
/* The sub-register configures the relative cut-off frequency fCUT
    where 1.0 refers to half the sample frequency fS. */
#define RF_RCUT_FS_BY_8                 (0x0 << RXDFE_RCUT_SHIFT)
#define RF_RCUT_FS_BY_5P3               (0x1 << RXDFE_RCUT_SHIFT)
#define RF_RCUT_FS_BY_4                 (0x2 << RXDFE_RCUT_SHIFT)
#define RF_RCUT_FS_BY_2P6               (0x3 << RXDFE_RCUT_SHIFT)
#define RF_RCUT_FS_BY_2                 (0x4 << RXDFE_RCUT_SHIFT)
 
#define RF_DTB_2_US                     0x0
#define RF_DTB_8_US                     0x1
#define RF_DTB_32_US                    0x2
#define RF_DTB_128_US                   0x3
#define BB_MCS_BPSK_REP4                0
#define BB_MCS_BPSK_REP2                1
#define BB_MCS_QPSK_REP2                2
#define BB_MCS_QPSK_1BY2                3
#define BB_MCS_QPSK_3BY4                4
#define BB_MCS_16QAM_1BY2               5
#define BB_MCS_16QAM_3BY4               6
 
#define RXM_MR_OQPSK                    0x0
#define RXM_LEGACY_OQPSK                0x1
#define RXM_BOTH_OQPSK                  0x2
#define RXM_DISABLE                     0x3
 
#define FSK_MORD_2SFK                   (0 << FSKC0_MORD_SHIFT)
#define FSK_MORD_4SFK                   (1 << FSKC0_MORD_SHIFT)
 
#define FSK_MIDX_3_BY_8                 (0 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_4_BY_8                 (1 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_6_BY_8                 (2 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_8_BY_8                 (3 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_10_BY_8                (4 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_12_BY_8                (5 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_14_BY_8                (6 << FSKC0_MIDX_SHIFT)
#define FSK_MIDX_16_BY_8                (7 << FSKC0_MIDX_SHIFT)
#define FSK_MIDXS_SCALE_7_BY_8          (0 << FSKC0_MIDXS_SHIFT)
#define FSK_MIDXS_SCALE_8_BY_8          (1 << FSKC0_MIDXS_SHIFT)
#define FSK_MIDXS_SCALE_9_BY_8          (2 << FSKC0_MIDXS_SHIFT)
#define FSK_MIDXS_SCALE_10_BY_8         (3 << FSKC0_MIDXS_SHIFT)
#define FSK_BT_05                       (0 << FSKC0_BT_SHIFT)
#define FSK_BT_10                       (1 << FSKC0_BT_SHIFT)
#define FSK_BT_15                       (2 << FSKC0_BT_SHIFT)
#define FSK_BT_20                       (3 << FSKC0_BT_SHIFT)
#define FSK_SRATE_50K                   0x0
#define FSK_SRATE_100K                  0x1
#define FSK_SRATE_150K                  0x2
#define FSK_SRATE_200K                  0x3
#define FSK_SRATE_300K                  0x4
#define FSK_SRATE_400K                  0x5
#define FSK_CHANNEL_SPACING_200K        0x0
#define FSK_CHANNEL_SPACING_400K        0x1
#define FSKC3_SFDT(n) (((n) << FSKC3_SFDT_SHIFT) & FSKC3_SFDT_MASK)
 
#define FSKC3_PDT(n)  (((n) << FSKC3_PDT_SHIFT) & FSKC3_PDT_MASK)
 
#ifdef __cplusplus
}
#endif
 
#endif /* AT86RF215_REGISTERS_H */




/************ Top level init and CLI ***********/

void init_at86_cli(SPI_HandleTypeDef * spi);

#endif