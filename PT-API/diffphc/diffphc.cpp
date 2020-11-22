#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/ptp_clock.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

const int MaxAttempts = 5;
const int64_t TAIOffset = 37'000'000;
const int64_t PHCCallMaxDelay = 100'000;

#define CHECK_FATAL(fnc)                                           \
  if (!(fnc)) {                                                    \
    std::cerr << "ERR: fatal: " << #fnc << " failed" << std::endl; \
    exit(-1);                                                      \
  }

bool debug = false;

std::string getPHCFileName(int phc_index) {
  std::stringstream s;
  s << "/dev/ptp" << phc_index;
  return s.str();
}

int64_t getCPUNow() {
  struct timespec ts = {};
  clock_gettime(CLOCK_REALTIME, &ts);
  std::chrono::seconds seconds(ts.tv_sec);
  return ts.tv_nsec + std::chrono::nanoseconds(seconds).count();
}

int64_t getPTPSysOffsetExtended(int clkPTPid, int samples);

int openPHC(const std::string& pch_path) {
  int phc_fd = open(pch_path.c_str(), O_RDONLY);
  if (phc_fd >= 0) {
    // close fd at process exit
    auto flags = fcntl(phc_fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(phc_fd, F_SETFD, flags);
  }
  return phc_fd;
}

bool printClockInfo(int phc_index) {
  auto name = getPHCFileName(phc_index);
  int phc_fd = open(name.c_str(), O_RDONLY);
  if (phc_fd < 0) {
    return false;
  }
  std::cout << "PTP device " << name << std::endl;
  ptp_clock_caps caps = {};
  if (ioctl(phc_fd, PTP_CLOCK_GETCAPS, &caps)) {
    std::cout << "ioctl(PTP_CLOCK_GETCAPS) failed. errno: " << strerror(errno)
              << std::endl;
  }

  struct ptp_sys_offset_extended sys_off_ext = {};
  sys_off_ext.n_samples = 1;
  bool supportOffsetExtended =
      !ioctl(phc_fd, PTP_SYS_OFFSET_EXTENDED, &sys_off_ext);

  std::cout << caps.max_adj
            << " maximum frequency adjustment in parts per billon.\n"
            << caps.n_ext_ts << " external time stamp channels.\n"
            << "PPS callback: " << (caps.pps ? "TRUE" : "FALSE") << "\n"
            << caps.n_pins << " input/output pins.\n"
            << "PTP_SYS_OFFSET_EXTENDED support: "
            << (supportOffsetExtended ? "TRUE" : "FALSE") << "\n"
            << std::endl;
  close(phc_fd);
  return true;
}

void printClockInfoAll() {
  int phc_index = 0;
  while (printClockInfo(phc_index++))
    ;
  std::cout << (phc_index - 1) << " PTP device(s) found." << std::endl;
}

int optArgToInt() {
  try {
    return std::stoi(optarg);
  } catch (...) {
    std::cerr << "ERR: invalid argument '" << optarg << "'" << std::endl;
    exit(-1);
  }
}

void printHelp() {
  std::cout
      << "diffphc - a tool designed to show the difference between the PHCs\n"
      << "\nUsage:\n"
      << "  -c, --count     number of iterations. default +inf\n"
      << "  -l, --delay     delay between iterations in microseconds. default "
         "250ms\n"
      << "  -s, --samples   number of PHC reads. default 10\n"
      << "  -d, --device    add PTP device in the list of sampled devices.\n"
      << "  -i, --info      PTP clock capabilities.\n"
      << "  -h, --help      display this help and exit.\n"
      << "\n"
      << "Examples:\n"
      << " diffphc -c 100 -l 250000 -d 2 -d 0\n"
      << " diffphc -d 0 -d 1 -d 0\n"
      << std::endl;
}

int main(int argc, char** argv) {
  int count = 0;
  int delay = 100000;
  int samples = 10;
  bool info = false;
  bool help = false;
  std::vector<int> devices;

  struct option longopts[] = {{"count", 1, nullptr, 'c'},
                              {"delay", 1, nullptr, 'l'},
                              {"samples", 1, nullptr, 's'},
                              {"device", 1, nullptr, 'd'},
                              {"info", 0, nullptr, 'i'},
                              {"help", 0, nullptr, 'h'},
                              {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "c:l:d:s:igh", longopts, NULL)) != -1) {
    if (debug) {
      std::cout << "DBG:"
                << "c=" << char(c) << " optarg=" << (optarg ? optarg : "null")
                << std::endl;
    }
    switch (c) {
      case 'd':
        devices.push_back(optArgToInt());
        break;
      case 'c':
        count = optArgToInt();
        break;
      case 'l':
        delay = optArgToInt();
        break;
      case 's':
        samples = optArgToInt();
        break;
      case 'h':
        printHelp();
        return -1;
      case 'g':
        debug = true;
        break;
      case 'i':
        info = true;
        break;
      case 0:
        // set a variable, just keep going
        break;
      case '?':
      default:
        printHelp();
        exit(-1);
    }
  }

  if (count < 0) {
    std::cerr << "ERR: invalide count parameter" << std::endl;
    exit(-1);
  }
  if (delay < 1) {
    std::cerr << "ERR: invalid delay parameter" << std::endl;
    exit(-1);
  }

  if (geteuid() != 0) {
    std::cerr << "run the app as root" << std::endl;
    exit(-2);
  }

  std::sort(devices.begin(), devices.end());
  std::unique(devices.begin(), devices.end());

  if (info) {
    if (devices.empty()) {
      printClockInfoAll();
    } else {
      for (auto d : devices) {
        if (!printClockInfo(d)) {
          std::cerr << "ERR: device /dev/ptp" << d << " open failed"
                    << std::endl;
        }
      }
    }
    exit(-1);
  }

  if (debug) {
    std::cout << "Number of iterations: "
              << ((count == 0) ? "+inf" : std::to_string(count)) << "\n"
              << "Delay between iterations: " << delay << " us\n"
              << "Number of samples: " << samples << "\n";

    std::cout << "Devices: ";
    for (auto d : devices) {
      std::cout << "/dev/ptp" << d << " ";
    }
    std::cout << std::endl;
  }

  if (devices.empty()) {
    std::cerr << "ERR: No devices specified" << std::endl;
    exit(-1);
  }

  std::vector<int> dev;
  for (auto d : devices) {
    auto name = getPHCFileName(d);
    int fd = openPHC(name);
    if (fd < 1) {
      std::cerr << "ERR: PTP device " << name << " open failed" << std::endl;
      exit(-1);
    }

    dev.push_back(fd);
  }

  const int numDev = dev.size();
  std::vector<int64_t> ts(numDev);

  for (int c = 0; count == 0 || c < count; ++c) {
    int64_t baseTimestamp = getCPUNow();
    for (int d = 0; d < numDev; ++d) {
      int64_t now = getCPUNow();
      ts[d] = getPTPSysOffsetExtended(dev[d], samples) - (now - baseTimestamp);
    }

    // for (int i = 1; i < numDev; ++i) {
    //   std::cout << (ts[i] - ts[i - 1]) << "\t";
    // }
    // std::cout << std::endl;

    std::cout << "          ";
    for (int i = 0; i < numDev; ++i) {
      std::cout << "ptp" << devices[i] << "\t";
    }
    std::cout << "\n";
    for (int i = 0; i < numDev; ++i) {
      std::cout << "ptp" << devices[i] << "\t";
      for (int j = 0; j <= i; ++j) {
        std::cout << long(ts[i]) - long(ts[j]) << "\t";
      }
      std::cout << "\n";
    }
    std::cout << std::endl;

    usleep(delay);
  }
}

