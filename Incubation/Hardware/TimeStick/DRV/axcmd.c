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

#define ASIX_IOCTL_VERSION "ASIX Linux Command Tool v1.0.0"

const char help_str1[] =
"./ioctl help [command]\n"
"    -- command description\n";
const char help_str2[] =
"        [command] - Display usage of specified command\n";

const char usbcommand_str1[] =
"./axcmd usb [ops] [cmd] [value] [index] [size] [data]\n"
"    -- ASIX USB Command\n";
const char usbcommand_str2[] =
"        [ops]     - 0: Read,  1: Write\n"
"        [cmd]     - USB Command (0 - 0xFF)\n"
"        [value]   - USB wvalue (0 - 0xFFFF)\n"
"        [index]   - USB windex (0 - 0xFFFF)\n"
"        [size]    - USB wlength (0 - 0xFFFF)\n"
"        [data]    - Data (0 - 0xFFFFFFFF)\n";

static int help_func(struct ax_command_info *info);
static int usbcommand_func(struct ax_command_info *info);

struct _command_list asix_cmd_list[] = {
	{	"help",
		AX_SIGNATURE,
		help_func,
		help_str1,
		help_str2
	},
	{	"usb",
		AX_USB_COMMAND,
		usbcommand_func,
		usbcommand_str1,
		usbcommand_str2
	},
	{NULL},
};

static void show_usage(void)
{
	int i;

	printf("Usage:\n");
	for (i = 0; asix_cmd_list[i].cmd != NULL; i++)
		printf("%s\n", asix_cmd_list[i].help_ins);
}

static unsigned long STR_TO_U32(const char *cp, char **endp,
				unsigned int base)
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
		for (i = 0; asix_cmd_list[i].cmd != NULL; i++) {
			printf("%s%s\n",
				asix_cmd_list[i].help_ins,
				asix_cmd_list[i].help_desc);
		}
	}

	for (i = 0; asix_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(info->argv[1],
			    asix_cmd_list[i].cmd,
			    strlen(asix_cmd_list[i].cmd)) == 0) {
			printf("%s%s\n",
				asix_cmd_list[i].help_ins,
				asix_cmd_list[i].help_desc);
			return -FAIL_INVALID_PARAMETER;
		}
	}

}

