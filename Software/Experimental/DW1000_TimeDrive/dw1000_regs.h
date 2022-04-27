/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file dw1000_regs.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief Register file
 *
 * @details This is the reg base class which includes all the dw1000 register's data.
 */

#ifndef _DW1000_REGS_H_
#define _DW1000_REGS_H_

#ifdef __cplusplus
extern "C" {
#endif


/****************************************************************************//**
 * @brief Bit definitions for register DEV_ID
**/
#define DEV_ID_ID               0x00            /* Device ID register, includes revision info (0xDECA0130) */
#define DEV_ID_LEN              (4)
/* mask and shift */
#define DEV_ID_REV_MASK         0x0000000FUL    /* Revision */
#define DEV_ID_VER_MASK         0x000000F0UL    /* Version */
#define DEV_ID_MODEL_MASK       0x0000FF00UL    /* The MODEL identifies the device. The DW1000 is device type 0x01 */
#define DEV_ID_RIDTAG_MASK      0xFFFF0000UL    /* Register Identification Tag 0XDECA */

/****************************************************************************//**
 * @brief Bit definitions for register EUI_64
**/
#define EUI_64_ID               0x01            /* IEEE Extended Unique Identifier (63:0) */
#define EUI_64_OFFSET           0x00
#define EUI_64_LEN              (8)

/****************************************************************************//**
 * @brief Bit definitions for register PANADR
**/
#define PANADR_ID               0x03            /* PAN ID (31:16) and Short Address (15:0) */
#define PANADR_LEN              (4)
/*mask and shift */
#define PANADR_SHORT_ADDR_OFFSET 0              /* In bytes */
#define PANADR_SHORT_ADDR_MASK  0x0000FFFFUL    /* Short Address */
#define PANADR_PAN_ID_OFFSET     2              /* In bytes */
#define PANADR_PAN_ID_MASK      0xFFFF00F0UL    /* PAN Identifier */

/****************************************************************************//**
 * @brief Bit definitions for register 0x05
**/
#define REG_05_ID_RESERVED      0x05

/****************************************************************************//**
 * @brief Bit definitions for register SYS_CFG
**/
#define SYS_CFG_ID              0x04            /* System Configuration (31:0) */
#define SYS_CFG_LEN             (4)
/*mask and shift */
#define SYS_CFG_MASK            0xF047FFFFUL    /* access mask to SYS_CFG_ID */
#define SYS_CFG_FF_ALL_EN       0x000001FEUL    /* Frame filtering options all frames allowed */
/*offset 0 */
#define SYS_CFG_FFE             0x00000001UL    /* Frame Filtering Enable. This bit enables the frame filtering functionality */
#define SYS_CFG_FFBC            0x00000002UL    /* Frame Filtering Behave as a Co-ordinator */
#define SYS_CFG_FFAB            0x00000004UL    /* Frame Filtering Allow Beacon frame reception */
#define SYS_CFG_FFAD            0x00000008UL    /* Frame Filtering Allow Data frame reception */
#define SYS_CFG_FFAA            0x00000010UL    /* Frame Filtering Allow Acknowledgment frame reception */
#define SYS_CFG_FFAM            0x00000020UL    /* Frame Filtering Allow MAC command frame reception */
#define SYS_CFG_FFAR            0x00000040UL    /* Frame Filtering Allow Reserved frame types */
#define SYS_CFG_FFA4            0x00000080UL    /* Frame Filtering Allow frames with frame type field of 4, (binary 100) */
/*offset 8 */
#define SYS_CFG_FFA5            0x00000100UL    /* Frame Filtering Allow frames with frame type field of 5, (binary 101) */
#define SYS_CFG_HIRQ_POL        0x00000200UL    /* Host interrupt polarity */
#define SYS_CFG_SPI_EDGE        0x00000400UL    /* SPI data launch edge */
#define SYS_CFG_DIS_FCE         0x00000800UL    /* Disable frame check error handling */
#define SYS_CFG_DIS_DRXB        0x00001000UL    /* Disable Double RX Buffer */
#define SYS_CFG_DIS_PHE         0x00002000UL    /* Disable receiver abort on PHR error */
#define SYS_CFG_DIS_RSDE        0x00004000UL    /* Disable Receiver Abort on RSD error */
#define SYS_CFG_FCS_INIT2F      0x00008000UL    /* initial seed value for the FCS generation and checking function */
/*offset 16 */
#define SYS_CFG_PHR_MODE_SHFT   16
#define SYS_CFG_PHR_MODE_00     0x00000000UL    /* Standard Frame mode */
#define SYS_CFG_PHR_MODE_11     0x00030000UL    /* Long Frames mode */
#define SYS_CFG_DIS_STXP        0x00040000UL    /* Disable Smart TX Power control */
#define SYS_CFG_RXM110K         0x00400000UL    /* Receiver Mode 110 kbps data rate */
/*offset 24 */
#define SYS_CFG_RXWTOE          0x10000000UL    /* Receive Wait Timeout Enable. */
#define SYS_CFG_RXAUTR          0x20000000UL    /* Receiver Auto-Re-enable. This bit is used to cause the receiver to re-enable automatically */
#define SYS_CFG_AUTOACK         0x40000000UL    /* Automatic Acknowledgement Enable */
#define SYS_CFG_AACKPEND        0x80000000UL    /* Automatic Acknowledgement Pending bit control */


/****************************************************************************//**
 * @brief Bit definitions for register SYS_TIME
**/
#define SYS_TIME_ID             0x06            /* System Time Counter (40-bit) */
#define SYS_TIME_OFFSET         0x00
#define SYS_TIME_LEN            (5)             /* Note 40 bit register */


/****************************************************************************//**
 * @brief Bit definitions for register  0x07
**/
#define REG_07_ID_RESERVED      0x07

/****************************************************************************//**
 * @brief Bit definitions for register TX_FCTRL
**/
#define TX_FCTRL_ID             0x08            /* Transmit Frame Control */
#define TX_FCTRL_LEN            (5)             /* Note 40 bit register */
/*masks (low 32 bit) */
#define TX_FCTRL_TFLEN_MASK     0x0000007FUL    /* bit mask to access Transmit Frame Length */
#define TX_FCTRL_TFLE_MASK      0x00000380UL    /* bit mask to access Transmit Frame Length Extension */
#define TX_FCTRL_FLE_MASK       0x000003FFUL    /* bit mask to access Frame Length field */
#define TX_FCTRL_TXBR_MASK      0x00006000UL    /* bit mask to access Transmit Bit Rate */
#define TX_FCTRL_TXPRF_MASK     0x00030000UL    /* bit mask to access Transmit Pulse Repetition Frequency */
#define TX_FCTRL_TXPSR_MASK     0x000C0000UL    /* bit mask to access Transmit Preamble Symbol Repetitions (PSR). */
#define TX_FCTRL_PE_MASK        0x00300000UL    /* bit mask to access Preamble Extension */
#define TX_FCTRL_TXPSR_PE_MASK  0x003C0000UL    /* bit mask to access Transmit Preamble Symbol Repetitions (PSR). */
#define TX_FCTRL_SAFE_MASK_32   0xFFFFE3FFUL    /* FSCTRL has fields which should always be writen zero */
/*offset 0 */
/*offset 8 */
#define TX_FCTRL_TXBR_110k      0x00000000UL    /* Transmit Bit Rate = 110k */
#define TX_FCTRL_TXBR_850k      0x00002000UL    /* Transmit Bit Rate = 850k */
#define TX_FCTRL_TXBR_6M        0x00004000UL    /* Transmit Bit Rate = 6.8M */
#define TX_FCTRL_TXBR_SHFT      (13)            /* shift to access Data Rate field */
#define TX_FCTRL_TR             0x00008000UL    /* Transmit Ranging enable */
#define TX_FCTRL_TR_SHFT        (15)            /* shift to access Ranging bit */
/*offset 16 */
#define TX_FCTRL_TXPRF_SHFT     (16)            /* shift to access Pulse Repetition Frequency field */
#define TX_FCTRL_TXPRF_4M       0x00000000UL    /* Transmit Pulse Repetition Frequency = 4 Mhz */
#define TX_FCTRL_TXPRF_16M      0x00010000UL    /* Transmit Pulse Repetition Frequency = 16 Mhz */
#define TX_FCTRL_TXPRF_64M      0x00020000UL    /* Transmit Pulse Repetition Frequency = 64 Mhz */
#define TX_FCTRL_TXPSR_SHFT     (18)            /* shift to access Preamble Symbol Repetitions field */
#define TX_FCTRL_PE_SHFT        (20)            /* shift to access Preamble length Extension to allow specification of non-standard values */
#define TX_FCTRL_TXPSR_PE_16    0x00000000UL    /* bit mask to access Preamble Extension = 16 */
#define TX_FCTRL_TXPSR_PE_64    0x00040000UL    /* bit mask to access Preamble Extension = 64 */
#define TX_FCTRL_TXPSR_PE_128   0x00140000UL    /* bit mask to access Preamble Extension = 128 */
#define TX_FCTRL_TXPSR_PE_256   0x00240000UL    /* bit mask to access Preamble Extension = 256 */
#define TX_FCTRL_TXPSR_PE_512   0x00340000UL    /* bit mask to access Preamble Extension = 512 */
#define TX_FCTRL_TXPSR_PE_1024  0x00080000UL    /* bit mask to access Preamble Extension = 1024 */
#define TX_FCTRL_TXPSR_PE_1536  0x00180000UL    /* bit mask to access Preamble Extension = 1536 */
#define TX_FCTRL_TXPSR_PE_2048  0x00280000UL    /* bit mask to access Preamble Extension = 2048 */
#define TX_FCTRL_TXPSR_PE_4096  0x000C0000UL    /* bit mask to access Preamble Extension = 4096 */
/*offset 22 */
#define TX_FCTRL_TXBOFFS_SHFT   (22)            /* Shift to access transmit buffer index offset */
#define TX_FCTRL_TXBOFFS_MASK   0xFFC00000UL    /* bit mask to access Transmit buffer index offset 10-bit field */
/*offset 32 */
#define TX_FCTRL_IFSDELAY_MASK  0xFF00000000ULL /* bit mask to access Inter-Frame Spacing field */

/****************************************************************************//**
 * @brief Bit definitions for register TX_BUFFER
**/
#define TX_BUFFER_ID            0x09            /* Transmit Data Buffer */
#define TX_BUFFER_LEN           (1024)

/****************************************************************************//**
 * @brief Bit definitions for register  DX_TIME
**/
#define DX_TIME_ID              0x0A            /* Delayed Send or Receive Time (40-bit) */
#define DX_TIME_LEN             (5)

/****************************************************************************//**
 * @brief Bit definitions for register 0x08
**/
#define REG_0B_ID_RESERVED      0x0B

/****************************************************************************//**
 * @brief Bit definitions for register RX_FWTO
**/
#define RX_FWTO_ID              0x0C            /* Receive Frame Wait Timeout Period */
#define RX_FWTO_OFFSET          0x00
#define RX_FWTO_LEN             (2)             /* doc bug*/
/*mask and shift */
#define RX_FWTO_MASK            0xFFFF

/****************************************************************************//**
 * @brief Bit definitions for register SYS_CTRL
**/
#define SYS_CTRL_ID             0x0D            /* System Control Register */
#define SYS_CTRL_OFFSET         0x00
#define SYS_CTRL_LEN            (4)
/*masks */
#define SYS_CTRL_MASK_32        0x010003CFUL    /* System Control Register access mask (all unused fields should always be writen as zero) */
/*offset 0 */
#define SYS_CTRL_SFCST          0x00000001UL    /* Suppress Auto-FCS Transmission (on this frame) */
#define SYS_CTRL_TXSTRT         0x00000002UL    /* Start Transmitting Now */
#define SYS_CTRL_TXDLYS         0x00000004UL    /* Transmitter Delayed Sending (initiates sending when SYS_TIME == TXD_TIME */
#define SYS_CTRL_CANSFCS        0x00000008UL    /* Cancel Suppression of auto-FCS transmission (on the current frame) */
#define SYS_CTRL_TRXOFF         0x00000040UL    /* Transceiver Off. Force Transciever OFF abort TX or RX immediately */
#define SYS_CTRL_WAIT4RESP      0x00000080UL    /* Wait for Response */
/*offset 8 */
#define SYS_CTRL_RXENAB         0x00000100UL    /* Enable Receiver Now */
#define SYS_CTRL_RXDLYE         0x00000200UL    /* Receiver Delayed Enable (Enables Receiver when SY_TIME[0x??] == RXD_TIME[0x??] CHECK comment*/
/*offset 16 */
/*offset 24 */
#define SYS_CTRL_HSRBTOGGLE     0x01000000UL    /* Host side receiver buffer pointer toggle - toggles 0/1 host side data set pointer */
#define SYS_CTRL_HRBT           (SYS_CTRL_HSRBTOGGLE)
#define SYS_CTRL_HRBT_OFFSET    (3)

/****************************************************************************//**
 * @brief Bit definitions for register  SYS_MASK
**/
#define SYS_MASK_ID             0x0E            /* System Event Mask Register */
#define SYS_MASK_LEN            (4)
/*masks */
#define SYS_MASK_MASK_32        0x3FF7FFFEUL    /* System Event Mask Register access mask (all unused fields should always be writen as zero) */
/*offset 0 */
#define SYS_MASK_MCPLOCK        0x00000002UL    /* Mask clock PLL lock event    */
#define SYS_MASK_MESYNCR        0x00000004UL    /* Mask clock PLL lock event    */
#define SYS_MASK_MAAT           0x00000008UL    /* Mask automatic acknowledge trigger event */
#define SYS_MASK_MTXFRB         0x00000010UL    /* Mask transmit frame begins event */
#define SYS_MASK_MTXPRS         0x00000020UL    /* Mask transmit preamble sent event    */
#define SYS_MASK_MTXPHS         0x00000040UL    /* Mask transmit PHY Header Sent event  */
#define SYS_MASK_MTXFRS         0x00000080UL    /* Mask transmit frame sent event   */
/*offset 8 */
#define SYS_MASK_MRXPRD         0x00000100UL    /* Mask receiver preamble detected event    */
#define SYS_MASK_MRXSFDD        0x00000200UL    /* Mask receiver SFD detected event */
#define SYS_MASK_MLDEDONE       0x00000400UL    /* Mask LDE processing done event   */
#define SYS_MASK_MRXPHD         0x00000800UL    /* Mask receiver PHY header detect event    */
#define SYS_MASK_MRXPHE         0x00001000UL    /* Mask receiver PHY header error event */
#define SYS_MASK_MRXDFR         0x00002000UL    /* Mask receiver data frame ready event */
#define SYS_MASK_MRXFCG         0x00004000UL    /* Mask receiver FCS good event */
#define SYS_MASK_MRXFCE         0x00008000UL    /* Mask receiver FCS error event    */
/*offset 16 */
#define SYS_MASK_MRXRFSL        0x00010000UL    /* Mask receiver Reed Solomon Frame Sync Loss event */
#define SYS_MASK_MRXRFTO        0x00020000UL    /* Mask Receive Frame Wait Timeout event    */
#define SYS_MASK_MLDEERR        0x00040000UL    /* Mask leading edge detection processing error event   */
#define SYS_MASK_MRXOVRR        0x00100000UL    /* Mask Receiver Overrun event  */
#define SYS_MASK_MRXPTO         0x00200000UL    /* Mask Preamble detection timeout event    */
#define SYS_MASK_MGPIOIRQ       0x00400000UL    /* Mask GPIO interrupt event    */
#define SYS_MASK_MSLP2INIT      0x00800000UL    /* Mask SLEEP to INIT event */
/*offset 24*/
#define SYS_MASK_MRFPLLLL       0x01000000UL    /* Mask RF PLL Loosing Lock warning event   */
#define SYS_MASK_MCPLLLL        0x02000000UL    /* Mask Clock PLL Loosing Lock warning event    */
#define SYS_MASK_MRXSFDTO       0x04000000UL    /* Mask Receive SFD timeout event   */
#define SYS_MASK_MHPDWARN       0x08000000UL    /* Mask Half Period Delay Warning event */
#define SYS_MASK_MTXBERR        0x10000000UL    /* Mask Transmit Buffer Error event */
#define SYS_MASK_MAFFREJ        0x20000000UL    /* Mask Automatic Frame Filtering rejection event   */

/****************************************************************************//**
 * @brief Bit definitions for register SYS_STATUS
**/
#define SYS_STATUS_ID           0x0F            /* System event Status Register */
#define SYS_STATUS_OFFSET       0x00
#define SYS_STATUS_LEN          (5)             /* Note 40 bit register */
/*masks */
#define SYS_STATUS_MASK_32      0xFFF7FFFFUL    /* System event Status Register access mask (all unused fields should always be writen as zero) */
/*offset 0 */
#define SYS_STATUS_IRQS         0x00000001UL    /* Interrupt Request Status READ ONLY */
#define SYS_STATUS_CPLOCK       0x00000002UL    /* Clock PLL Lock */
#define SYS_STATUS_ESYNCR       0x00000004UL    /* External Sync Clock Reset */
#define SYS_STATUS_AAT          0x00000008UL    /* Automatic Acknowledge Trigger */
#define SYS_STATUS_TXFRB        0x00000010UL    /* Transmit Frame Begins */
#define SYS_STATUS_TXPRS        0x00000020UL    /* Transmit Preamble Sent */
#define SYS_STATUS_TXPHS        0x00000040UL    /* Transmit PHY Header Sent */
#define SYS_STATUS_TXFRS        0x00000080UL    /* Transmit Frame Sent: This is set when the transmitter has completed the sending of a frame */
/*offset 8 */
#define SYS_STATUS_RXPRD        0x00000100UL    /* Receiver Preamble Detected status */
#define SYS_STATUS_RXSFDD       0x00000200UL    /* Receiver Start Frame Delimiter Detected. */
#define SYS_STATUS_LDEDONE      0x00000400UL    /* LDE processing done */
#define SYS_STATUS_RXPHD        0x00000800UL    /* Receiver PHY Header Detect */
#define SYS_STATUS_RXPHE        0x00001000UL    /* Receiver PHY Header Error */
#define SYS_STATUS_RXDFR        0x00002000UL    /* Receiver Data Frame Ready */
#define SYS_STATUS_RXFCG        0x00004000UL    /* Receiver FCS Good */
#define SYS_STATUS_RXFCE        0x00008000UL    /* Receiver FCS Error */
/*offset 16 */
#define SYS_STATUS_RXRFSL       0x00010000UL    /* Receiver Reed Solomon Frame Sync Loss */
#define SYS_STATUS_RXRFTO       0x00020000UL    /* Receive Frame Wait Timeout */
#define SYS_STATUS_LDEERR       0x00040000UL    /* Leading edge detection processing error */
#define SYS_STATUS_reserved     0x00080000UL    /* bit19 reserved */
#define SYS_STATUS_RXOVRR       0x00100000UL    /* Receiver Overrun */
#define SYS_STATUS_RXPTO        0x00200000UL    /* Preamble detection timeout */
#define SYS_STATUS_GPIOIRQ      0x00400000UL    /* GPIO interrupt */
#define SYS_STATUS_SLP2INIT     0x00800000UL    /* SLEEP to INIT */
/*offset 24 */
#define SYS_STATUS_RFPLL_LL     0x01000000UL    /* RF PLL Losing Lock */
#define SYS_STATUS_CLKPLL_LL    0x02000000UL    /* Clock PLL Losing Lock */
#define SYS_STATUS_RXSFDTO      0x04000000UL    /* Receive SFD timeout */
#define SYS_STATUS_HPDWARN      0x08000000UL    /* Half Period Delay Warning */
#define SYS_STATUS_TXBERR       0x10000000UL    /* Transmit Buffer Error */
#define SYS_STATUS_AFFREJ       0x20000000UL    /* Automatic Frame Filtering rejection */
#define SYS_STATUS_HSRBP        0x40000000UL    /* Host Side Receive Buffer Pointer */
#define SYS_STATUS_ICRBP        0x80000000UL    /* IC side Receive Buffer Pointer READ ONLY */
/*offset 32 */
#define SYS_STATUS_RXRSCS       0x0100000000ULL /* Receiver Reed-Solomon Correction Status */
#define SYS_STATUS_RXPREJ       0x0200000000ULL /* Receiver Preamble Rejection */
#define SYS_STATUS_TXPUTE       0x0400000000ULL /* Transmit power up time error */

#define SYS_STATUS_TXERR        (0x0408)        /* These bits are the 16 high bits of status register TXPUTE and HPDWARN flags */

/* All RX events after a correct packet reception mask. */
#define SYS_STATUS_ALL_RX_GOOD (SYS_STATUS_RXDFR | SYS_STATUS_RXFCG | SYS_STATUS_RXPRD | \
                                SYS_STATUS_RXSFDD | SYS_STATUS_RXPHD)

                                /* All TX events mask. */
