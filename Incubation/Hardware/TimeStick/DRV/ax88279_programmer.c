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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <getopt.h>
#include <endian.h>
#if NET_INTERFACE == INTERFACE_SCAN
#include <ifaddrs.h>
#endif
#include "ax_ioctl.h"
#ifdef ENABLE_IOCTL_DEBUG
#define NOT_PROGRAM
#endif
#define RELOAD_DELAY_TIME	10	// sec

#define PRINT_IOCTL_FAIL(ret) \
fprintf(stderr, "%s: ioctl failed. (err: %d)\n", __func__, ret)
#define PRINT_SCAN_DEV_FAIL \
fprintf(stderr, "%s: Scaning device failed.\n", __func__)
#define PRINT_ALLCATE_MEM_FAIL \
fprintf(stderr, "%s: Fail to allocate memory.\n", __func__)
#define PRINT_LOAD_FILE_FAIL \
fprintf(stderr, "%s: Read file failed.\n", __func__)

#define AX88179A_IOCTL_VERSION \
"AX88279 Linux Flash Programming Tool v1.0.0 beat1"

const char help_str1[] =
"./ax88279_programmer help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char readverion_str1[] =
"./ax88279_programmer rversion\n"
"    -- AX88279 Read Firmware Verion\n";
static const char readverion_str2[] = "";

const char readmac_str1[] =
"./ax88279_programmer rmacaddr\n"
"    -- AX88279 Read MAC Address\n";
static const char readmac_str2[] = "";

const char writeflash_str1[] =
"./ax88279_programmer wflash [file]\n"
"    -- AX88279 Write Flash\n";
const char writeflash_str2[] =
"        [file]    - Flash file path\n";

const char writeparameter_str1[] =
"./ax88279_programmer wpara -m [MAC] -s [SN] -p [PID] -v [VID] -P [PS] -M [MN] -D [dump]\n"
"                           -S [SS] -H [HS] -w [wol] -l [led0 value] -e [led1 value] -d [led2 value]\n"
"    -- AX88279 Write Parameter\n";
const char writeparameter_str2[] =
"        -m [MAC]   	 - MAC address (XX:XX:XX:XX:XX:XX) X:'0'-'F'\n"
"        -s [SN]    	 - Serial Number (Characters must be less than 19 bytes) X:'0'-'F'\n"
"        -p [PID]   	 - Product ID (XX:XX) X:'0'-'F'\n"
"        -v [VID]   	 - Vendor ID (XX:XX) X:'0'-'F'\n"
"        -P [PS]    	 - Product String (Characters must be less than 19 bytes)\n"
"        -M [MN]    	 - Manufacture Name (Characters must be less than 19 bytes)\n"
"        -D [dump]	 - The parameter content currently in flash (dump)\n"
"        -S [SS]    	 - SS bus power (XX) X:0-896\n"
"        -H [HS]    	 - HS bus power (XX) X:0-500\n"
"        -w [wol]    	 - wake on LAN (XXXXXXXX) X:digit\n"
"        -l [led0 value]	 - value: control_blink (XXXX_XXXX)\n"
"        -e [led1 value]	 - value: control_blink (XXXX_XXXX)\n";

const char reload_str1[] =
"./ax88279_programmer reload\n"
"    -- AX88279 Reload\n";
static const char reload_str2[] = "";

static int help_func(struct ax_command_info *info);
static int readversion_func(struct ax_command_info *info);
static int readmac_func(struct ax_command_info *info);
static int writeflash_func(struct ax_command_info *info);
static int writeparameter_func(struct ax_command_info *info);
static int reload_func(struct ax_command_info *info);
static int scan_ax_device(struct ifreq *ifr, int inet_sock);

struct _command_list ax88279_cmd_list[] = {
	{
		"help",
		AX_SIGNATURE,
		help_func,
		help_str1,
		help_str2
	},
	{
		"rversion",
		AX88179A_READ_VERSION,
		readversion_func,
		readverion_str1,
		readverion_str2
	},
	{
		"rmacaddr",
		~0,
		readmac_func,
		readmac_str1,
		readmac_str2
	},
	{
		"wflash",
		AX88179A_WRITE_FLASH,
		writeflash_func,
		writeflash_str1,
		writeflash_str2
	},
	{
		"wpara",
		AX88179A_PROGRAM_EFUSE,
		writeparameter_func,
		writeparameter_str1,
		writeparameter_str2
	},
	{
		"reload",
		~0,
		reload_func,
		reload_str1,
		reload_str2
	},
	{
		NULL,
		0,
		NULL,
		NULL,
		NULL
	}
};

static struct option const long_options[] =
{
  {"mac", required_argument, NULL, 'm'},
  {"serial", required_argument, NULL, 's'},
  {"pid", required_argument, NULL, 'p'},
  {"vid", required_argument, NULL, 'v'},
  {"Product", required_argument, NULL, 'P'},
  {"Manufacture", required_argument, NULL, 'M'},
  {"dump", required_argument, NULL, 'D'},
  {"ssbus", required_argument, NULL, 'S'},
  {"hsbus", required_argument, NULL, 'H'},
  {"wol", required_argument, NULL, 'w'},
  {"led0", required_argument, NULL, 'l'},
  {"led1", required_argument, NULL, 'e'},
  {NULL, 0, NULL, 0}
};

