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
	"fmt"
	"net"
	"sync"
	"time"
	"sync/atomic"

	"golang.org/x/sync/semaphore"
	ptp "github.com/facebook/time/ptp/protocol"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/kpango/fastime"
	log "github.com/sirupsen/logrus"
)


const rawOutQueueSize = 10000
const rawInQueueSize = 10000
const pktDecoderQueueSize = 10000
const reTransHeapOpQueueSize = 10000
const reTransHeapStartOversize = 10
const reStartHeapOpQueueSize = 10000
const reStartHeapStartOversize = 10

// StartClientGen is the top level function for the client traffic generator, runs based on cfg argument
func StartClientGen(cfg *ClientGenConfig) {
	var err error
	cfg.fastimeHolder = fastime.New().StartTimerD(*cfg.Ctx, 1*time.Microsecond)
	// make the client data structures
	var clientGenData ClientGenData
	startIp := net.ParseIP(cfg.ClientIPStart)
	endIp := net.ParseIP(cfg.ClientIPEnd)
	i := 0
	for ip := startIp; IpBetween(startIp, endIp, ip); ip =
		NextIP(ip, cfg.ClientIPStep) {
		single := SingleClientGen{
			ClientIP:      ip,
			state:         stateInit,
			laststate:     stateNone,
			genSequence:   0,
			eventSequence: 0,
			stateSem:      semaphore.NewWeighted(1),
			timeDoneInit:  time.Time{},
			index:         i,
		}
		single.RetransTimer.index = -1
		single.RestartTimer.index = -1
		clientGenData.clients = append(clientGenData.clients, single)
		atomic.AddUint64(&cfg.Counters.TotalClients, 1)
		i++
	}
	if cfg.DebugPrint || cfg.DebugLogClient  {
		log.Infof("Done making %v client data structures", len(clientGenData.clients))
	}

	cfg.RunData = &clientGenData
	cfg.RunData.commonSerializeOp.FixLengths = true
	cfg.RunData.commonSerializeOp.ComputeChecksums = true
	cfg.RunData.rawInput = make([]chan *inPacket, cfg.NumPacketParsers) // direct packet data
	cfg.RunData.rawOutput = make([]chan *outPacket, cfg.NumTXWorkers)
	cfg.RunData.pktToProc = make([]chan *PktDecoder, cfg.NumPacketProcessors)

	for i := 0; i < cfg.NumTXWorkers; i++ {
		cfg.RunData.rawOutput[i] = make(chan *outPacket, rawOutQueueSize)
	}
	for i := 0; i < cfg.NumPacketParsers; i++ {
		cfg.RunData.rawInput[i] = make(chan *inPacket, rawInQueueSize)
	}
	for i := 0; i < cfg.NumPacketProcessors; i++ {
		cfg.RunData.pktToProc[i] = make(chan *PktDecoder, pktDecoderQueueSize)
	}
	if cfg.DebugPrint {
		log.Infof("Done making part of config")
	}

	cfg.RunData.retransmitHeap = make([]parallelHeap, cfg.NumClientRetransmitProcs)
	cfg.RunData.restartHeap = make([]parallelHeap, cfg.NumClientRestartProcs)

	for i := 0; i < cfg.NumClientRetransmitProcs; i++ {
		startCap := (len(cfg.RunData.clients) / cfg.NumClientRetransmitProcs) + reTransHeapStartOversize
		cfg.RunData.retransmitHeap[i].heap.Init(startCap)
		cfg.RunData.retransmitHeap[i].operationChan = make(chan *parallelHeapOp, reTransHeapOpQueueSize)
	}
	for i := 0; i < cfg.NumClientRestartProcs; i++ {
		startCap := (len(cfg.RunData.clients) / cfg.NumClientRestartProcs) + reStartHeapStartOversize
		cfg.RunData.restartHeap[i].heap.Init(startCap)
		cfg.RunData.restartHeap[i].operationChan = make(chan *parallelHeapOp, reStartHeapOpQueueSize)
	}

	cfg.RunData.heapOpPool = sync.Pool{
		New: func() interface{} {
			return new(parallelHeapOp)
		},
	}
	cfg.RunData.outPacketPool = sync.Pool{
		New: func() interface{} {
			p := new(outPacket)
			buf := gopacket.NewSerializeBufferExpectedSize(200, 0) // arbitrary size I think should work for most cases
			p.data = &buf
			return p
		},
	}

	cfg.RunData.inPacketPool = sync.Pool{
		New: func() interface{} {
			return new(inPacket)
		},
	}

	cfg.RunData.bytePool = sync.Pool{
		New: func() interface{} {
			return make([]byte, 300)
		},
	}

	cfg.RunData.goPacketDecoderPool = sync.Pool{
		New: func() interface{} {
			item := new(PktDecoder)
			decoder := gopacket.NewDecodingLayerParser(layers.LayerTypeEthernet,
				&item.eth, &item.ip4, &item.ip6, &item.arp, &item.udp,
				&item.icmpv6, &item.icmpv6ns, &item.icmpv6na)
			item.layers = make([]gopacket.LayerType, 8)
			item.parser = decoder
			item.fromTX = false
			return item
		},
	}
	cfg.perIOTX = make([]uint64, cfg.NumTXWorkers)
	cfg.perIORX = make([]uint64, cfg.NumRXWorkers)

	cfg.parsedServerMac, err = net.ParseMAC(cfg.ServerMAC)
	if err != nil {
		log.Errorf("Failed to parse cfg ServerMAC %v", err)
		return
	}
	srcInterface, err := net.InterfaceByName(cfg.Iface)
	if err != nil {
		log.Errorf("Failed to get MAC from interface %v", cfg.Iface)
		return
	}
	cfg.srcMAC = srcInterface.HardwareAddr


	/**** Start worker goroutines *****/
	if cfg.DebugPrint {
		log.Infof("Starting workers")
	}
	startIOWorker(cfg)

	// takes data from raw_in_data, parses it into gopackets, puts in parsed_pkts channel
	startPacketParser(cfg)

	startPacketProcessor(cfg)

	startClientProcessor(cfg)

	startCounterProcessor(cfg)

	/**** Start it, put each client into client processor with retransmit time of now ****/
	// do it this way so it isn't single threaded
	startCount := cfg.SoftStartRate
	log.Infof("Starting clients at %v", fastime.Now())

	// use goroutines, kick off some quantity in a separate goroutine
	for start := uint64(0); ; {
		if (*cfg.Ctx).Err() != nil {
			log.Errorf("Ending kickoff early because context error %v", (*cfg.Ctx).Err())
			return
		}
		end := uint64(start + 9999) // try to do 10000 each time
		if end-start >= startCount {
			end = start + startCount - 1
		} // exceeding soft start, change the range

		if end >= uint64(len(cfg.RunData.clients)) {
			end = uint64(len(cfg.RunData.clients)) - 1
		} // at the end, go till the end
		// kick off clients in this index range
		go func(start uint64, end uint64) {
			for i := start; i <= end; i++ {
				pushClientRetransmit(cfg, &cfg.RunData.clients[i], fastime.Now())
			}
		}(start, end)
		if cfg.DebugPrint {
			log.Infof("Kicked off clients %v to %v", start, end)
		}
		// subtract from start count
		startCount = startCount - (end - start + 1)
		// move start to end
		start = end + 1
		if startCount <= 0 && end != uint64(len(cfg.RunData.clients))-1 {
			time.Sleep(1 * time.Second) // exceeding soft start, sleep
			startCount = cfg.SoftStartRate
		}
		if end == uint64(len(cfg.RunData.clients))-1 {
			break
		}
	}

	waitval := cfg.Eg.Wait()
	log.Infof("Eg wait ended! %v", waitval)

}

