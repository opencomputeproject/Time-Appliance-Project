// SPDX-License-Identifier: GPL-2.0
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
#include "ax_main.h"
#include "ax_ptp.h"
#include "ax88179a_772d.h"

#define ptp_to_dev(ptp) container_of(ptp, struct ax_ptp_cfg, ptp_caps)

static void ax_reset_ptp_queue(struct ax_device *axdev)
{
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;

	if (!ptp_cfg)
		return;

	ptp_cfg->ptp_head = 0;
	ptp_cfg->ptp_tail = 0;
	ptp_cfg->num_items = 0;
	ptp_cfg->get_timestamp_retry = 0;

	memset(ptp_cfg->tx_ptp_info, 0, AX_PTP_INFO_SIZE * AX_PTP_QUEUE_SIZE);
}

static int ax88179a_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u32 remainder = 0;
	u64 high_timer = 0;
	s64 sec = 0, rnsec = 0;
	s64 nsec = 0;
	u8 timestamp[10] = {0};
	int ret;

	ret = ax_read_cmd(axdev, AX_PTP_OP, AX_GET_LOCAL_CLOCK, 0,
			  AX_GET_LOCAL_CLOCK_SIZE, &timestamp, 0);
	if (ret < 0)
		return ret;
	memcpy(&nsec, timestamp, 4);
	memcpy(&sec, &timestamp[4], 6);
	sec *= NSEC_PER_SEC;
	rnsec = (nsec + sec + delta);

	high_timer = div_u64_rem(rnsec, NSEC_PER_SEC, &remainder);
	memcpy(timestamp, &remainder, 4);
	memcpy(&timestamp[4], &high_timer, 6);

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_LOCAL_CLOCK, 0,
			    AX_SET_LOCAL_CLOCK_SIZE, &timestamp);
	if (ret < 0)
		return ret;


	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
static int
ax88179a_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
#else
static int
ax88179a_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
	long ppb = scaled_ppm_to_ppb(scaled_ppm);
#endif
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u32 new_addend_val;
	u64 adjust_val;
	int neg_adj = 0, ret;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	adjust_val = AX_BASE_ADDEND;
	adjust_val *= ppb;
	adjust_val = div_u64(adjust_val, NSEC_PER_SEC);

	if (neg_adj)
		new_addend_val = (u32)(AX_BASE_ADDEND - adjust_val);
	else
		new_addend_val = (u32)(AX_BASE_ADDEND + adjust_val);

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_ADDEND, 0,
			   AX_SET_ADDEND_SIZE, &new_addend_val);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax88179a_ptp_gettime64
(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u64 sec = 0;
	u32 nsec = 0;
	u8 timestamp[10] = {0};
	int ret;

	ret = ax_read_cmd(axdev, AX_PTP_OP, AX_GET_LOCAL_CLOCK, 0,
			  AX_GET_LOCAL_CLOCK_SIZE, &timestamp, 0);
	if (ret < 0)
		return ret;

	memcpy(&nsec, timestamp, 4);
	memcpy(&sec, &timestamp[4], 6);
	ts->tv_nsec = nsec;
	ts->tv_sec = sec;

	return 0;
}

static int ax88179a_ptp_settime64
(struct ptp_clock_info *ptp, const struct timespec64 *ts)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u64 sec;
	u32 nsec;
	u8 timestamp[10] = {0};
	int ret;

	nsec = (u32)ts->tv_nsec;
	memcpy(timestamp, &nsec, 4);
	sec = (u64)ts->tv_sec;
	memcpy(&timestamp[4], &sec, 6);

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_LOCAL_CLOCK, 0,
			    AX_SET_LOCAL_CLOCK_SIZE, &timestamp);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax_ptp_enable(struct ptp_clock_info *ptp,
			 struct ptp_clock_request *rq, int on)
{
	return -EOPNOTSUPP;
}

static struct ptp_clock_info ax88179a_772d_ptp_clock = {
	.owner		= THIS_MODULE,
	.name		= "asix ptp",
	.max_adj	= 100000000,
	.n_ext_ts	= 0,
	.pps		= 0,
#if KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE
	.adjfine	= NULL,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
	.adjfine	= ax88179a_ptp_adjfine,
#else
	.adjfreq	= ax88179a_ptp_adjfreq,
#endif
	.adjtime	= ax88179a_ptp_adjtime,
	.gettime64	= ax88179a_ptp_gettime64,
	.settime64	= ax88179a_ptp_settime64,
	.n_per_out	= 0,
	.enable		= ax_ptp_enable,
	.n_pins		= 0,
	.verify		= NULL,
	.pin_config	= NULL,
};

