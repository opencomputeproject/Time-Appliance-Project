#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/ptp_clock.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "clock.h"

using namespace std;

#define CLOCKFD 3
#define FD_TO_CLOCKID(fd) ((clockid_t)((((unsigned int)~fd) << 3) | CLOCKFD))
#define CLOCKID_TO_FD(clk) ((unsigned int)~((clk) >> 3))

struct sk_ts_info {
  int valid;
  int phc_index;
  unsigned int so_timestamping;
  unsigned int tx_types;
  unsigned int rx_filters;
};

int sk_get_ts_info(const char *name, struct sk_ts_info *sk_info) {
#ifdef ETHTOOL_GET_TS_INFO
  struct ethtool_ts_info info;
  struct ifreq ifr;
  int fd, err;

  memset(&ifr, 0, sizeof(ifr));
  memset(&info, 0, sizeof(info));
  info.cmd = ETHTOOL_GET_TS_INFO;
  strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
  ifr.ifr_data = (char *)&info;
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (fd < 0) {
    pr_err("socket failed: %m");
    goto failed;
  }

  err = ioctl(fd, SIOCETHTOOL, &ifr);
  if (err < 0) {
    pr_err("ioctl SIOCETHTOOL failed: %m");
    close(fd);
    goto failed;
  }

  close(fd);

  /* copy the necessary data to sk_info */
  memset(sk_info, 0, sizeof(struct sk_ts_info));
  sk_info->valid = 1;
  sk_info->phc_index = info.phc_index;
  sk_info->so_timestamping = info.so_timestamping;
  sk_info->tx_types = info.tx_types;
  sk_info->rx_filters = info.rx_filters;

  return 0;
failed:
#endif
  /* clear data and ensure it is not marked valid */
  memset(sk_info, 0, sizeof(struct sk_ts_info));
  return -1;
}

clockid_t phc_open(const char *phc) {
  clockid_t clkid;
  struct timespec ts;
  struct timex tx;
  int fd;

  memset(&tx, 0, sizeof(tx));

  fd = open(phc, O_RDWR);
  if (fd < 0)
    return CLOCK_INVALID;

  clkid = FD_TO_CLOCKID(fd);
  /* check if clkid is valid */
  if (clock_gettime(clkid, &ts)) {
    close(fd);
    return CLOCK_INVALID;
  }
  if (clock_adjtime(clkid, &tx)) {
    close(fd);
    return CLOCK_INVALID;
  }

  return clkid;
}

void phc_close(clockid_t clkid) {
  if (clkid == CLOCK_INVALID)
    return;

  close(CLOCKID_TO_FD(clkid));
}

void posix_clock_close(clockid_t clock) {
  if (clock == CLOCK_REALTIME) {
    return;
  }
  phc_close(clock);
}

clockid_t posix_clock_open(const char *device, int *phc_index) {
  struct sk_ts_info ts_info;
  char phc_device[19];
  int clkid;

  /* check if device is valid phc device */
  clkid = phc_open(device);
  if (clkid != CLOCK_INVALID) {
    return clkid;
  }

  /* check if device is a valid ethernet device */
  if (sk_get_ts_info(device, &ts_info) || !ts_info.valid) {
    // pr_err("unknown clock %s: %m", device);
    return CLOCK_INVALID;
  }

  if (ts_info.phc_index < 0) {
    // pr_err("interface %s does not have a PHC", device);
    return CLOCK_INVALID;
  }

  snprintf(phc_device, sizeof(phc_device), "/dev/ptp%d", ts_info.phc_index);
  clkid = phc_open(phc_device);
  if (clkid == CLOCK_INVALID) {
    // pr_err("cannot open %s for %s: %m", phc_device, device);
  }
  *phc_index = ts_info.phc_index;
  return clkid;
}