#define SYS_STATUS_ALL_TX      (SYS_STATUS_AAT | SYS_STATUS_TXFRB | SYS_STATUS_TXPRS | \
                                SYS_STATUS_TXPHS | SYS_STATUS_TXFRS)

/* All double buffer events mask. */
#define SYS_STATUS_ALL_DBLBUFF (SYS_STATUS_RXDFR | SYS_STATUS_RXFCG)

/* All RX errors mask. */
#define SYS_STATUS_ALL_RX_ERR  (SYS_STATUS_RXPHE | SYS_STATUS_RXFCE | SYS_STATUS_RXRFSL | SYS_STATUS_RXOVRR |SYS_STATUS_RXSFDTO \
                                | SYS_STATUS_AFFREJ )
#define SYS_MASK_ALL_RX_ERR  (SYS_MASK_MRXPHE | SYS_MASK_MRXFCE | SYS_MASK_MRXRFSL | SYS_STATUS_RXOVRR | SYS_MASK_MRXSFDTO \
                                | SYS_MASK_MAFFREJ )

/* User defined RX timeouts (frame wait timeout and preamble detect timeout) mask. */
#define SYS_STATUS_ALL_RX_TO    (SYS_STATUS_RXRFTO | SYS_STATUS_RXPTO)
#define SYS_MASK_ALL_RX_TO      (SYS_MASK_MRXRFTO | SYS_MASK_MRXPTO)


/****************************************************************************//**
 * @brief Bit definitions for register RX_FINFO
**/
#define RX_FINFO_ID             0x10            /* RX Frame Information (in double buffer set) */
#define RX_FINFO_OFFSET         0x00
#define RX_FINFO_LEN            (4)
/*mask and shift */
#define RX_FINFO_MASK_32        0xFFFFFBFFUL    /* System event Status Register access mask (all unused fields should always be writen as zero) */
#define RX_FINFO_RXFLEN_MASK    0x0000007FUL    /* Receive Frame Length (0 to 127) */
#define RX_FINFO_RXFLE_MASK     0x00000380UL    /* Receive Frame Length Extension (0 to 7)<<7 */
#define RX_FINFO_RXFL_MASK_1023 0x000003FFUL    /* Receive Frame Length Extension (0 to 1023) */

#define RX_FINFO_RXNSPL_MASK    0x00001800UL    /* Receive Non-Standard Preamble Length */
#define RX_FINFO_RXPSR_MASK     0x000C0000UL    /* RX Preamble Repetition. 00 = 16 symbols, 01 = 64 symbols, 10 = 1024 symbols, 11 = 4096 symbols */