func getClientFromIP(cfg *ClientGenConfig, ip net.IP) (*SingleClientGen, error) {

	// if cfg is IPv6, make sure the argument is IPv6
	if cfg.RunData.clients[0].ClientIP.To4() == nil {
		if ip.To4() != nil {
			return nil, fmt.Errorf("getClientFromIP ipv4 ipv6 mismatch %v", ip)
		}
	}

	// do the byte subtraction

	diff, _ := ipSubtract(ip, cfg.RunData.clients[0].ClientIP)
	// check the subtract is a multiple of step

	if cfg.DebugPrint {
		log.Infof("getClientFromIP frompkt %v start %v diff %v ", ip,
			cfg.RunData.clients[0].ClientIP, diff)
	}
	if diff%uint64(cfg.ClientIPStep) == 0 {
		index := int(diff / uint64(cfg.ClientIPStep))
		if index >= len(cfg.RunData.clients) {
			return nil, fmt.Errorf("Could not find client with ip %v", ip)
		}
		return &cfg.RunData.clients[index], nil
	}

	return nil, fmt.Errorf("Could not find client with ip %v", ip)
}

func getTxChanNumToUse(cfg *ClientGenConfig) uint32 {
	newVal := atomic.AddUint32(&cfg.RunData.outChanToUse, 1)
	return newVal % uint32(cfg.NumTXWorkers)
}

func getRxChanNumToUse(cfg *ClientGenConfig) uint32 {
	newVal := atomic.AddUint32(&cfg.RunData.inChanToUse, 1)
	return newVal % uint32(cfg.NumPacketParsers)
}

func getPacketProcChanNumToUse(cfg *ClientGenConfig) uint32 {
	newVal := atomic.AddUint32(&cfg.RunData.pktProcChanToUse, 1)
	return newVal % uint32(cfg.NumPacketProcessors)
}

func pushClientRetransmit(cfg *ClientGenConfig, cl *SingleClientGen, t time.Time) {
	procNum := cl.index % cfg.NumClientRetransmitProcs
	opItem := cfg.RunData.heapOpPool.Get().(*parallelHeapOp)
	opItem.item = &cl.RetransTimer
	opItem.operation = heapUpdateInsert
	opItem.priorityReq = t
	opItem.value = cl
	if cfg.DebugRetransProc || cfg.DebugPrint {
		log.Infof("pushClientRetransmit %v procNum %v t %v",
			opItem.value.(*SingleClientGen).ClientIP, procNum, opItem.priorityReq)
	}
	cfg.RunData.retransmitHeap[procNum].operationChan <- opItem

}

