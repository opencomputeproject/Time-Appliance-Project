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
#if NET_INTERFACE == INTERFACE_SCAN
#include <ifaddrs.h>
#endif
#include "ax_ioctl.h"

#define AX88179_IOCTL_VERSION \
"AX88179/AX88178A Linux EEPROM/eFuse Programming Tool v1.5.0"

const char help_str1[] =
"./ax88179_programmer help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char readeeprom_str1[] =
"./ax88179_programmer reeprom [type] [file] [size]\n"
"    -- AX88179_178A EEPROM/eFuse read tool\n";
const char readeeprom_str2[] =
"        [type]    - 0: EEPROM,  1: eFuse\n"
"        [file]    - Output file\n"
"        [size]    - EEPROM/eFuse SIZE (bytes). EEPROM maximum 512 bytes,\n"
"                    eFuse maximum 64 bytes.\n";

const char writeeeprom_str1[] =
"./ax88179_programmer weeeprom [type] [file] [size]\n"
"    -- AX88179_178A EEPROM/eFuse write tool\n";
const char writeeeprom_str2[] =
"        [type]    - 0: EEPROM,  1: eFuse\n"
"        [file]    - Input file\n"
"        [size]    - EEPROM/eFuse SIZE (bytes). EEPROM size 12-512 bytes,\n"
"                    eFuse maximum 64 bytes.\n";

const char chgmac_str1[] =
"./ax88179_programmer chgmac [type] [mac_addr] [size]\n"
"    -- AX88179_178A EEPROM/eFuse write tool (specify MAC address)\n";
const char chgmac_str2[] =
"        [type]    - 0: EEPROM,  1: eFuse\n"
"        [mac_addr]- MAC address (xx:xx:xx:xx:xx:xx)\n"
"        [size]    - EEPROM/eFuse SIZE (bytes). EEPROM size 12-512 bytes,\n"
"                    eFuse maximum 64 bytes.\n";

static int help_func(struct ax_command_info *info);
static int readeeprom_func(struct ax_command_info *info);
static int writeeeprom_func(struct ax_command_info *info);
static int chgmac_func(struct ax_command_info *info);
struct _command_list ax88179_cmd_list[] = {
	{
		"help",
		AX_SIGNATURE,
		help_func,
		help_str1,
		help_str2
	},
	{
		"reeprom",
		AX88179_READ_EEPROM,
		readeeprom_func,
		readeeprom_str1,
		readeeprom_str2
	},
	{
		"weeprom",
		AX88179_WRITE_EEPROM,
		writeeeprom_func,
		writeeeprom_str1,
		writeeeprom_str2
	},
	{
		"chgmac",
		AX88179_WRITE_EEPROM,
		chgmac_func,
		chgmac_str1,
		chgmac_str2
	},
	{NULL},
};

static void show_usage(void)
{
	int i;

	printf("Usage:\n");
	for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++)
		printf("%s\n", ax88179_cmd_list[i].help_ins);
}

static unsigned long STR_TO_U32(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0, value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base)
			base = 8;
	}
	if (!base)
		base = 10;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;

	return result;
}

static int help_func(struct ax_command_info *info)
{
	int i;

	if (info->argv[2] == NULL) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			printf("%s%s\n", ax88179_cmd_list[i].help_ins,
			       ax88179_cmd_list[i].help_desc);
		}
	}

	for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
			    strlen(ax88179_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88179_cmd_list[i].help_ins,
			       ax88179_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}

	return SUCCESS;
}

static int compare_file(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	unsigned short *rout_buf;
	unsigned short *ori_buf;
	struct _ax_ioctl_command *ioctl_cmd =
				(struct _ax_ioctl_command *)(ifr->ifr_data);
	int i;

	rout_buf = malloc(sizeof(unsigned short) * ioctl_cmd->size);

	ori_buf = ioctl_cmd->buf;

	ioctl_cmd->ioctl_cmd = AX88179_READ_EEPROM;
	ioctl_cmd->buf = rout_buf;

	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	for (i = 0; i < ioctl_cmd->size; i++) {
		if (*(ioctl_cmd->buf + i) != *(ori_buf + i)) {
			ioctl_cmd->buf = ori_buf;
			free(rout_buf);
			return -1;
		}
	}

	ioctl_cmd->buf = ori_buf;
	free(rout_buf);
	return 0;
}

