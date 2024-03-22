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
#ifndef __ASIX_MAIN_H
#define __ASIX_MAIN_H

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <uapi/linux/mdio.h>
#include <linux/mdio.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/usb.h>
#include <linux/if_vlan.h>
#include <linux/usb/cdc.h>
#include <linux/suspend.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>
#include <linux/efi.h>
#include <linux/crc32.h>
#include <linux/time.h>
#include "ax_ioctl.h"

#define napi_alloc_skb(napi, length) netdev_alloc_skb_ip_align(netdev, length)
#define napi_complete_done(n, d) napi_complete(n)

typedef int (*_usb_read_function)(struct ax_device *dev, u8 cmd, u8 reqtype,
				  u16 value, u16 index, void *data, u16 size);
typedef int (*_usb_write_function)(struct ax_device *dev, u8 cmd, u8 reqtype,
				   u16 value, u16 index, const void *data,
				   u16 size);
typedef int (*usb_read_function)(struct ax_device *axdev, u8 cmd, u16 value,
				 u16 index, u16 size, void *data, int eflag);
typedef int (*usb_write_function)(struct ax_device *axdev, u8 cmd, u16 value,
				  u16 index, u16 size, void *data);

#define USB_VENDOR_ID_ASIX		0x0B95
#define USB_VENDOR_ID_SITECOM		0x0DF6
#define USB_VENDOR_ID_LENOVO		0x17EF
#define USB_VENDOR_ID_TOSHIBA		0x0930
#define USB_VENDOR_ID_SAMSUNG		0x04E8
#define USB_VENDOR_ID_DLINK		0x2001
#define USB_VENDOR_ID_MAGIC_CONTROL	0x0711

#define AX_DEVICE_ID_179X	0x1790
#define AX_BCDDEVICE_ID_179	0x0100
#define AX_BCDDEVICE_ID_179A	0x0200
#define AX_DEVICE_ID_178A	0x178A
#define AX_BCDDEVICE_ID_178A	0x0100
#define AX_DEVICE_ID_772D	AX_DEVICE_ID_179X
#define AX_BCDDEVICE_ID_772D	0x0300
#ifdef ENABLE_AX88279
#define AX_BCDDEVICE_ID_279	0x0400
#endif

#define AX_DRIVER_STRING_179_178A \
				"ASIX AX88179_178A USB Ethernet Controller"
#define AX_DRIVER_STRING_179A_772D \
				"ASIX AX88179B_179A_772E_772D USB Ethernet Controller"
#ifdef ENABLE_AX88279
#define AX_DRIVER_STRING_279 \
				"ASIX AX88279 USB Ethernet Controller"
#endif

#define DRIVER_VERSION		"3.1.0 beta8"
#define DRIVER_AUTHOR		"ASIX"
#define DRIVER_DESC		"ASIX USB Ethernet Controller"
#define MODULENAME		"ax_usb_nic"

#define PRINT_VERSION(axdev, str) \
	dev_info(&axdev->intf->dev, \
		"%s %s (%d.%d.%d.%d_%d.%d)", \
		str, DRIVER_VERSION, \
		axdev->fw_version[0], \
		axdev->fw_version[1], \
		axdev->fw_version[2], \
		axdev->fw_version[3], \
		axdev->chip_version, \
		axdev->sub_version)

#define AX88179_MAX_TX		4
#define AX88179_MAX_RX		10
#define AX88179_BUF_TX_SIZE	(81 * 1024)
#define AX_GSO_DEFAULT_SIZE	(16 * 1024)
#define INTBUFSIZE		8
#define TX_ALIGN		4
#define RX_ALIGN		8
#define TX_CASECADES_SIZE	AX_GSO_DEFAULT_SIZE

#define AX_TX_HEADER_LEN	8
#define AX_TX_TIMEOUT		(5 * HZ)
#define AX_MCAST_FILTER_SIZE	8
#define AX_MAX_MCAST		64
#ifdef ENABLE_QUEUE_PRIORITY
#define AX_TX_QUEUE_SIZE	2
#else
#define AX_TX_QUEUE_SIZE	1
#endif

#define US_TO_NS		1000