func removeClientRetransmit(cfg *ClientGenConfig, cl *SingleClientGen) {
	procNum := cl.index % cfg.NumClientRetransmitProcs
	if cfg.DebugRetransProc || cfg.DebugPrint {
		log.Infof("removeClientRetransmit %v procNum %v", cl.ClientIP, procNum)
	}
	opItem := cfg.RunData.heapOpPool.Get().(*parallelHeapOp)
	opItem.item = &cl.RetransTimer
	opItem.operation = heapRemove

	cfg.RunData.retransmitHeap[procNum].operationChan <- opItem

}

func pushClientRestart(cfg *ClientGenConfig, cl *SingleClientGen, t time.Time) {

	procNum := cl.index % cfg.NumClientRestartProcs

	opItem := cfg.RunData.heapOpPool.Get().(*parallelHeapOp)
	opItem.item = &cl.RestartTimer
	opItem.operation = heapUpdateInsert
	opItem.priorityReq = t
	opItem.value = cl

	cfg.RunData.restartHeap[procNum].operationChan <- opItem

}

/* Not actually used, but keeping for reference, may be used in the future
func removeClientRestart(cfg *ClientGenConfig, cl *SingleClientGen) {
	procNum := cl.index % cfg.NumClientRestartProcs

	opItem := cfg.RunData.heapOpPool.Get().(*parallelHeapOp)
	opItem.item = &cl.RestartTimer
	opItem.operation = heapRemove

	cfg.RunData.restartHeap[procNum].operationChan <- opItem
}
*/

func craftSinglePktToGM(cfg *ClientGenConfig, cl *SingleClientGen, udpport uint16, payload []byte, out *outPacket) {
	var isIP6 bool
	var eth layers.Ethernet
	var udp layers.UDP
	var err error
	if cl.ClientIP.To4() == nil {
		isIP6 = true
	} else {
		isIP6 = false
	}
	// a bit out of order, but these layers aren't affected by ipv6 vs ipv4
	buf := out.data
	payloadBuf := gopacket.Payload(payload)
	udp = layers.UDP{
		SrcPort: layers.UDPPort(udpport),
		DstPort: layers.UDPPort(udpport),
	}

	if isIP6 {
		eth = layers.Ethernet{
			SrcMAC:       cfg.srcMAC,
			DstMAC:       cfg.parsedServerMac,
			EthernetType: layers.EthernetTypeIPv6,
		}
	} else {
		eth = layers.Ethernet{
			SrcMAC:       cfg.srcMAC,
			DstMAC:       cfg.parsedServerMac,
			EthernetType: layers.EthernetTypeIPv4,
		}
	}

	if isIP6 {
		ip := layers.IPv6{
			SrcIP:      cl.ClientIP,
			DstIP:      net.ParseIP(cfg.ServerAddress),
			Version:    6,
			HopLimit:   255,
			NextHeader: layers.IPProtocolUDP,
		}
		err = udp.SetNetworkLayerForChecksum(&ip)
		if err != nil {
			log.Errorf("SetNetworkLayerForChecksum failed %v", err)
		}
		err = gopacket.SerializeLayers(*buf,
			cfg.RunData.commonSerializeOp, &eth, &ip, &udp, payloadBuf)
		if err != nil {
			log.Errorf("SerializeLayers failed %v", err)
		}
	} else {
		ip := layers.IPv4{
			SrcIP:    cl.ClientIP,
			DstIP:    net.ParseIP(cfg.ServerAddress),
			Version:  4,
			TTL:      255,
			Protocol: layers.IPProtocolUDP,
		}
		err = udp.SetNetworkLayerForChecksum(&ip)
		if err != nil {
			log.Errorf("SetNetworkLayerForChecksum failed %v", err)
		}
		err = gopacket.SerializeLayers(*buf,
			cfg.RunData.commonSerializeOp, &eth, &ip, &udp, payloadBuf)
		if err != nil {
			log.Errorf("SerializeLayers failed %v", err)
		}
	}

}