#define RX_FINFO_RXPEL_MASK     0x000C1800UL    /* Receive Preamble Length = RXPSR+RXNSPL */
#define RX_FINFO_RXPEL_64       0x00040000UL    /* Receive Preamble length = 64 */
#define RX_FINFO_RXPEL_128      0x00040800UL    /* Receive Preamble length = 128 */
#define RX_FINFO_RXPEL_256      0x00041000UL    /* Receive Preamble length = 256 */
#define RX_FINFO_RXPEL_512      0x00041800UL    /* Receive Preamble length = 512 */
#define RX_FINFO_RXPEL_1024     0x00080000UL    /* Receive Preamble length = 1024 */
#define RX_FINFO_RXPEL_1536     0x00080800UL    /* Receive Preamble length = 1536 */
#define RX_FINFO_RXPEL_2048     0x00081000UL    /* Receive Preamble length = 2048 */
#define RX_FINFO_RXPEL_4096     0x000C0000UL    /* Receive Preamble length = 4096 */

#define RX_FINFO_RXBR_MASK      0x00006000UL    /* Receive Bit Rate report. This field reports the received bit rate */
#define RX_FINFO_RXBR_110k      0x00000000UL    /* Received bit rate = 110 kbps */
#define RX_FINFO_RXBR_850k      0x00002000UL    /* Received bit rate = 850 kbps */
#define RX_FINFO_RXBR_6M        0x00004000UL    /* Received bit rate = 6.8 Mbps */
#define RX_FINFO_RXBR_SHIFT     (13)

#define RX_FINFO_RNG            0x00008000UL    /* Receiver Ranging. Ranging bit in the received PHY header identifying the frame as a ranging packet. */
#define RX_FINFO_RNG_SHIFT      (15)

#define RX_FINFO_RXPRF_MASK     0x00030000UL    /* RX Pulse Repetition Rate report */
#define RX_FINFO_RXPRF_16M      0x00010000UL    /* PRF being employed in the receiver = 16M */
#define RX_FINFO_RXPRF_64M      0x00020000UL    /* PRF being employed in the receiver = 64M */
#define RX_FINFO_RXPRF_SHIFT    (16)

#define RX_FINFO_RXPACC_MASK    0xFFF00000UL    /* Preamble Accumulation Count */
#define RX_FINFO_RXPACC_SHIFT   (20)


/****************************************************************************//**
 * @brief Bit definitions for register RX_BUFFER
**/
#define RX_BUFFER_ID            0x11            /* Receive Data Buffer (in double buffer set) */
#define RX_BUFFER_LEN           (1024)


/****************************************************************************//**
 * @brief Bit definitions for register RX_FQUAL
**/
#define RX_FQUAL_ID             0x12            /* Rx Frame Quality information (in double buffer set) */
#define RX_FQUAL_LEN            (8)             /* note 64 bit register*/
/*mask and shift */
/*offset 0 */
#define RX_EQUAL_STD_NOISE_MASK 0x0000FFFFULL   /* Standard Deviation of Noise */
#define RX_EQUAL_STD_NOISE_SHIFT (0)
#define STD_NOISE_MASK          RX_EQUAL_STD_NOISE_MASK
#define STD_NOISE_SHIFT         RX_EQUAL_STD_NOISE_SHIFT
/*offset 16 */
#define RX_EQUAL_FP_AMPL2_MASK  0xFFFF0000ULL   /* First Path Amplitude point 2 */
#define RX_EQUAL_FP_AMPL2_SHIFT (16)
#define FP_AMPL2_MASK           RX_EQUAL_FP_AMPL2_MASK
#define FP_AMPL2_SHIFT          RX_EQUAL_FP_AMPL2_SHIFT
/*offset 32*/
#define RX_EQUAL_PP_AMPL3_MASK  0x0000FFFF00000000ULL   /* First Path Amplitude point 3 */
#define RX_EQUAL_PP_AMPL3_SHIFT (32)
#define PP_AMPL3_MASK           RX_EQUAL_PP_AMPL3_MASK
#define PP_AMPL3_SHIFT          RX_EQUAL_PP_AMPL3_SHIFT
/*offset 48*/
#define RX_EQUAL_CIR_MXG_MASK   0xFFFF000000000000ULL   /* Channel Impulse Response Max Growth */
#define RX_EQUAL_CIR_MXG_SHIFT  (48)
#define CIR_MXG_MASK            RX_EQUAL_CIR_MXG_MASK
#define CIR_MXG_SHIFT           RX_EQUAL_CIR_MXG_SHIFT



/****************************************************************************//**
 * @brief Bit definitions for register RX_TTCKI
 *      The value here is the interval over which the timing offset reported
 *      in the RXTOFS field of Register file: 0x14 � RX_TTCKO is measured.
 *      The clock offset is calculated by dividing RXTTCKI by RXTOFS.
 *      The value in RXTTCKI will take just one of two values depending on the PRF: 0x01F00000 @ 16 MHz PRF,
 *      and 0x01FC0000 @ 64 MHz PRF.
**/
#define RX_TTCKI_ID             0x13            /* Receiver Time Tracking Interval (in double buffer set) */
#define RX_TTCKI_LEN            (4)

/****************************************************************************//**
 * @brief Bit definitions for register RX_TTCKO
**/
#define RX_TTCKO_ID             0x14            /* Receiver Time Tracking Offset (in double buffer set) */
#define RX_TTCKO_LEN            (5)             /* Note 40 bit register */
/*mask and shift */
#define RX_TTCKO_MASK_32        0xFF07FFFFUL    /* Receiver Time Tracking Offset access mask (all unused fields should always be writen as zero) */
/*offset 0 */
#define RX_TTCKO_RXTOFS_MASK    0x0007FFFFUL    /* RX time tracking offset. This RXTOFS value is a 19-bit signed quantity*/
/*offset 24 */
#define RX_TTCKO_RSMPDEL_MASK   0xFF000000UL    /* This 8-bit field reports an internal re-sampler delay value */
/*offset 32 */
#define RX_TTCKO_RCPHASE_MASK   0x7F0000000000ULL   /* This 7-bit field reports the receive carrier phase adjustment at time the ranging timestamp is made. */


/****************************************************************************//**
 * @brief Bit definitions for register RX_TIME
**/
#define RX_TIME_ID              0x15            /* Receive Message Time of Arrival (in double buffer set) */
#define RX_TIME_LLEN            (14)
#define RX_TIME_RX_STAMP_LEN    (5)             /* read only 5 bytes (the adjusted timestamp (40:0)) */
#define RX_STAMP_LEN            RX_TIME_RX_STAMP_LEN
/*mask and shift */
#define RX_TIME_RX_STAMP_OFFSET  (0) /* byte 0..4 40 bit Reports the fully adjusted time of reception. */
#define RX_TIME_FP_INDEX_OFFSET  (5)    /* byte 5..6 16 bit First path index. */
#define RX_TIME_FP_AMPL1_OFFSET  (7)    /* byte 7..8 16 bit First Path Amplitude point 1 */   /* doc bug */
#define RX_TIME_FP_RAWST_OFFSET  (9)    /* byte 9..13 40 bit Raw Timestamp for the frame */


/****************************************************************************//**
 * @brief Bit definitions for register
**/
#define REG_16_ID_RESERVED      0x16


/****************************************************************************//**
 * @brief Bit definitions for register
**/
#define TX_TIME_ID              0x17            /* Transmit Message Time of Sending */
#define TX_TIME_LLEN            (10)
#define TX_TIME_TX_STAMP_LEN    (5)             /* 40-bits = 5 bytes */
#define TX_STAMP_LEN            TX_TIME_TX_STAMP_LEN
/*mask and shift */
#define TX_TIME_TX_STAMP_OFFSET  (0) /* byte 0..4 40 bit Reports the fully adjusted time of transmission */
#define TX_TIME_TX_RAWST_OFFSET  (5) /* byte 5..9 40 bit Raw Timestamp for the frame */




/****************************************************************************//**
 * @brief Bit definitions for register TX_ANTD
**/
#define TX_ANTD_ID              0x18            /* 16-bit Delay from Transmit to Antenna */
#define TX_ANTD_OFFSET          0x00
#define TX_ANTD_LEN             (2)




/****************************************************************************//**
 * @brief Bit definitions for register SYS_STATES
 *        Register map register file 0x19 is reserved
 *
**/
#define SYS_STATE_ID            0x19            /* System State information READ ONLY */
#define SYS_STATE_LEN           (5)

#define TX_STATE_OFFSET         0x00            /* 7:0 TX _STATE Bits 3:0 */
#define TX_STATE_MASK           0x07
#define TX_STATE_IDLE           0x00
#define TX_STATE_PREAMBLE       0x01
#define TX_STATE_SFD            0x02
#define TX_STATE_PHR            0x03
#define TX_STATE_SDE            0x04
#define TX_STATE_DATA           0x05
#define TX_STATE_RSP_DATE       0x06
#define TX_STATE_TAIL           0x07

#define RX_STATE_OFFSET         0x01            /*  */
#define RX_STATE_IDLE           0x00
#define RX_STATE_START_ANALOG   0x01
#define RX_STATE_RX_RDY         0x04
#define RX_STATE_PREAMBLE_FOUND 0x05
#define RX_STATE_PRMBL_TIMEOUT  0x06
#define RX_STATE_SFD_FOUND      0x07
#define RX_STATE_CNFG_PHR_RX    0x08
#define RX_STATE_PHR_RX_STRT    0x09
#define RX_STATE_DATA_RATE_RDY  0x0A
#define RX_STATE_DATA_RX_SEQ    0x0C
#define RX_STATE_CNFG_DATA_RX   0x0D
#define RX_STATE_PHR_NOT_OK     0x0E
#define RX_STATE_LAST_SYMBOL    0x0F
#define RX_STATE_WAIT_RSD_DONE  0x10
#define RX_STATE_RSD_OK         0x11
#define RX_STATE_RSD_NOT_OK     0x12
#define RX_STATE_RECONFIG_110   0x13
#define RX_STATE_WAIT_110_PHR   0x14

#define PMSC_STATE_OFFSET       0x02            /*  */
#define PMSC_STATE_INIT         0x00
#define PMSC_STATE_IDLE         0x01
#define PMSC_STATE_TX_WAIT      0x02
#define PMSC_STATE_RX_WAIT      0x03
#define PMSC_STATE_TX           0x04
#define PMSC_STATE_RX           0x05


/****************************************************************************//**
 * @brief Bit definitions for register ACK_RESP_T
**/
/* Acknowledge (31:24 preamble symbol delay before auto ACK is sent) and respose (19:0 - unit 1us) timer */
#define ACK_RESP_T_ID           0x1A            /* Acknowledgement Time and Response Time */
#define ACK_RESP_T_LEN          (4)
/*mask and shift */
#define ACK_RESP_T_MASK         0xFF0FFFFFUL    /* Acknowledgement Time and Response access mask */
#define ACK_RESP_T_W4R_TIM_OFFSET 0             /* In bytes */
#define ACK_RESP_T_W4R_TIM_MASK 0x000FFFFFUL    /* Wait-for-Response turn-around Time 20 bit field */
#define W4R_TIM_MASK            ACK_RESP_T_W4R_TIM_MASK
#define ACK_RESP_T_ACK_TIM_OFFSET 3             /* In bytes */
#define ACK_RESP_T_ACK_TIM_MASK 0xFF000000UL    /* Auto-Acknowledgement turn-around Time */
#define ACK_TIM_MASK            ACK_RESP_T_ACK_TIM_MASK



/****************************************************************************//**
 * @brief Bit definitions for register 0x1B 0x1C
**/
#define REG_1B_ID_RESERVED      0x1B
#define REG_1C_ID_RESERVED      0x1C