int ax88179a_ptp_init(struct ax_device *axdev)
{
	struct ax_link_info *link_info = &axdev->link_info;
	u32 new_addend_val = AX_BASE_ADDEND;
	u8 reg8;
	u8 ptpset;
	u8 ptp_tx_delay[5] = { 0 };
	u8 ptp_rx_delay[5] = { 0 };
	u32 reg32;
	u32 timeout = 0;
	int ret;
#ifdef ENABLE_PTP_FUNC
	axdev->driver_info->ptp_pps_ctrl(axdev, 1);
#endif
	if (axdev->sub_version < 3)
		return 0;

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_ADDEND, 0,
			   AX_SET_ADDEND_SIZE, &new_addend_val);
	if (ret < 0)
		return ret;

	reg8 = AX_PTP_PERIOD;
	ret = ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_TIMER_PERIOD,
			    0, 1, &reg8);
	if (ret < 0)
		return ret;

	ret = ax_read_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_CTRL_2,
			   0, 1, &reg8, 0);
	if (ret < 0)
		return ret;
	reg8 |= AX_PTP_CTRL_SET_PERIOD;
	ret = ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_CTRL_2, 0, 1, &reg8);
	if (ret < 0)
		return ret;

	ret = ax_read_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_CTRL_1,
			   0, 1, &ptpset, 0);
	if (ret < 0)
		return ret;
	ptpset |= AX_PTP_CTRL_L3_EN | AX_PTP_CTRL_EN | AX_PTP_TX_PLUS_DELAY |
		  AX_PTP_TX_FILTER_GENERAL_MSG | AX_PTP_RX_FILTER_GENERAL_MSG;
	ret = ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_CTRL_1,
			    0, 1, &ptpset);
	if (ret < 0)
		return ret;

	reg8 = AX_179A_PTP_INFO_SEG_SIZE * AX_PTP_HW_QUEUE_SIZE;
	ret = ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_MEM_SEG_SIZE,
			   0, 1, &reg8);
	if (ret < 0)
		return ret;

	do {
		reg8 = 0;
		ret = ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_MEM_SEG_SET,
				   0, 1, &reg8);
		if (ret < 0)
			return ret;

		ret = ax_read_cmd(axdev, AX_PTP_CMD,
				  AX88179A_PTP_MEM_SEG_STATUS, 0, 1, &reg8, 0);
		if (ret < 0)
			return ret;
		reg8 &= AX_PTP_MEM_SEG_MASK;

		if (timeout++ > 5)
			break;
	} while (reg8 != AX_PTP_MEM_SEG_0);

	reg32 = AX_PPS_ACTIVE_DEFAULT_TIME;
	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_ACTIVE_TIME, 0,
			   AX_SET_ACTIVE_TIME_SIZE, &reg32);
	if (ret < 0)
		return ret;

	switch (link_info->eth_speed) {
	case ETHER_LINK_100:
		ptp_tx_delay[0] = 0x64;
		ptp_tx_delay[4] = ptpset;
		ptp_rx_delay[0] = 0x22;
		ptp_rx_delay[1] = 0x01;
		ptp_rx_delay[4] = ptpset;
		break;
	case ETHER_LINK_10:
		ptp_tx_delay[0] = 0xB7;
		ptp_tx_delay[1] = 0x0F;
		ptp_tx_delay[4] = ptpset;
		ptp_rx_delay[0] = 0xAC;
		ptp_rx_delay[1] = 0x0A;
		ptp_rx_delay[4] = ptpset;
		break;
	case ETHER_LINK_1000:
	default:
		ptp_tx_delay[0] = 0x6E;
		ptp_tx_delay[4] = ptpset;
		ptp_rx_delay[0] = 0xE4;
		ptp_rx_delay[4] = ptpset;
		break;
	}

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_TX_PHY_DELAY, 0,
			   AX_SET_TX_PHY_DELAY_SIZE, ptp_tx_delay);
	if (ret < 0)
		return ret;

	ret = ax_write_cmd(axdev, AX_PTP_OP, AX_SET_RX_PHY_DELAY, 0,
			   AX_SET_RX_PHY_DELAY_SIZE, ptp_rx_delay);
	if (ret < 0)
		return ret;

	axdev->netdev->features &= ~(NETIF_F_SG | NETIF_F_TSO);
	axdev->netdev->hw_features &= ~(NETIF_F_SG | NETIF_F_TSO);
	axdev->netdev->vlan_features &= ~(NETIF_F_SG | NETIF_F_TSO);

	return 0;
}

int ax88179a_ptp_pps_ctrl(struct ax_device *axdev, u8 enable)
{
	u32 reg32 = 0;
	int ret;
	
	ret = ax_read_cmd(axdev, AX88179A_PBUS_REG, 0x1894, 0x000F, 4, &reg32, 1);
	if (ret < 0)
		return ret;

	reg32 &= ~0x01000000;

	if (enable) 
		reg32 |= 0x01000000;

	ret = ax_write_cmd(axdev, AX88179A_PBUS_REG, 0x1894, 0x000F, 4, &reg32);
	if (ret < 0)
		return ret;
	
	return 0;
}

int ax88279_ptp_pps_ctrl(struct ax_device *axdev, u8 enable)
{
	u32 reg32 = 0;
	int ret;

	ret = ax_read_cmd(axdev, AX_PBUS_A32, 0xF8C8, 0x000C, 4, &reg32, 1);
	if (ret < 0)
		return ret;

	reg32 &= ~0x00004000;
	
	if (enable) 
		reg32 |= 0x00004000;

	ret = ax_write_cmd(axdev, AX_PBUS_A32, 0xF8C8, 0x000C, 4, &reg32);

	return 0;
}

