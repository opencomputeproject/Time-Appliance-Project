#include "clock.h"
#include "receiver.h"
#include "sender.h"

//############################ Configurations ###############################
int IPMODE;
// msg_stdout_freq: the sender/receiver sends the message to stdout once every
// msg_stdout_freq messages.
uint64_t msg_stdout_freq;

// Sender configuration
// send_interval_ns: the waiting time between two consecutive multicasting.
uint64_t send_interval_ns;
// max_earliest_ts_sync_freq: the sender syncs the received maximum earliest ts
// with the local
//      earliest once every max_earliest_ts_sync_freq messages.
uint64_t max_earliest_ts_sync_freq;
// peers: a list of peers to multicast the messages.
vector<pair<std::string, uint32_t>> peers;

// Receiver configuration
// listenc_port: listening port.
uint32_t listen_port;

// Clock configurations
// threshold_error_us: if the clock returns the max error above the threshold,
// log it.
uint64_t threshold_error_us;

// PTP clock configurations
// threshold_error_ns: if the PTP clock returns the max error above the
// threshold, log it.
uint64_t threshold_error_ns;

//###########################################################################
// TODO: ingest the configuration more dynamically instead of hardcoding here.
//   potential options are JSON files as the config format.

// Config file: TODO: integrate more configurations in the file
// e.g.
// 192.168.0.1:8080
// 192.168.0.2:8080
std::string configfile;

void ReadConfig() {
  IPMODE = IPv4;
  msg_stdout_freq = 5;

  // sender
  send_interval_ns = SecToNanoSec(1);
  max_earliest_ts_sync_freq = 5;

  // receiver
  listen_port = 8080;

  // clock
  threshold_error_us = 10000;
  threshold_error_ns = 50;

  // Read peers from files
  configfile = "config";
  std::ifstream in(configfile.c_str());
  if (!in) {
    std::cerr << "Cannot open the File : " << configfile << std::endl;
    exit(1);
  }
  std::string line;
  // Read the next line from the file untill it reaches the end.
  while (std::getline(in, line, ':')) {
    // Line contains string of length > 0 then save it in vector
    // e.g. 192.168.0.1:8080
    peers.emplace_back(make_pair("", 8080));
    if (line.size() > 0) {
      peers.back().first = line;
      if (!std::getline(in, line)) {
        std::cerr << "config file format errors" << std::endl;
        exit(1);
      }
      char *end;
      peers.back().second = std::strtoull(line.c_str(), &end, 10);
    }
  }
  // Close The File
  in.close();
}

int main() {
  ReadConfig();
  std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsMax =
      std::make_shared<std::atomic<uint64_t>>(0);
  std::shared_ptr<std::atomic<uint64_t>> maxEarliestTsEst =
      std::make_shared<std::atomic<uint64_t>>(0);
  std::shared_ptr<Clock> local_clock =
      std::make_shared<Clock>(threshold_error_us, threshold_error_ns);
  Sender sender(local_clock, peers, maxEarliestTsMax, maxEarliestTsEst,
                send_interval_ns, msg_stdout_freq, max_earliest_ts_sync_freq);
  Receiver receiver(local_clock, listen_port, maxEarliestTsMax,
                    maxEarliestTsEst, msg_stdout_freq);
  sender.startThread();
  receiver.startThread();
  sender.waitForThreads();
  receiver.waitForThreads();
  return 0;
}