/****************************************************************************//**
 * @brief Bit definitions for register RX_SNIFF
 *        Sniff Mode Configuration or Pulsed Preamble Reception Configuration
**/
#define RX_SNIFF_ID             0x1D            /* Sniff Mode Configuration */
#define RX_SNIFF_OFFSET         0x00
#define RX_SNIFF_LEN            (4)
/*mask and shift */
#define RX_SNIFF_MASK           0x0000FF0FUL    /*  */
#define RX_SNIFF_SNIFF_ONT_MASK 0x0000000FUL    /* SNIFF Mode ON time. Specified in units of PAC */
#define SNIFF_ONT_MASK          RX_SNIFF_SNIFF_ONT_MASK
#define RX_SNIFF_SNIFF_OFFT_MASK 0x0000FF00UL   /* SNIFF Mode OFF time specified in units of approximately 1mkS, or 128 system clock cycles.*/
#define SNIFF_OFFT_MASK         RX_SNIFF_SNIFF_OFFT_MASK



/****************************************************************************//**
 * @brief Bit definitions for register TX_POWER
**/
#define TX_POWER_ID             0x1E            /* TX Power Control */
#define TX_POWER_LEN            (4)
/*mask and shift definition for Smart Transmit Power Control*/
#define TX_POWER_BOOSTNORM_MASK 0x00000000UL    /* This is the normal power setting used for frames that do not fall */
#define BOOSTNORM_MASK          TX_POWER_BOOSTNORM_MASK
#define TX_POWER_BOOSTNORM_SHIFT (0)
#define TX_POWER_BOOSTP500_MASK 0x00000000UL    /* This value sets the power applied during transmission at the 6.8 Mbps data rate frames that are less than 0.5 ms duration */
#define BOOSTP500_MASK          TX_POWER_BOOSTP500_MASK
#define TX_POWER_BOOSTP500_SHIFT (8)
#define TX_POWER_BOOSTP250_MASK 0x00000000UL    /* This value sets the power applied during transmission at the 6.8 Mbps data rate frames that are less than 0.25 ms duration */
#define BOOSTP250_MASK          TX_POWER_BOOSTP250_MASK
#define TX_POWER_BOOSTP250_SHIFT (16)
#define TX_POWER_BOOSTP125_MASK 0x00000000UL    /* This value sets the power applied during transmission at the 6.8 Mbps data rate frames that are less than 0.125 ms */
#define BOOSTP125_MASK          TX_POWER_BOOSTP125_MASK
#define TX_POWER_BOOSTP125_SHIFT (24)
/*mask and shift definition for Manual Transmit Power Control (DIS_STXP=1 in SYS_CFG)*/
#define TX_POWER_MAN_DEFAULT    0x0E080222UL
#define TX_POWER_TXPOWPHR_MASK  0x0000FF00UL    /* This power setting is applied during the transmission of the PHY header (PHR) portion of the frame. */
#define TX_POWER_TXPOWSD_MASK   0x00FF0000UL    /* This power setting is applied during the transmission of the synchronisation header (SHR) and data portions of the frame. */


/****************************************************************************//**
 * @brief Bit definitions for register CHAN_CTRL
**/
#define CHAN_CTRL_ID            0x1F            /* Channel Control */
#define CHAN_CTRL_LEN           (4)
/*mask and shift */
#define CHAN_CTRL_MASK          0xFFFF00FFUL    /* Channel Control Register access mask */
#define CHAN_CTRL_TX_CHAN_MASK  0x0000000FUL    /* Supported channels are 1, 2, 3, 4, 5, and 7.*/
#define CHAN_CTRL_TX_CHAN_SHIFT (0)             /* Bits 0..3        TX channel number 0-15 selection */

#define CHAN_CTRL_RX_CHAN_MASK  0x000000F0UL
#define CHAN_CTRL_RX_CHAN_SHIFT (4)             /* Bits 4..7        RX channel number 0-15 selection */

#define CHAN_CTRL_RXFPRF_MASK   0x000C0000UL    /* Bits 18..19      Specify (Force) RX Pulse Repetition Rate: 00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz. */
#define CHAN_CTRL_RXFPRF_SHIFT  (18)
/* Specific RXFPRF configuration */
#define CHAN_CTRL_RXFPRF_4      0x00000000UL    /* Specify (Force) RX Pulse Repetition Rate: 00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz. */
#define CHAN_CTRL_RXFPRF_16     0x00040000UL    /* Specify (Force) RX Pulse Repetition Rate: 00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz. */
#define CHAN_CTRL_RXFPRF_64     0x00080000UL    /* Specify (Force) RX Pulse Repetition Rate: 00 = 4 MHz, 01 = 16 MHz, 10 = 64MHz. */
#define CHAN_CTRL_TX_PCOD_MASK  0x07C00000UL    /* Bits 22..26      TX Preamble Code selection, 1 to 24. */
#define CHAN_CTRL_TX_PCOD_SHIFT (22)
#define CHAN_CTRL_RX_PCOD_MASK  0xF8000000UL    /* Bits 27..31      RX Preamble Code selection, 1 to 24. */
#define CHAN_CTRL_RX_PCOD_SHIFT (27)
/*offset 16 */
#define CHAN_CTRL_DWSFD         0x00020000UL    /* Bit 17 This bit enables a non-standard DecaWave proprietary SFD sequence. */
#define CHAN_CTRL_DWSFD_SHIFT   (17)
#define CHAN_CTRL_TNSSFD        0x00100000UL    /* Bit 20 Non-standard SFD in the transmitter */
#define CHAN_CTRL_TNSSFD_SHIFT  (20)
#define CHAN_CTRL_RNSSFD        0x00200000UL    /* Bit 21 Non-standard SFD in the receiver */
#define CHAN_CTRL_RNSSFD_SHIFT  (21)




/****************************************************************************//**
 * @brief Bit definitions for register 0x20
**/
#define REG_20_ID_RESERVED      0x20

/****************************************************************************//**
 * @brief Bit definitions for register USR_SFD
 *        Please read User Manual : User defined SFD sequence
**/
#define USR_SFD_ID              0x21            /* User-specified short/long TX/RX SFD sequences */
#define USR_SFD_LEN             (41)
#define DW_NS_SFD_LEN_110K      64              /* Decawave non-standard SFD length for 110 kbps */
#define DW_NS_SFD_LEN_850K      16              /* Decawave non-standard SFD length for 850 kbps */
#define DW_NS_SFD_LEN_6M8       8               /* Decawave non-standard SFD length for 6.8 Mbps */


/****************************************************************************//**
 * @brief Bit definitions for register
**/
#define REG_22_ID_RESERVED      0x22

/****************************************************************************//**
 * @brief Bit definitions for register AGC_CTRL
 * Please take care to write to this register as doing so may cause the DW1000 to malfunction
**/
#define AGC_CTRL_ID             0x23            /* Automatic Gain Control configuration */
#define AGC_CTRL_LEN            (32)
#define AGC_CFG_STS_ID          AGC_CTRL_ID
/* offset from AGC_CTRL_ID in bytes */
#define AGC_CTRL1_OFFSET        (0x02)
#define AGC_CTRL1_LEN           (2)
#define AGC_CTRL1_MASK          0x0001          /* access mask to AGC configuration and control register */
#define AGC_CTRL1_DIS_AM        0x0001          /* Disable AGC Measurement. The DIS_AM bit is set by default. */
/* offset from AGC_CTRL_ID in bytes */
/* Please take care not to write other values to this register as doing so may cause the DW1000 to malfunction */
#define AGC_TUNE1_OFFSET        (0x04)
#define AGC_TUNE1_LEN           (2)
#define AGC_TUNE1_MASK          0xFFFF          /* It is a 16-bit tuning register for the AGC. */
#define AGC_TUNE1_16M           0x8870
#define AGC_TUNE1_64M           0x889B
/* offset from AGC_CTRL_ID in bytes */
/* Please take care not to write other values to this register as doing so may cause the DW1000 to malfunction */
#define AGC_TUNE2_OFFSET        (0x0C)
#define AGC_TUNE2_LEN           (4)
#define AGC_TUNE2_MASK          0xFFFFFFFFUL
#define AGC_TUNE2_VAL           0X2502A907UL
/* offset from AGC_CTRL_ID in bytes */
/* Please take care not to write other values to this register as doing so may cause the DW1000 to malfunction */
#define AGC_TUNE3_OFFSET        (0x12)
#define AGC_TUNE3_LEN           (2)
#define AGC_TUNE3_MASK          0xFFFF
#define AGC_TUNE3_VAL           0X0055
/* offset from AGC_CTRL_ID in bytes */
#define AGC_STAT1_OFFSET        (0x1E)
#define AGC_STAT1_LEN           (3)
#define AGC_STAT1_MASK          0x0FFFFF
#define AGC_STAT1_EDG1_MASK     0x0007C0        /* This 5-bit gain value relates to input noise power measurement. */
#define AGC_STAT1_EDG2_MASK     0x0FF800        /* This 9-bit value relates to the input noise power measurement. */

/****************************************************************************//**
 * @brief Bit definitions for register EXT_SYNC
**/
#define EXT_SYNC_ID             0x24            /* External synchronisation control */
#define EXT_SYNC_LEN            (12)
/* offset from EXT_SYNC_ID in bytes */
#define EC_CTRL_OFFSET          (0x00)
#define EC_CTRL_LEN             (4)
#define EC_CTRL_MASK            0x00000FFBUL    /* sub-register 0x00 is the External clock synchronisation counter configuration register */
#define EC_CTRL_OSTSM           0x00000001UL    /* External transmit synchronisation mode enable */
#define EC_CTRL_OSRSM           0x00000002UL    /* External receive synchronisation mode enable */
#define EC_CTRL_PLLLCK          0x04            /* PLL lock detect enable */
#define EC_CTRL_OSTRM           0x00000800UL    /* External timebase reset mode enable */
#define EC_CTRL_WAIT_MASK       0x000007F8UL    /* Wait counter used for external transmit synchronisation and external timebase reset */
/* offset from EXT_SYNC_ID in bytes */
#define EC_RXTC_OFFSET          (0x04)
#define EC_RXTC_LEN             (4)
#define EC_RXTC_MASK            0xFFFFFFFFUL    /* External clock synchronisation counter captured on RMARKER */
/* offset from EXT_SYNC_ID in bytes */
#define EC_GOLP                 (0x08)
#define EC_GOLP_LEN             (4)
#define EC_GOLP_MASK            0x0000003FUL    /* sub-register 0x08 is the External clock offset to first path 1 GHz counter, EC_GOLP */
#define EC_GOLP_OFFSET_EXT_MASK 0x0000003FUL    /* This register contains the 1 GHz count from the arrival of the RMARKER and the next edge of the external clock. */


/****************************************************************************//**
 * @brief Bit definitions for register ACC_MEM
**/
#define ACC_MEM_ID              0x25            /* Read access to accumulator data */
#define ACC_MEM_LEN             (4064)


/****************************************************************************//**
 * @brief Bit definitions for register GPIO_CTRL
**/
#define GPIO_CTRL_ID            0x26            /* Peripheral register bus 1 access - GPIO control */
#define GPIO_CTRL_LEN           (44)

/* offset from GPIO_CTRL in bytes */
#define GPIO_MODE_OFFSET        0x00            /* sub-register 0x00 is the GPIO Mode Control Register */
#define GPIO_MODE_LEN           (4)
#define GPIO_MODE_MASK          0x00FFFFC0UL

#define GPIO_MSGP0_MASK         0x000000C0UL    /* Mode Selection for GPIO0/RXOKLED */
#define GPIO_MSGP1_MASK         0x00000300UL    /* Mode Selection for GPIO1/SFDLED */
#define GPIO_MSGP2_MASK         0x00000C00UL    /* Mode Selection for GPIO2/RXLED */
#define GPIO_MSGP3_MASK         0x00003000UL    /* Mode Selection for GPIO3/TXLED */
#define GPIO_MSGP4_MASK         0x0000C000UL    /* Mode Selection for GPIO4/EXTPA */
#define GPIO_MSGP5_MASK         0x00030000UL    /* Mode Selection for GPIO5/EXTTXE */
#define GPIO_MSGP6_MASK         0x000C0000UL    /* Mode Selection for GPIO6/EXTRXE */
#define GPIO_MSGP7_MASK         0x00300000UL    /* Mode Selection for SYNC/GPIO7 */
#define GPIO_MSGP8_MASK         0x00C00000UL    /* Mode Selection for IRQ/GPIO8 */