struct __wpara {
	char *mac_address;
	char *serial_num;
	char *PID;
	char *VID;
	char *product_string;
	char *manufacture;
	char *dump;
	char *ss_bus;
	char *hs_bus;
	char *wol;
	char *led0;
	char *led1;
	unsigned int iss_bus;
	unsigned int ihs_bus;	
	unsigned int MAC[6];
	unsigned int LED0[9];
	unsigned int LED1[9];
	unsigned int pid[2];
	unsigned int vid[2];
	unsigned int ssbus[1];
	unsigned int hsbus[1];
};

static unsigned char sample_type1[] = {
 0x01, 0x0B, 0x95, 0x17,
 0x90, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x01, 0x04,
 0x00, 0x0A, 0x07, 0xFF,
 0x39, 0xE1, 0x20, 0x00
};

static unsigned char sample_type2[] = {
 0x02, 0x41, 0x53, 0x49,
 0x58, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00
};

static unsigned char sample_type3[] = {
 0x03, 0x41, 0x58, 0x38,
 0x38, 0x32, 0x37, 0x39,
 0x41, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00
};

static unsigned char sample_type4[] = {
 0x04, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x30,
 0x31, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00
};

static unsigned char sample_type11[] = {
 0x0B, 0x1F, 0x00, 0x00,
 0x00, 0x00, 0x1F, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x45, 0x0B
};

static unsigned char sample_type15[] = {
 0x0F, 0x7D, 0x01, 0x63,
 0x81, 0x7F, 0x7F, 0x5F,
 0x5D, 0x2F, 0x07, 0xE8,
 0x04, 0x7D, 0x00, 0xC8,
 0x08, 0x01, 0x04, 0x00
};

#pragma pack(push)
#pragma pack(1)
enum Para_Type_Def {
	TYPE_REV = 0x00,
	TYPE_01 = 0x01,
	TYPE_02 = 0x02,
	TYPE_03 = 0x03,
	TYPE_04 = 0x04,
	TYPE_11 = 0x0B,
	TYPE_15 = 0x0F,
};
struct _ef_type {
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char	checksum: 4;
	unsigned char	type	: 4;
#else
	unsigned char	type	: 4;
	unsigned char	checksum: 4;
#endif
};