void ax88179a_ptp_remove(struct ax_device *axdev)
{
	u8 reg8;
#ifdef ENABLE_PTP_FUNC
	axdev->driver_info->ptp_pps_ctrl(axdev, 0);
#endif
	if (axdev->sub_version < 3)
		return;

	ax_read_cmd(axdev, AX_PTP_CMD,  AX88179A_PTP_CTRL_1, 0, 1, &reg8, 0);
	reg8 &= ~(AX_PTP_CTRL_L3_EN | AX_PTP_CTRL_EN);
	ax_write_cmd(axdev, AX_PTP_CMD, AX88179A_PTP_CTRL_1, 0, 1, &reg8);
}

void ax_ptp_unregister(struct ax_device *axdev)
{
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;

	if (axdev->driver_info->ptp_remove)
		axdev->driver_info->ptp_remove(axdev);

	if (ptp_cfg) {
		if (ptp_cfg->ptp_clock)
			ptp_clock_unregister(ptp_cfg->ptp_clock);
	}
}

static u8 ax_find_ptp_item(struct ax_device *axdev, struct _ptp_header *ptp,
			   struct sk_buff *skb)
{
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;
	struct _ax_ptp_info *temp_ptp_info = ptp_cfg->tx_ptp_info;
	u16 sequence_id;
	u8 message_type = ptp->message_type;
	int i, read_ptr;

	read_ptr = ptp_cfg->ptp_head;
#ifdef ENABLE_PTP_DEBUG
	printk("%s - ptp_head: %d", __func__, read_ptr);
#endif

	if (axdev->chip_version == AX_VERSION_AX88179A_772D)
		sequence_id = ntohs(ptp->sequence_id) & 0xFF;
	else
		sequence_id = ntohs(ptp->sequence_id) & 0xFFFF;

	for (i = 0; i < ptp_cfg->num_items; i++) {
		if ((temp_ptp_info[read_ptr].sequence_id == sequence_id) &&
		    (temp_ptp_info[read_ptr].msg_type == message_type)) {
			struct skb_shared_hwtstamps shhwtstamps;
			u64 timestamp_h, timestamp_l, temp;
			u64 time64;

			ptp_cfg->num_items--;
			ptp_cfg->ptp_head++;
			if (ptp_cfg->ptp_head == AX_PTP_QUEUE_SIZE)
				ptp_cfg->ptp_head = 0;
			timestamp_l = temp_ptp_info[read_ptr].nsec;
			timestamp_h = temp_ptp_info[read_ptr].sec_l;
			temp = temp_ptp_info[read_ptr].sec_h;
			timestamp_h |= (temp << 32);
			time64 = timestamp_h * NSEC_PER_SEC;
			time64 += timestamp_l & 0xFFFFFFFF;
			memset(&shhwtstamps, 0, sizeof(shhwtstamps));
			shhwtstamps.hwtstamp = ns_to_ktime(time64);
			if (ptp->flags & 0x2 ||
			    (ptp->message_type != 0 && ptp->message_type != 3))
				skb_tstamp_tx(skb, &shhwtstamps);
#ifdef ENABLE_PTP_DEBUG
			printk("%s - skb_tstamp_tx return", __func__);
#endif
			dev_kfree_skb_any(skb);
			return 0;
		}
		read_ptr++;
		if (read_ptr == AX_PTP_QUEUE_SIZE)
			read_ptr = 0;
	}
	return AX_PTP_QUEUE_SIZE;
}

static void ax_tx_check_timestamp(struct ax_device *axdev, struct sk_buff *skb)
{
	if (skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) {
		struct _ptp_header ptp;
		unsigned int ptp_msg_offset;
		u16 tmp, tx_ethertype, vlan_id = 0;
		u8 vlan_size = 0, ptp_item = AX_PTP_QUEUE_SIZE;

		skb_copy_from_linear_data_offset(skb, AX_ETHTYPE_OFFSET,
						 &tmp, 2);
		tx_ethertype = ntohs(tmp);
		if (tx_ethertype == ETH_P_8021Q) {
			skb_copy_from_linear_data_offset(skb,
							 AX_ETHTYPE_OFFSET + 2,
							 &tmp, 2);
			vlan_id = ntohs(tmp) & 0xFFF;
			vlan_size = 4;
			skb_copy_from_linear_data_offset(skb,
							 AX_ETHTYPE_OFFSET + 4,
							 &tmp, 2);
			tx_ethertype = ntohs(tmp);
			if (tx_ethertype == ETH_P_8021Q) {
				vlan_size = 8;
				skb_copy_from_linear_data_offset(skb,
							 AX_ETHTYPE_OFFSET + 4,
							 &tmp, 2);
				tx_ethertype = ntohs(tmp);
			}
		}
		ptp_msg_offset = vlan_size;
		if (tx_ethertype == ETH_P_1588)
			ptp_msg_offset += AX_TX_PTPHDR_OFFSET_L2;
		else if (tx_ethertype == ETH_P_IP)
			ptp_msg_offset += AX_TX_PTPHDR_OFFSET_L3_IP;
		else if (tx_ethertype == ETH_P_IPV6)
			ptp_msg_offset += AX_TX_PTPHDR_OFFSET_L3_IPV6;
		else
			return;

		skb_copy_from_linear_data_offset(skb, ptp_msg_offset,
						 &ptp, PTP_HDR_SIZE);

		ptp_item = ax_find_ptp_item(axdev, &ptp, skb);
		if (ptp_item == AX_PTP_QUEUE_SIZE) {
			dev_err(&axdev->intf->dev,
				"Not found item from PTP queue");
			return;
		}
	}
}

