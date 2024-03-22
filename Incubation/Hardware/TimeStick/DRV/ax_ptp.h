/* SPDX-License-Identifier: GPL-2.0 */
/*******************************************************************************
 *     Copyright (c) 2022    ASIX Electronic Corporation    All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/
#ifndef __ASIX_PTP_H
#define __ASIX_PTP_H

#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>

#define AX_MAC_BFM_CTRL		0xC1
	#define AX_CS_TRAIL_UDPV4_EN		0x40
	#define AX_CS_TRAIL_UDPV6_EN		0x80

#define EP0_GET_TIMESTAMP_RETRY	10

#define AX_PTP_CMD		0x09
#define AX_PTP_OP		0x0E
	#define AX_SET_LOCAL_CLOCK	0x01
	#define AX_SET_LOCAL_CLOCK_SIZE		0x0A
	#define AX_GET_LOCAL_CLOCK	0x02
	#define AX_GET_LOCAL_CLOCK_SIZE		0x0A
	#define AX_SET_ADDEND		0x03
	#define AX_SET_ADDEND_SIZE		0x04
	#define AX_SET_ACTIVE_TIME	0x06
	#define AX_SET_ACTIVE_TIME_SIZE	0x04
	#define AX_SET_TX_PHY_DELAY	0x07
	#define AX_SET_TX_PHY_DELAY_SIZE	0x05
	#define AX_SET_RX_PHY_DELAY	0x08
	#define AX_SET_RX_PHY_DELAY_SIZE	AX_SET_TX_PHY_DELAY_SIZE
#define AX_PTP_TIMESTAMP	0x0A
#define AX_PTP_CLK		0x13
#define AX_PTP_TOD_CTRL	0x15
	#define AX_PTP_TS_INT		0x02
	#define AX_EXT_INT		0x04
	#define AX_EXT_INT_ON		0x10
	#define AX_PTP_TX_TX_INT_EN	0x40

// Ethernet: 14B, IPv4: 20B, UDP: 8B
#define AX_TX_PTPHDR_OFFSET_L3_IP	42
// Ethernet: 14B, IPv6: 40B, UDP: 8B
#define AX_TX_PTPHDR_OFFSET_L3_IPV6	62
// Ethernet: 14B
#define AX_TX_PTPHDR_OFFSET_L2	14
#define AX_ETHTYPE_OFFSET		12
#define AX_IP_PROTO_OFFSET		9
#define AX_UDP_PORT_OFFSET		22
#define AX_PTP_EVENT_PORT_NUM		0x13F
#define PTP_HDR_SIZE			34

#define AX_PPS_ACTIVE_DEFAULT_TIME	0x1DCD6500
#define AX_BASE_ADDEND			0xCCCCCCCC
#define AX_PTP_PERIOD			0xA

#define AX88179A_PTP_CTRL_1		0x00
	#define AX_PTP_TX_PLUS_DELAY		0x20
	#define AX_PTP_RX_FILTER_GENERAL_MSG	0x10
	#define AX_PTP_TX_FILTER_GENERAL_MSG	0x08
	#define AX_PTP_CTRL_L3_EN		0x02
	#define AX_PTP_CTRL_EN			0x01
#define AX88179A_PTP_CTRL_2		0x01
	#define AX_PTP_CTRL_SET_PERIOD		0x08
	#define AX_PTP_CTRL_SET_ADDEND		0x04
	#define AX_PTP_CTRL_GET_CLK		0x02
	#define AX_PTP_CTRL_SET_CLK		0x01
#define AX88179A_PTP_TIMER_PERIOD	0x0A
#define AX88179A_PTP_MEM_SEG_SIZE	0x21
#define AX88179A_PTP_MEM_SEG_SET	0x22
#define AX88179A_PTP_MEM_SEG_STATUS	0x23
	#define AX_PTP_MEM_SEG_MASK		0x01
	#define AX_PTP_MEM_SEG_0		0

#ifdef ENABLE_AX88279
#define AX_PTP_REG_BASE_ADDR_HI		0x0012
#define AX_PTP_TX_MEM_SETTING		0x0000
	#define AX_PTP_PTP_CPU_EN		0x0001
	/* TODO:: Change to Product code */
	#define AX_PTP_MEM_SEG_SIZE_279_4	0x34	// USB 2.0
	#define AX_PTP_MEM_SEG_SIZE_279_5	0x41
	#define AX_PTP_MEM_START_ADDR		0x800