#define GPIO_PIN2_RXLED         0x00000400UL    /* The pin operates as the RXLED output */
#define GPIO_PIN3_TXLED         0x00001000UL    /* The pin operates as the TXLED output */
#define GPIO_PIN4_EXTPA         0x00004000UL    /* The pin operates as the EXTPA output */
#define GPIO_PIN5_EXTTXE        0x00010000UL    /* The pin operates as the EXTTXE output */
#define GPIO_PIN6_EXTRXE        0x00040000UL    /* The pin operates as the EXTRXE output */

/* offset from GPIO_CTRL in bytes */
#define GPIO_DIR_OFFSET         0x08            /* sub-register 0x08 is the GPIO Direction Control Register */
#define GPIO_DIR_LEN            (3)
#define GPIO_DIR_MASK           0x0011FFFFUL

#define GxP0                    0x00000001UL    /* GPIO0 Only changed if the GxM0 mask bit has a value of 1 for the write operation*/
#define GxP1                    0x00000002UL    /* GPIO1. (See GDP0). */
#define GxP2                    0x00000004UL    /* GPIO2. (See GDP0). */
#define GxP3                    0x00000008UL    /* GPIO3. (See GDP0). */
#define GxP4                    0x00000100UL    /* GPIO4. (See GDP0). */
#define GxP5                    0x00000200UL    /* GPIO5. (See GDP0). */
#define GxP6                    0x00000400UL    /* GPIO6. (See GDP0). */
#define GxP7                    0x00000800UL    /* GPIO7. (See GDP0). */
#define GxP8                    0x00010000UL    /* GPIO8 */

#define GxM0                    0x00000010UL    /* Mask for GPIO0 */
#define GxM1                    0x00000020UL    /* Mask for GPIO1. (See GDM0). */
#define GxM2                    0x00000040UL    /* Mask for GPIO2. (See GDM0). */
#define GxM3                    0x00000080UL    /* Mask for GPIO3. (See GDM0). */
#define GxM4                    0x00001000UL    /* Mask for GPIO4. (See GDM0). */
#define GxM5                    0x00002000UL    /* Mask for GPIO5. (See GDM0). */
#define GxM6                    0x00004000UL    /* Mask for GPIO6. (See GDM0). */
#define GxM7                    0x00008000UL    /* Mask for GPIO7. (See GDM0). */
#define GxM8                    0x00100000UL    /* Mask for GPIO8. (See GDM0). */

#define GDP0    GxP0    /* Direction Selection for GPIO0. 1 = input, 0 = output. Only changed if the GDM0 mask bit has a value of 1 for the write operation*/
#define GDP1    GxP1    /* Direction Selection for GPIO1. (See GDP0). */
#define GDP2    GxP2    /* Direction Selection for GPIO2. (See GDP0). */
#define GDP3    GxP3    /* Direction Selection for GPIO3. (See GDP0). */
#define GDP4    GxP4    /* Direction Selection for GPIO4. (See GDP0). */
#define GDP5    GxP5    /* Direction Selection for GPIO5. (See GDP0). */
#define GDP6    GxP6    /* Direction Selection for GPIO6. (See GDP0). */
#define GDP7    GxP7    /* Direction Selection for GPIO7. (See GDP0). */
#define GDP8    GxP8    /* Direction Selection for GPIO8 */

#define GDM0    GxM0    /* Mask for setting the direction of GPIO0 */
#define GDM1    GxM1    /* Mask for setting the direction of GPIO1. (See GDM0). */
#define GDM2    GxM2    /* Mask for setting the direction of GPIO2. (See GDM0). */
#define GDM3    GxM3    /* Mask for setting the direction of GPIO3. (See GDM0). */
#define GDM4    GxM4    /* Mask for setting the direction of GPIO4. (See GDM0). */
#define GDM5    GxM5    /* Mask for setting the direction of GPIO5. (See GDM0). */
#define GDM6    GxM6    /* Mask for setting the direction of GPIO6. (See GDM0). */
#define GDM7    GxM7    /* Mask for setting the direction of GPIO7. (See GDM0). */
#define GDM8    GxM8    /* Mask for setting the direction of GPIO8. (See GDM0). */

/* offset from GPIO_CTRL in bytes */
#define GPIO_DOUT_OFFSET        0x0C            /* sub-register 0x0C is the GPIO data output register. */
#define GPIO_DOUT_LEN           (3)
#define GPIO_DOUT_MASK          GPIO_DIR_MASK

/* offset from GPIO_CTRL in bytes */
#define GPIO_IRQE_OFFSET        0x10            /* sub-register 0x10 is the GPIO interrupt enable register */
#define GPIO_IRQE_LEN           (4)
#define GPIO_IRQE_MASK          0x000001FFUL
#define GIRQx0                  0x00000001UL    /* IRQ bit0 */
#define GIRQx1                  0x00000002UL    /* IRQ bit1 */
#define GIRQx2                  0x00000004UL    /* IRQ bit2 */
#define GIRQx3                  0x00000008UL    /* IRQ bit3 */
#define GIRQx4                  0x00000010UL    /* IRQ bit4 */
#define GIRQx5                  0x00000020UL    /* IRQ bit5 */
#define GIRQx6                  0x00000040UL    /* IRQ bit6 */
#define GIRQx7                  0x00000080UL    /* IRQ bit7 */
#define GIRQx8                  0x00000100UL    /* IRQ bit8 */
#define GIRQE0  GIRQx0  /* GPIO IRQ Enable for GPIO0 input. Value 1 = enable, 0 = disable*/
#define GIRQE1  GIRQx1  /*  */
#define GIRQE2  GIRQx2  /*  */
#define GIRQE3  GIRQx3  /*  */
#define GIRQE4  GIRQx4  /*  */
#define GIRQE5  GIRQx5  /*  */
#define GIRQE6  GIRQx6  /*  */
#define GIRQE7  GIRQx7  /*  */
#define GIRQE8  GIRQx8  /* Value 1 = enable, 0 = disable */

/* offset from GPIO_CTRL in bytes */
#define GPIO_ISEN_OFFSET        0x14    /* sub-register 0x14 is the GPIO interrupt sense selection register */
#define GPIO_ISEN_LEN           (4)
#define GPIO_ISEN_MASK          GPIO_IRQE_MASK
#define GISEN0  GIRQx0  /* GPIO IRQ Sense selection GPIO0 input. Value 0 = High or Rising-Edge, 1 = Low or falling-edge.*/
#define GISEN1  GIRQx1  /*  */
#define GISEN2  GIRQx2  /*  */
#define GISEN3  GIRQx3  /*  */
#define GISEN4  GIRQx4  /*  */
#define GISEN5  GIRQx5  /*  */
#define GISEN6  GIRQx6  /*  */
#define GISEN7  GIRQx7  /*  */
#define GISEN8  GIRQx8  /* Value 0 = High or Rising-Edge, 1 = Low or falling-edge */

/* offset from GPIO_CTRL in bytes */
#define GPIO_IMODE_OFFSET       0x18    /* sub-register 0x18 is the GPIO interrupt mode selection register */
#define GPIO_IMODE_LEN          (4)
#define GPIO_IMODE_MASK         GPIO_IRQE_MASK
#define GIMOD0  GIRQx0  /* GPIO IRQ Mode selection for GPIO0 input. Value 0 = Level sensitive interrupt. Value 1 = Edge triggered interrupt */
#define GIMOD1  GIRQx1  /*  */
#define GIMOD2  GIRQx2  /*  */
#define GIMOD3  GIRQx3  /*  */
#define GIMOD4  GIRQx4  /*  */
#define GIMOD5  GIRQx5  /*  */
#define GIMOD6  GIRQx6  /*  */
#define GIMOD7  GIRQx7  /*  */
#define GIMOD8  GIRQx8  /* Value 0 = Level, 1 = Edge. */

/* offset from EXT_SYNC_ID in bytes */
#define GPIO_IBES_OFFSET        0x1C    /* sub-register 0x1C is the GPIO interrupt �Both Edge� selection register */
#define GPIO_IBES_LEN           (4)
#define GPIO_IBES_MASK          GPIO_IRQE_MASK  /*  */
#define GIBES0  GIRQx0  /* GPIO IRQ �Both Edge� selection for GPIO0 input. Value 0 = GPIO_IMODE register selects the edge. Value 1 = Both edges trigger the interrupt. */
#define GIBES1  GIRQx1  /*  */
#define GIBES2  GIRQx2  /*  */
#define GIBES3  GIRQx3  /*  */
#define GIBES4  GIRQx4  /*  */
#define GIBES5  GIRQx5  /*  */
#define GIBES6  GIRQx6  /*  */
#define GIBES7  GIRQx7  /*  */
#define GIBES8  GIRQx8  /* Value 0 = use GPIO_IMODE, 1 = Both Edges */

/* offset from GPIO_CTRL in bytes */
#define GPIO_ICLR_OFFSET        0x20    /* sub-register 0x20 is the GPIO interrupt clear register */
#define GPIO_ICLR_LEN           (4)
#define GPIO_ICLR_MASK          GPIO_IRQE_MASK  /*  */
#define GICLR0  GIRQx0  /* GPIO IRQ latch clear for GPIO0 input. Write 1 to clear the GPIO0 interrupt latch. Writing 0 has no effect. Reading returns zero */
#define GICLR1  GIRQx1  /*  */
#define GICLR2  GIRQx2  /*  */
#define GICLR3  GIRQx3  /*  */
#define GICLR4  GIRQx4  /*  */
#define GICLR5  GIRQx5  /*  */
#define GICLR6  GIRQx6  /*  */
#define GICLR7  GIRQx7  /*  */
#define GICLR8  GIRQx8  /* Write 1 to clear the interrupt latch */

/* offset from GPIO_CTRL in bytes */
#define GPIO_IDBE_OFFSET        0x24    /* sub-register 0x24 is the GPIO interrupt de-bounce enable register */
#define GPIO_IDBE_LEN           (4)
#define GPIO_IDBE_MASK          GPIO_IRQE_MASK
#define GIDBE0  GIRQx0  /* GPIO IRQ de-bounce enable for GPIO0. Value 1 = de-bounce enabled. Value 0 = de-bounce disabled */
#define GIDBE1  GIRQx1  /*  */
#define GIDBE2  GIRQx2  /*  */
#define GIDBE3  GIRQx3  /*  */
#define GIDBE4  GIRQx4  /*  */
#define GIDBE5  GIRQx5  /*  */
#define GIDBE6  GIRQx6  /*  */
#define GIDBE7  GIRQx7  /*  */
#define GIDBE8  GIRQx8  /* Value 1 = de-bounce enabled, 0 = de-bounce disabled */

/* offset from GPIO_CTRL in bytes */
#define GPIO_RAW_OFFSET         0x28    /* sub-register 0x28 allows the raw state of the GPIO pin to be read. */
#define GPIO_RAW_LEN            (4)
#define GPIO_RAW_MASK           GPIO_IRQE_MASK
#define GRAWP0  GIRQx0  /* This bit reflects the raw state of GPIO0 */
#define GRAWP1  GIRQx1  /*  */
#define GRAWP2  GIRQx2  /*  */
#define GRAWP3  GIRQx3  /*  */
#define GRAWP4  GIRQx4  /*  */
#define GRAWP5  GIRQx5  /*  */
#define GRAWP6  GIRQx6  /*  */
#define GRAWP7  GIRQx7  /*  */
#define GRAWP8  GIRQx8  /* This bit reflects the raw state of GPIO8 */

