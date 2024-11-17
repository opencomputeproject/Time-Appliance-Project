#ifndef ATRFIQ_SDR_H
#define ATRFIQ_SDR_H


#include <Arduino.h>
#include "WWVB_Arduino.h"
#include <stm32h7xx_hal_spi.h>
#include "menu_cli.h"




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


/*!
 * ============================================================================
 * AT86RF215 Internal registers Address
 * ============================================================================
 */

/* Common settings */
#define REG_RF_RST                          0x0005

/* Interrupt registers */
#define REG_RF_CFG                          0x0006
#define REG_RF09_IRQS                       0x0000
#define REG_RF24_IRQS                       0x0001
#define REG_BBC0_IRQS                       0x0002
#define REG_BBC1_IRQS                       0x0003

#define REG_RF09_IRQM                       0x0100
#define REG_RF24_IRQM                       0x0200
#define REG_BBC0_IRQM                       0x0300
#define REG_BBC1_IRQM                       0x0400

/* RF interrupt register values */
#define RF_IRQM_WAKEUP                 		0x00
#define RF_IRQM_TRXRDY                 		0x01
#define RF_IRQM_EDC                    		0x02
#define RF_IRQM_BATLOW                 		0x03
#define RF_IRQM_TRXERR                 		0x04
#define RF_IRQM_IQIFSF               		0x05

/* BB interrupt register values */
#define BB_INTR_RXFS                        0x01
#define BB_INTR_RXFE                        0x02
#define BB_INTR_RXAM                        0x04
#define BB_INTR_RXEM                        0x08
#define BB_INTR_TXFE                        0x10
#define BB_INTR_AGCH                        0x20
#define BB_INTR_AGCR                        0x40
#define BB_INTR_FBLI                        0x80

#define REG_RF_IQIFC0                       0x000A
#define REG_RF_IQIFC1                       0x000B
#define REG_RF_IQIFC2                       0x000C

/* Baseband registers */
#define REG_BBC0_TXFLL                      0x0306
#define REG_BBC0_TXFLH                      0x0307
#define REG_BBC0_FBTXS                      0x2800
#define REG_BBC0_FBTXE                      0x2FFE
#define REG_BBC0_FSKPHRTX                   0x036A
#define REG_BBC0_PC                         0x0301
#define REG_BBC0_FSKDM                      0x0372

#define REG_BBC1_TXFLL                      0x0406
#define REG_BBC1_TXFLH                      0x0407
#define REG_BBC1_FBTXS                      0x3800
#define REG_BBC1_FBTXE                      0x3FFE
#define REG_BBC1_FSKPHRTX                   0x046A
#define REG_BBC1_TXDFE                      0x0113
#define REG_BBC1_PC                         0x0401
#define REG_BBC1_FSKDM                      0x0372

/* Common RF */
#define REG_RF09_PLLCF                      0x0122
#define REG_RF_CLKO							0x0007
#define RF_CLKO_OFF					        0x00





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

/***** A lot of reference code from https://github.com/cariboulabs/cariboulite/blob/main/software/libcariboulite/src/at86rf215/at86rf215_radio.h */

typedef enum
{
    at86rf215_radio_cmd_nop = 0x0,              // No operation (dummy)
    at86rf215_radio_cmd_sleep = 0x1,            // Go to sleep
    at86rf215_radio_state_cmd_trx_off = 0x2,    // Transceiver off, SPI active
    at86rf215_radio_state_cmd_tx_prep = 0x3,    // Transmit preparation
    at86rf215_radio_state_cmd_tx = 0x4,         // Transmit
    at86rf215_radio_state_cmd_rx = 0x5,         // Receive
    at86rf215_radio_state_transition = 0x6,     // State transition in progress
    at86rf215_radio_state_cmd_reset = 0x7,      // Transceiver is in state RESET or SLEEP
                                                // if commanded reset, the transceiver state will
                                                // automatically end up in state TRXOFF
} at86rf215_radio_state_cmd_en;



typedef enum
{
    at86rf215_radio_channel_mode_ieee = 0x00,
    // IEEE compliant channel scheme; f=(CCF0+CN*CS)*25kHz+f offset ;
    // (f offset = 0Hz for sub-1GHz transceiver; f offset = 1.5GHz for 2.4GHz transceiver)

    at86rf215_radio_channel_mode_fine_low = 0x01,
    // Fine resolution (389.5-510.0)MHz with 99.182Hz channel stepping

    at86rf215_radio_channel_mode_fine_mid = 0x02,
    // Fine resolution (779-1020)MHz with 198.364Hz channel stepping

    at86rf215_radio_channel_mode_fine_high = 0x03,
    // Fine resolution (2400-2483.5)MHz with 396.728Hz channel stepping

} at86rf215_radio_channel_mode_en;

