#pragma once

#include "base.h"
#include "clock.h"
#include "logger.h"
#include "socket.h" // For UDPSocket and SocketException
#include <cstdlib>  // For atoi()
#include <iostream> // For cout and cerr
#include <thread>

const int MAXRCVSTRING = 4096; // Longest string to receive

class Receiver : public Base
{
public:
  Receiver(std::shared_ptr<Clock> local_clock, uint32_t listenPort,
           std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsMax,
           std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsEst,
           uint64_t msg_stdout_freq)
      : Base(local_clock, maxEarliestTsMax, maxEarliestTsEst, msg_stdout_freq),
        sock_(listenPort), recvString_() {}

  void receiveTime(Logger &logger, Logger &logTimstamp)
  {
    string sourceAddress;      // Address of datagram source
    unsigned short sourcePort; // Port of datagram source
    char *end;
    try
    {
      int bytesRcvd =
          sock_.recvFrom(recvString_, MAXRCVSTRING, sourceAddress, sourcePort);
      recvString_[bytesRcvd] = '\0'; // Terminate string

      TimeStamp remote;

      try
      {
        //  "ntp_max=[1585709907131772us,1585709908086640us,954868us]
        //  ntp_est=[1585709907609147us,1585709907609265us,118us]
        //  ptp=[25ns,75ns,50ns]"
        const std::regex msg_format(
            R"(ntp_max=\[(\d+)us,(\d+)us,(\d+)us\] ntp_est=\[(\d+)us,(\d+)us,(\d+)us\] ptp=\[(\d+)ns,(\d+)ns,(\d+)ns\])");

        std::cmatch match;
        if (std::regex_match(recvString_, match, msg_format) &&
            match.size() == 9 + 1)
        {
          remote.earliest_ts_max = std::strtoll(match.str(1).c_str(), &end, 10);
          remote.latest_ts_max = std::strtoll(match.str(2).c_str(), &end, 10);
          remote.earliest_ts_est = std::strtoll(match.str(4).c_str(), &end, 10);
          remote.latest_ts_est = std::strtoll(match.str(5).c_str(), &end, 10);
          remote.earliest_ts_ptp = std::strtoll(match.str(7).c_str(), &end, 10);
          remote.latest_ts_ptp = std::strtoll(match.str(8).c_str(), &end, 10);
        }
        else
        {
          std::stringstream str;
          str << "ERR: "
              << " " << sourceAddress << ":" << sourcePort << " |"
              << recvString_ << "|\n";
          cout << str.str();
          cout.flush();
          logger.invalidMessage(str.str());
          return;
        }

        updateMaxEarliestTs(remote.earliest_ts_max, true);
        updateMaxEarliestTs(remote.earliest_ts_est, false);

        if (doIt(1, msg_stdout_freq_))
        {
          std::stringstream str;
          str << "<<< " << sourceAddress << ":" << sourcePort << " "
              << recvString_ << "\n";
          cout << str.str();
          cout.flush();
        }
      }
      catch (const std::regex_error &ex)
      {
        std::stringstream str;
        str << "ERR: " << ex.what() << " | " << sourceAddress << ":"
            << sourcePort << " " << recvString_ << "\n";
        cout << str.str();
        cout.flush();
        logger.invalidMessage(recvString_);
        return;
      }

      std::vector<Time> local_ts = local_clock_->now(logger);

      if (local_ts[0] == INVALID_TIME)
      {
        return;
      }

      TimeStamp local;
      local.earliest_ts_max = (int64_t)local_ts[0].first;
      local.latest_ts_max = (int64_t)local_ts[0].second;
      local.earliest_ts_est = (int64_t)local_ts[1].first;
      local.latest_ts_est = (int64_t)local_ts[1].second;
      local.earliest_ts_ptp = (int64_t)local_ts[2].first;
      local.latest_ts_ptp = (int64_t)local_ts[2].second;

      int64_t value;

      logTimstamp.logTimestamp(local, remote);

      value = min(local.latest_ts_max, remote.latest_ts_max) -
              max(local.earliest_ts_max, remote.earliest_ts_max);
      if (value < 0)
      {
        logger.logInconsistency("MAX", value, local, remote);
      }
      value = min(local.latest_ts_est, remote.latest_ts_est) -
              max(local.earliest_ts_est, remote.earliest_ts_est);
      if (value < 0)
      {
        logger.logInconsistency("EST", value, local, remote);
      }
      value = min(local.latest_ts_ptp, remote.latest_ts_ptp) -
              max(local.earliest_ts_ptp, remote.earliest_ts_ptp);
      if (value < 0)
      {
        logger.logInconsistency("PTP", value, local, remote);
      }

      // if (local_ts[0].second < remote_earliest_ts_max) {
      //   logger.logInconsistency(remote_earliest_ts_max, local_ts[0].second,
      //                           true);
      // }
      // if (local_ts[1].second < remote_earliest_ts_est) {
      //   logger.logInconsistency(remote_earliest_ts_est, local_ts[1].second,
      //                           false);
      // }
    }
    catch (SocketException &e)
    {
      cerr << e.what() << endl;
      exit(1);
    }
  }

  void receiver()
  {
    Logger logger("receiver.log");
    Logger logTimstamp("timestamp.log");
    while (true)
    {
      receiveTime(logger, logTimstamp);
    }
  }

  void startThread() override
  {
    threads_.emplace_back(&Receiver::receiver, this);
  }

private:
  UDPSocket sock_;
  char recvString_[MAXRCVSTRING + 1];
};
