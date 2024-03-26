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

#define PRINT_IOCTL_FAIL(ret) \
fprintf(stderr, "%s: ioctl failed. (err: %d)\n", __func__, ret)

#define AX88179A_IOCTL_VERSION \
"AX88179A/AX88772D Linux IEEE Test Tool v1.0.0"

const char help_str1[] =
"./ax88179a_772d_ieee help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char ieeetest_str1[] =
"./ax88179a_772d_ieee ieeetest [speed] [option]\n"
"    -- AX88179A_772D IEEE Test Tool\n";
const char ieeetest_str2[] =
"        [speed]    - 1000: 1000Mbps,  100: 100Mbps,  10: 10Mbps\n"
"        [option]   - For 1000Mbps\n"
"			M1: Mode 1\n"
"			M2: Mode 2\n"
"			M3: Mode 3\n"
"			M4: Mode 4\n\n"
"		    - For 100Mbps\n"
"			CA: Channel A\n"
"			CB: Channel B\n\n"
"		    - For 10Mbps\n"
"			RP: Random Pattern\n"
"			FF: Fixed Pattern(FF)\n"
"			MDI: MDI\n\n";

static int help_func(struct ax_command_info *info);
static int ieeetest_func(struct ax_command_info *info);
static int scan_ax_device(struct ifreq *ifr, int inet_sock);

struct _command_list ax88179a_cmd_list[] = {
	{
		"help",
		AX_SIGNATURE,
		help_func,
		help_str1,
		help_str2
	},
	{
		"ieeetest",
		AX88179A_READ_VERSION,
		ieeetest_func,
		ieeetest_str1,
		ieeetest_str2
	},
	{NULL},
};

#pragma pack(push)
#pragma pack(1)
#pragma pack(pop)

static void show_usage(void)
{
	int i;

	printf("Usage:\n");
	for (i = 0; ax88179a_cmd_list[i].cmd != NULL; i++)
		printf("%s\n", ax88179a_cmd_list[i].help_ins);
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

	if (info->argv[2] == NULL)
		return -FAIL_INVALID_PARAMETER;

	for (i = 0; ax88179a_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(info->argv[2],
			    ax88179a_cmd_list[i].cmd,
			    strlen(ax88179a_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n", ax88179a_cmd_list[i].help_ins,
			       ax88179a_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}

	return SUCCESS;
}

static int ieeetest_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	int ret;

	if (info->argc != 4) {
		int i;

		for (i = 0; ax88179a_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179a_cmd_list[i].cmd,
				    strlen(ax88179a_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179a_cmd_list[i].help_ins,
				       ax88179a_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	if (!strcmp(info->argv[2], "1000") && strlen(info->argv[2]) == 4) {
		ioctl_cmd.ieee.speed = 1000;
		if (!strcmp(info->argv[3], "M1")) {
			printf("Test item: 1000M M1\n");
			ioctl_cmd.ieee.type = IEEE_1000M1;
		} else if (!strcmp(info->argv[3], "M2")) {
			printf("Test item: 1000M M2\n");
			ioctl_cmd.ieee.type = IEEE_1000M2;
		} else if (!strcmp(info->argv[3], "M3")) {
			printf("Test item: 1000M M3\n");
			ioctl_cmd.ieee.type = IEEE_1000M3;
		} else if (!strcmp(info->argv[3], "M4")) {
			printf("Test item: 1000M M4\n");
			ioctl_cmd.ieee.type = IEEE_1000M4;
		} else {
			printf("Invalid option\n");
			return -FAIL_IVALID_VALUE;
		}
	} else if (!strcmp(info->argv[2], "100") &&
		   strlen(info->argv[2]) == 3) {
		ioctl_cmd.ieee.speed = 100;
		if (!strcmp(info->argv[3], "CA")) {
			printf("Test item: 100M Channel A\n");
			ioctl_cmd.ieee.type = IEEE_100CA;
		} else if (!strcmp(info->argv[3], "CB")) {
			printf("Test item: 100M Channel B\n");
			ioctl_cmd.ieee.type = IEEE_100CB;
		} else {
			printf("Invalid option\n");
			return -FAIL_IVALID_VALUE;
		}
	} else if (!strcmp(info->argv[2], "10") &&
		   strlen(info->argv[2]) == 2) {
		ioctl_cmd.ieee.speed = 10;
		if (!strcmp(info->argv[3], "RP")) {
			printf("Test item: 10M Random Pattern\n");
			ioctl_cmd.ieee.type = IEEE_10R;
		} else if (!strcmp(info->argv[3], "FF")) {
			printf("Test item: 10M Fixed Pattern(FF)\n");
			ioctl_cmd.ieee.type = IEEE_10FF;
		} else if (!strcmp(info->argv[3], "MDI")) {
			printf("Test item: 10M MDI\n");
			ioctl_cmd.ieee.type = IEEE_10MDI;
		} else {
			printf("Invalid option\n");
			return -FAIL_IVALID_VALUE;
		}
	} else {
		int i;

		for (i = 0; ax88179a_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], ax88179a_cmd_list[i].cmd,
				    strlen(ax88179a_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", ax88179a_cmd_list[i].help_ins,
				       ax88179a_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	ioctl_cmd.ieee.stop = 0;

	ioctl_cmd.ioctl_cmd = AX88179A_IEEE_TEST;
	ifr->ifr_data = (caddr_t)&ioctl_cmd;
	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		return -FAIL_IOCTL;
	}

	printf("Press Enter to stop testing...");
	getchar();

	ioctl_cmd.ieee.stop = 1;
	ret = ioctl(info->inet_sock, AX_PRIVATE, ifr);
	if (ret < 0) {
		PRINT_IOCTL_FAIL(ret);
		if (ioctl_cmd.ieee.status == -ERR_IEEE_INVALID_CHIP)
			printf("Invalid speed and chip\n");
		return -FAIL_IOCTL;
	}

	printf("Test completely\n");
	return SUCCESS;
}

static int scan_ax_device(struct ifreq *ifr, int inet_sock)
{
	unsigned int retry;

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

			ifr->ifr_data = (caddr_t)&ioctl_cmd;
			tmp = tmp->ifa_next;


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
	int inet_sock, ret;

	printf("%s\n", AX88179A_IOCTL_VERSION);

	if (argc < 2) {
		show_usage();
		return SUCCESS;
	}

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (scan_ax_device(&ifr, inet_sock)) {
		printf("No %s found\n", AX88179A_SIGNATURE);
		return FAIL_SCAN_DEV;
	}

	for (i = 0; ax88179a_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(argv[1],
			    ax88179a_cmd_list[i].cmd,
			    strlen(ax88179a_cmd_list[i].cmd)) == 0) {
			info.help_ins = ax88179a_cmd_list[i].help_ins;
			info.help_desc = ax88179a_cmd_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = ax88179a_cmd_list[i].ioctl_cmd;
			ret = (ax88179a_cmd_list[i].OptFunc)(&info);
			goto out;
		}
	}

	if (ax88179a_cmd_list[i].cmd == NULL) {
		printf("%u\n", i);
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