func handleRetransmit(cfg *ClientGenConfig, cl *SingleClientGen, fromPop bool, clientProcNum int) {
	// basically need to send out a packet for this client, figure out what packet
	var payload *ptp.Signaling
	var pktType uint8

	err := cl.stateSem.Acquire(*cfg.Ctx, 1)
	if err != nil {
		log.Errorf("handleRetransmit semaphore acquire error %v", err)
	}
	curState := cl.state
	// technically race condition but I dont think its a problem

	if curState == stateDone {
		cl.stateSem.Release(1)
		cl.CountRetransmitDone++
		// check if duration is over
		if time.Since(cl.timeDoneInit) > time.Duration(cfg.DurationSec*float64(time.Second)) {
			// do nothing , handled by restart client
			if cfg.DebugPrint || cfg.DebugLogClient {
				log.Infof("Retransmit client %v, but time since init elapsed duration", cl.ClientIP)
			}
			return
		} else {
			// client is still valid and done, basically only thing is to
			// do DelayReq
			payload := reqDelay(0)
			payload.SetSequence(cl.eventSequence)
			b, _ := ptp.Bytes(payload)

			out := cfg.RunData.outPacketPool.Get().(*outPacket)
			craftSinglePktToGM(cfg, cl, ptp.PortEvent, b, out)
			out.getTS = true
			out.pktType = pktDelayReq
			out.cl = cl
			atomic.AddUint64(&cfg.Counters.TotalEventMsgSent, 1)
			atomic.AddUint64(&cfg.Counters.TotalDelayReqSent, 1)
			cl.CountDelayReq++
			if cfg.DebugLogClient || cfg.DebugPrint {
				log.Infof("Client %v reqDelay seq %v", cl.ClientIP, cl.eventSequence)
			}
			// push to transmit and add to retransmit
			pushClientRetransmit(cfg, cl, fastime.Now().Add(time.Duration(cfg.TimeBetweenDelayReqSec*float64(time.Second))))
			cfg.RunData.rawOutput[getTxChanNumToUse(cfg)] <- out
			return
		}
	} else if curState == stateInit {
		cl.stateSem.Release(1)
		// need to request announce grant
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("Init state cl %v state %v, reqUnicast MessageAnnounce seq=%d ", cl.ClientIP, curState, cl.genSequence)
		}
		payload = reqUnicast(ptp.ClockIdentity(cl.index), time.Duration(float64(time.Second)*cfg.DurationSec), ptp.MessageAnnounce)
		pktType = pktAnnounceGrantReq
		atomic.AddUint64(&cfg.Counters.TotalGenMsgSent, 1)
		atomic.AddUint64(&cfg.Counters.TotalClientAnnounceReq, 1)
		cl.CountAnnounceGrantReq++
		if cl.laststate == curState {
			atomic.AddUint64(&cfg.Counters.TotalClientAnnounceReqResend, 1)
		}
	} else if curState == stateGotGrantAnnounce {
		cl.stateSem.Release(1)
		// need to request sync grant
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("GotGrantAnnounce cl %v state %v, reqUnicast MessageSync seq=%d", cl.ClientIP, curState, cl.genSequence)
		}
		payload = reqUnicast(ptp.ClockIdentity(cl.index), time.Duration(float64(time.Second)*cfg.DurationSec), ptp.MessageSync)
		pktType = pktSyncGrantReq
		atomic.AddUint64(&cfg.Counters.TotalGenMsgSent, 1)
		atomic.AddUint64(&cfg.Counters.TotalClientSyncReq, 1)
		cl.CountSyncGrantReq++
		if cl.laststate == curState {
			atomic.AddUint64(&cfg.Counters.TotalClientSyncReqResend, 1)
		}
	} else if curState == stateGotGrantSync {
		cl.stateSem.Release(1)
		// need to request DelayResp grant
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("GotGrantSync cl %v state %v, reqUnicast MessageDelayResp seq=%d", cl.ClientIP, curState, cl.genSequence)
		}
		payload = reqUnicast(ptp.ClockIdentity(cl.index), time.Duration(float64(time.Second)*cfg.DurationSec), ptp.MessageDelayResp)
		pktType = pktDelayRespGrantReq
		atomic.AddUint64(&cfg.Counters.TotalGenMsgSent, 1)
		atomic.AddUint64(&cfg.Counters.TotalClientDelayRespReq, 1)
		cl.CountDelayRespGrantReq++
		if cl.laststate == curState {
			atomic.AddUint64(&cfg.Counters.TotalClientDelayRespReqResend, 1)
		}
	} else {
		cl.stateSem.Release(1)
		cl.CountRetransmitWierdState++
		log.Infof("Client in wierd state during retransmit %v", curState)
		return
	}

	cl.laststate = cl.state
	payload.SetSequence(cl.genSequence)
	b, err := ptp.Bytes(payload)
	if err != nil {
		log.Errorf("handleRetransmit ptp.Bytes error %v", err)
	}
	out := cfg.RunData.outPacketPool.Get().(*outPacket)
	craftSinglePktToGM(cfg, cl, ptp.PortGeneral, b, out)
	out.getTS = true
	out.pktType = pktType
	out.cl = cl

	// push to transmit and add to retransmit
	cfg.RunData.rawOutput[getTxChanNumToUse(cfg)] <- out

	clRetransTime := fastime.Now().Add( time.Duration(float64(time.Second)*cfg.ClientRetranTimeWhenNoResponseSec) )

	pushClientRetransmit(cfg, cl, clRetransTime)
	if pktType == pktAnnounceGrantReq {
		cl.timeAnnounceGrantReqRetransmit = clRetransTime
	} else if pktType == pktSyncGrantReq {
		cl.timeSyncGrantReqRetransmit = clRetransTime
	} else if pktType == pktDelayRespGrantReq {
		cl.timeDelayRespGrantReqRetransmit = clRetransTime
	}

}