static int readeeprom_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	unsigned short *buf;
	unsigned short wLen;
	char str_buf[50];
	unsigned char type;
	FILE *pFile;
	int i;

	if (info->argc != 5) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	type = STR_TO_U32(info->argv[2], NULL, 0);
	wLen = STR_TO_U32(info->argv[4], NULL, 0) / 2;

	if ((type > 1) ||
	    ((type == 0) && (wLen > 256)) ||
	    ((type == 1) && (wLen > 32))) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	pFile = fopen(info->argv[3], "w");
	if (pFile == NULL) {
		printf("fail to open %s file\n", info->argv[3]);
		return -FAIL_LOAD_FILE;
	}

	buf = (unsigned short *)malloc(sizeof(unsigned short) * wLen);

	ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
	ioctl_cmd.size = wLen;
	ioctl_cmd.buf = buf;
	ioctl_cmd.type = type;
	ioctl_cmd.delay = 0;

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
		perror("ioctl");
		free(buf);
		fclose(pFile);
		return -FAIL_IOCTL;
	}

	for (i = 0; i < wLen / 8; i++) {
		int j = 8 * i;

		snprintf(str_buf, 50,
			 "%04x %04x %04x %04x %04x %04x %04x %04x\n",
			 *(buf + j + 0), *(buf + j + 1),
			 *(buf + j + 2), *(buf + j + 3),
			 *(buf + j + 4), *(buf + j + 5),
			 *(buf + j + 6), *(buf + j + 7));

		fputs(str_buf, pFile);
	}

	free(buf);
	fclose(pFile);
	printf("read completely\n");

	return SUCCESS;
}

static int writeeeprom_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int i;
	unsigned short *buf;
	unsigned short wLen;
	char c[2] = {'\0'};
	FILE *pFile;
	unsigned char retried = 0;
	unsigned char type;

	if (info->argc != 5) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	type = STR_TO_U32(info->argv[2], NULL, 0);
	wLen = STR_TO_U32(info->argv[4], NULL, 0) / 2;

	if ((type > 1) ||
	    ((type == 0) && (wLen > 256)) ||
	    ((type == 0) && (wLen < 6)) ||
	    ((type == 1) && (wLen > 32))) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	pFile = fopen(info->argv[3], "r");
	if (pFile == NULL) {
		printf("fail to open %s file\n", info->argv[3]);
		return -FAIL_LOAD_FILE;
	}

	buf = (unsigned short *)malloc(sizeof(unsigned short) * wLen);

	for (i = 0; i < wLen / 8; i++) {
		int j = 8 * i;

		fscanf(pFile, "%04X %04X %04X %04X %04X %04X %04X %04X%c",
				(unsigned int *)&buf[j + 0],
				(unsigned int *)&buf[j + 1],
				(unsigned int *)&buf[j + 2],
				(unsigned int *)&buf[j + 3],
				(unsigned int *)&buf[j + 4],
				(unsigned int *)&buf[j + 5],
				(unsigned int *)&buf[j + 6],
				(unsigned int *)&buf[j + 7], c);
	}

	ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
	ioctl_cmd.size = wLen;
	ioctl_cmd.buf = buf;
	ioctl_cmd.delay = 5;

	if (type) {
		ioctl_cmd.type = 2;
		ifr->ifr_data = (caddr_t)&ioctl_cmd;
		if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
			free(buf);
			fclose(pFile);
			perror("ioctl");
			return -FAIL_IOCTL;
		}
		if (ioctl_cmd.type) {
			printf("EFuse has been programed.\n");
			return -FAIL_INVALID_PARAMETER;
		}
	}

	ioctl_cmd.type = type;
io:
	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
		free(buf);
		fclose(pFile);
		perror("ioctl");
		return -FAIL_IOCTL;
	}
	if (compare_file(info) && retried < 3) {
		ioctl_cmd.delay += 5;
		ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
		retried++;
		goto io;
	}
	if (retried == 3) {
		printf("Failure to write\n");
		free(buf);
		fclose(pFile);
		return -FAIL_GENERIAL_ERROR;
	}

	printf("Write completely\n");
	free(buf);
	fclose(pFile);

	return SUCCESS;
}

static int chgmac_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int i;
	unsigned short *buf;
	unsigned short wLen;
	unsigned char retried = 0;
	unsigned int MAC[6] = {0};
	int ret = 0;
	unsigned char type;
	char c[2] = {'\0'};
	FILE *pFile;

	if (info->argc != 5) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	type = STR_TO_U32(info->argv[2], NULL, 0);
	wLen = STR_TO_U32(info->argv[4], NULL, 0) / 2;

	if ((type > 1) ||
	    ((type == 0) && (wLen > 256)) ||
	    ((type == 0) && (wLen < 6)) ||
	    ((type == 1) && (wLen > 32))) {
		for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179_cmd_list[i].cmd,
				    strlen(ax88179_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179_cmd_list[i].help_ins,
				       ax88179_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	buf = (unsigned short *)malloc(sizeof(unsigned short) * wLen);

	if (type) {
		pFile = fopen("efuse", "r");
		if (pFile == NULL) {
			printf("fail to open 'efuse' file\n");
			free(buf);
			return -FAIL_LOAD_FILE;
		}

		for (i = 0; i < wLen / 8; i++) {
			int j = 8 * i;

			fscanf(pFile, "%04X %04X %04X %04X %04X %04X %04X %04X%c",
					(unsigned int *)&buf[j + 0],
					(unsigned int *)&buf[j + 1],
					(unsigned int *)&buf[j + 2],
					(unsigned int *)&buf[j + 3],
					(unsigned int *)&buf[j + 4],
					(unsigned int *)&buf[j + 5],
					(unsigned int *)&buf[j + 6],
					(unsigned int *)&buf[j + 7], c);
		}
	} else {
		ioctl_cmd.ioctl_cmd = AX88179_READ_EEPROM;
		ioctl_cmd.size = wLen;
		ioctl_cmd.buf = buf;
		ioctl_cmd.delay = 0;
		ioctl_cmd.type = type;

		ifr->ifr_data = (caddr_t)&ioctl_cmd;

		if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
			perror("ioctl");
			free(buf);
			return -FAIL_IOCTL;
		}
	}

	ret = sscanf(info->argv[3], "%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned int *)&MAC[0],
					(unsigned int *)&MAC[1],
					(unsigned int *)&MAC[2],
					(unsigned int *)&MAC[3],
					(unsigned int *)&MAC[4],
					(unsigned int *)&MAC[5]);
	if (ret != 6) {
		printf("Invalid MAC address\n");
		return -FAIL_INVALID_PARAMETER;
	}

	*(((char *)buf) + 0) = (unsigned char)MAC[1];
	*(((char *)buf) + 1) = (unsigned char)MAC[0];
	*(((char *)buf) + 2) = (unsigned char)MAC[3];
	*(((char *)buf) + 3) = (unsigned char)MAC[2];
	*(((char *)buf) + 4) = (unsigned char)MAC[5];
	*(((char *)buf) + 5) = (unsigned char)MAC[4];

	ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
	ioctl_cmd.size = wLen;
	ioctl_cmd.buf = buf;
	ioctl_cmd.delay = 5;
	ioctl_cmd.type = type;

	if (type) {
		ioctl_cmd.type = 2;
		ifr->ifr_data = (caddr_t)&ioctl_cmd;
		if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
			free(buf);
			perror("ioctl");
			return -FAIL_IOCTL;
		} else if (ioctl_cmd.type) {
			printf("EFuse has been programed.\n");
		}
	}