static void ax_tx_timestamp(struct ax_device *axdev)
{
	struct sk_buff_head *tx_timestamp = &axdev->tx_timestamp;

	while (!skb_queue_empty(tx_timestamp)) {
		struct sk_buff *skb;

		skb = __skb_dequeue(tx_timestamp);
		if (!skb)
			return;

		ax_tx_check_timestamp(axdev, skb);
	}
}

static struct _ax_ptp_info *ax_ptp_info_transform(struct ax_device *axdev,
						  void *data)
{
	struct _ax_ptp_info temp[AX_PTP_HW_QUEUE_SIZE] = {0};
	int i;

	switch (axdev->chip_version) {
	case AX_VERSION_AX88179A_772D:
	{
		struct _179a_ptp_info *_179a_ptp = (typeof(_179a_ptp))data;

		for (i = 0; i < AX_PTP_HW_QUEUE_SIZE; i++) {
			memcpy(&temp[i], &_179a_ptp[i], 2);
			temp[i].sequence_id &= 0xFF;
			memcpy(&temp[i].nsec, &_179a_ptp[i].nsec, 10);
		}
		memcpy(data, temp, AX_PTP_INFO_SIZE);
		break;
	}
	};

	return (struct _ax_ptp_info *)data;
}


#if KERNEL_VERSION(2, 6, 20) > LINUX_VERSION_CODE
static void ax_ptp_ts_callback(struct urb *urb, struct pt_regs *regs)
#else
static void ax_ptp_ts_callback(struct urb *urb)
#endif
{
	struct _ax_ptp_usb_info *ptp_info = (typeof(ptp_info))urb->context;
	struct ax_device *axdev = (struct ax_device *)ptp_info->axdev;
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;
	struct _ax_ptp_info *temp_ptp_info = ptp_info->ax_ptp_info;
	int i, count = 0;
#ifdef ENABLE_PTP_DEBUG
	printk("%s - Start urb->actual_length: %d",
		__func__, urb->actual_length);
#endif
	if (urb->status < 0) {
		printk(KERN_ERR "failed get ts (%d)", urb->status);
		goto free;
	}

	temp_ptp_info = ax_ptp_info_transform(axdev, ptp_info->ax_ptp_info);
	if (temp_ptp_info == NULL) {
		printk(KERN_ERR "Failed to transform ptp info.");
		goto free;
	}

	for (i = 0; i < AX_PTP_HW_QUEUE_SIZE; i++) {
		if (temp_ptp_info[i].status) {
			ptp_cfg->tx_ptp_info[ptp_cfg->ptp_tail++] = temp_ptp_info[i];
			if (ptp_cfg->ptp_tail == AX_PTP_QUEUE_SIZE)
				ptp_cfg->ptp_tail = 0;
			ptp_cfg->num_items++;
#ifdef ENABLE_PTP_DEBUG
printk("### ptp_tail: %ld, ptp_head: %ld, num_items: %ld",
	ptp_cfg->ptp_tail, ptp_cfg->ptp_head, ptp_cfg->num_items);
printk("### (%s) - DATA %d -------------###", __func__, i);
printk("### status: %d", temp_ptp_info[i].status);
printk("### type: 0x%02x", temp_ptp_info[i].msg_type);
printk("### s_id: 0x%04x", temp_ptp_info[i].sequence_id);
printk("### nsec: 0x%08x", temp_ptp_info[i].nsec);
printk("###  sec: 0x%04x%08x", temp_ptp_info[i].sec_h, temp_ptp_info[i].sec_l);
printk("### ----------------------------###\n");
#endif
			count++;
			ptp_cfg->get_timestamp_retry = 0;
		}
	}
	if (count == 0 &&
	    ptp_cfg->get_timestamp_retry < EP0_GET_TIMESTAMP_RETRY) {
		ax_ptp_ts_read_cmd_async(axdev);
		ptp_cfg->get_timestamp_retry++;
		goto free;
	}

	if (ptp_cfg->get_timestamp_retry == EP0_GET_TIMESTAMP_RETRY)
		dev_err(&axdev->intf->dev, "Get timestamp failed.");

	ax_tx_timestamp(axdev);
free:
	kfree(temp_ptp_info);
	usb_free_urb(urb);
}