func handleRestart(cfg *ClientGenConfig, cl *SingleClientGen) {
	// basically put the state to Init
	err := cl.stateSem.Acquire(*cfg.Ctx, 1)
	if err != nil {
		log.Errorf("handleRestart client semaphore acquire err %v", err)
	}
	cl.state = stateInit
	cl.genSequence = 0
	cl.eventSequence = 0
	for i := 0; i < latencyMeasCount; i++ {
		cl.lastAnnounceTimes[i] = time.Time{}
		cl.lastSyncTimes[i] = time.Time{}
		cl.lastFollowupTimes[i] = time.Time{}
	}
	cl.stateSem.Release(1)

	removeClientRetransmit(cfg, cl)
	handleRetransmit(cfg, cl, false, 0)
	if cfg.DebugPrint {
		log.Debugf("Restarted client %v", cl.ClientIP)
	}

}

func singleClientHandleIncomingPTP(cfg *ClientGenConfig, cl *SingleClientGen, in *PktDecoder, payload []byte) (*outPacket, error) {
	if cfg.DebugPrint {
		log.Debugf("singleClientHandleIncomingPTP client %v", cl.ClientIP)
	}
	msgType, err := ptp.ProbeMsgType(payload)

	if err != nil {
		log.Infof("Failed to get msgType from ProbeMsgType")
		return nil, err
	}

	if in.fromTX {
		// similar to below code, but do it here in the beginning
		// basically just need to figure out where to put TX timestamp
		switch msgType {
		case ptp.MessageSignaling: // sent a signaling message
			signaling := &ptp.Signaling{}
			if err := ptp.FromBytes(payload, signaling); err != nil {
				log.Errorf(`Failed to get signaling from payload err %v\n
					eth: %v\n
					ipv4: %v\n 
					udp (%v): %v\n
					payload (%v): %v\n`,
					err, in.eth, in.ip4, in.udp.Length, in.udp, len(payload), payload)
				return nil, fmt.Errorf("reading signaling msg: %w", err)
			}
			for _, tlv := range signaling.TLVs {
				switch v := tlv.(type) {
				case *ptp.RequestUnicastTransmissionTLV:
					// figure out which type of unicast request I sent
					msgType := v.MsgTypeAndReserved.MsgType()
					switch msgType {
					case ptp.MessageAnnounce: // sending announce grant req
						cl.SentAnnounceGrantReqTime = in.Timestamp
						if cfg.DebugPrint {
							log.Debugf("Client %v HW SentAnnounceGrantReqTime %v", cl.ClientIP, in.Timestamp)
						}
					case ptp.MessageSync: // sending sync grant req
						cl.SentlastSyncGrantReqTime = in.Timestamp
						if cfg.DebugPrint {
							log.Debugf("Client %v HW SentSyncGrantReqTime %v", cl.ClientIP, in.Timestamp)
						}
					case ptp.MessageDelayResp: // sending delay resp grant req
						cl.SentDelayRespGrantReqTime = in.Timestamp
						if cfg.DebugPrint {
							log.Debugf("Client %v HW SentDelayRespGrantReqTime %v", cl.ClientIP, in.Timestamp)
						}
					}
				}
			}
		case ptp.MessageDelayReq: // sent a DelayReq
			cl.SentDelayReqTime = in.Timestamp
			if cfg.DebugPrint {
				log.Debugf("Client %v HW SentDelayReqTime %v", cl.ClientIP, in.Timestamp)
			}
		}
		return nil, nil
	}

	var toSendPayload []byte
	cl.CountIncomingPTPPackets++

	switch msgType {
	case ptp.MessageSignaling:
		atomic.AddUint64(&cfg.Counters.TotalGenMsgRcvd, 1)
		signaling := &ptp.Signaling{}
		if err := ptp.FromBytes(payload, signaling); err != nil {
			log.Infof("Failed to get signaling from payload")
			return nil, fmt.Errorf("reading signaling msg: %w", err)
		}

		for _, tlv := range signaling.TLVs {
			switch v := tlv.(type) {
			case *ptp.GrantUnicastTransmissionTLV:
				msgType := v.MsgTypeAndReserved.MsgType()
				if v.DurationField == 0 {
					log.Infof("!!!!!!!!!!!!!!!!! Server denied us grant for %s !!!!!!!!!!!!!!!!", msgType)
					return nil, fmt.Errorf("Server denied us grant for %s", msgType)
				}
				switch msgType {
				case ptp.MessageAnnounce:
					if cfg.DebugLogClient || cfg.DebugPrint {
						log.Debugf("Got Announce grant %v", cl.ClientIP)
					}
					atomic.AddUint64(&cfg.Counters.TotalClientAnnounceGrant, 1)
					err := cl.stateSem.Acquire(*cfg.Ctx, 1)
					if err != nil {
						log.Errorf("singleClientHandleIncomingPTP cl semaphore acquire err %v", err)
					}
					cl.state = stateGotGrantAnnounce
					cl.genSequence++
					cl.stateSem.Release(1)
					// one comment for these removeClientRetransmit
					// retransmit is only valid for certain cases
					// I only want to remove it for some packets, not for all
					// for instance, I don't want to remove transmit when I get
					// an Announce or Sync message, only for messages that required
					// me to send something as the client
					if cfg.DebugPrint || cfg.DebugRetransProc {
						log.Debugf("removeClientRetransmit in singleClientHandleIncomingPTP got announce grant %d TS %v",
							cl.RetransTimer.index, in.Timestamp)
					}
					cl.CountAnnounceGrant++
					removeClientRetransmit(cfg, cl)
					handleRetransmit(cfg, cl, false, 0)
					// handle statistic
					cl.GotAnnounceGrantReqTime = in.Timestamp

				case ptp.MessageSync:
					if cfg.DebugLogClient || cfg.DebugPrint {
						log.Debugf("Got Sync grant %v", cl.ClientIP)
					}
					err := cl.stateSem.Acquire(*cfg.Ctx, 1)
					if err != nil {
						log.Errorf("singleClientHandleIncomingPTP cl semaphore acquire err %v", err)
					}
					cl.state = stateGotGrantSync
					cl.genSequence++
					cl.stateSem.Release(1)
					cl.CountSyncGrant++
					atomic.AddUint64(&cfg.Counters.TotalClientSyncGrant, 1)
					if cfg.DebugPrint || cfg.DebugRetransProc {
						log.Debugf("removeClientRetransmit in singleClientHandleIncomingPTP got sync grant")
					}
					removeClientRetransmit(cfg, cl)
					handleRetransmit(cfg, cl, false, 0)
					// handle statistics
					cl.GotlastSyncGrantReqTime = in.Timestamp
				case ptp.MessageDelayResp:
					if cfg.DebugLogClient || cfg.DebugPrint {
						log.Debugf("Got DelayResp grant %v", cl.ClientIP)
					}
					err := cl.stateSem.Acquire(*cfg.Ctx, 1)
					if err != nil {
						log.Errorf("singleClientHandleIncomingPTP cl semaphore acquire err %v", err)
					}
					cl.state = stateDone
					cl.genSequence++
					cl.timeDoneInit = fastime.Now()
					cl.stateSem.Release(1)
					cl.CountDelayRespGrant++
					if cfg.RestartClientsAfterDuration {
						pushClientRestart(cfg, cl,
							fastime.Now().Add(time.Duration(float64(time.Second)*cfg.TimeAfterDurationBeforeRestartSec)).Add(
								time.Duration(cfg.DurationSec*float64(time.Second))))
					}
					atomic.AddUint64(&cfg.Counters.TotalClientDelayRespGrant, 1)
					if cfg.DebugPrint || cfg.DebugRetransProc {
						log.Debugf("removeClientRetransmit in singleClientHandleIncomingPTP got delayresp grant")
					}
					removeClientRetransmit(cfg, cl)
					handleRetransmit(cfg, cl, false, 0)
					// handle statistics
					cl.GotDelayRespGrantReqTime = in.Timestamp
				}
			case *ptp.CancelUnicastTransmissionTLV:
				log.Infof("!!!!!!!!!!!!!!Got cancel Unicast transmission TLV!!!!!!!!!!!!!!!!!!!!!!!!")
				reqAck := reqAckCancelUnicast(0, v.MsgTypeAndFlags.MsgType())
				toSendPayload, err = ptp.Bytes(reqAck)
				if err != nil {
					return nil, err
				}
			default:
				log.Infof("!!!!!!!!!!!!!! Got unsupported TLV !!!!!!!!!!!!!!")
				return nil, fmt.Errorf("got unsupported TLV type %s(%d)", tlv.Type(), tlv.Type())
			}
		}
	case ptp.MessageAnnounce:
		atomic.AddUint64(&cfg.Counters.TotalGenMsgRcvd, 1)
		atomic.AddUint64(&cfg.Counters.TotalAnnounceRcvd, 1)
		announce := &ptp.Announce{}
		if err := ptp.FromBytes(payload, announce); err != nil {
			return nil, fmt.Errorf("reading announce msg: %w", err)
		}
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("Announce %v seq=%d, gmIdentity=%s, gmTimeSource=%s, stepsRemoved=%d",
				cl.ClientIP,
				announce.SequenceID, announce.GrandmasterIdentity, announce.TimeSource,
				announce.StepsRemoved)
		}
		for i := latencyMeasCount - 1; i > 0; i-- {
			cl.lastAnnounceTimes[i] = cl.lastAnnounceTimes[i-1]
		}
		cl.lastAnnounceTimes[0] = in.Timestamp
	case ptp.MessageSync:
		atomic.AddUint64(&cfg.Counters.TotalEventMsgRcvd, 1)
		atomic.AddUint64(&cfg.Counters.TotalSyncRcvd, 1)
		b := &ptp.SyncDelayReq{}
		if err := ptp.FromBytes(payload, b); err != nil {
			return nil, fmt.Errorf("reading sync msg: %w", err)
		}
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("Sync %v seq=%d, our ReceiveTimestamp=%v ", cl.ClientIP,
				b.SequenceID,
				in.Timestamp)
		}
		for i := latencyMeasCount - 1; i > 0; i-- {
			cl.lastSyncTimes[i] = cl.lastSyncTimes[i-1]
		}
		cl.lastSyncTimes[0] = in.Timestamp
	case ptp.MessageDelayResp:
		atomic.AddUint64(&cfg.Counters.TotalEventMsgRcvd, 1)
		atomic.AddUint64(&cfg.Counters.TotalDelayRespRcvd, 1)
		cl.CountDelayResp++
		b := &ptp.DelayResp{}
		if err := ptp.FromBytes(payload, b); err != nil {
			return nil, fmt.Errorf("reading delay_resp msg: %w", err)
		}
		// increase genSequence here so next DelayReq is one higher
		cl.eventSequence++
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("DelayResp %v seq=%d, server ReceiveTimestamp=%v our RcvTS=%v ",
				cl.ClientIP,
				b.SequenceID, b.ReceiveTimestamp.Time(),
				in.Timestamp)
		}
		// handle statistics
		cl.GotDelayRespTime = in.Timestamp
	case ptp.MessageFollowUp:
		atomic.AddUint64(&cfg.Counters.TotalGenMsgRcvd, 1)
		atomic.AddUint64(&cfg.Counters.TotalFollowUpRcvd, 1)
		b := &ptp.FollowUp{}
		if err := ptp.FromBytes(payload, b); err != nil {
			return nil, fmt.Errorf("reading follow_up msg: %w", err)
		}
		if cfg.DebugLogClient || cfg.DebugPrint {
			log.Infof("FollowUp %v seq=%d, server PreciseOriginTimestamp=%v our RcvTS=%v ",
				cl.ClientIP,
				b.SequenceID, b.PreciseOriginTimestamp.Time(),
				in.Timestamp)
		}
		for i := latencyMeasCount - 1; i > 0; i-- {
			cl.lastFollowupTimes[i] = cl.lastFollowupTimes[i-1]
		}
		cl.lastFollowupTimes[0] = in.Timestamp
	default:
		return nil, fmt.Errorf("Unknown ptp message")
	}

	if toSendPayload != nil {
		// craft the other layers of the packet
		// only used in the cancel case, don't bother for now

	} else {
		return nil, nil
	}
	return nil, nil
}

