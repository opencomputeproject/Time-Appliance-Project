#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/thread.h>
#include <event2/util.h>
#include <fcntl.h>
#include <linux/ethtool.h>
#include <linux/ptp_clock.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "chrony.h"

#define CLOCK_INVALID -1
#define FD_TO_CLOCKID(fd) ((clockid_t)((((unsigned int)~fd) << 3) | 3))
#define CLOCKID_TO_FD(clk) ((unsigned int)~((clk) >> 3))
#define EVUTIL_ERR_RW_RETRIABLE(e) ((e) == EINTR || (e) == EAGAIN)

struct Msg {
  uint64_t ptpNow;
};

#define PORT 8088

struct Node {
  std::string addr;
  evutil_socket_t fd;
};
std::vector<Node> nodes;

evutil_socket_t chronyCmdMon;

const size_t TimestampVectorSize = 16 * 1024;
std::mutex mutexPacketTimestamp;
struct PacketTimestamp {
  uint64_t remote;
  uint64_t local;
  char remoteaddr[INET_ADDRSTRLEN];
};
std::vector<PacketTimestamp> timestampBuffer[2];
int activeTimestampBuffer;

clockid_t clkPTPid;

clockid_t phc_open(const char *phc) {
  clockid_t clkid;
  struct timespec ts;
  struct timex tx = {};
  int fd;

  fd = open(phc, O_RDWR);
  if (fd < 0) return CLOCK_INVALID;

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

struct sk_ts_info {
  int valid;
  int phc_index;
  unsigned int so_timestamping;
  unsigned int tx_types;
  unsigned int rx_filters;
};

int sk_get_ts_info(const char *name, struct sk_ts_info *sk_info) {
  //#ifndef ETHTOOL_GET_TS_INFO
  struct ethtool_ts_info info = {};
  struct ifreq ifr = {};
  int fd, err;

  info.cmd = ETHTOOL_GET_TS_INFO;
  strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
  ifr.ifr_data = (char *)&info;
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket failed: ");
    goto failed;
  }

  err = ioctl(fd, SIOCETHTOOL, &ifr);
  if (err < 0) {
    perror("ioctl SIOCETHTOOL failed: ");
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
  //#endif
  /* clear data and ensure it is not marked valid */
  memset(sk_info, 0, sizeof(struct sk_ts_info));
  return -1;
}

clockid_t posix_clock_open(const char *device, int *phc_index) {
  struct sk_ts_info ts_info = {};
  char phc_device[19];
  int clkid;

  /* check if device is valid phc device */
  clkid = phc_open(device);
  if (clkid != CLOCK_INVALID) {
    return clkid;
  }

  /* check if device is a valid ethernet device */
  if (sk_get_ts_info(device, &ts_info) || !ts_info.valid) {
    printf("unknown clock %s: %m\n", device);
    return CLOCK_INVALID;
  }

  if (ts_info.phc_index < 0) {
    printf("interface %s does not have a PHC\n", device);
    return CLOCK_INVALID;
  }

  snprintf(phc_device, sizeof(phc_device), "/dev/ptp%d", ts_info.phc_index);
  clkid = phc_open(phc_device);
  if (clkid == CLOCK_INVALID) {
    printf("cannot open %s for %s: %m\n", phc_device, device);
    return CLOCK_INVALID;
  }
  *phc_index = ts_info.phc_index;
  return clkid;
}

evutil_socket_t getUdpSocket(const char *hostname, int port) {
  evutil_addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags = EVUTIL_AI_ADDRCONFIG;

  evutil_addrinfo *answer = nullptr;
  int err = evutil_getaddrinfo(hostname, std::to_string(port).c_str(), &hints,
                               &answer);
  if (err != 0) {
    fprintf(stderr, "Error while resolving '%s': %s", hostname,
            evutil_gai_strerror(err));
    exit(-1);
  }
  evutil_socket_t sock =
      socket(answer->ai_family, answer->ai_socktype, answer->ai_protocol);
  if (sock < 0) {
    exit(-1);
  }
  if (connect(sock, answer->ai_addr, answer->ai_addrlen)) {
    EVUTIL_CLOSESOCKET(sock);
    exit(-1);
  }
  return sock;
}

uint64_t getPTPNow() {
  struct timespec ts = {};
  clock_gettime(clkPTPid, &ts);
  std::chrono::seconds seconds(ts.tv_sec);
  return ts.tv_nsec + std::chrono::nanoseconds(seconds).count();
}

void timeout_cb(evutil_socket_t /*fd*/, short /*ev*/, void * /*arg*/) {
  for (auto &n : nodes) {
    Msg msg;
    msg.ptpNow = getPTPNow();
    send(n.fd, &msg, sizeof(msg), MSG_CONFIRM);
  }

  CMD_Request request = {};
  request.command = htons(REQ_TRACKING);
  request.pkt_type = PKT_TYPE_CMD_REQUEST;
  request.res1 = 0;
  request.res2 = 0;
  request.pad1 = 0;
  request.pad2 = 0;

  static int sequence = 0;
  request.sequence = sequence++;
  request.attempt = htons(0);
  request.version = PROTO_VERSION_NUMBER;

  send(chronyCmdMon, (void *)&request, MAX_PADDING_LENGTH, MSG_CONFIRM);

  struct timeval tv;
  UTI_DoubleToTimeval(0.5, &tv);

  fd_set rdfd;
  FD_ZERO(&rdfd);
  FD_SET(chronyCmdMon, &rdfd);
  auto select_status = select(chronyCmdMon + 1, &rdfd, NULL, NULL, &tv);
  if (select_status < 0) {
    printf("select failed : %s", strerror(errno));
  } else if (select_status == 0) {
    printf("reading from chronyd timeout");
  } else {
    CMD_Reply reply = {};
    auto recv_status = recv(chronyCmdMon, &reply, sizeof(reply), 0);
    if (recv_status < 0) {
      printf("reading from chronyd failed");
    } else {
      if ((reply.version != PROTO_VERSION_NUMBER) ||
          reply.pkt_type != PKT_TYPE_CMD_REPLY || reply.res1 != 0 ||
          reply.res2 != 0 || reply.command != request.command ||
          reply.sequence != request.sequence) {
        printf("chronyd invalid reply");
      } else {
        // auto ref_id = ntohl(reply.data.tracking.ref_id);
        printf(
            "Offset: %.3fus "
            "RMSOffset: %.3fus "
            "Frequency: %.3fppm "
            "ResidualFreq: %.3fppm "
            "Skew: %.3fppm "
            "RootDelay: %.3fus "
            "RootDispersion: %.3fus "
            "UpdateInterval: %.1f sec\n",
            1e6 * UTI_FloatNetworkToHost(reply.data.tracking.last_offset),
            1e6 * UTI_FloatNetworkToHost(reply.data.tracking.rms_offset),
            UTI_FloatNetworkToHost(reply.data.tracking.freq_ppm),
            UTI_FloatNetworkToHost(reply.data.tracking.resid_freq_ppm),
            UTI_FloatNetworkToHost(reply.data.tracking.skew_ppm),
            1e6 * UTI_FloatNetworkToHost(reply.data.tracking.root_delay),
            1e6 * UTI_FloatNetworkToHost(reply.data.tracking.root_dispersion),
            UTI_FloatNetworkToHost(reply.data.tracking.last_update_interval));
      }
    }
  }

  {
    std::lock_guard<std::mutex> guard(mutexPacketTimestamp);
    activeTimestampBuffer = 1 - activeTimestampBuffer;
  }

  for (auto &ts : timestampBuffer[activeTimestampBuffer]) {
    if (ts.local < ts.remote) {
      printf("TIMEERROR local:%lu remote:%lu difference -%ld %s\n", ts.local,
             ts.remote, (ts.remote - ts.local), ts.remoteaddr);
    }
    printf("%lu %lu %ld %s\n", ts.local, ts.remote,
           (long)(ts.local - ts.remote), ts.remoteaddr);
  }

  timestampBuffer[activeTimestampBuffer].clear();
  timestampBuffer[activeTimestampBuffer].reserve(TimestampVectorSize);
}

void do_read(evutil_socket_t fd, short events, void *arg) {
  auto now = getPTPNow();

  Msg msg;
  sockaddr_storage their_addr;
  ev_socklen_t addrlen = sizeof(their_addr);
  auto r = recvfrom(fd, &msg, sizeof(msg), 0, (struct sockaddr *)&their_addr,
                    &addrlen);

  if (r < 0) {
    int err = evutil_socket_geterror(fd);
    if (EVUTIL_ERR_RW_RETRIABLE(err)) {
      return;
    }
  }

  if (r > 0) {
    PacketTimestamp packet;
    packet.local = now;
    packet.remote = msg.ptpNow;
    inet_ntop(AF_INET, &(((struct sockaddr_in *)&their_addr)->sin_addr),
              packet.remoteaddr, INET_ADDRSTRLEN);
    std::lock_guard<std::mutex> guard(mutexPacketTimestamp);
    timestampBuffer[activeTimestampBuffer].emplace_back(packet);
  }
}

static void write_to_file_cb(int severity, const char *msg) {
  std::stringstream str;
  switch (severity) {
    case _EVENT_LOG_DEBUG:
      str << "DBG (libevent): ";
      break;
    case _EVENT_LOG_MSG:
      str << "MSG (libevent): ";
      break;
    case _EVENT_LOG_WARN:
      str << "WRN (libevent): ";
      break;
    case _EVENT_LOG_ERR:
      str << "ERR (libevent): ";
      break;
    default:
      str << "??? (libevent): ";
      break;
  }
  str << msg;
  std::cout << str.str() << std::endl;
}

void initLibEvent() {
  setvbuf(stdout, NULL, _IONBF, 0);
  event_set_log_callback(write_to_file_cb);
  if (evthread_use_pthreads() == -1) {
    perror("evthread_use_pthreads");
    exit(-1);
  }
}

void printHeader() {
  std::cout << "Linearizable Clock UDP" << std::endl
            << event_get_version() << " (";
  const char **methods = event_get_supported_methods();
  for (int i = 0; methods[i] != NULL; ++i) {
    std::cout << methods[i] << ",";
  }
  std::cout << "\b)" << std::endl;
}

void printBaseInfo(event_base *base) {
  if (!base) {
    perror("event_base_new");
    exit(-2);
  }
  std::cout << "Using Libevent with backend method "
            << event_base_get_method(base) << std::endl;
  auto f = event_base_get_features(base);
  if ((f & EV_FEATURE_ET))
    std::cout << "  Edge-triggered events are supported." << std::endl;
  if ((f & EV_FEATURE_O1))
    std::cout << "  O(1) event notification is supported." << std::endl;
  if ((f & EV_FEATURE_FDS))
    std::cout << "  All FD types are supported." << std::endl;
}

void initPTPClock() {
  int fp;
  char defaultPtpName[] = "/dev/ptp0";
  auto ptpName = getenv("PTPCLOCK");
  if (ptpName == nullptr) {
    ptpName = defaultPtpName;
  }
  std::cout << "PTP clock: " << ptpName << std::endl;
  clkPTPid = posix_clock_open(ptpName, &fp);
  if (clkPTPid == CLOCK_INVALID) {
    std::cout << "posix_clock_open(\"" << ptpName
              << "\") failed switching to CLOCK_REALTIME!" << std::endl;
    clkPTPid = CLOCK_REALTIME;
  }
  std::cout << "Clock: " << clkPTPid << std::endl;
}

int main(int argc, char **argv) {
  initLibEvent();
  printHeader();

  initPTPClock();

  activeTimestampBuffer = 0;
  timestampBuffer[0].reserve(TimestampVectorSize);
  timestampBuffer[1].reserve(TimestampVectorSize);

  event_base *base = event_base_new();
  printBaseInfo(base);

  std::cout << "Sends packets to: " << std::endl;
  for (int i = 1; i < argc; ++i) {
    Node n;
    n.addr = argv[i];
    n.fd = getUdpSocket(n.addr.c_str(), PORT);
    nodes.emplace_back(n);
    std::cout << "  " << n.addr << ":" << PORT << std::endl;
  }

  evutil_socket_t listener = socket(AF_INET, SOCK_DGRAM, 0);
  evutil_make_socket_nonblocking(listener);

  chronyCmdMon = getUdpSocket("127.0.0.1", 323);

  sockaddr_in sin = {};
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(PORT);
  if (bind(listener, (sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    return -3;
  }

  event *read_event =
      event_new(base, listener, EV_READ | EV_PERSIST, do_read, (void *)base);
  event_add(read_event, NULL);

  event timeOut;
  event_assign(&timeOut, base, -1, EV_PERSIST, timeout_cb, (void *)&timeOut);

  timeval tv;
  evutil_timerclear(&tv);
  tv.tv_sec = 1;
  event_add(&timeOut, &tv);

  event_base_dispatch(base);

  return 0;
}