#define AX_ACCESS_MAC			0x01
#define AX_ACCESS_PHY			0x02
#define AX_ACCESS_WAKEUP		0x03
#define AX_ACCESS_EEPROM		0x04
#define AX_ACCESS_EFUSE			0x05
#define AX_RELOAD_EEPROM_EFUSE		0x06
#define AX_RELOAD_FLASH_EFUSE		0x06
#define AX_FW_MODE			0x08
	#define AX_FW_MODE_179A			0x0001
	#define AX_USB_EP5_EN			0x0001
#ifdef ENABLE_AX88279
#ifdef ENABLE_PTP_FUNC
	#define AX_USB_EP4_EN			0x0002
#endif
#endif
#define AX_WRITE_EFUSE_EN		0x09
#define AX_WRITE_EFUSE_DIS		0x0A
#define AX_ACCESS_MFAB			0x10
#define AX_PHY_POLLING			0x90

#define PHYSICAL_LINK_STATUS		0x02
	#define	AX_USB_SS			0x04
	#define	AX_USB_HS			0x02
	#define	AX_USB_FS			0x01
#define GENERAL_STATUS			0x03
	#define	AX_SECLD			0x04
#define AX_CHIP_STATUS			0x05
	#define AX_CHIP_CODE_MASK		0x70
	#define CHIP_CODE(x)		((x & AX_CHIP_CODE_MASK) >> 4)
#define AX_SROM_ADDR			0x07
#define AX_SROM_CMD			0x0a
	#define EEP_RD				0x04
	#define EEP_WR				0x08
	#define EEP_BUSY			0x10
#define AX_SROM_DATA_LOW		0x08
#define AX_SROM_DATA_HIGH		0x09
#define AX_RX_CTL			0x0b
#define AX_RX_CTL_HI			0x0c
	#define AX_RX_CTL_DROPCRCERR_HI		0x01
	#define AX_RX_CTL_DROPCRCERR		0x0100
	#define AX_RX_CTL_IPE			0x0200
	#define AX_RX_CTL_TXPADCRC		0x0400
	#define AX_RX_CTL_START			0x0080
	#define AX_RX_CTL_AP			0x0020
	#define AX_RX_CTL_AM			0x0010
	#define AX_RX_CTL_AB			0x0008
	#define AX_RX_CTL_HA8B			0x0004
	#define AX_RX_CTL_AMALL			0x0002
	#define AX_RX_CTL_PRO			0x0001
	#define AX_RX_CTL_STOP			0x0000
#define AX_NODE_ID			0x10
#define AX_MULTI_FILTER_ARRY		0x16
#define AX_MEDIUM_STATUS_MODE		0x22
	#define AX_MEDIUM_GIGAMODE		0x0001
	#define AX_MEDIUM_FULL_DUPLEX		0x0002
	#define AX_MEDIUM_RXFLOW_CTRLEN		0x0010
	#define AX_MEDIUM_TXFLOW_CTRLEN		0x0020
	#define AX_MEDIUM_RECEIVE_EN		0x0100
	#define AX_MEDIUM_PS			0x0200
	#define AX_MEDIUM_JUMBO_EN		0x8040
#define AX_MONITOR_MODE			0x24
	#define AX_MONITOR_MODE_RWLC		0x02
	#define AX_MONITOR_MODE_RWMP		0x04
	#define AX_MONITOR_MODE_RWWF		0x08
	#define AX_MONITOR_MODE_RW_FLAG		0x10
	#define AX_MONITOR_MODE_PMEPOL		0x20
	#define AX_MONITOR_MODE_PMETYPE		0x40
#define AX_GPIO_CTRL			0x25
	#define AX_GPIO_CTRL_GPIO3EN		0x80
	#define AX_GPIO_CTRL_GPIO2EN		0x40
	#define AX_GPIO_CTRL_GPIO1EN		0x20
#define AX_PHYPWR_RSTCTL		0x26
	#define AX_PHYPWR_RSTCTL_BZ		0x0010
	#define AX_PHYPWR_RSTCTL_IPRL		0x0020
	#define AX_PHYPWR_RSTCTL_AUTODETACH	0x1000