struct _ef_type01 {
	struct _ef_type	type;
	unsigned short	vid;
	unsigned short	pid;
	unsigned char	mac[6];
	unsigned short	bcdDevice;
	unsigned char	bU1DevExitLat;
	unsigned short	wU2DevExitLat;
	unsigned char	SS_Max_Bus_Pw;
	unsigned char	HS_Max_Bus_Pw;
	unsigned char	IPSleep_Polling_Count;
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_01	sizeof(struct _ef_type01)

struct _ef_type02 {
	struct _ef_type	type;
	unsigned char	m_string[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_02	sizeof(struct _ef_type02)

struct _ef_type03 {
	struct _ef_type	type;
	unsigned char	p_string[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_03	sizeof(struct _ef_type03)

struct _ef_type04 {
	struct _ef_type	type;
	unsigned char	serial[18];
	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_04	sizeof(struct _ef_type04)

struct _ef_type11 {
	struct _ef_type	type;
	unsigned char	dev_type0;
	unsigned short	reg0;
	unsigned short	value0;

	unsigned char	dev_type1;
	unsigned short	reg1;
	unsigned short	value1;

	unsigned char	dev_type2;
	unsigned short	reg2;
	unsigned short	value2;

	unsigned char	reserved1[2];
	unsigned char	subtype;
	unsigned char	endofs;
};
#define EF_TYPE_STRUCT_SIZE_11	sizeof(struct _ef_type11)

struct _ef_type15 {
	struct _ef_type	type;
	unsigned char	flag1;
	unsigned char	flag2;
	unsigned char	flag3;
	unsigned char	flag4;
	unsigned char	U1_inact_timer;
	unsigned char	U2_inact_timer;
	unsigned char	Lpm_besl_u3;
	unsigned char	Lpm_besl;
	unsigned char	Lpm_besld;
	unsigned short	Ltm_belt_down;
	unsigned short	Ltm_belt_up;
	unsigned short	Ephy_poll_timer;

	unsigned char	Pme_gpio_sel;
	unsigned char	Pme_pulse_width;
	unsigned char	Wol_mask_timer;

	unsigned char	reserve;
};
#define EF_TYPE_STRUCT_SIZE_15	sizeof(struct _ef_type15)

struct _ef_data_struct {
	union {
		struct _ef_type01 type01;
		struct _ef_type02 type02;
		struct _ef_type03 type03;
		struct _ef_type04 type04;
		struct _ef_type11 type11;
		struct _ef_type15 type15;
	} ef_data;
};
#define EF_DATA_STRUCT_SIZE	sizeof(struct _ef_data_struct)
#pragma pack(pop)

static void show_usage(void)
{
	int i;

	printf("Usage:\n");
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++)
		printf("%s\n", ax88279_cmd_list[i].help_ins);
}

static int help_func(struct ax_command_info *info)
{
	int i;

	if (info->argv[2] == NULL)
		return -FAIL_INVALID_PARAMETER;

	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(info->argv[2],
			    ax88279_cmd_list[i].cmd,
			    strlen(ax88279_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88279_cmd_list[i].help_ins,
			       ax88279_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}

	return SUCCESS;
}

static int autosuspend_enable(struct ax_command_info *info,
			      unsigned char enable)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_AUTOSUSPEND_EN;

	ioctl_cmd.autosuspend.enable = enable;

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int read_version(struct ax_command_info *info, char *version)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_READ_VERSION;

	memset(ioctl_cmd.version.version, 0, 16);

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	memcpy(version, ioctl_cmd.version.version, 16);

	return SUCCESS;
}

static int read_mac_address(struct ax_command_info *info, unsigned char *mac)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ret = scan_ax_device(ifr, info->inet_sock);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		return ret;
	}

	ifr->ifr_flags &= 0;
	ret = ioctl(info->inet_sock, SIOCSIFFLAGS, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	usleep(20000);

	ifr->ifr_flags = IFF_UP | IFF_BROADCAST | IFF_MULTICAST;
	ret = ioctl(info->inet_sock, SIOCSIFFLAGS, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	usleep(20000);

	ret = ioctl(info->inet_sock, SIOCGIFHWADDR, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	memcpy(mac, ifr->ifr_hwaddr.sa_data, 6);

	return SUCCESS;
}

static int readversion_func(struct ax_command_info *info)
{
	char version[16] = {0};
	int i, ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	ret = scan_ax_device(info->ifr, info->inet_sock);
    if (ret < 0) {
            PRINT_SCAN_DEV_FAIL;
            return ret;
    }

	autosuspend_enable(info, 0);

	ret = read_version(info, version);
	if (ret == SUCCESS)
		printf("Firmware Version: %s\n", version);

	usleep(20000);

	autosuspend_enable(info, 1);

	return ret;
}

static int readmac_func(struct ax_command_info *info)
{
	unsigned char mac[6] = {0};
	int i, ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	ret = read_mac_address(info, mac);
	if (ret == SUCCESS)
		printf("MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
			mac[0],
			mac[1],
			mac[2],
			mac[3],
			mac[4],
			mac[5]);

	return ret;
}

static int write_flash(struct ax_command_info *info, unsigned char *data,
		       unsigned long offset, unsigned long len)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_WRITE_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ioctl_cmd.flash.length = len;
	ioctl_cmd.flash.buf = data;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		if (ioctl_cmd.flash.status)
			fprintf(stderr, "FLASH WRITE status: %d",
				ioctl_cmd.flash.status);
		PRINT_IOCTL_FAIL(ret);
		return ret;
	}

	return SUCCESS;
}

static int read_flash(struct ax_command_info *info, unsigned char *data,
		      unsigned long offset, unsigned long len)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_READ_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ioctl_cmd.flash.length = len;
	ioctl_cmd.flash.buf = data;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		if (ioctl_cmd.flash.status)
			fprintf(stderr, "FLASH READ status: %d",
				ioctl_cmd.flash.status);
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int erase_flash(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ERASE_FLASH;
	ioctl_cmd.flash.status = 0;

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int erase_sector_flash(struct ax_command_info *info, int offset)
{

	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ERASE_SECTOR_FLASH;
	ioctl_cmd.flash.status = 0;
	ioctl_cmd.flash.offset = offset;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	return SUCCESS;
}

static int boot_to_rom(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_ROOT_2_ROM;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	ioctl(info->inet_sock, AX_PRIVATE, ifr);

	return SUCCESS;
}

static int sw_reset(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	ioctl_cmd.ioctl_cmd = AX88179A_SW_RESET;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	ioctl(info->inet_sock, AX_PRIVATE, ifr);

	usleep(RELOAD_DELAY_TIME * 100000);

	return SUCCESS;
}

static int find_offest_min_index(int *offset_arr, int size)
{
	int i = 0;
	int min = offset_arr[0];

	for (i = 1; i < size; i++) {
        if (offset_arr[i] < min) {
            min = offset_arr[i];
        }
    }

	return min;
}

static int find_offest_max_index(int *offset_arr, int size)
{
	int i = 0;
	int max = offset_arr[0];

	for (i = 1; i < size; i++) {
		if (offset_arr[i] >= max) {
				max = i;
		}
	}

	return max;
}

static int writeflash_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned char *wbuf = NULL, *rbuf = NULL;
	FILE *pFile = NULL;
	size_t result;
	int file_length = 0;
	int i, offset, ret;
	char fw_version[16] = {0};

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 3) {
		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	autosuspend_enable(info, 0);

	ret = scan_ax_device(ifr, info->inet_sock);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		return ret;
	}

	ret = erase_flash(info);
	if (ret < 0) 
		return ret;

	pFile = fopen(info->argv[2], "rb");
	if (pFile == NULL) {
		fprintf(stderr, "%s: Fail to open %s file.\n",
			__func__, info->argv[2]);
		ret = -FAIL_LOAD_FILE;
		goto fail;
	}

	fseek(pFile, 0, SEEK_END);
	file_length = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	wbuf = (unsigned char *)malloc((file_length + 256) & ~(0xFF));
	if (!wbuf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto fail;
	}
	memset(wbuf, 0, (file_length + 256) & ~(0xFF));

	result = fread(wbuf, 1, file_length, pFile);
	if (result != file_length) {
		PRINT_LOAD_FILE_FAIL;
		ret = -PRINT_LOAD_FILE_FAIL;
		goto fail;
	}

	ret = write_flash(info, wbuf, FLASH_PARA_OFFSET, result);
	if (ret < 0)
		goto fail;

	ret = write_flash(info, wbuf, 0, FLASH_PARA_OFFSET);
	if (ret < 0)
		goto fail;

	offset = SWAP_32(*(unsigned long *)&wbuf[0]);
	sprintf(fw_version, "v%d.%d.%d",
		wbuf[offset + 0x1000],
		wbuf[offset + 0x1001],
		wbuf[offset + 0x1002]);
	printf("File FW Version: %s\n", fw_version);
	
	ret = SUCCESS;
	goto out;

fail:
	erase_flash(info);
out:
	if (wbuf)
		free(wbuf);
	if (pFile)
		fclose(pFile);

	autosuspend_enable(info, 1);

	return ret;
}

static int print_msg(char *cmd)
{
	int i;

	printf("\n");
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(cmd, ax88279_cmd_list[i].cmd,
				strlen(ax88279_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				ax88279_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}
}

static int find_block_index(unsigned char *rpara_databuf, int para_size
							, enum Para_Type_Def type)
{
	int i = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < para_size; i += 20) {
		if ((rpara_databuf[i] & 0x0F) == type)
			return i / 20;
	}

	return -FAIL_GENERIAL_ERROR;
}

static int change_para_macaddr(unsigned char *rpara_databuf, int block_index, unsigned int *mac)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 6; i++)
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5 + i] = (unsigned char)mac[i];

