# Clientgen
An open source stateful PTP client traffic generator based on PF_RING and simpleclient example. 

## Operation details

This utility is designed to simulate large number of PTP clients that go through the following sequence
1. Request Sync Unicast Grant for a specified duration
2. Request Announce Unicast Grant for a specified duration
3. Request Delay Response Unicast Grant for a specified duration
4. Periodically send DelayResp requests to the server while the Grants are active
5. Potentially restart after all the Grants have expired

### Configuration
The utility is configured through a single json configuration file, detailed below, clientgen_config.json
* The clients are specified by IP address start, IP address end, and IP address step size, similar to commercial traffic generators

### Operation

The utility runs via CLI , and provides printed output periodically (period specified in config) with detailed output, configurable based on the config.
* An example of the CLI output is below. Each section printed can be disabled if desired in the config.

### Incoming packets
The utility is based on pipelining incoming packets in the following manner:
1. RX ioWkr: Read an incoming packet from PF_RING buffer (including RX timestamp), pass it to packetParser
2. packetParser: Parse the incoming packet using gopacket to decode the packet layers, pass it to packetProcessor
3. packetProcessor: 
   * If packet is ARP (for IPv4) or ICMPv6 (for IPv6), craft a response based on client configuration. Pass response to txWkr
   * if packet is UDP , look into the UDP packet to figure out if it's for a simulated client and craft a response if needed. Pass response to txWkr.
   * If packet was from TX path (see below) , then this packet is just used to figure out where to store the timestamp that came with it (TX timestamp)


The utility works using low level packet crafting and packet capture to control each individual packet sent and handle each packet received.
* Because each packet is crafted, the utility requires pre-knowledge about the DUT MAC address in the config file
* The utility also requires the name of the interface, like ens1f0, in the config file to bind the sockets and pcap library to
* PF_RING is used for packet ingress. 
  * It allows all incoming packets on an interface to be round-robin distributed to an arbitrary number of worker goroutines
  * It allows for much larger packet buffering, buffering packets in the CPU memory rather than NIC buffer spaces
  * It removes polling the NIC from the user space, instead directly polling the NIC in the kernel and buffering them packets in user space buffers

### Outgoing packets
The utility has txWkrs to handle sending packets:
* Each packet sent to the txWkr has a flag if it needs timestamping or not
* If timestamping is not required, the packet is sent with a simple socket
* If timestamping is required, the packet is sent on a socket with timestamping enabled.
* Each txWkr has multiple goroutines polling its timestamped enabled socket to pull TX timestamps as soon as possible
* For each TX timestamp, the full packet sent is also read back. This packet is passed to packetParser with a flag indicating it was a sent packet

### Client processing 
The utility has retransmitWorkers to handle retransmitting packets:
* Each retransmitWorker maintains a heap based on time to specify when a client should be retransmitted if no response came in.
* Each heap item is associated with a particular client.
* This heap is used to retransmit grants. For example, if a client did not get Sync Grant from the server within a time, this processor will retransmit Sync Grant Request
* It's also used to retransmit if needed. For example, when a client has all the grants, it is used to send DelayReq periodically.

The utility has restartWorkers to handle restarting clients:
* Each restartWorker maintains a heap based on time if restart is enabled in the config
* When a client receives all three grants from the server, it pushes itself onto the heap to restart after all grants have expired.

## Installation guide

### PF_RING
1. Download and install pf_ring from ntop.org
```console
git clone https://github.com/ntop/PF_RING.git
```
2. Build the pf_ring kernel module
```console
cd PF_RING/kernel
make 
sudo make install
insmod ./pf_ring.ko
```
3. Build and install the API libraries for pf_ring
```console
cd PF_RING/userland/lib
./configure && make
sudo make install
cd ../libpcap
./configure && make
sudo make install
```
More details can be found here
	* https://www.ntop.org/guides/pf_ring/get_started/git_installation.html

### Pull clientgen
```console
git clone https://github.com/opencomputeproject/Time-Appliance-Project
```

### Build clientgen
1. Be sure to link LDD to where the pf_ring libraries are
```console
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/
```
2. Be sure to provide path to pf_ring header file. Change this path to where PF_RING was built.
```console
C_INCLUDE_PATH=~/ptp/PF_RING/kernel; export C_INCLUDE_PATH
```
3. Go to clientgen directory and build
```console
cd Software/Experimental/clientgen/
go build
```

## Usage guide
Clientgen is configured by a json config file, clientgen_config.json, in the same directory as the executable. Run it by running Clientgen in the clientgen directory. Change the json configuration prior to running to console the execution.