int ax_ptp_ts_read_cmd_async(struct ax_device *axdev)
{
	struct usb_ctrlrequest *req;
	int status = 0;
	struct urb *urb;
	struct _ax_ptp_usb_info *info;
	u16 size = AX_PTP_INFO_SIZE * AX_PTP_HW_QUEUE_SIZE;

	if (axdev->chip_version > AX_VERSION_AX88179A_772D)
		return 0;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (urb == NULL) {
		dev_err(&axdev->intf->dev,
			   "Error allocating URB in write_cmd_async!");
		return -ENOMEM;
	}

	info = kzalloc(sizeof(struct _ax_ptp_usb_info), GFP_ATOMIC);
	if (!info) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	info->axdev = axdev;
	req = &info->req;

	req->bRequestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
	req->bRequest = AX_PTP_TIMESTAMP;
	req->wValue = cpu_to_le16(0);
	req->wIndex = cpu_to_le16(0);
	req->wLength = cpu_to_le16(size);

	memset(info->ax_ptp_info, 0, AX_PTP_INFO_SIZE * AX_PTP_QUEUE_SIZE);

	usb_fill_control_urb(urb, axdev->udev,
			     usb_sndctrlpipe(axdev->udev, 0),
			     (void *)req, info->ax_ptp_info, size,
			     ax_ptp_ts_callback, info);

	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status < 0) {
		dev_err(&axdev->intf->dev,
			   "Error submitting the control message: status=%d",
			   status);
		kfree(info);
		usb_free_urb(urb);
	}

	return 0;
}

void ax_rx_get_timestamp(struct sk_buff *skb, u64 *pkt_hdr)
{
	struct skb_shared_hwtstamps *shhwtstamps = skb_hwtstamps(skb);
	u64 timestamp_h, timestamp_l;
	u64 time64;

	timestamp_l = *((u64 *)(++pkt_hdr));
	timestamp_h = *((u64 *)(++pkt_hdr));
#ifdef ENABLE_PTP_DEBUG
	printk("### (%s) h: 0x%llx, l: 0x%llx %lld###", __func__,
		timestamp_h, timestamp_l, timestamp_l);
#endif
	timestamp_h <<= 32;
	timestamp_h |= (timestamp_l >> 32) & 0xFFFFFFFF;
	time64 = (timestamp_h * NSEC_PER_SEC);
	time64 += (timestamp_l & 0xFFFFFFFF);
#ifdef ENABLE_PTP_DEBUG
	printk("### (%s) h: %lld, time64: %lld ###", __func__,
		timestamp_h, time64);
#endif
	memset(shhwtstamps, 0, sizeof(struct skb_shared_hwtstamps));
	shhwtstamps->hwtstamp = ns_to_ktime(time64);
}

#ifdef ENABLE_AX88279
static int ax_ptp_pbus_write(struct ax_device *axdev, u16 offset, u16 len,
			     void *data)
{
	int ret = 0;

	ret = ax_write_cmd(axdev,
			    AX_PBUS_A32,
			    offset,
			    AX_PTP_REG_BASE_ADDR_HI,
			    len,
			    data);

	if (ret < 0)
		return ret;
	return 0;
}

static int ax_ptp_clk_write(struct ax_device *axdev, u16 offset, u16 len,
			    void *data)
{
	int ret = 0;

	ret = ax_write_cmd(axdev,
			    AX_PBUS_A32,
			    offset,
			    AX_PTP_REG_BASE_ADDR_HI,
			    len,
			    data);

	if (ret < 0)
		return ret;
	return 0;
}

static int ax_ptp_clk_read(struct ax_device *axdev, u16 offset, u16 len,
			   void *data)
{
	int ret = 0;

	ret = ax_read_cmd(axdev,
			   AX_PTP_CLK,
			   0x0002,
			   offset,
			   len,
			   data,
			   0);

	if (ret < 0)
		return ret;
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
static int ax88279_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
#else
static int ax88279_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
	long ppb = scaled_ppm_to_ppb(scaled_ppm);
#endif
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u32 new_addend_val;
	u64 adjust_val;
	int neg_adj = 0, ret;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	adjust_val = AX_BASE_ADDEND;
	adjust_val *= ppb;
	adjust_val = div_u64(adjust_val, NSEC_PER_SEC);

	if (neg_adj)
		new_addend_val = (u32)(AX_BASE_ADDEND - adjust_val);
	else
		new_addend_val = (u32)(AX_BASE_ADDEND + adjust_val);

	ret = ax_ptp_pbus_write(axdev,
			   AX_PTP_TIMER_ADDEND,
			   sizeof(new_addend_val),
			   &new_addend_val);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax88279_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u32 remainder = 0;
	u64 high_timer = 0;
	s64 sec = 0;
	s64 nsec = 0;
	u8 timestamp[12] = {0};
	int ret;

	ret = ax_ptp_clk_read(axdev, AX_PTP_GET_80B_LCK_VAL0, 10, timestamp);
	if (ret < 0)
		return ret;

	memcpy(&nsec, timestamp, 4);
	memcpy(&sec, &timestamp[4], 6);
	sec *= NSEC_PER_SEC;
	nsec = (sec + delta);

	high_timer = div_u64_rem(nsec, NSEC_PER_SEC, &remainder);
	memcpy(timestamp, &remainder, 4);
	memcpy(&timestamp[4], &high_timer, 6);

	ret = ax_ptp_clk_write(axdev, AX_PTP_SET_80B_LCK_VAL0, 12, timestamp);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax88279_ptp_gettime64(struct ptp_clock_info *ptp,
				struct timespec64 *ts)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u64 sec = 0;
	u32 nsec = 0;
	u8 timestamp[12] = {0};
	int ret;

	ret = ax_ptp_clk_read(axdev, AX_PTP_GET_80B_LCK_VAL0, 10, timestamp);
	if (ret < 0)
		return ret;

	memcpy(&nsec, timestamp, 4);
	memcpy(&sec, &timestamp[4], 6);
	ts->tv_nsec = nsec;
	ts->tv_sec = sec;

	return 0;
}

