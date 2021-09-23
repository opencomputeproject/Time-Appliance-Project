#pragma once

#include "base.h"
#include "clock.h"
#include "logger.h"
#include "socket.h" // For UDPSocket and SocketException
#include <algorithm>
#include <chrono>
#include <cstdlib>  // For atoi()
#include <iostream> // For cout and cerr
#include <memory>
#include <random>
#include <string>
#include <thread>

class Sender : public Base
{
public:
  Sender(std::shared_ptr<Clock> local_clock,
         vector<pair<std::string, uint32_t>> &peers,
         std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsMax,
         std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsEst,
         uint64_t interval_ns, uint64_t msg_stdout_freq,
         uint64_t maxEarliestTsSyncFreq = 100)
      : Base(local_clock, maxEarliestTsMax, maxEarliestTsEst, msg_stdout_freq),
        peers_(peers), interval_ns_(interval_ns), sock_(),
        maxEarliestTsSyncFreq_(maxEarliestTsSyncFreq) {}

  std::string preparePayload(Logger &logger)
  {
    vector<Time> timestamps = local_clock_->now(logger);
    if (timestamps[0] == INVALID_TIME)
    {
      return "INVALID TIME";
    }
    uint64_t earliestTsMax = timestamps[0].first;
    uint64_t latestTsMax = timestamps[0].second;
    uint64_t earliestTsEst = timestamps[1].first;
    uint64_t latestTsEst = timestamps[1].second;
    uint64_t earliestTsPTP = timestamps[2].first;
    uint64_t latestTsPTP = timestamps[2].second;

    return "ntp_max=[" + std::to_string(earliestTsMax) + "us," +
           std::to_string(latestTsMax) + "us," +
           std::to_string(latestTsMax - earliestTsMax) + "us]" + " ntp_est=[" +
           std::to_string(earliestTsEst) + "us," + std::to_string(latestTsEst) +
           "us," + std::to_string(latestTsEst - earliestTsEst) + "us]" +
           " ptp=[" + std::to_string(earliestTsPTP) + "ns," +
           std::to_string(latestTsPTP) + "ns," +
           std::to_string(latestTsPTP - earliestTsPTP) + "ns]";
  }

  void sender(int tindex, std::string addr, uint32_t port)
  {
    Logger logger("sender_" + std::to_string(tindex) + ".log");
    Address address(IPMODE, addr, port);
    try
    {
      // Repeatedly send the string (not including \0) to the server
      for (;;)
      {
        std::string payload = preparePayload(logger);
        if (doIt(1, msg_stdout_freq_))
        {
          std::stringstream str;
          str << ">>> " << addr << ":" << port << " " << payload << "\n";
          cout << str.str();
          cout.flush();
        }
        sock_.sendTo(payload.c_str(), payload.size(), address);
        std::this_thread::sleep_for(std::chrono::nanoseconds(interval_ns_));
      }
    }
    catch (SocketException &e)
    {
      cerr << e.what() << endl;
      exit(1);
    }
  }

  void startThread() override
  {
    int index = 0;
    for (auto &peer : peers_)
    {
      threads_.emplace_back(&Sender::sender, this, index++, peer.first,
                            peer.second);
    }
  }

private:
  vector<pair<std::string, uint32_t>> peers_;
  uint64_t interval_ns_;
  UDPSocket sock_;

  uint64_t maxEarliestTsSyncFreq_; // 1/maxEarliestTsSyncFreq_ is the actual
                                   // frequency
};