static int usbcommand_func(struct ax_command_info *info)
{
	struct ifreq *ifr = (struct ifreq *)info->ifr;
	struct _ax_ioctl_command ioctl_cmd;
	struct _ax_usb_command *usb_cmd;
	int i;

	if (info->argc > 8 || info->argc < 7) {
		for (i = 0; asix_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], asix_cmd_list[i].cmd,
				    strlen(asix_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", asix_cmd_list[i].help_ins,
						 asix_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	usb_cmd = &ioctl_cmd.usb_cmd;
	memset(usb_cmd, 0, sizeof(*usb_cmd));

	usb_cmd->ops = STR_TO_U32(info->argv[2], NULL, 16);
	usb_cmd->cmd = STR_TO_U32(info->argv[3], NULL, 16);
	usb_cmd->value = STR_TO_U32(info->argv[4], NULL, 16);
	usb_cmd->index = STR_TO_U32(info->argv[5], NULL, 16);
	usb_cmd->size = STR_TO_U32(info->argv[6], NULL, 16);

	if (usb_cmd->ops > USB_WRITE_OPS) {
		for (i = 0; asix_cmd_list[i].cmd != NULL; i++) {
			if (strncmp(info->argv[1], asix_cmd_list[i].cmd,
				    strlen(asix_cmd_list[i].cmd)) == 0) {
				printf("%s%s\n", asix_cmd_list[i].help_ins,
						 asix_cmd_list[i].help_desc);
				return -FAIL_INVALID_PARAMETER;
			}
		}
	}

	if (usb_cmd->ops == USB_WRITE_OPS)
		usb_cmd->cmd_data = STR_TO_U32(info->argv[7], NULL, 16);

	ioctl_cmd.ioctl_cmd = info->ioctl_cmd;
	ioctl_cmd.size = usb_cmd->size;
	ioctl_cmd.buf = NULL;
	ioctl_cmd.type = 0;
	ioctl_cmd.delay = 0;

	ifr->ifr_data = (caddr_t)&ioctl_cmd;

	if (ioctl(info->inet_sock, AX_PRIVATE, ifr) < 0) {
		perror("ioctl");
		return -FAIL_IOCTL;
	}

	if (usb_cmd->ops == USB_READ_OPS) {
		printf("Read Command: CMD: 0x%02X\n", usb_cmd->cmd);
		printf("wValue: 0x%04X, wIndex: 0x%04X, wLength: 0x%04X",
			usb_cmd->value, usb_cmd->index, usb_cmd->size);
		printf("\nData: 0x%08lX\n", usb_cmd->cmd_data);
	}

	printf("Command completely\n");

	return SUCCESS;
}

int main(int argc, char **argv)
{
	int inet_sock;
	struct ifreq ifr;
	struct ax_command_info info;
	unsigned char i;
	unsigned char count = 0;
	struct _ax_ioctl_command ioctl_cmd;
#if NET_INTERFACE == INTERFACE_SCAN
	struct ifaddrs *addrs, *tmp;
	unsigned char	dev_exist;
#endif

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
		memset(&ioctl_cmd, 0, sizeof(ioctl_cmd));
		ioctl_cmd.ioctl_cmd = AX_SIGNATURE;
		sprintf(ifr.ifr_name, "%s", tmp->ifa_name);
		ifr.ifr_data = (caddr_t)&ioctl_cmd;
		tmp = tmp->ifa_next;

		if (ioctl(inet_sock, AX_PRIVATE, &ifr) < 0)
			continue;

		if (strncmp(ioctl_cmd.sig, AX88179_DRV_NAME,
			    strlen(AX88179_DRV_NAME)) == 0) {
			dev_exist = 1;
			break;
		}

		if (strncmp(ioctl_cmd.sig, AX88179A_DRV_NAME,
			    strlen(AX88179A_DRV_NAME)) == 0) {
			dev_exist = 1;
			break;
		}
	}

	freeifaddrs(addrs);

	if (dev_exist == 0) {
		printf("No ASIX device found\n");
		return 0;
	}
#else
	for (i = 0; i < 255; i++) {

		memset(&ioctl_cmd, 0, sizeof(ioctl_cmd));
		ioctl_cmd.ioctl_cmd = AX_SIGNATURE;

		sprintf(ifr.ifr_name, "eth%d", i);

		ifr.ifr_data = (caddr_t)&ioctl_cmd;

		if (ioctl(inet_sock, AX_PRIVATE, &ifr) < 0)
			continue;

		if (strncmp(ioctl_cmd.sig, AX88179_DRV_NAME,
			    strlen(AX88179_DRV_NAME)) == 0)
			break;

		if (strncmp(ioctl_cmd.sig, AX88179A_DRV_NAME,
			    strlen(AX88179A_DRV_NAME)) == 0)
			break;
	}

	if (i == 255) {
		printf("No ASIX device found\n");
		return 0;
	}
#endif
	for (i = 0; asix_cmd_list[i].cmd != NULL; i++) {
		if (strncmp(argv[1], asix_cmd_list[i].cmd,
			    strlen(asix_cmd_list[i].cmd)) == 0) {
			info.help_ins = asix_cmd_list[i].help_ins;
			info.help_desc = asix_cmd_list[i].help_desc;
			info.ifr = &ifr;
			info.argc = argc;
			info.argv = argv;
			info.inet_sock = inet_sock;
			info.ioctl_cmd = asix_cmd_list[i].ioctl_cmd;
			(asix_cmd_list[i].OptFunc)(&info);
			return 0;
		}
	}

	printf("Wrong command\n");

	return 0;
}