#define AX_RX_BULKIN_QCTRL		0x2e
	#define AX_RX_BULKIN_QCTRL_TIME		0x01
	#define AX_RX_BULKIN_QCTRL_IFG		0x02
	#define AX_RX_BULKIN_QCTRL_SIZE		0x04
#define AX_RX_BULKIN_QTIMR_LOW		0x2f
#define AX_RX_BULKIN_QTIMR_HIGH			0x30
#define AX_RX_BULKIN_QSIZE			0x31
#define AX_RX_BULKIN_QIFG			0x32
#define AX_CLK_SELECT			0x33
	#define AX_CLK_SELECT_BCS		0x01
	#define AX_CLK_SELECT_ACS		0x02
	#define AX_CLK_SELECT_ACSREQ		0x10
	#define AX_CLK_SELECT_ULR		0x08
#define AX_RXCOE_CTL			0x34
	#define AX_RXCOE_IP			0x01
	#define AX_RXCOE_TCP			0x02
	#define AX_RXCOE_UDP			0x04
	#define AX_RXCOE_ICMP			0x08
	#define AX_RXCOE_IGMP			0x10
	#define AX_RXCOE_TCPV6			0x20
	#define AX_RXCOE_UDPV6			0x40
	#define AX_RXCOE_ICMV6			0x80

#if KERNEL_VERSION(2, 6, 22) < LINUX_VERSION_CODE
	#define AX_RXCOE_DEF_CSUM	(AX_RXCOE_IP	| AX_RXCOE_TCP  | \
					 AX_RXCOE_UDP	| AX_RXCOE_ICMV6 | \
					 AX_RXCOE_TCPV6	| AX_RXCOE_UDPV6)
#else
	#define AX_RXCOE_DEF_CSUM	(AX_RXCOE_IP	| AX_RXCOE_TCP | \
					 AX_RXCOE_UDP)
#endif

#define AX_TXCOE_CTL			0x35
	#define AX_TXCOE_IP			0x01
	#define AX_TXCOE_TCP			0x02
	#define AX_TXCOE_UDP			0x04
	#define AX_TXCOE_ICMP			0x08
	#define AX_TXCOE_IGMP			0x10
	#define AX_TXCOE_TCPV6			0x20
	#define AX_TXCOE_UDPV6			0x40
	#define AX_TXCOE_ICMV6			0x80
#if KERNEL_VERSION(2, 6, 22) < LINUX_VERSION_CODE
	#define AX_TXCOE_DEF_CSUM	(AX_TXCOE_TCP   | AX_TXCOE_UDP | \
					 AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6)
#else
	#define AX_TXCOE_DEF_CSUM	(AX_TXCOE_TCP	| AX_TXCOE_UDP)
#endif
#define AX_PAUSE_WATERLVL_HIGH		0x54
#define AX_PAUSE_WATERLVL_LOW		0x55
#define AX_RX_FREE_BUF_LOW		0x57

#define GMII_PHY_CONTROL		0x00
	#define GMII_CONTROL_RESET		0x8000
	#define GMII_CONTROL_LOOPBACK		0x4000
	#define GMII_CONTROL_10MB		0x0000
	#define GMII_CONTROL_100MB		0x2000
	#define GMII_CONTROL_1000MB		0x0040
	#define GMII_CONTROL_SPEED_BITS		0x2040
	#define GMII_CONTROL_ENABLE_AUTO	0x1000
	#define GMII_CONTROL_POWER_DOWN		0x0800
	#define GMII_CONTROL_ISOLATE		0x0400
	#define GMII_CONTROL_START_AUTO		0x0200
	#define GMII_CONTROL_FULL_DUPLEX	0x0100
#define GMII_PHY_STATUS			0x01
	#define GMII_STATUS_100MB_MASK		0xE000
	#define GMII_STATUS_10MB_MASK		0x1800
	#define GMII_STATUS_AUTO_DONE		0x0020
	#define GMII_STATUS_AUTO		0x0008
	#define GMII_STATUS_LINK_UP		0x0004
	#define GMII_STATUS_EXTENDED		0x0001
	#define GMII_STATUS_100T4		0x8000
	#define GMII_STATUS_100TXFD		0x4000
	#define GMII_STATUS_100TX		0x2000
	#define GMII_STATUS_10TFD		0x1000
	#define GMII_STATUS_10T			0x0800