	return SUCCESS;
}

static int change_para_serialnum(unsigned char *rpara_databuf, int block_index, char *serial)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (serial[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = serial[i];
	}

	return SUCCESS;
}

static int change_para_pid(unsigned char *rpara_databuf, int block_index, unsigned int *pid)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 2; i++)
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3 + i] = (unsigned char)pid[i];

	return SUCCESS;
}

static int change_para_vid(unsigned char *rpara_databuf, int block_index, unsigned int *vid)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 2; i++)
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = (unsigned char)vid[i];

	return SUCCESS;
}

static int change_para_productstr(unsigned char *rpara_databuf, int block_index, char *productstr)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (productstr[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = productstr[i];
	}

	return SUCCESS;
}

static int change_para_manufacture(unsigned char *rpara_databuf, int block_index, char *manufac)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 18; i++) {
		if (manufac[i] == '-')
			break;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1 + i] = manufac[i];
	}

	return SUCCESS;
}

static int change_para_ssbus(unsigned char *rpara_databuf, int block_index, int issbus)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 1; i++) 
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 16 + i] = (issbus / 8);

	return SUCCESS;
}

static int change_para_hsbus(unsigned char *rpara_databuf, int block_index, int ihsbus)
{
	int i;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 1; i++) 
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 17 + i] = (ihsbus / 2);

	return SUCCESS;
}

static int change_para_wol(unsigned char *rpara_databuf, int block_index, char *wol)
{
	int i = 0;
	unsigned int dwolEn = 0;
	unsigned int s5wolEn = 0;
	unsigned int pmeEn = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (i = 0; i < 8; i++) {
		unsigned char bit_value = 0;
		bit_value = wol[i] & 1;

		if (i == 0 && bit_value == 1) {
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] &= ~0x01;
			break; 
		}
		if (i == 1 && bit_value == 1) {
			pmeEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x10;
		}
		if (i == 2 && bit_value == 1) {
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x1C;
		}
		if (i == 3 && bit_value == 1) {
			dwolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x1A;
		}
		if (i == 4 && bit_value == 1) {
			s5wolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x18;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] |= 0x28;
		}
		if (i == 5 && bit_value == 1) {
			s5wolEn = 1;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] |= 0x02;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x18;
			rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] |= 0x18;
		}

		if (dwolEn || s5wolEn || pmeEn) {
			if (i == 6 && bit_value == 1) {
				rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x80;
			}
			if (i == 7 && bit_value == 1) {
				rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] |= 0x40;
			}

		}
	}

	return SUCCESS;
}

static void set_para_led0(unsigned char *rpara_databuf, int block_index,  unsigned int *led)
{
	DEBUG_PRINT("=== %s - Start\n", __func__);

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3] = 0x24;

	if (led) {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] = led[0];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5] = led[1];

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9] = led[2];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = led[3];
	} else {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] = 0xC1;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5] = 0x03;

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9] = 0x00;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = 0x00;
	}
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 6] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 7] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 8] = 0x25;

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 18] = 0x45;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 19] = 0x0B;

}