/****************************************************************************//**
 * @brief Bit definitions for register  DRX_CONF
 * Digital Receiver configuration block
**/
#define DRX_CONF_ID             0x27            /* Digital Receiver configuration */
#define DRX_CONF_LEN            (44)
/* offset from DRX_CONF_ID in bytes */
#define DRX_TUNE0b_OFFSET       (0x02)  /* sub-register 0x02 is a 16-bit tuning register. */
#define DRX_TUNE0b_LEN          (2)
#define DRX_TUNE0b_MASK         0xFFFF  /* 7.2.40.2 Sub-Register 0x27:02 � DRX_TUNE0b */
#define DRX_TUNE0b_110K_STD     0x000A
#define DRX_TUNE0b_110K_NSTD    0x0016
#define DRX_TUNE0b_850K_STD     0x0001
#define DRX_TUNE0b_850K_NSTD    0x0006
#define DRX_TUNE0b_6M8_STD      0x0001
#define DRX_TUNE0b_6M8_NSTD     0x0002

/* offset from DRX_CONF_ID in bytes */
#define DRX_TUNE1a_OFFSET       0x04    /* 7.2.40.3 Sub-Register 0x27:04 � DRX_TUNE1a */
#define DRX_TUNE1a_LEN          (2)
#define DRX_TUNE1a_MASK         0xFFFF
#define DRX_TUNE1a_PRF16        0x0087
#define DRX_TUNE1a_PRF64        0x008D

/* offset from DRX_CONF_ID in bytes */
#define DRX_TUNE1b_OFFSET       0x06    /* 7.2.40.4 Sub-Register 0x27:06 � DRX_TUNE1b */
#define DRX_TUNE1b_LEN          (2)
#define DRX_TUNE1b_MASK         0xFFFF
#define DRX_TUNE1b_110K         0x0064
#define DRX_TUNE1b_850K_6M8     0x0020
#define DRX_TUNE1b_6M8_PRE64    0x0010

/* offset from DRX_CONF_ID in bytes */
#define DRX_TUNE2_OFFSET        0x08    /* 7.2.40.5 Sub-Register 0x27:08 � DRX_TUNE2 */
#define DRX_TUNE2_LEN           (4)
#define DRX_TUNE2_MASK          0xFFFFFFFFUL
#define DRX_TUNE2_PRF16_PAC8    0x311A002DUL
#define DRX_TUNE2_PRF16_PAC16   0x331A0052UL
#define DRX_TUNE2_PRF16_PAC32   0x351A009AUL
#define DRX_TUNE2_PRF16_PAC64   0x371A011DUL
#define DRX_TUNE2_PRF64_PAC8    0x313B006BUL
#define DRX_TUNE2_PRF64_PAC16   0x333B00BEUL
#define DRX_TUNE2_PRF64_PAC32   0x353B015EUL
#define DRX_TUNE2_PRF64_PAC64   0x373B0296UL

/* offset from DRX_CONF_ID in bytes */
/* WARNING: Please do NOT set DRX_SFDTOC to zero (disabling SFD detection timeout)
 * since this risks IC malfunction due to prolonged receiver activity in the event of false preamble detection.
 */
#define DRX_SFDTOC_OFFSET       0x20    /* 7.2.40.7 Sub-Register 0x27:20 � DRX_SFDTOC */
#define DRX_SFDTOC_LEN          (2)
#define DRX_SFDTOC_MASK         0xFFFF

/* offset from DRX_CONF_ID in bytes */
#define DRX_PRETOC_OFFSET       0x24    /* 7.2.40.9 Sub-Register 0x27:24 � DRX_PRETOC */
#define DRX_PRETOC_LEN          (2)
#define DRX_PRETOC_MASK         0xFFFF

/* offset from DRX_CONF_ID in bytes */
#define DRX_TUNE4H_OFFSET       0x26    /* 7.2.40.10 Sub-Register 0x27:26 � DRX_TUNE4H */
#define DRX_TUNE4H_LEN          (2)
#define DRX_TUNE4H_MASK         0xFFFF
#define DRX_TUNE4H_PRE64        0x0010
#define DRX_TUNE4H_PRE128PLUS   0x0028

/* offset from DRX_CONF_ID in bytes to 21-bit signed RX carrier integrator value */
#define DRX_CARRIER_INT_OFFSET  0x28
#define DRX_CARRIER_INT_LEN     (3)
#define DRX_CARRIER_INT_MASK    0x001FFFFF

/* offset from DRX_CONF_ID in bytes */
#define RPACC_NOSAT_OFFSET      0x2C    /* 7.2.40.11 Sub-Register 0x27:2C - RXPACC_NOSAT */
#define RPACC_NOSAT_LEN         (2)
#define RPACC_NOSAT_MASK        0xFFFF


/****************************************************************************//**
 * @brief Bit definitions for register  RF_CONF
 * Analog RF Configuration block
 * Refer to section 7.2.41 Register file: 0x28 � Analog RF configuration block
**/
#define RF_CONF_ID              0x28            /* Analog RF Configuration */
#define RF_CONF_LEN             (58)
#define RF_CONF_TXEN_MASK       0x00400000UL   /* TX enable */
#define RF_CONF_RXEN_MASK       0x00200000UL   /* RX enable */
#define RF_CONF_TXPOW_MASK      0x001F0000UL   /* turn on power all LDOs */
#define RF_CONF_PLLEN_MASK      0x0000E000UL   /* enable PLLs */
#define RF_CONF_TXBLOCKSEN_MASK 0x00001F00UL   /* enable TX blocks */
#define RF_CONF_TXPLLPOWEN_MASK (RF_CONF_PLLEN_MASK | RF_CONF_TXPOW_MASK)
#define RF_CONF_TXALLEN_MASK    (RF_CONF_TXEN_MASK | RF_CONF_TXPOW_MASK | RF_CONF_PLLEN_MASK | RF_CONF_TXBLOCKSEN_MASK)
/* offset from TX_CAL_ID in bytes */
#define RF_RXCTRLH_OFFSET       0x0B            /* Analog RX Control Register */
#define RF_RXCTRLH_LEN          (1)
#define RF_RXCTRLH_NBW          0xD8            /* RXCTRLH value for narrow bandwidth channels */
#define RF_RXCTRLH_WBW          0xBC            /* RXCTRLH value for wide bandwidth channels */
/* offset from TX_CAL_ID in bytes */
#define RF_TXCTRL_OFFSET        0x0C            /* Analog TX Control Register */
#define RF_TXCTRL_LEN           (4)
#define RF_TXCTRL_TXMTUNE_MASK  0x000001E0UL    /* Transmit mixer tuning register */
#define RF_TXCTRL_TXTXMQ_MASK   0x00000E00UL    /* Transmit mixer Q-factor tuning register */
#define RF_TXCTRL_CH1           0x00005C40UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */
#define RF_TXCTRL_CH2           0x00045CA0UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */
#define RF_TXCTRL_CH3           0x00086CC0UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */
#define RF_TXCTRL_CH4           0x00045C80UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */
#define RF_TXCTRL_CH5           0x001E3FE0UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */
#define RF_TXCTRL_CH7           0x001E7DE0UL    /* 32-bit value to program to Sub-Register 0x28:0C � RF_TXCTRL */

/* offset from TX_CAL_ID in bytes */
#define RF_STATUS_OFFSET        0x2C

/****************************************************************************//**
 * @brief Bit definitions for register
**/
#define REG_29_ID_RESERVED      0x29

/****************************************************************************//**
 * @brief Bit definitions for register TX_CAL
 * Refer to section 7.2.43 Register file: 0x2A � Transmitter Calibration block
**/
#define TX_CAL_ID               0x2A            /* Transmitter calibration block */
#define TX_CAL_LEN              (52)
/* offset from TX_CAL_ID in bytes */
#define TC_SARL_SAR_C                   (0)         /* SAR control */
/*cause bug in register block TX_CAL, we need to read 1 byte in a time*/
#define TC_SARL_SAR_LVBAT_OFFSET    (3)         /* Latest SAR reading for Voltage level */
#define TC_SARL_SAR_LTEMP_OFFSET    (4)         /* Latest SAR reading for Temperature level */
#define TC_SARW_SAR_WTEMP_OFFSET    0x06            /* SAR reading of Temperature level taken at last wakeup event */
#define TC_SARW_SAR_WVBAT_OFFSET    0x07            /* SAR reading of Voltage level taken at last wakeup event */
/* offset from TX_CAL_ID in bytes */
#define TC_PGDELAY_OFFSET       0x0B            /* Transmitter Calibration � Pulse Generator Delay */
#define TC_PGDELAY_LEN          (1)
#define TC_PGDELAY_CH1          0xC9            /* Recommended value for channel 1 */
#define TC_PGDELAY_CH2          0xC2            /* Recommended value for channel 2 */
#define TC_PGDELAY_CH3          0xC5            /* Recommended value for channel 3 */
#define TC_PGDELAY_CH4          0x95            /* Recommended value for channel 4 */
#define TC_PGDELAY_CH5          0xC0            /* Recommended value for channel 5 */
#define TC_PGDELAY_CH7          0x93            /* Recommended value for channel 7 */
/* offset from TX_CAL_ID in bytes */
#define TC_PGTEST_OFFSET        0x0C            /* Transmitter Calibration � Pulse Generator Test */
#define TC_PGTEST_LEN           (1)
#define TC_PGTEST_NORMAL        0x00            /* Normal operation */
#define TC_PGTEST_CW            0x13            /* Continuous Wave (CW) Test Mode */

/****************************************************************************//**
 * @brief Bit definitions for register
 * Refer to section 7.2.44 Register file: 0x2B � Frequency synthesiser control block
**/
#define FS_CTRL_ID              0x2B            /* Frequency synthesiser control block */
#define FS_CTRL_LEN             (21)
/* offset from FS_CTRL_ID in bytes */
#define FS_RES1_OFFSET          0x00            /* reserved area. Please take care not to write to this area as doing so may cause the DW1000 to malfunction. */
#define FS_RES1_LEN             (7)
/* offset from FS_CTRL_ID in bytes */
#define FS_PLLCFG_OFFSET        0x07            /* Frequency synthesiser � PLL configuration */
#define FS_PLLCFG_LEN           (5)
#define FS_PLLCFG_CH1           0x09000407UL    /* Operating Channel 1 */
#define FS_PLLCFG_CH2           0x08400508UL    /* Operating Channel 2 */
#define FS_PLLCFG_CH3           0x08401009UL    /* Operating Channel 3 */
#define FS_PLLCFG_CH4           FS_PLLCFG_CH2   /* Operating Channel 4 (same as 2) */
#define FS_PLLCFG_CH5           0x0800041DUL    /* Operating Channel 5 */
#define FS_PLLCFG_CH7           FS_PLLCFG_CH5   /* Operating Channel 7 (same as 5) */
/* offset from FS_CTRL_ID in bytes */
#define FS_PLLTUNE_OFFSET       0x0B            /* Frequency synthesiser � PLL Tuning */
#define FS_PLLTUNE_LEN          (1)
#define FS_PLLTUNE_CH1          0x1E            /* Operating Channel 1 */
#define FS_PLLTUNE_CH2          0x26            /* Operating Channel 2 */
#define FS_PLLTUNE_CH3          0x56            /* Operating Channel 3 */
#define FS_PLLTUNE_CH4          FS_PLLTUNE_CH2  /* Operating Channel 4 (same as 2) */
#define FS_PLLTUNE_CH5          0xBE            /* Operating Channel 5 */
#define FS_PLLTUNE_CH7          FS_PLLTUNE_CH5  /* Operating Channel 7 (same as 5) */
/* offset from FS_CTRL_ID in bytes */
#define FS_RES2_OFFSET          0x0C    /* reserved area. Please take care not to write to this area as doing so may cause the DW1000 to malfunction. */
#define FS_RES2_LEN             (2)
/* offset from FS_CTRL_ID in bytes */
#define FS_XTALT_OFFSET         0x0E    /* Frequency synthesiser � Crystal trim */
#define FS_XTALT_LEN            (1)
#define FS_XTALT_MASK           0x1F    /* Crystal Trim. Crystals may be trimmed using this register setting to tune out errors, see 8.1 � IC Calibration � Crystal Oscillator Trim. */
#define FS_XTALT_MIDRANGE       0x10
/* offset from FS_CTRL_ID in bytes */
#define FS_RES3_OFFSET          0x0F    /* reserved area. Please take care not to write to this area as doing so may cause the DW1000 to malfunction. */
#define FS_RES3_LEN             (6)