#define AX_PTP_LCK_CTRL0		0x1000
	#define AX_PTP_LCK_CTRL0_EN		0x0001
	#define AX_PTP_LCK_CTRL0_80B_NS_EN	0x0002
	#define AX_PTP_LCK_CTRL0_80B_S_EN	0x0004
	#define AX_PTP_LCK_CTRL0_48B_EN		0x0008
	#define AX_PTP_LCK_CTRL0_PPS_EN		0x0010
	#define AX_PTP_LCK_CTRL0_TX_DEL_VEC	0x0040
#define AX_PTP_LCK_CTRL1			0x1004
#define AX_PTP_SET_80B_LCK_VAL0		0x1008
#define AX_PTP_GET_80B_LCK_VAL0		0x1014
#define AX_PTP_GET_80B_LCK_VAL1		0x1018
#define AX_PTP_GET_80B_LCK_VAL2		0x101C
#define AX_PTP_TIMER_ADDEND			0x1030
#define AX_PTP_TIMER_PERIOD			0x1034
#define AX_PTP_TX_DELAY			0x1038
#define AX_PTP_RX_DELAY			0x103C
#define AX_PTP_PPS_ACTIVE_TIME		0x1040

#define AX_PTP_RX_CTRL0			0x2000
	#define AX_PTP_RXC0_EN			0x0001
	#define AX_PTP_RXC0_XGMII_EN		0x0002
	#define AX_RXC0_PARSER_L2_EN		0x0010
	#define AX_RXC0_PARSER_L2_PTP_EN	0x0020
	#define AX_RXC0_PARSER_IPV4_EN	0x0040
	#define AX_RXC0_PARSER_IPV6_EN	0x0080
	#define AX_RXC0_PARSER_VLAN_EN	0x0100
	#define AX_RXC0_PARSER_PPPOE_EN	0x0200
	#define AX_RXC0_PARSER_SNAP_EN	0x0400
	#define AX_RXC0_PARSER_UDP_EN		0x0800
	#define AX_RXC0_PARSER_UDPV6_EN	0x1000
	#define AX_RXC0_PARSER_UDP_PTP_EN	0x2000
	#define AX_RXC0_PARSER_UDPV6_PTP_EN	0x4000
	#define AX_RXC0_PARSER_EVENT_PORT_EN	0x00010000
	#define AX_RXC0_VAL_DELAY_CNT_SHIFT	24

#define AX_PTP_RX_CTRL0_DEFAULT	\
	(AX_PTP_RXC0_EN | AX_RXC0_PARSER_L2_EN | AX_RXC0_PARSER_L2_PTP_EN | \
	AX_RXC0_PARSER_IPV4_EN | AX_RXC0_PARSER_IPV6_EN | \
	AX_RXC0_PARSER_VLAN_EN | AX_RXC0_PARSER_PPPOE_EN | \
	AX_RXC0_PARSER_SNAP_EN | AX_RXC0_PARSER_UDP_EN | \
	AX_RXC0_PARSER_UDPV6_EN | AX_RXC0_PARSER_UDP_PTP_EN | \
	AX_RXC0_PARSER_UDPV6_PTP_EN | AX_RXC0_PARSER_EVENT_PORT_EN)

#define AX_PTP_TX_CTRL0			0x3000
	#define AX_PTP_TXC0_EN			0x0001
	#define AX_PTP_TXC0_XGMII_EN		0x0002
	#define AX_TXC0_PARSER_L2_EN		0x0010
	#define AX_TXC0_PARSER_L2_PTP_EN	0x0020
	#define AX_TXC0_PARSER_IPV4_EN	0x0040
	#define AX_TXC0_PARSER_IPV6_EN	0x0080
	#define AX_TXC0_PARSER_VLAN_EN	0x0100
	#define AX_TXC0_PARSER_PPPOE_EN	0x0200
	#define AX_TXC0_PARSER_SNAP_EN	0x0400
	#define AX_TXC0_PARSER_UDP_EN		0x0800
	#define AX_TXC0_PARSER_UDPV6_EN	0x1000
	#define AX_TXC0_PARSER_UDP_PTP_EN	0x2000
	#define AX_TXC0_PARSER_UDPV6_PTP_EN	0x4000
	#define AX_TXC0_PARSER_EVENT_PORT_EN	0x00010000
	#define AX_TXC0_VAL_DELAY_CNT_SHIFT	24

