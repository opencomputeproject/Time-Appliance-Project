/*
Copyright (c) Facebook, Inc. and its affiliates.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package clientgenlib

import (
	"bytes"
	"context"
	"fmt"
	"net"
	"sync"
	"time"
	"math/big"

	"golang.org/x/sync/errgroup"
	"golang.org/x/sync/semaphore"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/kpango/fastime"
)

type state int

const (
	stateInit = iota
	stateDone
	stateGotGrantAnnounce
	stateGotGrantSync
	stateNone
)

const (
	pktIgnore = iota
	pktAnnounceGrantReq
	pktSyncGrantReq
	pktDelayRespGrantReq
	pktDelayReq
)

type RunningStatistics struct {
	MinAnnounceGrantLatency time.Duration
	MaxAnnounceGrantLatency time.Duration

	MinSyncGrantLatency time.Duration
	MaxSyncGrantLatency time.Duration

	MinDelayRespGrantLatency time.Duration
	MaxDelayRespGrantLatency time.Duration

	MinDelayRespLatency time.Duration
	MaxDelayRespLatency time.Duration
}

type GlobalStatistics struct {
	TotalClients     uint64
	TotalPacketsSent uint64
	TotalPacketsRcvd uint64

	TotalTXTSPacketsSent uint64
	TotalTXTSRead        uint64

	MaxTXTSBytesOutstanding uint64

	TotalGenMsgSent   uint64
	TotalGenMsgRcvd   uint64
	TotalEventMsgSent uint64
	TotalEventMsgRcvd uint64

	// Unicast grant counters
	TotalClientAnnounceReq       uint64
	TotalClientAnnounceReqResend uint64
	TotalClientAnnounceGrant     uint64

	TotalClientSyncReq       uint64
	TotalClientSyncReqResend uint64
	TotalClientSyncGrant     uint64

	TotalClientDelayRespReq       uint64
	TotalClientDelayRespReqResend uint64
	TotalClientDelayRespGrant     uint64

	TotalSyncRcvd               uint64
	TotalPDelayRespRcvd         uint64
	TotalFollowUpRcvd           uint64
	TotalPDelayRespFollowUpRcvd uint64
	TotalAnnounceRcvd           uint64

	TotalDelayReqSent  uint64
	TotalDelayRespRcvd uint64
}

type ClientGenConfig struct {
	ServerMAC       string
	srcMAC          net.HardwareAddr
	parsedServerMac net.HardwareAddr
	// grand master IP address
	ServerAddress string
	// Define client IPs to run with
	// Define start / stop / step
	ClientIPStart string
	ClientIPEnd   string
	ClientIPStep  uint
	SoftStartRate uint64 // how many clients / second to start

	Iface       string
	TimeoutSec  float64
	DurationSec float64

	NumTXWorkers        int
	NumTXTSWorkerPerTx  int
	NumRXWorkers        int
	NumPacketParsers    int
	NumPacketProcessors int
	NumClientRestartProcs int
	NumClientRetransmitProcs int

	DebugPrint       bool // very low level debug, prints everything
	DebugLogClient   bool // prints higher level ptp protocol messages received or sent
	DebugIoWkrRX     bool
	DebugIoWkrTX     bool
	DebugDetailPerf  bool
	DebugRestartProc bool
	DebugRetransProc bool
	DebugProfilers   bool

	PrintTxRxCounts    bool
	PrintPerformance   bool
	PrintClientData    bool
	PrintClientReqData bool
	PrintLatencyData   bool

	RestartClientsAfterDuration       bool
	TimeAfterDurationBeforeRestartSec float64
	TimeBetweenDelayReqSec            float64
	ClientRetranTimeWhenNoResponseSec float64

	Counters GlobalStatistics

	StatMu     sync.Mutex
	Statistics RunningStatistics

	CounterPrintIntervalSecs uint

	PerfProfilers []*Profiler
	perIOTX       []uint64
	perIORX       []uint64

	Eg  *errgroup.Group
	Ctx *context.Context

	RunData *ClientGenData

	fastimeHolder *fastime.Fastime
}

// make simple queues with arrays to measure latencies per client
const (
	latencyMeasCount = 3
)

type SingleClientGen struct {
	ClientIP net.IP

	stateSem *semaphore.Weighted
	state    state

	genSequence   uint16
	eventSequence uint16

	laststate state // used to keep track of retransmits

	timeDoneInit time.Time // keep track of when I got all my grants

	// just keep track of these times
	SentAnnounceGrantReqTime time.Time
	GotAnnounceGrantReqTime  time.Time

	SentlastSyncGrantReqTime time.Time
	GotlastSyncGrantReqTime  time.Time

	SentDelayRespGrantReqTime time.Time
	GotDelayRespGrantReqTime  time.Time

	SentDelayReqTime time.Time
	GotDelayRespTime time.Time

	lastAnnounceTimes [latencyMeasCount]time.Time
	lastSyncTimes     [latencyMeasCount]time.Time
	lastFollowupTimes [latencyMeasCount]time.Time

	timeAnnounceGrantReqRetransmit  time.Time
	timeSyncGrantReqRetransmit      time.Time
	timeDelayRespGrantReqRetransmit time.Time

	index int

	CountOutgoingPackets    uint64
	CountIncomingPTPPackets uint64

	CountAnnounceGrantReq uint64
	CountAnnounceGrant    uint64

	CountSyncGrantReq uint64
	CountSyncGrant    uint64

	CountDelayRespGrantReq uint64
	CountDelayRespGrant    uint64

	CountDelayReq  uint64
	CountDelayResp uint64

	CountRetransmitDone       uint64
	CountRetransmitWierdState uint64

	CountHandleRetransmitFromHeap   uint64

	RetransTimer timeItem
	RestartTimer timeItem
}

type parallelHeapOp struct {
	item        *timeItem
	operation   uint8
	priorityReq time.Time
	value       interface{}
}

const (
	heapUpdateInsert = iota
	heapRemove
)

type parallelHeap struct {
	heap          timeHeap
	operationChan chan *parallelHeapOp
}

type PktDecoder struct {
	eth       layers.Ethernet
	ip4       layers.IPv4
	ip6       layers.IPv6
	arp       layers.ARP
	udp       layers.UDP
	icmpv6    layers.ICMPv6
	icmpv6ns  layers.ICMPv6NeighborSolicitation
	icmpv6na  layers.ICMPv6NeighborAdvertisement
	layers    []gopacket.LayerType
	parser    *gopacket.DecodingLayerParser
	rawData   []byte
	Timestamp time.Time
	fromTX    bool
}

type ClientGenData struct {
	clients   []SingleClientGen
	rawInput  []chan *inPacket
	rawOutput []chan *outPacket
	pktToProc []chan *PktDecoder

	outChanToUse     uint32 // which rawOutput to write to, round robin basically
	inChanToUse      uint32 // which rawInput chan to write to, round robin
	pktProcChanToUse uint32

	retransmitHeap []parallelHeap
	restartHeap    []parallelHeap

	// create a pool for heap operations
	heapOpPool sync.Pool

	// create pools for packet sending
	commonSerializeOp gopacket.SerializeOptions
	outPacketPool     sync.Pool

	// create pools for packet receiving and parsing
	goPacketDecoderPool sync.Pool

	inPacketPool sync.Pool

	// create pool for generate byte array, used for TXTS stuff
	bytePool sync.Pool
}

// ipSubtract returns the difference between two IPs
// a - b, just returns difference as a value
// a should be greater than b! unsigned result
// returns error if a < b
func ipSubtract(a net.IP, b net.IP) (uint64, error) {

	if a.To4() != nil && b.To4() != nil {
		a = a.To4()
		b = b.To4()
		var aInt, bInt uint32
		aInt = (uint32(a[0]) << 24) + (uint32(a[1]) << 16) +
			(uint32(a[2]) << 8) + (uint32(a[3]))
		bInt = (uint32(b[0]) << 24) + (uint32(b[1]) << 16) +
			(uint32(b[2]) << 8) + (uint32(b[3]))

		if aInt < bInt {
			return 0, fmt.Errorf("ipSubtractv4 a < b!")
		} else {
			return uint64(aInt - bInt), nil
		}
	} else {
		aBig := big.NewInt(0)
		bBig := big.NewInt(0)
		aBig.SetBytes(a.To16())
		bBig.SetBytes(b.To16())
		if aBig.Cmp(bBig) == -1 { // a < b
			return 0, fmt.Errorf("ipSubtractv6 a < b!")
		} else {
			sub := big.NewInt(0).Sub(aBig, bBig)
			return sub.Uint64(), nil
		}
	}
}

// NextIP computes the next IP address after adding step to it
func NextIP(ip net.IP, step uint) net.IP {
	// https://play.golang.org/p/Osy0XSa7CY
	// Increment increments the given net.IP by one bit. Incrementing the last IP in an IP space (IPv4, IPV6) is undefined.
	if ip.To4() != nil { // ipv4
		result := net.IPv4(0, 0, 0, 0)
		copy(result, ip)
		for j := step; j > 0; j-- {
			for i := len(result) - 1; i >= 0; i-- {
				result[i]++
				//only add to the next byte if we overflowed
				if result[i] != 0 {
					break
				}
			}
		}
		return result
	} else {
		result := net.IPv4(0, 0, 0, 0).To16()
		copy(result, ip)
		for j := step; j > 0; j-- {
			for i := len(result) - 1; i >= 0; i-- {
				result[i]++
				//only add to the next byte if we overflowed
				if result[i] != 0 {
					break
				}
			}
		}
		return result
	}
}

// IpBetween returns whether an IP is between a start and end IP address
func IpBetween(from net.IP, to net.IP, test net.IP) bool {
	// https://stackoverflow.com/questions/19882961/go-golang-check-ip-address-in-range
	//test to determine if a given ip is between two others (inclusive)
	if from == nil || to == nil || test == nil {
		return false
	}
	// check for mismatch IP, return false in that case
	if from.To4() != nil && to.To4() == nil {
		return false
	}
	if from.To4() == nil && to.To4() != nil {
		return false
	}
	from16 := from.To16()
	to16 := to.To16()
	test16 := test.To16()
	if from16 == nil || to16 == nil || test16 == nil {
		return false
	}
	if bytes.Compare(test16, from16) >= 0 && bytes.Compare(test16, to16) <= 0 {
		return true
	}
	return false
}
