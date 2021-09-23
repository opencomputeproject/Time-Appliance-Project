#pragma once

#include "clock.h"
#include <algorithm>
#include <atomic>
#include <memory>
#include <random>
#include <regex>
#include <thread>

#define PTP_ERROR_NSEC 25
struct TimeStamp
{
  int64_t earliest_ts_max = 0;
  int64_t latest_ts_max = 0;
  int64_t earliest_ts_est = 0;
  int64_t latest_ts_est = 0;
  int64_t earliest_ts_ptp = 0;
  int64_t latest_ts_ptp = 0;
};

class Clock;

class Base
{
public:
  Base(std::shared_ptr<Clock> local_clock,
       std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsMax,
       std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsEst,
       uint64_t msg_stdout_freq)
      : local_clock_(local_clock), maxEarliestTsMax_(maxEarliestTsMax),
        maxEarliestTsEst_(maxEarliestTsEst), msg_stdout_freq_(msg_stdout_freq),
        threads_(), gen(std::random_device()()) {}

  void updateMaxEarliestTs(uint64_t ts, bool isMax)
  {
    std::shared_ptr<std::atomic<uint64_t>> maxEarliestTs = maxEarliestTsEst_;
    if (isMax)
      maxEarliestTs = maxEarliestTsMax_;
    while (true)
    {
      uint64_t oldVal = maxEarliestTs->load();
      ts = std::max(ts, oldVal);
      if (maxEarliestTs->compare_exchange_weak(oldVal, ts))
      {
        return;
      }
    }
  }

  virtual void startThread() = 0;

  void waitForThreads()
  {
    // Leave this as a sequential joining since we don't
    // need the parallelism here.
    for (auto &thread : threads_)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
  }

  uint64_t drawNumber(uint64_t min, uint64_t max)
  {
    // Creating a distribution object is very lightweight, thus not performance
    // concern.
    return std::uniform_int_distribution<uint64_t>{min, max}(gen);
  }

  bool doIt(uint64_t min, uint64_t max) { return drawNumber(min, max) == min; }

protected:
  std::shared_ptr<Clock> local_clock_;
  std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsMax_;
  std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsEst_;
  uint64_t msg_stdout_freq_;
  std::vector<std::thread> threads_;
  std::mt19937 gen;
};