#define AX_PTP_TX_CTRL0_DEFAULT	\
	(AX_PTP_TXC0_EN | AX_TXC0_PARSER_L2_EN | AX_TXC0_PARSER_L2_PTP_EN | \
	AX_TXC0_PARSER_IPV4_EN | AX_TXC0_PARSER_IPV6_EN | \
	AX_TXC0_PARSER_VLAN_EN | AX_TXC0_PARSER_PPPOE_EN | \
	AX_TXC0_PARSER_SNAP_EN | AX_TXC0_PARSER_UDP_EN | \
	AX_TXC0_PARSER_UDPV6_EN | AX_TXC0_PARSER_UDP_PTP_EN | \
	AX_TXC0_PARSER_UDPV6_PTP_EN | AX_TXC0_PARSER_EVENT_PORT_EN)
#endif


struct _ptp_header {
	u16	message_type		:4,
		transport_specific	:4,
		reserved		:4,
		version_ptp		:4;
	u16	message_len;
	u8	domain_num;
	u8	reserved1;
	u16	flags;
	u64	correction_field;
	u32	reserved2;
	u8	source_port_id[10];
	u16	sequence_id;
	u8	ctrl_field;
	u8	log_msg_interval;
} __packed;

struct _179a_ptp_info {
	u8	reserved	:3,
		status		:1,
		msg_type	:4;
	u8	sequence_id;
	u32	nsec;
	u32	sec_l;
	u16	sec_h;
} __packed;
#define AX_179A_PTP_INFO_SEG_SIZE sizeof(struct _179a_ptp_info)

struct _ax_ptp_info {
	u8	reserved	:3,
		status		:1,
		msg_type	:4;
	u16	sequence_id;
	u32	nsec;
	u32	sec_l;
	u16	sec_h;
} __packed;
#define AX_PTP_HW_QUEUE_SIZE	5
#define AX_PTP_QUEUE_SIZE	AX_PTP_HW_QUEUE_SIZE
#define AX_PTP_INFO_SIZE	sizeof(struct _ax_ptp_info)

struct _ax_ptp_usb_info {
	struct _ax_ptp_info ax_ptp_info[AX_PTP_HW_QUEUE_SIZE];
	struct usb_ctrlrequest req;
	void *axdev;
};

struct ax_ptp_cfg {
	void *axdev;
	struct ptp_clock_info ptp_caps;
	struct ptp_clock *ptp_clock;
	unsigned int phc_index;
	struct _ax_ptp_info tx_ptp_info[AX_PTP_QUEUE_SIZE];
	unsigned long ptp_head, ptp_tail, num_items;
	int get_timestamp_retry;
#ifdef ENABLE_AX88279
#define AX_PTP_EP4_SIZE	((2 * AX_PTP_INFO_SIZE * AX_PTP_HW_QUEUE_SIZE) + 1)
#define AX_TS_SEG_1		1
#define AX_EP4_INFO_SIZE (AX_PTP_QUEUE_SIZE * AX_PTP_EP4_SIZE)
	struct urb *urb;
	unsigned char ep4_buf[AX_PTP_EP4_SIZE];
	struct _ax_ptp_info ep4_ptp_info[AX_EP4_INFO_SIZE];
#endif
};

int ax_ptp_register(struct ax_device *axdev);
void ax_ptp_unregister(struct ax_device *axdev);
int ax88179a_ptp_pps_ctrl(struct ax_device *axdev, u8 enable);
int ax88179a_ptp_init(struct ax_device *axdev);
void ax88179a_ptp_remove(struct ax_device *axdev);
#ifdef ENABLE_AX88279
int ax88279_ptp_pps_ctrl(struct ax_device *axdev, u8 enable);
int ax88279_ptp_init(struct ax_device *axdev);
void ax88279_ptp_remove(struct ax_device *axdev);
int ax88279_start_get_ts(struct ax_device *axdev);
void ax88279_stop_get_ts(struct ax_device *axdev);
#endif
int ax_ptp_ts_read_cmd_async(struct ax_device *axdev);
void ax_rx_get_timestamp(struct sk_buff *skb, u64 *pkt_hdr);
#endif /* End of __ASIX_PTP_H */