static void set_para_led1(unsigned char *rpara_databuf, int block_index,  unsigned int *led)
{
	DEBUG_PRINT("=== %s - Start\n", __func__);

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 1] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 2] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 3] = 0x26;

	if (led) {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] = led[0];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5] = led[1];

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9] = led[2];
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = led[3];
	} else {
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 4] = 0xC0;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 5] = 0x00;

		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 9] = 0x0C;
		rpara_databuf[block_index * FLASH_BLOCK_SIZE + 10] = 0x0F;
	}
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 6] = 0x1F;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 7] = 0x00;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 8] = 0x27;

	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 18] = 0x45;
	rpara_databuf[block_index * FLASH_BLOCK_SIZE + 19] = 0x0B;
}

static int program_para_block(struct ax_command_info *info, unsigned char *rpara_databuf, int para_size)
{
	int ret;

	ret = write_flash(info, rpara_databuf, 0, para_size);
	if (ret < 0)
		return -1;
	
	return 0;
}

static int calculate_para_offset(void *buf)
{
	unsigned char *data = (unsigned char *)buf;
	int offset[8];
	int len[8];
	int max;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	offset[0] = SWAP_32(*(unsigned int *)&data[PRAM_PRI_FW1_OFFSET]);
	len[0] = SWAP_32(*(unsigned int *)&data[PRAM_PRI_FW1_LENGTH]);
	offset[1] = SWAP_32(*(unsigned int *)&data[MD32_PRI_FW1_OFFSET]);
	len[1] = SWAP_32(*(unsigned int *)&data[MD32_PRI_FW1_LENGTH]);
	offset[2] = SWAP_32(*(unsigned int *)&data[PRAM_SEC_FW1_OFFSET]);
	len[2] = SWAP_32(*(unsigned int *)&data[PRAM_SEC_FW1_LENGTH]);
	offset[3] = SWAP_32(*(unsigned int *)&data[MD32_SEC_FW1_OFFSET]);
	len[3] = SWAP_32(*(unsigned int *)&data[MD32_SEC_FW1_LENGTH]);
	offset[4] = SWAP_32(*(unsigned int *)&data[PRAM_PRI_FW2_OFFSET]);
	len[4] = SWAP_32(*(unsigned int *)&data[PRAM_PRI_FW2_LENGTH]);
	offset[5] = SWAP_32(*(unsigned int *)&data[MD32_PRI_FW2_OFFSET]);
	len[5] = SWAP_32(*(unsigned int *)&data[MD32_PRI_FW2_LENGTH]);
	offset[6] = SWAP_32(*(unsigned int *)&data[PRAM_SEC_FW2_OFFSET]);
	len[6] = SWAP_32(*(unsigned int *)&data[PRAM_SEC_FW2_LENGTH]);
	offset[7] = SWAP_32(*(unsigned int *)&data[MD32_SEC_FW2_OFFSET]);
	len[7] = SWAP_32(*(unsigned int *)&data[MD32_SEC_FW2_LENGTH]);

	max = find_offest_max_index(offset, 8);

	return (offset[max] + len[max] + 0x10000) & ~(0xFFFF);
}

void dump(unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("%02X ", buf[i]);
		if (i % 4 == 3)
			printf("\n");
	}
	printf("\n");
}

static void checksum_efuse_block(unsigned char *block)
{
	unsigned int Sum = 0;
	int j;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (j = 0; j < 4; j++) {
		if (j == 0)
			Sum += block[j] & 0xF;
		else
			Sum += block[j];
	}

	while (Sum > 0xF)
		Sum = (Sum & 0xF) + (Sum >> 4);

	Sum = 0xF - Sum;

	block[0] = (block[0] & 0xF) | ((Sum << 4) & 0xF0);
}

static void checksum_para_header(unsigned short *header)
{
	unsigned int Sum = 0;
	unsigned short j;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (j = 0; j < 5; j++)
		Sum += header[j];

	while (Sum > 0xFFFF)
		Sum = (Sum & 0xFFFF) + (Sum >> 16);

	Sum = 0xFFFF - Sum;

	header[5] = (Sum);
}

static int check_hex(char *temp, int size)
{
	int i = 0;
	unsigned char *ptemp = temp;

	for (i = 0; i < size; i++) {
		if (ptemp[i] == ':') {
			i++;
			continue;
		}
		
		if(!(isxdigit(ptemp[i])))
			return -FAIL_INVALID_PARAMETER;
	}

	return 0;
}

static int check_led_parameter(char *led)
{
	if (!led)
		return 1;

	if (strlen(led) != 9 || led[4] != '_')
		return 1;
	do {
		if (*led++ == '_')
			continue;
		if (!isxdigit(*led++))
			return 1;
	} while (*led);

	return 0;
}