#define GMII_PHY_OUI			0x02
#define GMII_PHY_MODEL			0x03
#define GMII_PHY_ANAR			0x04
	#define GMII_ANAR_ASYM_PAUSE		0x0800
	#define GMII_ANAR_PAUSE			0x0400
	#define GMII_ANAR_100T4			0x0200
	#define GMII_ANAR_100TXFD		0x0100
	#define GMII_ANAR_100TX			0x0080
	#define GMII_ANAR_10TFD			0x0040
	#define GMII_ANAR_10T			0x0020
	#define GMII_SELECTOR_FIELD		0x001F

#define GMII_PHY_ANLPAR			0x05
	#define GMII_ANLPAR_100T4		0x0200
	#define GMII_ANLPAR_100TXFD		0x0100
	#define GMII_ANLPAR_100TX		0x0080
	#define GMII_ANLPAR_10TFD		0x0040
	#define GMII_ANLPAR_10T			0x0020
	#define GMII_ANLPAR_PAUSE		0x0400
	#define GMII_ANLPAR_ASYM_PAUSE		0x0800
	#define GMII_ANLPAR_ACK			0x4000
	#define GMII_SELECTOR_8023		0x0001
#define GMII_PHY_ANER			0x06
#define GMII_PHY_1000BT_CONTROL		0x09
#define GMII_PHY_1000BT_STATUS		0x0A
#define GMII_PHY_MACR			0x0D
#define GMII_PHY_MAADR			0x0E
#define GMII_PHY_PHYSR			0x11
	#define GMII_PHY_PHYSR_SMASK		0xc000
	#define GMII_PHY_PHYSR_GIGA		0x8000
	#define GMII_PHY_PHYSR_100		0x4000
	#define GMII_PHY_PHYSR_FULL		0x2000
	#define GMII_PHY_PHYSR_LINK		0x400

#define GMII_1000_AUX_CTRL_MASTER_SLAVE		0x1000
#define GMII_1000_AUX_CTRL_FD_CAPABLE		0x0200
#define GMII_1000_AUX_CTRL_HD_CAPABLE		0x0100
#define GMII_1000_AUX_STATUS_FD_CAPABLE		0x0800
#define GMII_1000_AUX_STATUS_HD_CAPABLE		0x0400
#define GMII_AUX_CTRL_STATUS		0x1C
#define GMII_AUX_ANEG_CPLT			0x8000
#define GMII_AUX_FDX				0x0020
#define GMII_AUX_SPEED_1000			0x0010
#define GMII_AUX_SPEED_100			0x0008
#define GMII_LED_ACTIVE			0x1a
	#define GMII_LED_ACTIVE_MASK		0xff8f
	#define GMII_LED0_ACTIVE		(1 << 4)
	#define GMII_LED1_ACTIVE		(1 << 5)
	#define GMII_LED2_ACTIVE		(1 << 6)
#define GMII_LED_LINK			0x1c
	#define GMII_LED_LINK_MASK		0xf888
	#define GMII_LED0_LINK_10		(1 << 0)
	#define GMII_LED0_LINK_100		(1 << 1)
	#define GMII_LED0_LINK_1000		(1 << 2)
	#define GMII_LED1_LINK_10		(1 << 4)
	#define GMII_LED1_LINK_100		(1 << 5)
	#define GMII_LED1_LINK_1000		(1 << 6)
	#define GMII_LED2_LINK_10		(1 << 8)
	#define GMII_LED2_LINK_100		(1 << 9)
	#define GMII_LED2_LINK_1000		(1 << 10)
	#define	LED_VALID			(1 << 15)
	#define	LED0_ACTIVE			(1 << 0)
	#define	LED0_LINK_10			(1 << 1)
	#define	LED0_LINK_100			(1 << 2)
	#define	LED0_LINK_1000			(1 << 3)
	#define	LED0_FD				(1 << 4)
	#define LED0_USB3_MASK			0x001f
	#define	LED1_ACTIVE			(1 << 5)
	#define	LED1_LINK_10			(1 << 6)
	#define	LED1_LINK_100			(1 << 7)
	#define	LED1_LINK_1000			(1 << 8)
	#define	LED1_FD				(1 << 9)
	#define LED1_USB3_MASK			0x03e0
	#define	LED2_ACTIVE			(1 << 10)
	#define	LED2_LINK_1000			(1 << 13)
	#define	LED2_LINK_100			(1 << 12)
	#define	LED2_LINK_10			(1 << 11)
	#define	LED2_FD				(1 << 14)
	#define LED2_USB3_MASK			0x7c00