io:
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
		perror("ioctl");
		free(buf);
		return -FAIL_IOCTL;
	}
	if (compare_file(info) && retried < 3) {
		ioctl_cmd.delay += 5;
		ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
		retried++;
		goto io;
	}
	if (retried == 3) {
		printf("Failure to write\n");
		free(buf);
		return -FAIL_GENERIAL_ERROR;
	}

	printf("Chgmac completely\n");
	free(buf);

	return SUCCESS;
}

int main(int argc, char **argv)
{
	int inet_sock;
	struct ifreq ifr;
	struct ax_command_info info;
	unsigned char i;
	unsigned char count = 0;
	const unsigned char length = sizeof(char);
	struct _ax_ioctl_command ioctl_cmd;
#if NET_INTERFACE == INTERFACE_SCAN
	struct ifaddrs *addrs, *tmp;
	unsigned char	dev_exist;
#endif

	printf("\n%s\n", AX88179_IOCTL_VERSION);

	if (argc < 2) {
		show_usage();
		return 0;
	}

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
#if NET_INTERFACE == INTERFACE_SCAN
	getifaddrs(&addrs);
	tmp = addrs;
	dev_exist = 0;

	while (tmp) {
		memset(&ioctl_cmd, 0, sizeof(struct _ax_ioctl_command));
		ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

		sprintf(ifr.ifr_name, "%s", tmp->ifa_name);
		tmp = tmp->ifa_next;

		ioctl(inet_sock, SIOCGIFFLAGS, &ifr);
		if (!(ifr.ifr_flags & IFF_UP))
			continue;

		ifr.ifr_data = (caddr_t)&ioctl_cmd;

		if (ioctl(inet_sock, AX_PRIVATE, &ifr) < 0)
			continue;

		if (strncmp(ioctl_cmd.sig, AX88179_DRV_NAME,
			    strlen(AX88179_DRV_NAME)) == 0) {
			dev_exist = 1;
			break;
		}
	}

	freeifaddrs(addrs);

	if (dev_exist == 0) {
		printf("No %s found\n", AX88179_SIGNATURE);
		return 0;
	}
#else
	for (i = 0; i < 255; i++) {

		memset(&ioctl_cmd, 0, sizeof(struct _ax_ioctl_command));
		ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

		sprintf(ifr.ifr_name, "eth%d", i);

		ioctl(inet_sock, SIOCGIFFLAGS, &ifr);
		if (!(ifr.ifr_flags & IFF_UP))
			continue;

		ifr.ifr_data = (caddr_t)&ioctl_cmd;

		if (ioctl(inet_sock, AX_PRIVATE, &ifr) < 0)
			continue;

		if (strncmp(ioctl_cmd.sig, AX88179_DRV_NAME,
			    strlen(AX88179_DRV_NAME)) == 0)
			break;
	}

	if (i == 255) {
		printf("No %s found\n", AX88179_SIGNATURE);
		return 0;
	}
#endif
	for (i = 0; ax88179_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(argv[1], ax88179_cmd_list[i].cmd,
			    strlen(ax88179_cmd_list[i].cmd)) == 0) {
			info.help_ins = ax88179_cmd_list[i].help_ins;
			info.help_desc = ax88179_cmd_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = ax88179_cmd_list[i].ioctl_cmd;
			(ax88179_cmd_list[i].OptFunc)(&info);
			return 0;
		}
	}
	printf("Wrong command\n");

	return 0;
}