static int writeparameter_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned char *rpara_buf = NULL;
	int para_offset, block_count, para_size, ret, block_index, c, i;
	int oi = -1;
	struct __wpara argument = {0};
	int temp_para_offset = 0;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	while ((c = getopt_long(info->argc, info->argv,
				"m:s:p:v:P:M:D:S:H:w:l:e:d:",
				long_options, &oi)) != -1) {
		switch (c) {
		case 'm':
			argument.mac_address = optarg;
			DEBUG_PRINT("%s \r\n", argument.mac_address);
			i = sscanf(argument.mac_address,
				   "%02X:%02X:%02X:%02X:%02X:%02X",
				   (unsigned int *)&argument.MAC[0],
				   (unsigned int *)&argument.MAC[1],
				   (unsigned int *)&argument.MAC[2],
				   (unsigned int *)&argument.MAC[3],
				   (unsigned int *)&argument.MAC[4],
				   (unsigned int *)&argument.MAC[5]);
			if (i != 6)
				return print_msg("wpara");
			break;
		case 's':
			argument.serial_num = optarg;
			DEBUG_PRINT("%s \r\n", argument.serial_num);
			if (strlen(argument.serial_num) > 18) {
				printf("characters must be less than 19 bytes\n");
				return print_msg("wpara");
			}
			break;
		case 'p':
			argument.PID = optarg;
			DEBUG_PRINT("%s \r\n", argument.PID);
			i = sscanf(argument.PID,
				   "%02X:%02X",
				   (unsigned int *)&argument.pid[0],
				   (unsigned int *)&argument.pid[1]);
			if (i != 2)
				return print_msg("wpara");
			break;
		case 'v':
			argument.VID = optarg;
			DEBUG_PRINT("%s \r\n", argument.VID);
			i = sscanf(argument.VID,
				   "%02X:%02X",
				   (unsigned int *)&argument.vid[0],
				   (unsigned int *)&argument.vid[1]);
			if (i != 2)
				return print_msg("wpara");
			break;
		case 'P':
			argument.product_string = optarg;
			DEBUG_PRINT("%s \r\n", argument.product_string);
			if (strlen(argument.product_string) > 18)
				return print_msg("wpara");
			break;
		case 'D':
			argument.dump = optarg;
			DEBUG_PRINT("%s \r\n", argument.dump);
			if (strcasecmp(argument.dump , "dump"))
				return print_msg("wpara");
			break;
		case 'M':
			argument.manufacture = optarg;
			DEBUG_PRINT("%s \r\n", argument.manufacture);
			if (strlen(argument.manufacture) > 18)
				return print_msg("wpara");
			break;
		case 'S':
			argument.ss_bus = optarg;
			argument.iss_bus = atoi(argument.ss_bus);
			DEBUG_PRINT("%s \r\n", argument.ss_bus);
			break;
		case 'H':
			argument.hs_bus = optarg;
			argument.ihs_bus = atoi(argument.hs_bus);
			DEBUG_PRINT("%s \r\n", argument.hs_bus);
			break;
		case 'w':
			argument.wol = optarg;
			DEBUG_PRINT("%s \r\n", argument.wol);
			break;
		case 'l':
			argument.led0 = optarg;
			DEBUG_PRINT("%s \r\n", argument.led0);
			i = sscanf(argument.led0,
				   "%02X%02X_%02X%02X",
				   (unsigned int *)&argument.LED0[0],
				   (unsigned int *)&argument.LED0[1],
				   (unsigned int *)&argument.LED0[2],
				   (unsigned int *)&argument.LED0[3]);
			break;
		case 'e':
			argument.led1 = optarg;
			DEBUG_PRINT("%s \r\n", argument.led1);
			i = sscanf(argument.led1,
				   "%02X%02X_%02X%02X",
				   (unsigned int *)&argument.LED1[0],
				   (unsigned int *)&argument.LED1[1],
				   (unsigned int *)&argument.LED1[2],
				   (unsigned int *)&argument.LED1[3]);
			break;
		case '?':
		default:
			return -FAIL_INVALID_PARAMETER;
		}
	}

	ret = scan_ax_device(ifr, info->inet_sock);
	if (ret < 0) {
		PRINT_SCAN_DEV_FAIL;
		return ret;
	}

	rpara_buf = (unsigned char *)malloc((FLASH_SIZE + 256) & ~(0xFF));
	if (!rpara_buf) {
		PRINT_ALLCATE_MEM_FAIL;
		ret = -FAIL_ALLCATE_MEM;
		goto fail;
	}
	memset(rpara_buf, 0xFF, (FLASH_SIZE + 256) & ~(0xFF));

	ret = read_flash(info, rpara_buf, 0, 0x3000);
	if (ret < 0)
		goto fail;

	//temp_para_offset = calculate_para_offset(rpara_buf);

	if (*(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET] != 0xA55A) {
		//printf("Not 0xA55A, use sample\n");
		para_offset = 0x3000;
		block_count = 0;
	} else {
		para_offset = SWAP_32(*(unsigned long *)&rpara_buf[PARAMETER_PRI_OFFSET]);
		block_count = SWAP_16(*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT]);
	}

	para_size = 0;
	if (block_count) {
		para_size = block_count * FLASH_BLOCK_SIZE;
		ret = read_flash(info, rpara_buf, para_offset, para_size);
		if (ret < 0)
			goto fail;
	}

	block_index = -FAIL_GENERIAL_ERROR;
	if (argument.mac_address) {
		if(check_hex(argument.mac_address, 17) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if(strlen(argument.mac_address) != 17) {
			printf("FAIL: MAC address should be 6 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type1, 20);
			para_size += 20;
		}

		ret = change_para_macaddr(&rpara_buf[para_offset], block_index, argument.MAC);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing MAC address failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.serial_num) {
		if(check_hex(argument.serial_num, strlen(argument.serial_num)) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_04);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type4, 20);
			para_size += 20;
		}

		ret = change_para_serialnum(&rpara_buf[para_offset], block_index, argument.serial_num);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing serial number failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.PID) {
		if(check_hex(argument.PID, 5) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if(strlen(argument.PID) != 5) {
			printf("FAIL: PID be 2 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type1, 20);
			para_size += 20;
		}

		ret = change_para_pid(&rpara_buf[para_offset], block_index, argument.pid);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing PID failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.VID) {
		if(check_hex(argument.VID, 5) == -FAIL_INVALID_PARAMETER) {
			printf("\nFAIL: Char should be '0'-'9' & 'A(a)'-'F(f)'\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if(strlen(argument.VID) != 5) {
			printf("FAIL: VID be 2 bytes\n");
			return -FAIL_INVALID_PARAMETER;
		}

		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type1, 20);
			para_size += 20;
		}

		ret = change_para_vid(&rpara_buf[para_offset], block_index, argument.vid);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing VID failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.product_string) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_03);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type3, 20);
			para_size += 20;
		}

		ret = change_para_productstr(&rpara_buf[para_offset], block_index, argument.product_string);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing Product String failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.manufacture) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_02);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type2, 20);
			para_size += 20;
		}

		ret = change_para_manufacture(&rpara_buf[para_offset], block_index, argument.manufacture);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing manufacture failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.ss_bus) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type1, 20);
			para_size += 20;
		}

		if (argument.iss_bus > 896)	{
			printf("FAIL: The value is between 0-896\n");
			return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_ssbus(&rpara_buf[para_offset], block_index, argument.iss_bus);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing SS_MAX_BUS_PW failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.hs_bus) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_01);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type1, 20);
			para_size += 20;
		}

		if (argument.ihs_bus > 500)	{
			printf("FAIL: The value is between 0-500\n");
			return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_hsbus(&rpara_buf[para_offset], block_index, argument.ihs_bus);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing HS_MAX_BUS_PW failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.wol) {
		if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_15);
		if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type15, 20);
			para_size += 20;
		}

		if (strlen(argument.wol) > 8)	{
			printf("FAIL: The value must be 8 digit\n");
			return -FAIL_INVALID_PARAMETER;
		}

		ret = change_para_wol(&rpara_buf[para_offset], block_index, argument.wol);
		if (ret < 0) {
			fprintf(stderr,
				"%s: Changing HS_MAX_BUS_PW failed.\n",
				__func__);
			goto fail;
		}
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.led0) {
	/*	if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_11);*/
	//	if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type11, 20);
			para_size += 20;
	//	}
		if (check_led_parameter(argument.led0)) {
			printf("FAIL: The value invaild.\n");
			return -FAIL_INVALID_PARAMETER;
		}

		set_para_led0(&rpara_buf[para_offset], block_index, argument.LED0);

		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.led1) {
	/*	if (para_size)
			block_index = find_block_index(&rpara_buf[para_offset], para_size, TYPE_11);*/
		//if (block_index == -FAIL_GENERIAL_ERROR) {
			block_index = para_size / 20;
			memcpy(&rpara_buf[para_offset + block_index * 20], sample_type11, 20);
			para_size += 20;
		//}
		if (check_led_parameter(argument.led1)) {
			printf("FAIL: The value invaild.\n");
			return -FAIL_INVALID_PARAMETER;
		}

		set_para_led1(&rpara_buf[para_offset], block_index, argument.LED1);
		
		checksum_efuse_block(&rpara_buf[para_offset + block_index * 20]);
	}

	if (argument.dump) {
		int i = 0;
		unsigned char buf[32 * 20];
		if (para_size == 0) {
			printf("\nThe parameter content currently in flash has no data\n");
			return -FAIL_INVALID_PARAMETER;
		}
		printf("\nDump the parameter content currently in flash as parameter.txt file\n");

		FILE *file = fopen("parameter.txt", "w");
		if (file == NULL) {
			perror("Error opening file");
			return -FAIL_INVALID_PARAMETER;
		}

		for (i = 0; i < para_size; i++) {
			buf[i] = rpara_buf[para_offset + i];
		}

		for (i = 0; i < para_size; i++) {
			fprintf(file, "%02X", buf[i]);

			if(((i + 1) % 4 == 0))
				fprintf(file, "\n");
			else
				fprintf(file, " ");
		}

		fclose(file);

		dump(&rpara_buf[para_offset], para_size);
	}

	erase_sector_flash(info, PARAMETER_PRI_HEADER_OFFSET);
	erase_sector_flash(info, para_offset);


	if (*(unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET] != 0xA55A) {
		//printf("Not 0xA55A, create header\n");
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET] = 0x5A;
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 1] = 0xA5;
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 2] = 0x04;
		rpara_buf[PARAMETER_PRI_HEADER_OFFSET + 3] = 0;
	}
	*(unsigned int *)&rpara_buf[PARAMETER_PRI_OFFSET] = SWAP_32(para_offset);
	*(unsigned short *)&rpara_buf[PARAMETER_PRI_BLOCK_COUNT] = SWAP_16(para_size / 20);
	checksum_para_header((unsigned short *)&rpara_buf[PARAMETER_PRI_HEADER_OFFSET]);

	write_flash(info, rpara_buf, PARAMETER_PRI_HEADER_OFFSET, 256);
	write_flash(info, rpara_buf, para_offset, para_size);

	sw_reset(info);
