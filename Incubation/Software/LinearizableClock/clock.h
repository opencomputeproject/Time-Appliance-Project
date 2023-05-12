#pragma once

#include "logger.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <sys/timex.h>
#include <vector>

using namespace std;

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif

clockid_t posix_clock_open(const char *device, int *phc_index);
void posix_clock_close(clockid_t clock);

using Time = pair<uint64_t, uint64_t>;
const Time INVALID_TIME = make_pair(0, 0);

inline uint64_t SecToMicroSec(uint64_t sec)
{
  std::chrono::seconds seconds(sec);
  return std::chrono::microseconds(seconds).count();
}

inline uint64_t SecToNanoSec(uint64_t sec)
{
  std::chrono::seconds seconds(sec);
  return std::chrono::nanoseconds(seconds).count();
}

class Clock
{
  clockid_t clkPTPid;

public:
  Clock(uint64_t threshold_error_us, uint64_t threshold_error_ns)
      : numOfTotalNowCalls_(0), numOfFailedNowCalls_(0),
        threshold_error_us_(threshold_error_us),
        threshold_error_ns_(threshold_error_ns)
  {
    int fp;
    clkPTPid = posix_clock_open("/dev/ptp0", &fp);
    if (clkPTPid == CLOCK_INVALID)
    {
      cout << "posix_clock_open failed!" << endl;
    }
  }

  ~Clock()
  {
    if (clkPTPid != CLOCK_INVALID)
    {
      posix_clock_close(clkPTPid);
    }
  }

  uint64_t getPTPTime(Logger &logger)
  {
    if (clkPTPid == CLOCK_INVALID)
    {
      logger.unavailablePosixClock();
      return threshold_error_ns_;
    }

    struct timespec ts = {};
    clock_gettime(clkPTPid, &ts);
    return SecToNanoSec(ts.tv_sec) + ts.tv_nsec;
  }

  // Returns the current time/max error and checks if the clock is synchronized.
  std::vector<Time> now(Logger &logger)
  {
    numOfTotalNowCalls_++;
    std::vector<Time> result({INVALID_TIME, INVALID_TIME, INVALID_TIME});
    Time &timeMaxError = result[0];
    Time &timeEstError = result[1];
    Time &timePTP = result[2];

    ntptimeval timeval;
    if (!getClockTime(&timeval))
    {
      numOfFailedNowCalls_++;
      logger.unavailable(numOfFailedNowCalls_.load(),
                         numOfTotalNowCalls_.load());
    }
    else
    {
      uint64_t now_usec =
          SecToMicroSec(timeval.time.tv_sec) + timeval.time.tv_usec;

      uint64_t error_usec = timeval.maxerror;
      timeMaxError.first = now_usec - error_usec;
      timeMaxError.second = now_usec + error_usec;
      if (error_usec > threshold_error_us_)
      {
        logger.errorBound(timeMaxError.first, timeMaxError.second, true);
      }

      error_usec = timeval.esterror;
      timeEstError.first = now_usec - error_usec;
      timeEstError.second = now_usec + error_usec;
      if (error_usec > threshold_error_us_)
      {
        logger.errorBound(timeEstError.first, timeEstError.second, false);
      }

      // TODO: Find the error boundary. For now set to 50ns
      uint64_t error_nsec = PTP_ERROR_NSEC;
      uint64_t now_nsec = getPTPTime(logger);
      timePTP.first = now_nsec - error_nsec;
      timePTP.second = now_nsec + error_nsec;
      if (error_nsec > threshold_error_ns_)
      {
        logger.errorBound(timePTP.first, timePTP.second, true);
      }
    }
    return result;
  }

private:
  bool getClockTime(ntptimeval *timeval)
  {
    int rc = ntp_gettime(timeval);
    switch (rc)
    {
    case TIME_OK:
      return true;
    case -1: // generic error
      std::cerr << "Error reading clock. ntp_gettime() failed : rc=" << rc
                << std::endl;
    case TIME_ERROR:
      std::cerr << "Error reading clock. Clock considered unsynchronized"
                << std::endl;
    default:
      // TODO what to do about leap seconds? see KUDU-146
      std::cerr << "Server undergoing leap second. This may cause consistency "
                   "issues (rc="
                << rc << ")";
      return false;
    }
  }

private:
  std::atomic<uint64_t> numOfTotalNowCalls_;
  std::atomic<uint64_t> numOfFailedNowCalls_;
  uint64_t threshold_error_us_;
  uint64_t threshold_error_ns_;
};
