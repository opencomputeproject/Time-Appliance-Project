#pragma once

#include "base.h"
#include <fstream>
#include <sstream>

class Logger
{
public:
  Logger(std::string filename) : filename_(filename)
  {
    logger.open(filename_, std::ofstream::app);
  }

  ~Logger() { logger.close(); }

  void logInconsistency(uint64_t received_ts, uint64_t local_ts, bool is_max)
  {
    std::string LOGNAME = "INCONSISTENCY";
    LOGNAME += (is_max ? "(max error): " : "(est error): ");

    logger << LOGNAME << "received timestamp is " << received_ts
           << ", local timestamp is " << local_ts << std::endl;
  }

  void errorBound(uint64_t earliestTs, uint64_t latestTs, bool is_max)
  {
    std::string LOGNAME = "LOCAL CLOCK EXCEEDS ERROR BOUND";
    LOGNAME += (is_max ? "(max error us): " : "(est error us): ");
    logger << LOGNAME << (latestTs - earliestTs) << std::endl;
  }

  void unavailable(uint64_t numOfFailedNowCalls, uint64_t numOfTotalNowCalls)
  {
    static const std::string LOGNAME = "UNAVAILABLE: ";
    logger << LOGNAME << numOfFailedNowCalls << " failed calls among "
           << numOfTotalNowCalls << "since the beginning." << std::endl;
  }

  void logClock(uint64_t earliestTs, uint64_t latestTs, bool is_max)
  {
    std::string LOGNAME = "LOCAL CLOCK";
    LOGNAME += (is_max ? "(max error us): " : "(est error us): ");
    logger << LOGNAME << (latestTs - earliestTs) << std::endl;
  }

  void unavailablePosixClock()
  {
    static const std::string LOGNAME = "UNAVAILABLE: ";
    logger << LOGNAME << "posix_clock_open failed" << std::endl;
  }

  void invalidMessage(std::string msg)
  {
    static const std::string LOGNAME = "INVALID: ";
    logger << LOGNAME << msg << std::endl;
  }

  void logInconsistency(std::string where, int64_t value,
                        TimeStamp const &local, TimeStamp const &remote)
  {
    std::string LOGNAME = "INCONSISTENCY";
    std::stringstream str;
    str << LOGNAME << where << " error=" << value << " "
        << "local("
        << "ntp_est=[" << std::to_string(local.earliest_ts_max) << "us,"
        << std::to_string(local.latest_ts_max) << "us, "
        << std::to_string(local.latest_ts_max - local.earliest_ts_max) << "us] "
        << "ntp_max=[" << std::to_string(local.earliest_ts_est) << "us,"
        << std::to_string(local.latest_ts_est) << "us, "
        << std::to_string(local.latest_ts_est - local.earliest_ts_est) << "us] "
        << "ptp=[" << std::to_string(local.earliest_ts_ptp) << "ns,"
        << std::to_string(local.latest_ts_ptp) << "ns, "
        << std::to_string(local.latest_ts_ptp - local.earliest_ts_ptp) << "ns] "
        << ") remote("
        << "ntp_est=[" << std::to_string(remote.earliest_ts_max) << "us,"
        << std::to_string(remote.latest_ts_max) << "us, "
        << std::to_string(remote.latest_ts_max - remote.earliest_ts_max)
        << "us] "
        << "ntp_max=[" << std::to_string(remote.earliest_ts_est) << "us,"
        << std::to_string(remote.latest_ts_est) << "us] "
        << std::to_string(remote.latest_ts_est - remote.earliest_ts_est)
        << "us] "
        << "ptp=[" << std::to_string(remote.earliest_ts_ptp) << "ns,"
        << std::to_string(remote.latest_ts_ptp) << "ns]"
        << std::to_string(remote.latest_ts_ptp - remote.earliest_ts_ptp)
        << "ns] "
        << ")";
    logger << str.str() << std::endl;
  }

  void logTimestamp(TimeStamp const &local, TimeStamp const &remote)
  {
    std::stringstream str;
    str << std::to_string(local.earliest_ts_ptp + PTP_ERROR_NSEC) << " "
        << std::to_string(local.earliest_ts_max) << " "
        << std::to_string(local.latest_ts_max) << " "
        << std::to_string(local.latest_ts_max - local.earliest_ts_max) << " "
        << std::to_string(local.earliest_ts_est) << " "
        << std::to_string(local.latest_ts_est) << " "
        << std::to_string(local.latest_ts_est - local.earliest_ts_est) << " "
        << std::to_string(local.earliest_ts_ptp) << " "
        << std::to_string(local.latest_ts_ptp) << " "
        << std::to_string(local.latest_ts_ptp - local.earliest_ts_ptp) << " "
        << std::to_string(remote.earliest_ts_max) << " "
        << std::to_string(remote.latest_ts_max) << " "
        << std::to_string(remote.latest_ts_max - remote.earliest_ts_max) << " "
        << std::to_string(remote.earliest_ts_est) << " "
        << std::to_string(remote.latest_ts_est) << " "
        << std::to_string(remote.latest_ts_est - remote.earliest_ts_est) << " "
        << std::to_string(remote.earliest_ts_ptp) << " "
        << std::to_string(remote.latest_ts_ptp) << " "
        << std::to_string(remote.latest_ts_ptp - remote.earliest_ts_ptp);
    logger << str.str() << std::endl;
  }

private:
  std::string filename_;
  std::ofstream logger;
};