fail:
	if (rpara_buf)
		free(rpara_buf);

	return 0;
}

static int reload_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	char fw_version[16] = {0};

	DEBUG_PRINT("=== %s - Start\n", __func__);

	if (info->argc != 2) {
		int i;

		for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88279_cmd_list[i].cmd,
				    strlen(ax88279_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88279_cmd_list[i].help_ins,
				       ax88279_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	if (scan_ax_device(ifr, info->inet_sock)) {
		PRINT_SCAN_DEV_FAIL;
		return -FAIL_SCAN_DEV;
	}

	autosuspend_enable(info, 0);

	sw_reset(info);

	if (scan_ax_device(ifr, info->inet_sock)) {
		PRINT_SCAN_DEV_FAIL;
		return -FAIL_SCAN_DEV;
	}

	return SUCCESS;
}

static int scan_ax_device(struct ifreq *ifr, int inet_sock)
{
	unsigned int retry;

	DEBUG_PRINT("=== %s - Start\n", __func__);

	for (retry = 0; retry < SCAN_DEV_MAX_RETRY; retry++) {
		unsigned int i;
		struct _ax_ioctl_command ioctl_cmd;
#if NET_INTERFACE == INTERFACE_SCAN
		struct ifaddrs *addrs, *tmp;
		unsigned char	dev_exist;

		getifaddrs(&addrs);
		tmp = addrs;
		dev_exist = 0;

		while (tmp) {
			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

			sprintf(ifr->ifr_name, "%s", tmp->ifa_name);
			tmp = tmp->ifa_next;

			ioctl(inet_sock, SIOCGIFFLAGS, ifr);
			if (!(ifr->ifr_flags & IFF_UP))
				continue;

			ifr->ifr_data = (caddr_t)&ioctl_cmd;

			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88179A_DRV_NAME,
				    strlen(AX88179A_DRV_NAME)) == 0) {
				dev_exist = 1;
				break;
			}
		}

		freeifaddrs(addrs);

		if (dev_exist)
			break;
#else
		for (i = 0; i < 255; i++) {

			memset(&ioctl_cmd, 0,
			       sizeof(struct _ax_ioctl_command));
			ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

			sprintf(ifr->ifr_name, "eth%u", i);

			ioctl(inet_sock, SIOCGIFFLAGS, ifr);
			if (!(ifr->ifr_flags & IFF_UP))
				continue;

			ifr->ifr_data = (caddr_t)&ioctl_cmd;

			if (ioctl(inet_sock, AX_PRIVATE, ifr) < 0)
				continue;

			if (strncmp(ioctl_cmd.sig,
				    AX88179A_DRV_NAME,
				    strlen(AX88179A_DRV_NAME)) == 0)
				break;

		}

		if (i < 255)
			break;
#endif
		usleep(500000);
	}

	if (retry >= SCAN_DEV_MAX_RETRY)
		return -FAIL_SCAN_DEV;

	return SUCCESS;
}