func handleUDPIncoming(cfg *ClientGenConfig, in *PktDecoder) {
	udp := &in.udp

	// sanity check, make sure UDP port is for PTP, 319 or 320
	if (udp.DstPort != ptp.PortEvent) && (udp.DstPort != ptp.PortGeneral) {
		return
	}
	var isIP4 bool
	var isIP6 bool
	var index int
	isIP4 = false
	isIP6 = false
	// could be ipv4 or ipv6
	payload := in.udp.LayerPayload()
	for index = range in.layers {
		if in.layers[index] == layers.LayerTypeIPv6 {
			isIP6 = true
			if cfg.DebugPrint {
				log.Debugf("Found ipv6 layer")
			}
		} else if in.layers[index] == layers.LayerTypeIPv4 {
			isIP4 = true
			if cfg.DebugPrint {
				log.Debugf("Found ipv4 layer")
			}
		}
	}

	var cl *SingleClientGen
	var err error
	if isIP4 {
		// ipv4 udp
		ip4 := &in.ip4
		if cfg.DebugPrint {
			log.Debugf("Got ipv4 %+v", ip4)
		}
		// check if IP is in client range
		// get the client structure for this
		if !in.fromTX {
			cl, err = getClientFromIP(cfg, ip4.DstIP)
		} else {
			cl, err = getClientFromIP(cfg, ip4.SrcIP)
		}
		if cl == nil || err != nil {
			if cfg.DebugPrint {
				log.Errorf("handleUDPIncoming ipv4 not found %v, err %v", ip4.DstIP, err)
			}
			return
		}
	} else if isIP6 {
		ip6 := &in.ip6
		if cfg.DebugPrint {
			log.Debugf("Got ipv6 %+v", ip6)
		}
		if !in.fromTX {
			cl, err = getClientFromIP(cfg, ip6.DstIP)
		} else {
			cl, err = getClientFromIP(cfg, ip6.SrcIP)
		}
		if cl == nil || err != nil {
			if cfg.DebugPrint {
				log.Errorf("handleUDPIncoming ipv6 not found %v, err %v", ip6.DstIP, err)
			}
			return
		}
	} else {
		return
	}
	// ok this is a client I'm simulating, look at what the PTP message is

	toSend, err := singleClientHandleIncomingPTP(cfg, cl, in, payload)
	if err != nil {
		return
	}
	if toSend != nil {
		cfg.RunData.rawOutput[getTxChanNumToUse(cfg)] <- toSend
	}

}