/** offset (in Hz) for CCF0 in 2.4 GHz mode */
#define CCF0_24G_OFFSET          1500000U

typedef enum
{
    at86rf215_radio_rx_bw_BW160KHZ_IF250KHZ = 0x0,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW200KHZ_IF250KHZ = 0x1,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW250KHZ_IF250KHZ = 0x2,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW320KHZ_IF500KHZ = 0x3,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW400KHZ_IF500KHZ = 0x4,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW500KHZ_IF500KHZ = 0x5,      // at86rf215_radio_rx_f_cut_0_25_half_fs
    at86rf215_radio_rx_bw_BW630KHZ_IF1000KHZ = 0x6,     // at86rf215_radio_rx_f_cut_0_375_half_fs
    at86rf215_radio_rx_bw_BW800KHZ_IF1000KHZ = 0x7,     // at86rf215_radio_rx_f_cut_0_5_half_fs
    at86rf215_radio_rx_bw_BW1000KHZ_IF1000KHZ = 0x8,    // at86rf215_radio_rx_f_cut_0_5_half_fs
    at86rf215_radio_rx_bw_BW1250KHZ_IF2000KHZ = 0x9,    // at86rf215_radio_rx_f_cut_0_75_half_fs
    at86rf215_radio_rx_bw_BW1600KHZ_IF2000KHZ = 0xA,    // at86rf215_radio_rx_f_cut_half_fs
    at86rf215_radio_rx_bw_BW2000KHZ_IF2000KHZ = 0xB,    // at86rf215_radio_rx_f_cut_half_fs
} at86rf215_radio_rx_bw_en;

typedef enum
{
    at86rf215_radio_rx_f_cut_0_25_half_fs = 0,      // whan 4MSPS => 500 KHz
    at86rf215_radio_rx_f_cut_0_375_half_fs = 1,     // whan 4MSPS => 750 KHz
    at86rf215_radio_rx_f_cut_0_5_half_fs = 2,       // whan 4MSPS => 1000 KHz
    at86rf215_radio_rx_f_cut_0_75_half_fs = 3,      // whan 4MSPS => 1500 KHz
    at86rf215_radio_rx_f_cut_half_fs = 4,           // whan 4MSPS => 2000 KHz
} at86rf215_radio_f_cut_en;

typedef enum
{
    at86rf215_radio_rx_sample_rate_4000khz = 0x1,
    at86rf215_radio_rx_sample_rate_2000khz = 0x2,
    at86rf215_radio_rx_sample_rate_1333khz = 0x3,
    at86rf215_radio_rx_sample_rate_1000khz = 0x4,
    at86rf215_radio_rx_sample_rate_800khz = 0x5,
    at86rf215_radio_rx_sample_rate_666khz = 0x6,
    at86rf215_radio_rx_sample_rate_500khz = 0x8,
    at86rf215_radio_rx_sample_rate_400khz = 0xA,
} at86rf215_radio_sample_rate_en;


typedef enum
{
    at86rf215_radio_agc_averaging_8 = 0,
    at86rf215_radio_agc_averaging_16 = 1,
    at86rf215_radio_agc_averaging_32 = 2,
    at86rf215_radio_agc_averaging_64 = 3,
} at86rf215_radio_agc_averaging_en;

typedef enum
{
    at86rf215_radio_agc_relative_atten_21_db = 0,
    at86rf215_radio_agc_relative_atten_24_db = 1,
    at86rf215_radio_agc_relative_atten_27_db = 2,
    at86rf215_radio_agc_relative_atten_30_db = 3,
    at86rf215_radio_agc_relative_atten_33_db = 4,
    at86rf215_radio_agc_relative_atten_36_db = 5,
    at86rf215_radio_agc_relative_atten_39_db = 6,
    at86rf215_radio_agc_relative_atten_41_db = 7,
} at86rf215_radio_agc_relative_atten_en;