/****************************************************************************//**
 * @brief Bit definitions for register
**/
#define AON_ID                  0x2C            /* Always-On register set */
#define AON_LEN                 (12)
/* offset from AON_ID in bytes */
#define AON_WCFG_OFFSET         0x00    /* used to control what the DW1000 IC does as it wakes up from low-power SLEEP or DEEPSLEEPstates. */
#define AON_WCFG_LEN            (2)
#define AON_WCFG_MASK           0x09CB  /* access mask to AON_WCFG register*/
#define AON_WCFG_ONW_RADC       0x0001  /* On Wake-up Run the (temperature and voltage) Analog-to-Digital Convertors */
#define AON_WCFG_ONW_RX         0x0002  /* On Wake-up turn on the Receiver */
#define AON_WCFG_ONW_LEUI       0x0008  /* On Wake-up load the EUI from OTP memory into Register file: 0x01 � Extended Unique Identifier. */
#define AON_WCFG_ONW_LDC        0x0040  /* On Wake-up load configurations from the AON memory into the host interface register set */
#define AON_WCFG_ONW_L64P       0x0080  /* On Wake-up load the Length64 receiver operating parameter set */
#define AON_WCFG_PRES_SLEEP     0x0100  /* Preserve Sleep. This bit determines what the DW1000 does with respect to the ARXSLP and ATXSLP sleep controls */
#define AON_WCFG_ONW_LLDE       0x0800  /* On Wake-up load the LDE microcode. */
#define AON_WCFG_ONW_LLDO       0x1000  /* On Wake-up load the LDO tune value. */
/* offset from AON_ID in bytes */
#define AON_CTRL_OFFSET         0x02    /* The bits in this register in general cause direct activity within the AON block with respect to the stored AON memory */
#define AON_CTRL_LEN            (1)
#define AON_CTRL_MASK           0x8F    /* access mask to AON_CTRL register */
#define AON_CTRL_RESTORE        0x01    /* When this bit is set the DW1000 will copy the user configurations from the AON memory to the host interface register set. */
#define AON_CTRL_SAVE           0x02    /* When this bit is set the DW1000 will copy the user configurations from the host interface register set into the AON memory */
#define AON_CTRL_UPL_CFG        0x04    /* Upload the AON block configurations to the AON  */
#define AON_CTRL_DCA_READ       0x08    /* Direct AON memory access read */
#define AON_CTRL_DCA_ENAB       0x80    /* Direct AON memory access enable bit */
/* offset from AON_ID in bytes */
#define AON_RDAT_OFFSET         0x03    /* AON Direct Access Read Data Result */
#define AON_RDAT_LEN            (1)
/* offset from AON_ID in bytes */
#define AON_ADDR_OFFSET         0x04    /* AON Direct Access Address */
#define AON_ADDR_LEN            (1)
#define AON_ADDR_LPOSC_CAL_0    117     /* Address of low-power oscillator calibration value (lower byte) */
#define AON_ADDR_LPOSC_CAL_1    118     /* Address of low-power oscillator calibration value (lower byte) */

/* offset from AON_ID in bytes */
#define AON_CFG0_OFFSET         0x06    /* 32-bit configuration register for the always on block. */
#define AON_CFG0_LEN            (4)
#define AON_CFG0_SLEEP_EN           0x00000001UL    /* This is the sleep enable configuration bit */
#define AON_CFG0_WAKE_PIN           0x00000002UL    /* Wake using WAKEUP pin */
#define AON_CFG0_WAKE_SPI           0x00000004UL    /* Wake using SPI access SPICSn */
#define AON_CFG0_WAKE_CNT           0x00000008UL    /* Wake when sleep counter elapses */
#define AON_CFG0_LPDIV_EN           0x00000010UL    /* Low power divider enable configuration */
#define AON_CFG0_LPCLKDIVA_MASK     0x0000FFE0UL    /* divider count for dividing the raw DW1000 XTAL oscillator frequency to set an LP clock frequency */
#define AON_CFG0_LPCLKDIVA_SHIFT    (5)
#define AON_CFG0_SLEEP_TIM          0xFFFF0000UL    /* Sleep time. This field configures the sleep time count elapse value */
#define AON_CFG0_SLEEP_SHIFT        (16)
#define AON_CFG0_SLEEP_TIM_OFFSET   2               /* In bytes */
/* offset from AON_ID in bytes */
#define AON_CFG1_OFFSET         0x0A
#define AON_CFG1_LEN            (2)
#define AON_CFG1_MASK           0x0007  /* aceess mask to AON_CFG1 */
#define AON_CFG1_SLEEP_CEN      0x0001  /* This bit enables the sleep counter */
#define AON_CFG1_SMXX           0x0002  /* This bit needs to be set to 0 for correct operation in the SLEEP state within the DW1000 */
#define AON_CFG1_LPOSC_CAL      0x0004  /* This bit enables the calibration function that measures the period of the IC�s internal low powered oscillator */

/****************************************************************************//**
 * @brief Bit definitions for register OTP_IF
 * Refer to section 7.2.46 Register file: 0x2D � OTP Memory Interface
**/
#define OTP_IF_ID               0x2D            /* One Time Programmable Memory Interface */
#define OTP_IF_LEN              (18)
/* offset from OTP_IF_ID in bytes */
#define OTP_WDAT                0x00            /* 32-bit register. The data value to be programmed into an OTP location  */
#define OTP_WDAT_LEN            (4)
/* offset from OTP_IF_ID in bytes */
#define OTP_ADDR                0x04            /* 16-bit register used to select the address within the OTP memory block */
#define OTP_ADDR_LEN            (2)
#define OTP_ADDR_MASK           0x07FF          /* This 11-bit field specifies the address within OTP memory that will be accessed read or written. */
/* offset from OTP_IF_ID in bytes */
#define OTP_CTRL                0x06            /* used to control the operation of the OTP memory */
#define OTP_CTRL_LEN            (2)
#define OTP_CTRL_MASK           0x8002
#define OTP_CTRL_OTPRDEN        0x0001          /* This bit forces the OTP into manual read mode */
#define OTP_CTRL_OTPREAD        0x0002          /* This bit commands a read operation from the address specified in the OTP_ADDR register */
#define OTP_CTRL_LDELOAD        0x8000          /* This bit forces a load of LDE microcode */
#define OTP_CTRL_OTPPROG        0x0040          /* Setting this bit will cause the contents of OTP_WDAT to be written to OTP_ADDR. */
/* offset from OTP_IF_ID in bytes */
#define OTP_STAT                0x08
#define OTP_STAT_LEN            (2)
#define OTP_STAT_MASK           0x0003
#define OTP_STAT_OTPPRGD        0x0001          /* OTP Programming Done */
#define OTP_STAT_OTPVPOK        0x0002          /* OTP Programming Voltage OK */
/* offset from OTP_IF_ID in bytes */
#define OTP_RDAT                0x0A            /* 32-bit register. The data value read from an OTP location will appear here */
#define OTP_RDAT_LEN            (4)
/* offset from OTP_IF_ID in bytes */
#define OTP_SRDAT               0x0E            /* 32-bit register. The data value stored in the OTP SR (0x400) location will appear here after power up */
#define OTP_SRDAT_LEN           (4)
/* offset from OTP_IF_ID in bytes */
#define OTP_SF                  0x12            /*8-bit special function register used to select and load special receiver operational parameter */
#define OTP_SF_LEN              (1)
#define OTP_SF_MASK             0x63
#define OTP_SF_OPS_KICK         0x01            /* This bit when set initiates a load of the operating parameter set selected by the OPS_SEL */
#define OTP_SF_LDO_KICK         0x02            /* This bit when set initiates a load of the LDO tune code */
#define OTP_SF_OPS_SEL_SHFT     5
#define OTP_SF_OPS_SEL_MASK     0x60
#define OTP_SF_OPS_SEL_L64      0x00            /* Operating parameter set selection: Length64 */
#define OTP_SF_OPS_SEL_TIGHT    0x40            /* Operating parameter set selection: Tight */

/****************************************************************************//**
 * @brief Bit definitions for register LDE_IF
 * Refer to section 7.2.47 Register file: 0x2E � Leading Edge Detection Interface
 * PLEASE NOTE: Other areas within the address space of Register file: 0x2E � Leading Edge Detection Interface
 * are reserved. To ensure proper operation of the LDE algorithm (i.e. to avoid loss of performance or a malfunction),
 * care must be taken not to write to any byte locations other than those defined in the sub-sections below.
**/
#define LDE_IF_ID               0x2E            /* Leading edge detection control block */
#define LDE_IF_LEN              (0)
/* offset from LDE_IF_ID in bytes */
#define LDE_THRESH_OFFSET       0x0000  /* 16-bit status register reporting the threshold that was used to find the first path */
#define LDE_THRESH_LEN          (2)
/* offset from LDE_IF_ID in bytes */
#define LDE_CFG1_OFFSET         0x0806  /*8-bit configuration register*/
#define LDE_CFG1_LEN            (1)
#define LDE_CFG1_NSTDEV_MASK    0x1F    /* Number of Standard Deviations mask. */
#define LDE_CFG1_PMULT_MASK     0xE0    /* Peak Multiplier mask. */
/* offset from LDE_IF_ID in bytes */
#define LDE_PPINDX_OFFSET       0x1000  /* reporting the position within the accumulator that the LDE algorithm has determined to contain the maximum */
#define LDE_PPINDX_LEN          (2)
/* offset from LDE_IF_ID in bytes */
#define LDE_PPAMPL_OFFSET       0x1002  /* reporting the magnitude of the peak signal seen in the accumulator data memory */
#define LDE_PPAMPL_LEN          (2)
/* offset from LDE_IF_ID in bytes */
#define LDE_RXANTD_OFFSET       0x1804  /* 16-bit configuration register for setting the receive antenna delay */
#define LDE_RXANTD_LEN          (2)
/* offset from LDE_IF_ID in bytes */
#define LDE_CFG2_OFFSET         0x1806  /* 16-bit LDE configuration tuning register */
#define LDE_CFG2_LEN            (2)
/* offset from LDE_IF_ID in bytes */
#define LDE_REPC_OFFSET         0x2804  /* 16-bit configuration register for setting the replica avoidance coefficient */
#define LDE_REPC_LEN            (2)
#define LDE_REPC_PCODE_1        0x5998
#define LDE_REPC_PCODE_2        0x5998
#define LDE_REPC_PCODE_3        0x51EA
#define LDE_REPC_PCODE_4        0x428E
#define LDE_REPC_PCODE_5        0x451E
#define LDE_REPC_PCODE_6        0x2E14
#define LDE_REPC_PCODE_7        0x8000
#define LDE_REPC_PCODE_8        0x51EA
#define LDE_REPC_PCODE_9        0x28F4
#define LDE_REPC_PCODE_10       0x3332
#define LDE_REPC_PCODE_11       0x3AE0
#define LDE_REPC_PCODE_12       0x3D70
#define LDE_REPC_PCODE_13       0x3AE0
#define LDE_REPC_PCODE_14       0x35C2
#define LDE_REPC_PCODE_15       0x2B84
#define LDE_REPC_PCODE_16       0x35C2
#define LDE_REPC_PCODE_17       0x3332
#define LDE_REPC_PCODE_18       0x35C2
#define LDE_REPC_PCODE_19       0x35C2
#define LDE_REPC_PCODE_20       0x47AE
#define LDE_REPC_PCODE_21       0x3AE0
#define LDE_REPC_PCODE_22       0x3850
#define LDE_REPC_PCODE_23       0x30A2
#define LDE_REPC_PCODE_24       0x3850