static int ax88279_ptp_settime64(struct ptp_clock_info *ptp,
				const struct timespec64 *ts)
{
	struct ax_ptp_cfg *ptp_cfg = ptp_to_dev(ptp);
	struct ax_device *axdev = (struct ax_device *)ptp_cfg->axdev;
	u64 sec;
	u32 nsec;
	u8 timestamp[10] = {0};
	int ret;

	nsec = (u32)ts->tv_nsec;
	memcpy(timestamp, &nsec, 4);
	sec = (u64)ts->tv_sec;
	memcpy(&timestamp[4], &sec, 6);

	ret = ax_ptp_clk_write(axdev, AX_PTP_SET_80B_LCK_VAL0, 10, timestamp);
	if (ret < 0)
		return ret;

	return 0;
}

static struct ptp_clock_info ax88279_ptp_clock = {
	.owner		= THIS_MODULE,
	.name		= "asix ptp",
	.max_adj	= 100000000,
	.n_ext_ts	= 0,
	.pps		= 0,
#if KERNEL_VERSION(4, 10, 0) <= LINUX_VERSION_CODE
	.adjfine	= NULL,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,2,0)
	.adjfine	= ax88279_ptp_adjfine,
#else
	.adjfreq	= ax88279_ptp_adjfreq,
#endif
	.adjtime	= ax88279_ptp_adjtime,
	.gettime64	= ax88279_ptp_gettime64,
	.settime64	= ax88279_ptp_settime64,
	.n_per_out	= 0,
	.enable		= ax_ptp_enable,
	.n_pins		= 0,
	.verify		= NULL,
	.pin_config	= NULL,
};