typedef struct
{
    // commands
    int agc_measure_source_not_filtered;        // AGC Input (0 - filterred, 1 - unfiltered, faster operation)
    at86rf215_radio_agc_averaging_en avg;       // AGC Average Time in Number of Samples
    int reset_cmd;                              // AGC Reset - resets the AGC and sets the maximum receiver gain.
    int freeze_cmd;                             // AGC Freeze Control - A value of one forces the AGC to
                                                // freeze to its current value.
    int enable_cmd;                             // AGC Enable - a value of zero allows a manual setting of
                                                // the RX gain control by sub-register AGCS.GCW

    at86rf215_radio_agc_relative_atten_en att;  // AGC Target Level - sets the AGC target level relative to ADC full scale.
    int gain_control_word;                      // If AGCC_EN is set to 1, a read of bit AGCS.GCW indicates the current
                                                // receiver gain setting. If AGCC_EN is set to 0, a write access to GCW
                                                // manually sets the receiver gain. An integer value of 23 indicates
                                                // the maximum receiver gain; each integer step changes the gain by 3dB.

    // status
    int freeze_status;                          // AGC Freeze Status - A value of one indicates that the AGC is on hold.
} at86rf215_radio_agc_ctrl_st;


typedef struct
{
    int inverter_sign_if;                   // A value of one configures the receiver to implement the
                                            // inverted-sign IF frequency. Use default setting for normal operation.
    int shift_if_freq;                      // A value of one configures the receiver to shift the IF frequency
                                            // by factor of 1.25. This is useful to place the image frequency according
                                            // to channel scheme.
    at86rf215_radio_rx_bw_en bw;            // The sub-register controls the receiver filter bandwidth settings.
    at86rf215_radio_f_cut_en fcut;          // RX filter relative cut-off frequency
    at86rf215_radio_sample_rate_en fs;      // RX Sample Rate
} at86rf215_radio_set_rx_bw_samp_st;


typedef enum
{
    at86rf215_radio_energy_detection_mode_auto = 0,
    // Energy detection measurement is automatically triggered if the AGC is held by the internal
    // baseband or by setting bit FRZC.

    at86rf215_radio_energy_detection_mode_single = 1,
    // A single energy detection measurement is started.

    at86rf215_radio_energy_detection_mode_continous = 2,
    // A continuous energy detection measurements of configured interval defined in
    // register EDD is started.

    at86rf215_radio_energy_detection_mode_off = 3,
    // Energy detection measurement is disabled

} at86rf215_radio_energy_detection_mode_en;

typedef struct
{
    // cmd
    at86rf215_radio_energy_detection_mode_en mode;      // Energy Detection Mode
    float average_duration_us;                          // T[μs]=DF*DTB - the DTB will be calculated accordingly

    // status
    float energy_detection_value;                       // Receiver Energy Detection Value
} at86rf215_radio_energy_detection_st;

typedef enum
{
    at86rf215_radio_tx_pa_ramp_4usec = 0,
    at86rf215_radio_tx_pa_ramp_8usec = 1,
    at86rf215_radio_tx_pa_ramp_16usec = 2,
    at86rf215_radio_tx_pa_ramp_32usec = 3,
} at86rf215_radio_tx_pa_ramp_en;

typedef enum
{
    at86rf215_radio_tx_cut_off_80khz = 0x0,
    at86rf215_radio_tx_cut_off_100khz = 0x1,
    at86rf215_radio_tx_cut_off_125khz = 0x2,
    at86rf215_radio_tx_cut_off_160khz = 0x3,
    at86rf215_radio_tx_cut_off_200khz = 0x4,
    at86rf215_radio_tx_cut_off_250khz = 0x5,
    at86rf215_radio_tx_cut_off_315khz = 0x6,
    at86rf215_radio_tx_cut_off_400khz = 0x7,
    at86rf215_radio_tx_cut_off_500khz = 0x8,
    at86rf215_radio_tx_cut_off_625khz = 0x9,
    at86rf215_radio_tx_cut_off_800khz = 0xA,
    at86rf215_radio_tx_cut_off_1000khz = 0xB,
} at86rf215_radio_tx_cut_off_en;


typedef enum
{
    at86rf215_radio_pa_current_reduction_22ma = 0, // 3dB reduction of gain
    at86rf215_radio_pa_current_reduction_18ma = 1, // 2dB reduction of gain
    at86rf215_radio_pa_current_reduction_11ma = 2, // 1dB reduction of gain
    at86rf215_radio_pa_current_reduction_0ma = 3, // no reduction
} at86rf215_radio_pa_current_reduction_en;