```console
cd clientgen/
./clientgen -config clientgen_config.json [-profilelog <cpuprofiler file>]
```

Description of items in this json file:
### Traffic and client configuration
* "Iface" - the interface on the server to generate client traffic from, ex. "ens1f0"
* "ServerMAC" - the MAC address of the PTP Grandmaster Server, ex. "0c:42:a1:80:31:66"
* "ServerAddress" - the IPv4 or IPv6 address of the PTP Grandmaster Server, ex. "10.254.254.254"
* "ClientIPStart" - IPv4 or IPv6. For the range of clients to generate, this is the IP address of the first client. ex. "10.1.1.2"
* "ClientIPEnd" - IPv4 or IPv6. For the range of clients to generate, this is the last client IP. ex. "10.1.1.10"
* "ClientIPStep" - For the clients to generate, how much to increment from ClientIPStart for each client. If ClientIPStart is 10.1.1.2 and this is 2, then it will generate clients 10.1.1.2 -> 10.1.1.4 -> 10.1.1.6 -> 10.1.1.8 etc. until ClientIPEnd
* "SoftStartRate" - The maximum number of clients to start per second
* "TimeoutSec" - how many seconds to run clientgen for, after which the program will stop generating traffic.
* "DurationSec" - The Grant Duration each client tries to subscribe to the PTP grandmaster when asking for UDP Grants, like Sync/Announce/DelayResp.
* "TimeAfterDurationBeforeRestartSec" - Time after a client's last grant expires to wait before restarting the client in seconds
* "TimeBetweenDelayReqSec" - After a client has all its grants, how many DelayReqs to send per second
* "ClientRetranTimeWhenNoResponseSec" - How many seconds a client should wait when requesting a grant before retransmitting the grant request if no response is received in seconds
### Performance controls
* "NumTXWorkers" - How many goroutines to spawn to handle transmitting packets. This may be the main bottleneck, due to TX Timestamping performance.
* "NumTXTSWorkerPerTx" - How many goroutines to spawn per TX worker to read TX timestamps.
* "NumRXWorkers" - How many goroutines to spawn to handle RX work. 
* "NumPacketParsers" - How many goroutines to spawn to Parse each received packet into a gopacket format for internal use.
* "NumPacketProcessors" - How many goroutines to spawn to handle each Parsed packet and possibly generate a response.
* "NumClientRetransmitProcs" - How many goroutines to spawn to manage internal timers for possible retransmit for each client. Work per goroutine will scale with client count.
* "NumClientRestartProcs" - How many goroutines to spawn to manage internal timers for restarting clients after their grants are over. Work per goroutine will scale with client count.
### Debug logging
* Enable these only for development and debug purposes, should be left false in almost all cases.
### Periodic statistics printing
* "PrintPerformance" - Prints the percentage each worker goroutine is busy. Use this to help fine tune the Performance Controls above to get the desired performance.
* "PrintClientData" - Prints information about all the clients, such as Total Announce Requests sent and Total Announce Grants received
* "PrintTxRxCounts" - Prints simple TX and RX packet counters
* "PrintClientReqData" - Prints histogram information for Announce Requests / Sync Requests / Delay Response Grant Requests / Delay Requests for all the clients
* "PrintLatencyData" - Prints statistical information for the server latency when responding to Announce Requests / Sync Requests / Delay Response Grant Requests / Delay Requests , and statistical information on the time between Sync packets from the grandmaster.
* "CounterPrintIntervalSecs" - How often to print the enabled statistics

## Example CLI output
When running with everything printing, this is an example of what the output statistics encompass.