int ax88279_ptp_init(struct ax_device *axdev)
{
	struct ax_link_info *link_info = &axdev->link_info;
	u32 reg32;
	u8 reg8;
	int ret;

	ax_reset_ptp_queue(axdev);

#ifdef ENABLE_PTP_FUNC
	axdev->driver_info->ptp_pps_ctrl(axdev, 1);
#endif

	reg32 = (AX_PTP_MEM_SEG_SIZE_279_5 << 24) |
		(AX_PTP_MEM_START_ADDR << 8) | AX_PTP_PTP_CPU_EN;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_TX_MEM_SETTING, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = AX_PPS_ACTIVE_DEFAULT_TIME;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_PPS_ACTIVE_TIME, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = AX_PTP_LCK_CTRL0_EN | AX_PTP_LCK_CTRL0_80B_NS_EN |
		AX_PTP_LCK_CTRL0_80B_S_EN | AX_PTP_LCK_CTRL0_48B_EN |
		AX_PTP_LCK_CTRL0_PPS_EN | AX_PTP_LCK_CTRL0_TX_DEL_VEC;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_LCK_CTRL0, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = AX_BASE_ADDEND;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_TIMER_ADDEND, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = AX_PTP_PERIOD;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_TIMER_PERIOD, 4, &reg32);
	if (ret < 0)
		return ret;

	ret = ax_read_cmd(axdev, AX_ACCESS_MAC, AX_MAC_BFM_CTRL, 1, 1, &reg8, 0);
	if (ret < 0)
		return ret;
	reg8 |= AX_CS_TRAIL_UDPV4_EN | AX_CS_TRAIL_UDPV6_EN;
	ret = ax_write_cmd(axdev, AX_ACCESS_MAC, AX_MAC_BFM_CTRL, 1, 1, &reg8);
	if (ret < 0)
		return ret;

	switch (link_info->eth_speed) {
	case ETHER_LINK_100:
		reg32 = (AX_IPG_COUNTER_100M) |
			(AX_SOF_DELAY_COUNTER_100M << 8) |
			(AX_VAILD_DELAY_COUNTER_100M << 16);
		ret = ax_write_cmd(axdev, AX_PBUS_A32, AX_TX_READY_CTRL,
				   AX_PBUS_REG_BASE_ADDR_HI, 4, &reg32);
		if (ret < 0)
			return ret;

		reg32 = AX_PTP_RX_CTRL0_DEFAULT;
		ret = ax_ptp_pbus_write(axdev, AX_PTP_RX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;

		reg32 = AX_PTP_TX_CTRL0_DEFAULT |
			(0x10 << AX_TXC0_VAL_DELAY_CNT_SHIFT);
		ret = ax_ptp_pbus_write(axdev, AX_PTP_TX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;
		break;
	case ETHER_LINK_1000:
		reg32 = (AX_IPG_COUNTER_1G) |
			(AX_SOF_DELAY_COUNTER_1G << 8) |
			(AX_VAILD_DELAY_COUNTER_1G << 16);
		ret = ax_write_cmd(axdev, AX_PBUS_A32, AX_TX_READY_CTRL,
				   AX_PBUS_REG_BASE_ADDR_HI, 4, &reg32);
		if (ret < 0)
			return ret;

		reg32 = AX_PTP_RX_CTRL0_DEFAULT;
		ret = ax_ptp_pbus_write(axdev, AX_PTP_RX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;

		reg32 = AX_PTP_TX_CTRL0_DEFAULT |
			(0x8 << AX_TXC0_VAL_DELAY_CNT_SHIFT);
		ret = ax_ptp_pbus_write(axdev, AX_PTP_TX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;
		break;
	case ETHER_LINK_2500:
		reg32 = AX_PTP_RX_CTRL0_DEFAULT | AX_PTP_RXC0_XGMII_EN;
		ret = ax_ptp_pbus_write(axdev, AX_PTP_RX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;

		reg32 = AX_PTP_TX_CTRL0_DEFAULT | AX_PTP_TXC0_XGMII_EN |
			(0x8 << AX_TXC0_VAL_DELAY_CNT_SHIFT);
		ret = ax_ptp_pbus_write(axdev, AX_PTP_TX_CTRL0, 4, &reg32);
		if (ret < 0)
			return ret;
		break;
	default:
		break;
	}

	reg32 = 0;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_TX_DELAY, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = 0;
	ret = ax_ptp_pbus_write(axdev, AX_PTP_RX_DELAY, 4, &reg32);
	if (ret < 0)
		return ret;

	reg8 = AX_EXT_INT_ON | AX_PTP_TX_TX_INT_EN;
	ret = ax_write_cmd(axdev, AX_PTP_TOD_CTRL,
			   (AX_PTP_TS_INT | AX_EXT_INT), 0, 1, &reg8);
	if (ret < 0)
		return ret;

	reg32 = 0;
	ret = ax_write_cmd(axdev, AX_PBUS_A32, AX_MAC_CLK_CTRL,
			   AX_PBUS_REG_BASE_ADDR_HI, 4, &reg32);
	if (ret < 0)
		return ret;

	reg32 = (0 << AX_DIVIDE_PTP_CLK_SHIFT) |
		(1 << AX_DIVIDE_AES_CLK_SHIFT) |
		AX_PTP_CLK_EN | AX_AES_CLK_EN |
		AX_PTP_CLK_SELECT_DIVIDE | AX_AES_CLK_SELECT_DIVIDE |
		AX_XGMAC_TX_CLK_EN | AX_XGMAC_RX_CLK_EN;
	ret = ax_write_cmd(axdev, AX_PBUS_A32, AX_MAC_CLK_CTRL,
			   AX_PBUS_REG_BASE_ADDR_HI, 4, &reg32);
	if (ret < 0)
		return ret;

	return 0;
}

void ax88279_ptp_remove(struct ax_device *axdev)
{
	u32 reg32 = 0;
#ifdef ENABLE_PTP_FUNC
	axdev->driver_info->ptp_pps_ctrl(axdev, 0);
#endif
	ax_ptp_pbus_write(axdev, AX_PTP_LCK_CTRL0, 4, &reg32);
	ax_ptp_pbus_write(axdev, AX_PTP_RX_CTRL0, 4, &reg32);
	ax_ptp_pbus_write(axdev, AX_PTP_TX_CTRL0, 4, &reg32);
}

static int ax88279_submit_ts(struct ax_device *axdev);
static void ax88279_read_ts_callback(struct urb *urb)
{
	struct net_device *netdev;
	struct ax_device *axdev;
	struct ax_ptp_cfg *ptp_cfg;
	struct _ax_ptp_info *temp_ptp_info;
	int i, index;

	axdev = urb->context;
	if (!axdev)
		return;

	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags))
		return;

	netdev = axdev->netdev;
	if (!netif_carrier_ok(netdev))
		return;

	usb_mark_last_busy(axdev->udev);

	ptp_cfg = axdev->ptp_cfg;

	if (urb->status < 0) {
		dev_err(&axdev->intf->dev,
			"failed get ts (%d)", urb->status);
		goto out;
	}
#ifdef ENABLE_PTP_DEBUG
	printk("EP4 Valid: 0x%x", ptp_cfg->ep4_buf[AX_PTP_EP4_SIZE - 1]);
#endif
	index = (ptp_cfg->ep4_buf[AX_PTP_EP4_SIZE - 1] & AX_TS_SEG_1) ?
		0 : (AX_PTP_INFO_SIZE * AX_PTP_HW_QUEUE_SIZE);
#ifdef ENABLE_PTP_DEBUG
	printk("index: %d", index);
#endif
	temp_ptp_info = (struct _ax_ptp_info *)&ptp_cfg->ep4_buf[index];
	for (i = 0; i < AX_PTP_HW_QUEUE_SIZE; i++) {
		if (temp_ptp_info[i].status) {
			ptp_cfg->tx_ptp_info[ptp_cfg->ptp_tail++] =
							temp_ptp_info[i];
			if (ptp_cfg->ptp_tail == AX_PTP_QUEUE_SIZE)
				ptp_cfg->ptp_tail = 0;
			ptp_cfg->num_items++;
#ifdef ENABLE_PTP_DEBUG
printk("### ptp_tail: %ld, ptp_head: %ld, num_items: %ld",
	ptp_cfg->ptp_tail, ptp_cfg->ptp_head, ptp_cfg->num_items);
printk("### (%s) - DATA %d -------------###", __func__, i);
printk("### status: %d", temp_ptp_info[i].status);
printk("### type: 0x%02x", temp_ptp_info[i].msg_type);
printk("### s_id: 0x%04x", temp_ptp_info[i].sequence_id);
printk("### nsec: 0x%08x", temp_ptp_info[i].nsec);
printk("###  sec: 0x%04x%08x", temp_ptp_info[i].sec_h, temp_ptp_info[i].sec_l);
printk("### ----------------------------###\n");
#endif
		}
	}

	ax_tx_timestamp(axdev);
out:
	ax88279_submit_ts(axdev);
}

static int ax88279_submit_ts(struct ax_device *axdev)
{
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;
	struct urb *urb = ptp_cfg->urb;
	int ret;

	if (test_bit(AX_UNPLUG, &axdev->flags) ||
	    !test_bit(AX_ENABLE, &axdev->flags))
		return 0;

	memset(ptp_cfg->ep4_buf, 0, AX_PTP_EP4_SIZE);

	usb_fill_bulk_urb(urb, axdev->udev,
			   usb_rcvbulkpipe(axdev->udev, 4),
			   (void *)ptp_cfg->ep4_buf, AX_PTP_EP4_SIZE,
			   (usb_complete_t)ax88279_read_ts_callback, axdev);

	ret = usb_submit_urb(urb, GFP_KERNEL);
	if (ret == -ENODEV)
		netif_device_detach(axdev->netdev);

	urb->actual_length = 0;

	return ret;
}

int ax88279_start_get_ts(struct ax_device *axdev)
{
	int ret;

	if (axdev->chip_version <= AX_VERSION_AX88179A_772D)
		return 0;

	ret = ax88279_submit_ts(axdev);
	if (ret < 0)
		dev_err(&axdev->intf->dev, "Failed to submit EP4 for TS\n");

	return ret;
}

void ax88279_stop_get_ts(struct ax_device *axdev)
{
	struct ax_ptp_cfg *ptp_cfg = axdev->ptp_cfg;

	if (axdev->chip_version <= AX_VERSION_AX88179A_772D)
		return;

	if (ptp_cfg->urb)
		usb_kill_urb(ptp_cfg->urb);
}
#endif

int ax_ptp_register(struct ax_device *axdev)
{
	struct ax_ptp_cfg *ptp_cfg;
	int ret;

	ptp_cfg = kzalloc(sizeof(struct ax_ptp_cfg), GFP_KERNEL);
	if (!ptp_cfg)
		return -ENOMEM;
	axdev->ptp_cfg = ptp_cfg;

	switch (axdev->chip_version) {
#ifdef ENABLE_AX88279
	case AX_VERSION_AX88279:
		ptp_cfg->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!ptp_cfg->urb)
			goto fail;

		ptp_cfg->ptp_caps = ax88279_ptp_clock;
		break;
#endif
	case AX_VERSION_AX88179A_772D:
		if (axdev->sub_version < 3)
			return 0;
		ptp_cfg->ptp_caps = ax88179a_772d_ptp_clock;
		break;
	default:
		return 0;
	};

	ptp_cfg->ptp_clock = ptp_clock_register(&ptp_cfg->ptp_caps,
						&axdev->udev->dev);
	if (IS_ERR(ptp_cfg->ptp_clock)) {
		dev_err(&axdev->intf->dev, "ptp_clock_register failed\n");
		ret = PTR_ERR(ptp_cfg->ptp_clock);
		goto fail;
	}

	ptp_cfg->phc_index = ptp_clock_index(ptp_cfg->ptp_clock);
	skb_queue_head_init(&axdev->tx_timestamp);

	ptp_cfg->axdev = axdev;

	return 0;
fail:
#ifdef ENABLE_AX88279
	if (ptp_cfg->urb)
		usb_free_urb(axdev->intr_urb);
#endif
	kfree(axdev->ptp_cfg);

	return ret;
}