typedef struct
{
    at86rf215_radio_tx_pa_ramp_en pa_ramping_time;
    at86rf215_radio_pa_current_reduction_en current_reduction;
    int tx_power;
    // Maximum output power is TXPWR=31, minimum output power is TXPWR=0.
    // The output power can be set by about 1dB step resolution.

    at86rf215_radio_tx_cut_off_en analog_bw;
    at86rf215_radio_f_cut_en digital_bw;
    at86rf215_radio_sample_rate_en fs;
    int direct_modulation;

} at86rf215_radio_tx_ctrl_st;


typedef enum
{
    at86rf215_radio_pll_loop_bw_default = 0,
    at86rf215_radio_pll_loop_bw_dec_15perc = 1,
    at86rf215_radio_pll_loop_bw_inc_15perc = 2,
} at86rf215_radio_pll_loop_bw_en;

typedef struct
{
    at86rf215_radio_pll_loop_bw_en loop_bw;
    // This sub-register controls the PLL loop bandwidth.The sub-register is applicable for the sub-1GHz transceiver only
    // (RF09). The TX modulation quality (i.e. FSK eye diagram) can be adjusted when direct modulation is used.

    int pll_center_freq;
    // Center frequency calibration is performed from state TRXOFF to state TXPREP and at channel change. The register
    // displays the center frequency value of the current channel.

    // statuses
    int pll_locked;
} at86rf215_radio_pll_ctrl_st;



typedef enum
{
    at86rf215_rf_channel_900mhz = 0,
    at86rf215_rf_channel_2400mhz = 1,
} at86rf215_rf_channel_en;

typedef struct
{
    uint8_t wake_up_por:1;
    uint8_t trx_ready:1;
    uint8_t energy_detection_complete:1;
    uint8_t battery_low:1;
    uint8_t trx_error:1;
    uint8_t IQ_if_sync_fail:1;
    uint8_t res :2;
} at86rf215_radio_irq_st;

typedef struct
{
    uint8_t frame_rx_started:1;
    uint8_t frame_rx_complete:1;
    uint8_t frame_rx_address_match:1;
    uint8_t frame_rx_match_extended:1;
    uint8_t frame_tx_complete:1;
    uint8_t agc_hold:1;
    uint8_t agc_release :1;
    uint8_t frame_buffer_level :1;
} at86rf215_baseband_irq_st;

typedef struct
{
    at86rf215_radio_irq_st radio09;
    at86rf215_radio_irq_st radio24;
    at86rf215_baseband_irq_st bb0;
    at86rf215_baseband_irq_st bb1;
} at86rf215_irq_st;


typedef enum
{
    at86rf215_iq_drive_current_1ma = 0,
    at86rf215_iq_drive_current_2ma = 1,
    at86rf215_iq_drive_current_3ma = 2,
    at86rf215_iq_drive_current_4ma = 3,
} at86rf215_iq_drive_current_en;

typedef enum
{
    at86rf215_iq_common_mode_v_150mv = 0,
    at86rf215_iq_common_mode_v_200mv = 1,
    at86rf215_iq_common_mode_v_250mv = 2,
    at86rf215_iq_common_mode_v_300mv = 3,
    at86rf215_iq_common_mode_v_ieee1596_1v2 = 4,
} at86rf215_iq_common_mode_v_en;

typedef enum
{
    at86rf215_baseband_mode = 0,
    at86rf215_iq_if_mode = 1,
} at86rf215_baseband_iq_mode_en;

typedef enum
{
    at86rf215_iq_clock_data_skew_1_906ns = 0,
    at86rf215_iq_clock_data_skew_2_906ns = 1,
    at86rf215_iq_clock_data_skew_3_906ns = 2,
    at86rf215_iq_clock_data_skew_4_906ns = 3,
} at86rf215_iq_clock_data_skew_en;
typedef enum
{
    at86rf215_drive_current_2ma = 0,
    at86rf215_drive_current_4ma = 1,
    at86rf215_drive_current_6ma = 2,
    at86rf215_drive_current_8ma = 3,
} at86rf215_drive_current_en;

typedef struct
{
    int low_ch_i;
    int low_ch_q;
    int hi_ch_i;
    int hi_ch_q;
} at86rf215_cal_results_st;


typedef struct
{
    uint8_t loopback_enable;
    at86rf215_iq_drive_current_en drv_strength;
    at86rf215_iq_common_mode_v_en common_mode_voltage;
    uint8_t tx_control_with_iq_if;
    at86rf215_baseband_iq_mode_en radio09_mode;
    at86rf215_baseband_iq_mode_en radio24_mode;
    at86rf215_iq_clock_data_skew_en clock_skew;

    // status
    uint8_t synchronization_failed;
    uint8_t in_failsafe_mode;
    uint8_t synchronized_incoming_iq;
} at86rf215_iq_interface_config_st;