#define GMII_PHYPAGE			0x1e
#define GMII_PHY_PAGE_SELECT		0x1f
	#define GMII_PHY_PAGE_SELECT_EXT	0x0007
	#define GMII_PHY_PAGE_SELECT_PAGE0	0X0000
	#define GMII_PHY_PAGE_SELECT_PAGE1	0X0001
	#define GMII_PHY_PAGE_SELECT_PAGE2	0X0002
	#define GMII_PHY_PAGE_SELECT_PAGE3	0X0003
	#define GMII_PHY_PAGE_SELECT_PAGE4	0X0004
	#define GMII_PHY_PAGE_SELECT_PAGE5	0X0005
	#define GMII_PHY_PAGE_SELECT_PAGE6	0X0006

enum ax_driver_flags {
	AX_UNPLUG = 0,
	AX_ENABLE,
	AX_LINK_CHG,
	AX_SELECTIVE_SUSPEND,
	AX_SCHEDULE_NAPI,
	AX_EN_RX,
	AX_SCHEDULE_TASKLET_TX,
	AX_SCHEDULE_TASKLET_RX,
};

enum ax_chip_version {
	AX_VERSION_INVALID		= 0,
	AX_VERSION_AX88179		= 4,
	AX_VERSION_AX88179A_772D	= 6,
#ifdef ENABLE_AX88279
	AX_VERSION_AX88279		= 7,
#endif
};

struct ax_device;

struct rx_desc {
	struct list_head list;
	struct urb *urb;
	struct ax_device *context;
	void *buffer;
	void *head;
};

enum __ax_tx_flags {
	AX_TX_NONE		= 0,
	AX_TX_TIMESTAMPS	= 1,
};

enum __mac_pass_ctrl {
	MAC_PASS_DISABLED	= 0,
	MAC_PASS_ENABLE_0	= 1,
	MAC_PASS_ENABLE_1	= 2,
};

struct mac_pass {
	u8	control;
	u8	mac0[8];
	u8	mac1[8];
} __packed;

struct tx_desc {
	struct list_head list;
	struct urb *urb;
	struct ax_device *context;
	void *buffer;
	void *head;
	u32 skb_num;
	u32 skb_len;
	unsigned long flags;
	int q_index;
};

struct ax_bulkin_setting {
	u8 custom;
	u8 bulkin_setting[5];
};

enum ax_ether_link_speed {
	ETHER_LINK_NONE	= 0,
	ETHER_LINK_10	= 1,
	ETHER_LINK_100	= 2,
	ETHER_LINK_1000	= 3,
	ETHER_LINK_2500	= 4,
};

struct ax_link_info {
	u8 eth_speed	: 3,
	   full_duplex	: 1,
	   usb_speed	: 4;
} __packed;

struct ax_device {
	unsigned long flags;
	struct usb_device *udev;
	struct usb_interface *intf;
	struct net_device *netdev;
	const struct driver_info *driver_info;
#ifndef ENABLE_RX_TASKLET
	struct napi_struct napi;
#endif
	struct urb *intr_urb;
	struct tx_desc tx_list[32];
	struct rx_desc rx_list[32];
	struct list_head rx_done, tx_free;
	struct sk_buff_head tx_queue[AX_TX_QUEUE_SIZE];
	struct sk_buff_head rx_queue;
	spinlock_t rx_lock, tx_lock;
	struct delayed_work schedule;
	struct mii_if_info mii;
	struct mutex control;
#ifdef ENABLE_TX_TASKLET
	struct tasklet_struct tx_tl;
#endif
#ifdef ENABLE_RX_TASKLET
	struct tasklet_struct rx_tl;
#endif