int main(int argc, char **argv)
{
	struct ifreq ifr;
	struct ax_command_info info;
	unsigned int i;
	int inet_sock, ret = -FAIL_GENERIAL_ERROR;

	//printf("%s\n", AX88179A_IOCTL_VERSION);

	if (argc < 2) {
		show_usage();
		return SUCCESS;
	}

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifndef NOT_PROGRAM 
	if (scan_ax_device(&ifr, inet_sock)) {
		printf("No %s found\n", AX88179A_SIGNATURE);
		return FAIL_SCAN_DEV;
	}
#endif
	for (i = 0; ax88279_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(argv[1],
			    ax88279_cmd_list[i].cmd,
			    strlen(ax88279_cmd_list[i].cmd)) == 0) {
			info.help_ins = ax88279_cmd_list[i].help_ins;
			info.help_desc = ax88279_cmd_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = ax88279_cmd_list[i].ioctl_cmd;
			ret = (ax88279_cmd_list[i].OptFunc)(&info);
			goto out;
		}
	}

	if (ax88279_cmd_list[i].cmd == NULL) {
		show_usage();
		return SUCCESS;
	}
out:
	if (ret == SUCCESS)
		printf("SUCCESS\n");
	else if (ret != -FAIL_INVALID_PARAMETER)
		printf("FAIL\n");

	return ret;
}