```console
========================Statistics after 999.684ms============
==ClientData=============
0: TotalClients = 228, rate 0
1: TotalPacketsSent = 912, rate 912
2: TotalPacketsRcvd = 1596, rate 1596
3: TotalTXTSPacketsSent = 912, rate 912
4: TotalTXTSRead = 912, rate 912
5: MaxTXTSBytesOutstanding = 3332, rate 3332
6: TotalGenMsgSent = 684, rate 684
7: TotalGenMsgRcvd = 1140, rate 1140
8: TotalEventMsgSent = 228, rate 228
9: TotalEventMsgRcvd = 456, rate 456
10: TotalClientAnnounceReq = 228, rate 228
11: TotalClientAnnounceReqResend = 0, rate 0
12: TotalClientAnnounceGrant = 228, rate 228
13: TotalClientSyncReq = 228, rate 228
14: TotalClientSyncReqResend = 0, rate 0
15: TotalClientSyncGrant = 228, rate 228
16: TotalClientDelayRespReq = 228, rate 228
17: TotalClientDelayRespReqResend = 0, rate 0
18: TotalClientDelayRespGrant = 228, rate 228
19: TotalSyncRcvd = 228, rate 228
20: TotalPDelayRespRcvd = 0, rate 0
21: TotalFollowUpRcvd = 228, rate 228
22: TotalPDelayRespFollowUpRcvd = 0, rate 0
23: TotalAnnounceRcvd = 228, rate 228
24: TotalDelayReqSent = 228, rate 228
25: TotalDelayRespRcvd = 228, rate 228
26: TotalRetransmitHeapAdd = 0, rate 0
27: TotalRetransmitHeapAddAlreadyIn = 0, rate 0
28: TotalRetransmitHeapAddNotIn = 0, rate 0
29: TotalRetransmitHeapTryRemove = 0, rate 0
30: TotalRetransmitHeapRemove = 0, rate 0
31: TotalRetransmitHeapPop = 228, rate 228
Client states
Total: 228, Max: 1 , Min: 1
Count 1:228
==Tx Rx Counters=============
TX worker 0 pkt send: 912
RX worker 0 pkt recv: 1596
==Client Request Data============
Announce Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Sync Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Delay Resp Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Delay Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Announce Grant Latency
 228 samples of 228 events
Cumulative:	320.040312ms
HMean:		1.024843ms
Avg.:		1.403685ms
p50: 		1.16235ms
p75:		1.683062ms
p95:		3.27251ms
p99:		4.523026ms
p999:		5.568122ms
Long 5%:	4.00771ms
Short 5%:	471.566µs
Max:		5.568122ms
Min:		307.806µs
Range:		5.260316ms
StdDev:		888.435µs
Rate/sec.:	712.41
Sync Grant Latency
 228 samples of 228 events
Cumulative:	300.154828ms
HMean:		247.17µs
Avg.:		1.316468ms
p50: 		1.18881ms
p75:		1.857054ms
p95:		3.285182ms
p99:		4.467874ms
p999:		5.387614ms
Long 5%:	3.841688ms
Short 5%:	42.421µs
Max:		5.387614ms
Min:		35.566µs
Range:		5.352048ms
StdDev:		1.003697ms
Rate/sec.:	759.61
Delay Resp Grant Latency
 228 samples of 228 events
Cumulative:	229.549872ms
HMean:		145.619µs
Avg.:		1.006797ms
p50: 		1.095026ms
p75:		1.194654ms
p95:		2.357494ms
p99:		3.319574ms
p999:		3.398314ms
Long 5%:	3.005904ms
Short 5%:	29.675µs
Max:		3.398314ms
Min:		22.114µs
Range:		3.3762ms
StdDev:		814.495µs
Rate/sec.:	993.25
Delay Req Latency
 228 samples of 228 events
Cumulative:	173.381584ms
HMean:		127.768µs
Avg.:		760.445µs
p50: 		997.978µs
p75:		1.193058ms
p95:		2.155666ms
p99:		2.336506ms
p999:		3.223606ms
Long 5%:	2.408821ms
Short 5%:	29.538µs
Max:		3.223606ms
Min:		24.546µs
Range:		3.19906ms
StdDev:		672.425µs
Rate/sec.:	1315.02
Time Between Syncs
 0 samples of 0 events
Cumulative:	0s
HMean:		0s
Avg.:		0s
p50: 		0s
p75:		0s
p95:		0s
p99:		0s
p999:		0s
Long 5%:	0s
Short 5%:	0s
Max:		0s
Min:		0s
Range:		0s
StdDev:		0s
Rate/sec.:	0.00
==Software Performance=============
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler RX Worker 0 last busy 0.50%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler PacketParser 0 last busy 0.50%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler TX worker 0 last busy 0.50%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler TX worker 0 TSRead worker 0 last busy 0.50%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler PacketProcessor 0 last busy 0.50%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler CounterProcessor last busy 0.00%"
time="2021-08-24T10:46:40-07:00" level=info msg="Profiler Client Retransmit Proc 0 last busy 0.00%"
========================Statistics after 2.005134s============
==ClientData=============
0: TotalClients = 228, rate 0
1: TotalPacketsSent = 1140, rate 228
2: TotalPacketsRcvd = 1824, rate 228
3: TotalTXTSPacketsSent = 1140, rate 228
4: TotalTXTSRead = 1140, rate 228
5: MaxTXTSBytesOutstanding = 3332, rate 0
6: TotalGenMsgSent = 684, rate 0
7: TotalGenMsgRcvd = 1140, rate 0
8: TotalEventMsgSent = 456, rate 228
9: TotalEventMsgRcvd = 684, rate 228
10: TotalClientAnnounceReq = 228, rate 0
11: TotalClientAnnounceReqResend = 0, rate 0
12: TotalClientAnnounceGrant = 228, rate 0
13: TotalClientSyncReq = 228, rate 0
14: TotalClientSyncReqResend = 0, rate 0
15: TotalClientSyncGrant = 228, rate 0
16: TotalClientDelayRespReq = 228, rate 0
17: TotalClientDelayRespReqResend = 0, rate 0
18: TotalClientDelayRespGrant = 228, rate 0
19: TotalSyncRcvd = 228, rate 0
20: TotalPDelayRespRcvd = 0, rate 0
21: TotalFollowUpRcvd = 228, rate 0
22: TotalPDelayRespFollowUpRcvd = 0, rate 0
23: TotalAnnounceRcvd = 228, rate 0
24: TotalDelayReqSent = 456, rate 228
25: TotalDelayRespRcvd = 456, rate 228
26: TotalRetransmitHeapAdd = 0, rate 0
27: TotalRetransmitHeapAddAlreadyIn = 0, rate 0
28: TotalRetransmitHeapAddNotIn = 0, rate 0
29: TotalRetransmitHeapTryRemove = 0, rate 0
30: TotalRetransmitHeapRemove = 0, rate 0
31: TotalRetransmitHeapPop = 456, rate 228
Client states
Total: 228, Max: 1 , Min: 1
Count 1:228
==Tx Rx Counters=============
TX worker 0 pkt send: 1140
RX worker 0 pkt recv: 1824
==Client Request Data============
Announce Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Sync Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Delay Resp Grant Requests sent
Total: 228, Max: 1 , Min: 1
Count 1:228
Delay Requests sent
Total: 228, Max: 2 , Min: 2
Count 2:228
Announce Grant Latency
 228 samples of 228 events
Cumulative:	320.040312ms
HMean:		1.024843ms
Avg.:		1.403685ms
p50: 		1.16235ms
p75:		1.683062ms
p95:		3.27251ms
p99:		4.523026ms
p999:		5.568122ms
Long 5%:	4.00771ms
Short 5%:	471.566µs
Max:		5.568122ms
Min:		307.806µs
Range:		5.260316ms
StdDev:		888.435µs
Rate/sec.:	712.41
Sync Grant Latency
 228 samples of 228 events
Cumulative:	300.154828ms
HMean:		247.17µs
Avg.:		1.316468ms
p50: 		1.18881ms
p75:		1.857054ms
p95:		3.285182ms
p99:		4.467874ms
p999:		5.387614ms
Long 5%:	3.841688ms
Short 5%:	42.421µs
Max:		5.387614ms
Min:		35.566µs
Range:		5.352048ms
StdDev:		1.003697ms
Rate/sec.:	759.61
Delay Resp Grant Latency
 228 samples of 228 events
Cumulative:	229.549872ms
HMean:		145.619µs
Avg.:		1.006797ms
p50: 		1.095026ms
p75:		1.194654ms
p95:		2.357494ms
p99:		3.319574ms
p999:		3.398314ms
Long 5%:	3.005904ms
Short 5%:	29.675µs
Max:		3.398314ms
Min:		22.114µs
Range:		3.3762ms
StdDev:		814.495µs
Rate/sec.:	993.25
Delay Req Latency
 228 samples of 228 events
Cumulative:	143.902592ms
HMean:		469.397µs
Avg.:		631.151µs
p50: 		654.054µs
p75:		798.782µs
p95:		1.171114ms
p99:		1.304222ms
p999:		1.486662ms
Long 5%:	1.282575ms
Short 5%:	173.455µs
Max:		1.486662ms
Min:		130.678µs
Range:		1.355984ms
StdDev:		298.877µs
Rate/sec.:	1584.41
Time Between Syncs
 0 samples of 0 events
Cumulative:	0s
HMean:		0s
Avg.:		0s
p50: 		0s
p75:		0s
p95:		0s
p99:		0s
p999:		0s
Long 5%:	0s
Short 5%:	0s
Max:		0s
Min:		0s
Range:		0s
StdDev:		0s
Rate/sec.:	0.00
==Software Performance=============
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler RX Worker 0 last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler PacketParser 0 last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler TX worker 0 last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler TX worker 0 TSRead worker 0 last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler PacketProcessor 0 last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler CounterProcessor last busy 0.00%"
time="2021-08-24T10:46:41-07:00" level=info msg="Profiler Client Retransmit Proc 0 last busy 0.00%"
```