	int intr_interval;
	u32 saved_wolopts;
	u32 msg_enable;
	u32 tx_qlen;
	u32 coalesce;
	u16 speed;
	u8 *intr_buff;
	u8 tx_align_len;
	u8 link;
	u16 rxctl;
	u8 m_filter[8];
	u32 tx_casecade_size;
	u32 gso_max_size;
	u8 fw_version[4];

	struct ax_link_info link_info;
	struct ax_link_info intr_link_info;
	u8  eee_enabled;
	u8  eee_active;
	int autodetach;

	struct ax_bulkin_setting bin_setting;
	bool tx_header_cksum;
	unsigned char chip_version;
	unsigned char sub_version;
#define AX_ETH_SPEED_MASK	0xF
#define AX_ETH_DUPLEX_FULL	0x10
	u8 int_link_info;
	u8 int_link_chg;

#ifdef ENABLE_PTP_FUNC
	struct ax_ptp_cfg *ptp_cfg;
	struct sk_buff_head tx_timestamp;
#endif
#ifdef ENABLE_MACSEC_FUNC
	struct ax_macsec_cfg *macsec_cfg;
#endif
	u64 bulkin_complete;
	u64 bulkin_error;
	u64 bulkout_complete;
	u64 bulkout_error;
	u64 bulkint_complete;
	u64 bulkint_error;
#ifdef ENABLE_QUEUE_PRIORITY
	u64 ep5_count;
	u64 ep3_count;
#endif
#define CHIP_40PIN	0x03
#define CHIP_32PIN	0x02
	u8 chip_pin;
#ifdef ENABLE_DWC3_ENHANCE
	u8 intr_not_first_link_up;
#endif
#ifdef ENABLE_INT_POLLING
#define INT_POLLING_TIMER	128	/* in milliseconds */
	struct delayed_work int_polling_work;
#endif
	unsigned autosuspend_is_supported:1;
};

struct driver_info {
	int	(*bind)(struct ax_device *axdev);
	void	(*unbind)(struct ax_device *axdev);
	int	(*hw_init)(struct ax_device *axdev);
	int	(*stop)(struct ax_device *axdev);
#ifdef ENABLE_QUEUE_PRIORITY
	int	(*queue_priority)(struct ax_device *axdev);
#endif
	void	(*rx_fixup)(struct ax_device *axdev, struct rx_desc *desc,
			    int *work_done, int budget);
	int	(*tx_fixup)(struct ax_device *axdev, struct tx_desc *desc);
	int	(*link_reset)(struct ax_device *axdev);
	int	(*link_setting)(struct ax_device *axdev);
	int	(*system_suspend)(struct ax_device *axdev);
	int	(*system_resume)(struct ax_device *axdev);
	int	(*runtime_suspend)(struct ax_device *axdev);
	int	(*runtime_resume)(struct ax_device *axdev);

#ifdef ENABLE_PTP_FUNC
	int (*ptp_pps_ctrl)(struct ax_device *axdev, u8 enable);
	int	(*ptp_init)(struct ax_device *axdev);
	void	(*ptp_remove)(struct ax_device *axdev);
#endif

	unsigned long napi_weight;
	size_t	buf_rx_size;
};

struct _async_cmd_handle {
	struct ax_device *axdev;
	struct usb_ctrlrequest *req;
	u8 m_filter[8];
	u16 rxctl;
} __packed;

#define AX_INT_PPLS_LINK	(1 << 0)
#define AX_INT_SPLS_LINK	(1 << 1)
#define AX_INT_CABOFF_UNPLUG	(1 << 7)
struct ax_device_int_data {
#ifdef __BIG_ENDIAN
	u16 res3;
	u8 status;
	u16 res2;
	u8 link;
	union {
		struct ax_link_info link_info;
		u8 link_info_u8;
	};
	u8 res1;
#else
	u8 res1;
	union {
		struct ax_link_info link_info;
		u8 link_info_u8;
	};
	u8 link;
	u16 res2;
	u8 status;
	u16 res3;
#endif
} __packed;

struct _ax_buikin_setting {
	u8 ctrl;
	u8 timer_l;
	u8 timer_h;
	u8 size;
	u8 ifg;
} __packed;