void at86rf215_radio_set_state(at86rf215_rf_channel_en ch, at86rf215_radio_state_cmd_en cmd);

double at86rf215_radio_get_frequency( /*IN*/ at86rf215_radio_channel_mode_en mode,
                                     /*IN*/ int channel_spacing_25khz_res,
                                     /*IN*/ double wanted_frequency_hz,
                                     /*OUT*/ int *center_freq_25khz_res,
                                     /*OUT*/ int *channel_number);

void at86rf215_radio_setup_channel(at86rf215_rf_channel_en ch,
                                        int channel_spacing_25khz_res,
                                        int center_freq_25khz_res,
                                        int channel_number,
                                        at86rf215_radio_channel_mode_en mode);

void at86rf215_radio_set_rx_bandwidth_sampling(at86rf215_rf_channel_en ch,
                                                at86rf215_radio_set_rx_bw_samp_st* cfg);

void at86rf215_radio_get_rx_bandwidth_sampling(at86rf215_rf_channel_en ch,
                                                at86rf215_radio_set_rx_bw_samp_st* cfg);

void at86rf215_radio_setup_agc(at86rf215_rf_channel_en ch,
                                                    at86rf215_radio_agc_ctrl_st *agc_ctrl);

void at86rf215_radio_get_agc(at86rf215_rf_channel_en ch,
                                                at86rf215_radio_agc_ctrl_st *agc_ctrl);

float at86rf215_radio_get_rssi_dbm(at86rf215_rf_channel_en ch);

void at86rf215_radio_setup_energy_detection(at86rf215_rf_channel_en ch,
                                            at86rf215_radio_energy_detection_st* ed);

void at86rf215_radio_get_energy_detection(at86rf215_rf_channel_en ch,
                                        at86rf215_radio_energy_detection_st* ed);


void at86rf215_radio_setup_tx_ctrl(at86rf215_rf_channel_en ch,
                                                at86rf215_radio_tx_ctrl_st* cfg);

void at86rf215_radio_get_tx_ctrl(at86rf215_rf_channel_en ch,
                                at86rf215_radio_tx_ctrl_st* cfg);

void at86rf215_radio_setup_pll_ctrl(at86rf215_rf_channel_en ch,
                                        at86rf215_radio_pll_ctrl_st* cfg);

void at86rf215_radio_get_pll_ctrl(at86rf215_rf_channel_en ch,
                                        at86rf215_radio_pll_ctrl_st* cfg);

void at86rf215_radio_set_tx_iq_calibration(at86rf215_rf_channel_en ch,
                                                int cal_i, int cal_q);

void at86rf215_radio_get_tx_iq_calibration(at86rf215_rf_channel_en ch,
                                                int *cal_i, int *cal_q);

void at86rf215_radio_set_tx_dac_input_iq(at86rf215_rf_channel_en ch,
                                                int enable_dac_i_dc, int dac_i_val,
                                                int enable_dac_q_dc, int dac_q_val);

void at86rf215_radio_get_tx_dac_input_iq(at86rf215_rf_channel_en ch,
                                                int *enable_dac_i_dc, int *dac_i_val,
                                                int *enable_dac_q_dc, int *dac_q_val);
int at86rf215_radio_get_good_channel(double wanted_frequency_hz, at86rf215_radio_channel_mode_en *mode,
                                                                at86rf215_rf_channel_en *ch);

void at86rf215_setup_iq_if(at86rf215_iq_interface_config_st* cfg);

void at86rf215_setup_rf_irq(uint8_t active_low,
                                                uint8_t show_masked_irq,
                                                at86rf215_drive_current_en drive);

at86rf215_radio_state_cmd_en at86rf215_radio_get_state(at86rf215_rf_channel_en ch);





/*********** Lowest level code *********/
bool at86rf215_write_byte(uint16_t address, uint8_t val);
uint8_t at86rf215_read_byte(uint16_t address);
bool at86rf215_write_buffer(uint16_t startaddr, uint8_t * vals, int count);
bool at86rf215_read_buffer(uint16_t startaddr, uint8_t * vals, int count);


/************ Top level init and CLI ***********/

void init_at86_cli(SPI_HandleTypeDef * spi);




#endif