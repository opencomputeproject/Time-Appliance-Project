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
#ifndef __ASIX_AX88179_178A_H
#define __ASIX_AX88179_178A_H

#define AX88179_NAPI_WEIGHT		64
#define AX88179_BUF_RX_SIZE		(48 * 1024)
#define AX88179_PHY_ID			0x03

extern const struct net_device_ops ax88179_netdev_ops;
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
int ax88179_siocdevprivate(struct net_device *netdev, struct ifreq *rq,
			   void __user *udata, int cmd);
#endif
int ax88179_ioctl(struct net_device *net, struct ifreq *rq, int cmd);

int ax88179_set_mac_addr(struct net_device *net, void *p);
void ax88179_set_multicast(struct net_device *net);

extern const struct driver_info ax88179_info;
#endif