#define AX_RXHDR_L4_ERR		(1 << 8)
#define AX_RXHDR_L3_ERR		(1 << 9)

#define AX_RXHDR_L4_TYPE_ICMP	2
#define AX_RXHDR_L4_TYPE_IGMP	3
#define AX_RXHDR_L4_TYPE_TCMPV6	5

#define AX_RXHDR_L3_TYPE_IP	1
#define AX_RXHDR_L3_TYPE_IPV6	2

#define AX_RXHDR_L4_TYPE_MASK	0x1c
#define AX_RXHDR_L4_TYPE_UDP	4
#define AX_RXHDR_L4_TYPE_TCP	16
#define AX_RXHDR_L3CSUM_ERR	2
#define AX_RXHDR_L4CSUM_ERR	1
#define AX_RXHDR_CRC_ERR	0x20000000
#define AX_RXHDR_MII_ERR	0x40000000
#define AX_RXHDR_DROP_ERR	0x80000000

static inline void *__rx_buf_align(void *data)
{
	return (void *)ALIGN((uintptr_t)data, RX_ALIGN);
}
static inline void *__tx_buf_align(void *data, u8 tx_align_len)
{
	return (void *)ALIGN((uintptr_t)data, tx_align_len);
}
static inline struct net_device_stats *ax_get_stats(struct net_device *netdev)
{
	return &netdev->stats;
}

int ax_get_mac_pass(struct ax_device *axdev, u8 *mac);
void ax_set_tx_qlen(struct ax_device *dev);
void ax_write_bulk_callback(struct urb *urb);

void ax_get_drvinfo(struct net_device *net, struct ethtool_drvinfo *info);
#if KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE
int ax_get_settings(struct net_device *net, struct ethtool_cmd *cmd);
int ax_set_settings(struct net_device *net, struct ethtool_cmd *cmd);
#else
int ax_get_link_ksettings(struct net_device *netdev,
			  struct ethtool_link_ksettings *cmd);
int ax_set_link_ksettings(struct net_device *netdev,
			  const struct ethtool_link_ksettings *cmd);
#endif
u32 ax_get_msglevel(struct net_device *netdev);
void ax_set_msglevel(struct net_device *netdev, u32 value);
void ax_get_wol(struct net_device *net, struct ethtool_wolinfo *wolinfo);
int ax_set_wol(struct net_device *net, struct ethtool_wolinfo *wolinfo);
int ax_get_sset_count(struct net_device *dev, int sset);
void ax_get_ethtool_stats
(struct net_device *dev, struct ethtool_stats *stats, u64 *data);
void ax_get_strings(struct net_device *netdev, u32 stringset, u8 *data);
void ax_get_pauseparam
(struct net_device *netdev, struct ethtool_pauseparam *pause);
int ax_set_pauseparam
(struct net_device *netdev, struct ethtool_pauseparam *pause);
int ax_get_regs_len(struct net_device *netdev);
void ax_get_regs
(struct net_device *netdev, struct ethtool_regs *regs, void *buf);

int ax_read_cmd
(struct ax_device *axdev, u8 cmd, u16 value, u16 index, u16 size, void *data,
int eflag);

int ax_write_cmd
(struct ax_device *axdev, u8 cmd, u16 value, u16 index, u16 size, void *data);

int ax_read_cmd_nopm
(struct ax_device *axdev, u8 cmd, u16 value, u16 index, u16 size, void *data,
int eflag);

int ax_write_cmd_nopm
(struct ax_device *axdev, u8 cmd, u16 value, u16 index, u16 size, void *data);

int ax_write_cmd_async
(struct ax_device *axdev, u8 cmd, u16 value, u16 index, u16 size, void *data);

int ax_mmd_read(struct net_device *netdev, int dev_addr, int reg);
void ax_mmd_write(struct net_device *netdev, int dev_addr, int reg, int val);

int ax_mdio_read(struct net_device *netdev, int phy_id, int reg);
void ax_mdio_write(struct net_device *netdev, int phy_id, int reg, int val);


int ax_usb_command(struct ax_device *axdev, struct _ax_ioctl_command *info);
#endif /* __ASIX_MAIN_H */
