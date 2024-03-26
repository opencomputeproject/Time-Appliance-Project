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
#ifndef __ASIX_IOCTL_H
#define __ASIX_IOCTL_H

#ifdef ENABLE_IOCTL_DEBUG
#define DEBUG_PRINT(fmt, args...) printf(fmt, ## args)
#define DEBUG_PRINTK(fmt, args...) printk(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)
#define DEBUG_PRINTK(fmt, args...)
#endif

// CHANGE NETWORK INTERFACE WAY
// DEFAULT_SCAN   : scan "eth0" - "eth255"
// INTERFACE_SCAN : scan all available network interfaces
#define NET_INTERFACE	INTERFACE_SCAN
#define DEFAULT_SCAN	0x00
#define INTERFACE_SCAN	0x01

#define AX88179_SIGNATURE	"AX88179_178A"
#define AX88179_DRV_NAME	"AX88179_178A"
#define AX88179A_SIGNATURE	"AX88179B_179A_772E_772D"
#define AX88179A_DRV_NAME	"AX88179B_179A_772E_772D"

#define AX_PRIVATE		SIOCDEVPRIVATE

#define AX_SIGNATURE			0
#define AX_USB_COMMAND			1
#define AX88179_READ_EEPROM		2
#define AX88179_WRITE_EEPROM		3
#define AX88179A_READ_VERSION		2
#define AX88179A_WRITE_FLASH		3
#define AX88179A_ROOT_2_ROM		4
#define AX88179A_ERASE_FLASH		5
#define AX88179A_SW_RESET		6
#define AX88179A_READ_FLASH		7
#define AX88179A_PROGRAM_EFUSE		8
#define AX88179A_DUMP_EFUSE		9
#define AX88179A_IEEE_TEST		10
#define AX88179A_AUTOSUSPEND_EN		11
#define AX88179A_ERASE_SECTOR_FLASH	12

#define IEEE_1000M1			0
#define IEEE_1000M2			1
#define IEEE_1000M3			2
#define IEEE_1000M4			3
#define IEEE_100CA			4
#define IEEE_100CB			5
#define IEEE_10R			6
#define IEEE_10FF			7
#define IEEE_10MDI			8

#define ERR_FALSH_WRITE_EN		1
#define ERR_FALSH_WRITE_DIS		2
#define ERR_FALSH_ERASE_ALL		3
#define ERR_FALSH_WRITE			4
#define ERR_FALSH_READ			5
#define ERR_EFUSE_READ			6
#define ERR_EFUSE_WRITE			7
#define ERR_IEEE_INVALID_CHIP		8
#define ERR_BOOT_CODE			9
#define ERR_FALSH_ERASE_SECTOR	10

#define USB_READ_OPS			0
#define USB_WRITE_OPS			1

#define SCAN_DEV_MAX_RETRY		10

#define FLASH_BLOCK_SIZE	20 //bytes
#define WRITE_PARA_HEADER 0x2100
#define FLASH_SIZE (16*1024*1024)
#define FLASH_PARA_OFFSET 8448

#define PRAM_PRI_FW1_OFFSET 0x0
#define PRAM_PRI_FW1_LENGTH 0x4
#define MD32_PRI_FW1_OFFSET 0x14
#define MD32_PRI_FW1_LENGTH 0x18

#define PRAM_SEC_FW1_OFFSET 0x28
#define PRAM_SEC_FW1_LENGTH 0x2C
#define MD32_SEC_FW1_OFFSET 0x3C
#define MD32_SEC_FW1_LENGTH 0x40

#define PRAM_PRI_FW2_OFFSET 0x1000
#define PRAM_PRI_FW2_LENGTH 0x1004
#define MD32_PRI_FW2_OFFSET 0x1014
#define MD32_PRI_FW2_LENGTH 0x1018

#define PRAM_SEC_FW2_OFFSET 0x1028
#define PRAM_SEC_FW2_LENGTH 0x102C
#define MD32_SEC_FW2_OFFSET 0x103C
#define MD32_SEC_FW2_LENGTH 0x1040

#define PARAMETER_PRI_HEADER_OFFSET 0x2000
#define PARAMETER_SEC_HEADER_OFFSET 0x200C

#define PARAMETER_PRI_OFFSET 0x2004
#define PARAMETER_PRI_BLOCK_COUNT 0x2008
#define PARAMETER_SEC_OFFSET 0x2010
#define PARAMETER_SEC_BLOCK_COUNT 0x2014


enum _exit_code {
	SUCCESS			= 0,
	FAIL_INVALID_PARAMETER	= 1,
	FAIL_IOCTL,
	FAIL_SCAN_DEV,
	FAIL_ALLCATE_MEM,
	FAIL_LOAD_FILE,
	FAIL_FLASH_WRITE,
	FAIL_IVALID_VALUE,
	FAIL_IVALID_CHKSUM,
	FAIL_NON_EMPTY_RFUSE_BLOCK,
	FAIL_EFUSE_WRITE,
	FAIL_GENERIAL_ERROR,
};

struct _ax_usb_command {
	unsigned char	ops;
	unsigned char	cmd;
	unsigned short	value;
	unsigned short	index;
	unsigned short	size;
	unsigned char	*data;
	unsigned long	cmd_data;
};

struct _ax88179a_flash {
	int status;
	int length;
	int offset;
	unsigned char *buf;
};

struct _ax88179a_version {
	unsigned char version[16];
};

struct _ax88179a_ieee {
	unsigned int type;
	unsigned int speed;
	unsigned int stop;
	int status;
};

struct _ax88179a_autosuspend {
	unsigned int enable;
};

struct _ax_ioctl_command {
	unsigned short	ioctl_cmd;
	unsigned char	sig[32];
	unsigned char	type;
	unsigned short *buf;
	unsigned short size;
	unsigned char delay;
	union {
		struct _ax88179a_flash		flash;
		struct _ax88179a_version	version;
		struct _ax88179a_ieee		ieee;
		struct _ax_usb_command		usb_cmd;
		struct _ax88179a_autosuspend	autosuspend;
	};
};

#define SWAP_32(val)	(((val >> 24) & 0x000000FF) | \
			 ((val >>  8) & 0x0000FF00) | \
			 ((val <<  8) & 0x00FF0000) | \
			 ((val << 24) & 0xFF000000))
#define SWAP_16(val)    ((val >> 8) & 0x00FF) | \
                         ((val <<  8) & 0xFF00)

struct ax_device;
typedef int (*IOCTRL_TABLE)(struct ax_device *axdev,
			    struct _ax_ioctl_command *info);

struct ax_command_info {
	int inet_sock;
	struct ifreq *ifr;
	int argc;
	char **argv;
	unsigned short ioctl_cmd;
	const char *help_ins;
	const char *help_desc;
};

typedef int (*MENU_FUNC)(struct ax_command_info *info);

struct _command_list {
	char *cmd;
	unsigned short ioctl_cmd;
	MENU_FUNC OptFunc;
	const char *help_ins;
	const char *help_desc;
};

#endif /* __ASIX_IOCTL_H */