int64_t getPTPSysOffsetExtended(int clkPTPid, int samples) {
  int64_t t0[PTP_MAX_SAMPLES];
  int64_t t1[PTP_MAX_SAMPLES];
  int64_t t2[PTP_MAX_SAMPLES];
  int64_t delay[PTP_MAX_SAMPLES];

  samples = std::min(PTP_MAX_SAMPLES, samples);

  struct ptp_sys_offset_extended sys_off = {};
  sys_off.n_samples = samples;
  if (ioctl(clkPTPid, PTP_SYS_OFFSET_EXTENDED, &sys_off)) {
    std::cerr << "ERR: ioctl(PTP_SYS_OFFSET_EXTENDED) failed : "
              << strerror(errno) << std::endl;
    return 0;
  }

  int64_t mindelay = uint64_t(-1);
  for (int i = 0; i < samples; ++i) {
    t0[i] = sys_off.ts[i][0].nsec + 1000'000ULL * sys_off.ts[i][0].sec;
    t1[i] = sys_off.ts[i][1].nsec + 1000'000ULL * sys_off.ts[i][1].sec;
    t2[i] = sys_off.ts[i][2].nsec + 1000'000ULL * sys_off.ts[i][2].sec;
    delay[i] = t2[i] - t0[i];
    if (mindelay > delay[i]) {
      mindelay = delay[i];
    }
  }
  int count = 0;
  int64_t phcTotal = 0;
  int64_t sysTotal = 0;
  int64_t sysTime = 0;
  int64_t phcTime = 0;

  double delayTotal = 0.0;
  for (int i = 0; i < samples; ++i) {
    if (t2[i] < t0[i] || delay[i] > mindelay + PHCCallMaxDelay) {
      continue;
    }
    count++;
    if (count == 1) {
      sysTime = t0[i];
      phcTime = t1[i];
    }
    sysTotal += t0[i] - sysTime;
    phcTotal += t1[i] - phcTime;
    delayTotal += delay[i] / 2.0;
  }

  if (!count) {
    // TODO: fix
    return 0;
  }

  sysTime += (sysTotal + count / 2) / count + int64_t(delayTotal / count);
  phcTime += (phcTotal + count / 2) / count;

  if (debug) {
    std::cout << "offset=" << (phcTime - sysTime) << " delays = ";
    for (int i = 0; i < samples; ++i) {
      std::cout << " " << delay[i];
    }
    std::cout << "\n";
  }

  return getCPUNow() + phcTime - sysTime;
}