/****************************************************************************//**
 * @brief Bit definitions for register DIG_DIAG
 * Digital Diagnostics interface.
 * It contains a number of sub-registers that give diagnostics information.
**/
#define DIG_DIAG_ID             0x2F        /* Digital Diagnostics Interface */
#define DIG_DIAG_LEN            (41)

/* offset from DIG_DIAG_ID in bytes */
#define EVC_CTRL_OFFSET         0x00        /* Event Counter Control */
#define EVC_CTRL_LEN            (4)
#define EVC_CTRL_MASK           0x00000003UL/* access mask to Register for bits should always be set to zero to avoid any malfunction of the device. */
#define EVC_EN                  0x00000001UL/* Event Counters Enable bit */
#define EVC_CLR                 0x00000002UL

/* offset from DIG_DIAG_ID in bytes */
#define EVC_PHE_OFFSET          0x04        /* PHR Error Event Counter */
#define EVC_PHE_LEN             (2)
#define EVC_PHE_MASK            0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_RSE_OFFSET          0x06        /* Reed Solomon decoder (Frame Sync Loss) Error Event Counter */
#define EVC_RSE_LEN             (2)
#define EVC_RSE_MASK            0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_FCG_OFFSET          0x08        /* The EVC_FCG field is a 12-bit counter of the frames received with good CRC/FCS sequence. */
#define EVC_FCG_LEN             (2)
#define EVC_FCG_MASK            0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_FCE_OFFSET          0x0A        /* The EVC_FCE field is a 12-bit counter of the frames received with bad CRC/FCS sequence. */
#define EVC_FCE_LEN             (2)
#define EVC_FCE_MASK            0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_FFR_OFFSET          0x0C        /* The EVC_FFR field is a 12-bit counter of the frames rejected by the receive frame filtering function. */
#define EVC_FFR_LEN             (2)
#define EVC_FFR_MASK            0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_OVR_OFFSET          0x0E        /* The EVC_OVR field is a 12-bit counter of receive overrun events */
#define EVC_OVR_LEN             (2)
#define EVC_OVR_MASK            0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_STO_OFFSET          0x10        /* The EVC_STO field is a 12-bit counter of SFD Timeout Error events */
#define EVC_STO_LEN             (2)
#define EVC_STO_MASK            0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_PTO_OFFSET          0x12        /* The EVC_PTO field is a 12-bit counter of Preamble detection Timeout events */
#define EVC_PTO_LEN             (2)
#define EVC_PTO_MASK            0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_FWTO_OFFSET         0x14        /* The EVC_FWTO field is a 12-bit counter of receive frame wait timeout events */
#define EVC_FWTO_LEN            (2)
#define EVC_FWTO_MASK           0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_TXFS_OFFSET         0x16        /* The EVC_TXFS field is a 12-bit counter of transmit frames sent. This is incremented every time a frame is sent */
#define EVC_TXFS_LEN            (2)
#define EVC_TXFS_MASK           0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_HPW_OFFSET          0x18        /* The EVC_HPW field is a 12-bit counter of �Half Period Warnings�. */
#define EVC_HPW_LEN             (2)
#define EVC_HPW_MASK            0x0FFF
/* offset from DIG_DIAG_ID in bytes */
#define EVC_TPW_OFFSET          0x1A        /* The EVC_TPW field is a 12-bit counter of �Transmitter Power-Up Warnings�. */
#define EVC_TPW_LEN             (2)
#define EVC_TPW_MASK            0x0FFF

/* offset from DIG_DIAG_ID in bytes */
#define EVC_RES1_OFFSET         0x1C        /* Please take care not to write to this register as doing so may cause the DW1000 to malfunction. */

/* offset from DIG_DIAG_ID in bytes */
#define DIAG_TMC_OFFSET         0x24
#define DIAG_TMC_LEN            (2)
#define DIAG_TMC_MASK           0x0010
#define DIAG_TMC_TX_PSTM        0x0010      /* This test mode is provided to help support regulatory approvals spectral testing. When the TX_PSTM bit is set it enables a repeating transmission of the data from the TX_BUFFER */


/****************************************************************************//**
 * @brief Bit definitions for register 0x30-0x35
 * Please take care not to write to these registers as doing so may cause the DW1000 to malfunction.
**/
#define REG_30_ID_RESERVED      0x30
#define REG_31_ID_RESERVED      0x31
#define REG_32_ID_RESERVED      0x32
#define REG_33_ID_RESERVED      0x33
#define REG_34_ID_RESERVED      0x34
#define REG_35_ID_RESERVED      0x35

/****************************************************************************//**
 * @brief Bit definitions for register PMSC
**/
#define PMSC_ID                 0x36            /* Power Management System Control Block */
#define PMSC_LEN                (48)
/* offset from PMSC_ID in bytes */
#define PMSC_CTRL0_OFFSET       0x00
#define PMSC_CTRL0_LEN          (4)
#define PMSC_CTRL0_MASK         0xF18F847FUL    /* access mask to register PMSC_CTRL0 */
#define PMSC_CTRL0_SYSCLKS_AUTO 0x00000000UL    /* The system clock will run off the 19.2 MHz XTI clock until the PLL is calibrated and locked, then it will switch over the 125 MHz PLL clock */
#define PMSC_CTRL0_SYSCLKS_19M  0x00000001UL    /* Force system clock to be the 19.2 MHz XTI clock. */
#define PMSC_CTRL0_SYSCLKS_125M 0x00000002UL    /* Force system clock to the 125 MHz PLL clock. */
#define PMSC_CTRL0_RXCLKS_AUTO  0x00000000UL    /* The RX clock will be disabled until it is required for an RX operation */
#define PMSC_CTRL0_RXCLKS_19M   0x00000004UL    /* Force RX clock enable and sourced clock from the 19.2 MHz XTI clock */
#define PMSC_CTRL0_RXCLKS_125M  0x00000008UL    /* Force RX clock enable and sourced from the 125 MHz PLL clock */
#define PMSC_CTRL0_RXCLKS_OFF   0x0000000CUL    /* Force RX clock off. */
#define PMSC_CTRL0_TXCLKS_AUTO  0x00000000UL    /* The TX clock will be disabled until it is required for a TX operation */
#define PMSC_CTRL0_TXCLKS_19M   0x00000010UL    /* Force TX clock enable and sourced clock from the 19.2 MHz XTI clock */
#define PMSC_CTRL0_TXCLKS_125M  0x00000020UL    /* Force TX clock enable and sourced from the 125 MHz PLL clock */
#define PMSC_CTRL0_TXCLKS_OFF   0x00000030UL    /* Force TX clock off */
#define PMSC_CTRL0_FACE         0x00000040UL    /* Force Accumulator Clock Enable */
#define PMSC_CTRL0_GPCE         0x00010000UL    /* GPIO clock enable */
#define PMSC_CTRL0_GPRN         0x00020000UL    /* GPIO reset (NOT), active low */
#define PMSC_CTRL0_GPDCE        0x00040000UL    /* GPIO De-bounce Clock Enable */
#define PMSC_CTRL0_KHZCLEN      0x00800000UL    /* Kilohertz Clock Enable */
#define PMSC_CTRL0_PLL2_SEQ_EN  0x01000000UL    /* Enable PLL2 on/off sequencing by SNIFF mode */
#define PMSC_CTRL0_SOFTRESET_OFFSET 3           /* In bytes */
#define PMSC_CTRL0_RESET_ALL    0x00            /* Assuming only 4th byte of the register is read */
#define PMSC_CTRL0_RESET_RX     0xE0            /* Assuming only 4th byte of the register is read */
#define PMSC_CTRL0_RESET_CLEAR  0xF0            /* Assuming only 4th byte of the register is read */
/* offset from PMSC_ID in bytes */
#define PMSC_CTRL1_OFFSET       0x04
#define PMSC_CTRL1_LEN          (4)
#define PMSC_CTRL1_MASK         0xFC02F802UL    /* access mask to register PMSC_CTRL1 */
#define PMSC_CTRL1_ARX2INIT     0x00000002UL    /* Automatic transition from receive mode into the INIT state */
#define PMSC_CTRL1_ATXSLP       0x00000800UL    /* If this bit is set then the DW1000 will automatically transition into SLEEP or DEEPSLEEP mode after transmission of a frame */
#define PMSC_CTRL1_ARXSLP       0x00001000UL    /* this bit is set then the DW1000 will automatically transition into SLEEP mode after a receive attempt */
#define PMSC_CTRL1_SNOZE        0x00002000UL    /* Snooze Enable */
#define PMSC_CTRL1_SNOZR        0x00004000UL    /* The SNOZR bit is set to allow the snooze timer to repeat twice */
#define PMSC_CTRL1_PLLSYN       0x00008000UL    /* This enables a special 1 GHz clock used for some external SYNC modes */
#define PMSC_CTRL1_LDERUNE      0x00020000UL    /* This bit enables the running of the LDE algorithm */
#define PMSC_CTRL1_KHZCLKDIV_MASK   0xFC000000UL    /* Kilohertz clock divisor */
#define PMSC_CTRL1_PKTSEQ_DISABLE   0x00        /* writing this to PMSC CONTROL 1 register (bits 10-3) disables PMSC control of analog RF subsystems */
#define PMSC_CTRL1_PKTSEQ_ENABLE    0xE7        /* writing this to PMSC CONTROL 1 register (bits 10-3) enables PMSC control of analog RF subsystems */
/* offset from PMSC_ID in bytes */
#define PMSC_RES1_OFFSET        0x08
/* offset from PMSC_ID in bytes */
#define PMSC_SNOZT_OFFSET       0x0C            /* PMSC Snooze Time Register */
#define PMSC_SNOZT_LEN          (1)
/* offset from PMSC_ID in bytes */
#define PMSC_RES2_OFFSET        0x10
/* offset from PMSC_ID in bytes */
#define PMSC_RES3_OFFSET        0x24
/* offset from PMSC_ID in bytes */
#define PMSC_TXFINESEQ_OFFSET   0x26
#define PMSC_TXFINESEQ_DISABLE  0x0             /* Writing this disables fine grain sequencing in the transmitter */
#define PMSC_TXFINESEQ_ENABLE   0x0B74          /* Writing this enables fine grain sequencing in the transmitter */
/* offset from PMSC_ID in bytes */
#define PMSC_LEDC_OFFSET        0x28
#define PMSC_LEDC_LEN           (4)
#define PMSC_LEDC_MASK          0x000001FFUL    /* 32-bit LED control register. */
#define PMSC_LEDC_BLINK_TIM_MASK 0x000000FFUL   /* This field determines how long the LEDs remain lit after an event that causes them to be set on. */
#define PMSC_LEDC_BLNKEN        0x00000100UL    /* Blink Enable. When this bit is set to 1 the LED blink feature is enabled. */
/* Default blink time. Blink time is expressed in multiples of 14 ms. The value defined here is ~225 ms. */
#define PMSC_LEDC_BLINK_TIME_DEF 0x10
/* Command a blink of all LEDs */
#define PMSC_LEDC_BLINK_NOW_ALL 0x000F0000UL

/****************************************************************************//**
 * @brief Bit definitions for register 0x37-0x3F
 * Please take care not to write to these registers as doing so may cause the DW1000 to malfunction.
**/
#define REG_37_ID_RESERVED      0x37
#define REG_38_ID_RESERVED      0x38
#define REG_39_ID_RESERVED      0x39
#define REG_3A_ID_RESERVED      0x3A
#define REG_3B_ID_RESERVED      0x3B
#define REG_3C_ID_RESERVED      0x3C
#define REG_3D_ID_RESERVED      0x3D
#define REG_3E_ID_RESERVED      0x3E
#define REG_3F_ID_RESERVED      0x3F

/* END DW1000 REGISTER DEFINITION */

#ifdef __cplusplus
}
#endif

#endif