func startClientProcessor(cfg *ClientGenConfig) {
	retransStartDone := make(chan bool)
	for retransProc := 0; retransProc < cfg.NumClientRetransmitProcs; retransProc++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				doneChan := make(chan error, 1)
				var profiler Profiler
				var minItem *timeItem
				var heapOp *parallelHeapOp
				var err error
				profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("Client Retransmit Proc %d", i))
				cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)
				myHeap := &cfg.RunData.retransmitHeap[i]

				go func() {
					if cfg.DebugPrint {
						log.Debugf("Starting client retransmit processor %d\n", i)
					}
					retransStartDone <- true
					for {
						select {
						case heapOp = <-myHeap.operationChan:
							if cfg.DebugPrint || cfg.DebugRetransProc {
								log.Debugf("Retransmit proc %d operation %+v\n", i, heapOp)
								log.Debugf("Retransmit proc %d start operation %+v\n", i, heapOp)
							}
							profiler.Tick()
							if heapOp.operation == heapRemove {
								myHeap.heap.SafeRemove(heapOp.item)
								heapOp.item.index = -1
							} else if heapOp.operation == heapUpdateInsert {
								myHeap.heap.SafeUpdate(heapOp.item, heapOp.value, heapOp.priorityReq)
							}
							cfg.RunData.heapOpPool.Put(heapOp) // put it back in the pool
							profiler.Tock()
							if cfg.DebugPrint || cfg.DebugRetransProc {
								log.Debugf("Retransmit proc %d done %v\n", i, heapOp.item.value.(*SingleClientGen).ClientIP)
							}
						default:
							if myHeap.heap.SafePeek() == nil {
								continue
							}
							minItem = myHeap.heap.SafePeek().(*timeItem)
							if minItem != nil && fastime.Now().After(minItem.priority) {
								profiler.Tick()
								debugLenBefore := myHeap.heap.Len()
								minItem = myHeap.heap.SafePop().(*timeItem)
								if cfg.DebugPrint || cfg.DebugRetransProc {
									log.Debugf("Handling retransmit client %v len before %d after %d\n",
										minItem.value.(*SingleClientGen).ClientIP,
										debugLenBefore, myHeap.heap.Len())
								}
								cl := minItem.value.(*SingleClientGen)
								cl.CountHandleRetransmitFromHeap++
								handleRetransmit(cfg, cl, true, i)
								profiler.Tock()
							}
						}
					}
				}()
				select {
				case <-(*cfg.Ctx).Done():
					log.Infof("Client Retransmit Proc %d cancelling", i)
					return (*cfg.Ctx).Err()
				case err = <-doneChan:
					log.Infof("Client Retransmit Proc %d donechan", i)
					return err
				}
			})
		}(retransProc)
		select {
		case <-retransStartDone:
			if cfg.DebugPrint || cfg.DebugRetransProc {
				log.Debugf("Client Retransmit worker %d running", retransProc)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Client Retransmit worker startup error")
			return 
		}
	}

	if !cfg.RestartClientsAfterDuration {
		return
	}
	restartStartDone := make(chan bool)
	for restartProc := 0; restartProc < cfg.NumClientRestartProcs; restartProc++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				doneChan := make(chan error, 1)
				var profiler Profiler
				var minItem *timeItem
				profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("Client Restart Proc %d", i))
				cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)
				myHeap := &cfg.RunData.restartHeap[i]
				go func() {
					if cfg.DebugPrint {
						log.Debugf("Starting client restart processor %d\n", i)
					}

					restartStartDone <- true
					for {
						select {
						case heapOp := <-myHeap.operationChan:
							profiler.Tick()
							if heapOp.operation == heapRemove {
								myHeap.heap.SafeRemove(heapOp.item)
								heapOp.item.index = -1
							} else if heapOp.operation == heapUpdateInsert {
								myHeap.heap.SafeUpdate(heapOp.item, heapOp.value, heapOp.priorityReq)
							}
							cfg.RunData.heapOpPool.Put(heapOp)
							profiler.Tock()
						default:
							peekItem := myHeap.heap.SafePeek()
							if peekItem == nil {
								continue
							}
							minItem = peekItem.(*timeItem)
							if minItem != nil && fastime.Now().After(minItem.priority) {
								profiler.Tick()
								minItem = myHeap.heap.SafePop().(*timeItem)
								if cfg.DebugPrint || cfg.DebugRestartProc {
									log.Debugf("Handling restart client %v\n",
										minItem.value.(*SingleClientGen).ClientIP)
								}
								handleRestart(cfg, minItem.value.(*SingleClientGen))
								profiler.Tock()
							}
						}
					}
				}()
				select {
				case <-(*cfg.Ctx).Done():
					log.Infof("Client Restart Proc %d cancelling", i)
					return (*cfg.Ctx).Err()
				case err := <-doneChan:
					log.Infof("Client Restart Proc %d donechan", i)
					return err
				}
			})
		}(restartProc)
		select {
		case <-restartStartDone:
			if cfg.DebugPrint || cfg.DebugRestartProc {
				log.Infof("Client Restart worker %d running", restartProc)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Client Restart worker startup error")
			return
		}
	}
}
