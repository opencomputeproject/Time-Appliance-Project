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
	"sync/atomic"
	"syscall"
	"time"

	"golang.org/x/sys/unix"
	ptp "github.com/facebookincubator/ptp/protocol"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/google/gopacket/pfring"
	"github.com/kpango/fastime"
	log "github.com/sirupsen/logrus"
)

// inPacket is input packet data + receive timestamp
type inPacket struct {
	data   []byte
	ts     time.Time
	fromTX bool
}

type outPacket struct {
	data    *gopacket.SerializeBuffer
	getTS   bool
	pktType uint8
	sentTS  time.Time
	cl      *SingleClientGen
}

func startIOWorker(cfg *ClientGenConfig) {
	rxStartDone := make(chan bool)
	for rxwkr := 0; rxwkr < cfg.NumRXWorkers; rxwkr++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				doneChan := make(chan error, 1)
				go func() {
					var profiler Profiler
					profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("RX Worker %d", i))
					cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)
					var ring *pfring.Ring
					var rawIn *inPacket
					var err error
					// 1<<24 is PF_RING_DISCARD_INJECTED_PKTS , if you transmit a packet via the ring, doesn't read it back
					if ring, err = pfring.NewRing(cfg.Iface, 4096, (1<<24)|pfring.FlagPromisc|pfring.FlagHWTimestamp); err != nil {
						log.Errorf("pfring ring creation error:", err)
						doneChan <- err
						return
					}
					defer ring.Close()
					// just use fixed cluster number 1, round robin packets
					if err = ring.SetCluster(1, pfring.ClusterType(pfring.ClusterRoundRobin)); err != nil {
						log.Errorf("pfring SetCluster error:", err)
						doneChan <- err
						return
					}
					if err = ring.SetDirection(pfring.ReceiveOnly); err != nil {
						log.Errorf("pfring failed to set direction")
						doneChan <- err
						return
					}
					if err = ring.SetPollWatermark(1); err != nil {
						log.Errorf("pfring failed to set poll watermark")
						doneChan <- err
						return
					}
					if err = ring.SetPollDuration(1); err != nil {
						log.Errorf("pfring failed to set poll watermark")
						doneChan <- err
						return
					}
					if err = ring.SetSamplingRate(1); err != nil {
						log.Errorf("pfring failed to set sample rate")
						doneChan <- err
						return
					}
					// only using read for now
					if err = ring.SetSocketMode(pfring.ReadOnly); err != nil {
						log.Errorf("pfring SetSocketMode error: %v", err)
						doneChan <- err
						return
					} else if err = ring.Enable(); err != nil {
						log.Errorf("pfring Enable error: %v", err)
						doneChan <- err
						return
					}
					if cfg.DebugPrint || cfg.DebugIoWkrRX {
						log.Debugf("RX wkr %d pfring done!", i)
					}

					var data []byte
					var ci gopacket.CaptureInfo
					rxStartDone <- true
					for {
						// try to read from handle
						data, ci, err = ring.ReadPacketData()
						if err != nil || data == nil || len(data) == 0 {
							continue
						}
						profiler.Tick()
						if cfg.DebugPrint || cfg.DebugIoWkrRX {
							log.Debugf("PFring listener %d got data ts %v", i, ci.Timestamp)
						}
						rawIn = cfg.RunData.inPacketPool.Get().(*inPacket)
						rawIn.data = data
						rawIn.ts = ci.Timestamp
						rawIn.fromTX = false

						cfg.RunData.rawInput[getRxChanNumToUse(cfg)] <- rawIn
						atomic.AddUint64(&cfg.Counters.TotalPacketsRcvd, 1)
						atomic.AddUint64(&cfg.perIORX[i], 1)
						profiler.Tock()
					}
				}()
				select {
				case <-(*cfg.Ctx).Done():
					log.Errorf("RX %d done due to context", i)
					return (*cfg.Ctx).Err()
				case err := <-doneChan:
					return err
				}
			})
		}(rxwkr)
		select {
		case <-rxStartDone:
			if cfg.DebugPrint || cfg.DebugIoWkrRX {
				log.Debugf("RX worker %d running", rxwkr)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Rx worker startup error")
			return 
		}
	}


	txStartDone := make(chan bool)
	for txwkr := 0; txwkr < cfg.NumTXWorkers; txwkr++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				doneChan := make(chan error, 1)
				go func() {
					// PFring doesn't implement TX timestamps actually
					// API documentation lists it, but at a low level, its not actually used
					// create a raw socket and send packets via it , read TS similar to Oleg's method
					var profiler Profiler
					profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("TX worker %d", i))
					cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)

					txTSworker := make([]Profiler, cfg.NumTXTSWorkerPerTx)
					for j := 0; j < cfg.NumTXTSWorkerPerTx; j++ {
						txTSworker[j].Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("TX worker %d TSRead worker %d", i, j))
						cfg.PerfProfilers = append(cfg.PerfProfilers, &txTSworker[j])
					}

					ifInfo, err := net.InterfaceByName(cfg.Iface)
					if err != nil {
						log.Errorf("Interface by name failed in start tx worker")
						doneChan <- err
						return
					}
					var haddr [8]byte
					copy(haddr[0:7], ifInfo.HardwareAddr[0:7])
					addr := syscall.SockaddrLinklayer{
						Protocol: syscall.ETH_P_ALL,
						Ifindex:  ifInfo.Index,
						Halen:    uint8(len(ifInfo.HardwareAddr)),
						Addr:     haddr,
					}

					fdTS, err := syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, syscall.ETH_P_ALL)
					if err != nil {
						log.Errorf("Failed to make raw socket for TS worker %d err %v", i, err)
					}
					defer syscall.Close(fdTS)
					err = syscall.Bind(fdTS, &addr)
					if err != nil {
						log.Errorf("Failed to bind TS socket %v", err)
					}

					if err := ptp.IoctlTimestamp(fdTS, cfg.Iface); err != nil {
						log.Errorf("Failed to ioctl timestamp tx worker %v", i)
						return
					}
					// Enable hardware timestamp capabilities on socket
					flags := unix.SOF_TIMESTAMPING_TX_HARDWARE |
						unix.SOF_TIMESTAMPING_RX_HARDWARE |
						unix.SOF_TIMESTAMPING_RAW_HARDWARE
					if err := unix.SetsockoptInt(fdTS, unix.SOL_SOCKET, ptp.Timestamping(), flags); err != nil {
						log.Errorf("Failed to set flags tx worker %v err %v", i, err)
						return
					}
					if err := unix.SetsockoptInt(fdTS, unix.SOL_SOCKET, unix.SO_SELECT_ERR_QUEUE, 1); err != nil {
						log.Errorf("Failed to select err queue tx worker %v", i)
						return
					}

					/* simple socket for non-timestamping */
					fd, err := syscall.Socket(syscall.AF_PACKET, syscall.SOCK_RAW, syscall.ETH_P_ALL)
					if err != nil {
						log.Errorf("Creating simple socket for tx worker %d failed err %v", i, err)
					}
					defer syscall.Close(fd)
					err = syscall.Bind(fd, &addr)
					if err != nil {
						log.Errorf("Simple socket bind failed tx worker %d err %v", i, err)
					}

					var txTSBytesReceived uint64

					// start go-routines to handle TX TS
					txTSStartDone := make(chan bool)
					for j := 0; j < cfg.NumTXTSWorkerPerTx; j++ {
						go func(workerNum int) {
							var pktSent []byte
							var inPkt *inPacket
							var pktSentLen int
							var err error
							var msgs []byte
							var txTS time.Time
							msgs = make([]byte, 1000)
							pktSent = cfg.RunData.bytePool.Get().([]byte)
							// check if there are control messages on timestamp socket
							txTSStartDone <- true
							for {
								// ideally should use ptp.PeekRecvMsgs , but maybe similar overhead, just leave this
								txTSworker[workerNum].Tick()
								pktSentLen, _, _, _, err = unix.Recvmsg(fdTS, pktSent, msgs, unix.MSG_ERRQUEUE)
								if err != nil || pktSentLen == 0 {
									continue
								}
								txTS, err = ptp.SocketControlMessageTimestamp(msgs)
								if err != nil {
									log.Errorf("SocketControlMessageTimestamp err %v", err)
								}
								inPkt = cfg.RunData.inPacketPool.Get().(*inPacket)
								inPkt.data = pktSent
								inPkt.ts = txTS
								inPkt.fromTX = true
								cfg.RunData.rawInput[getRxChanNumToUse(cfg)] <- inPkt
								pktSent = cfg.RunData.bytePool.Get().([]byte)
								atomic.AddUint64(&cfg.Counters.TotalTXTSRead, 1)
								atomic.AddUint64(&txTSBytesReceived, uint64(pktSentLen))
								txTSworker[workerNum].Tock()
							}
						}(j)
						select {
						case <-txTSStartDone:
							if cfg.DebugPrint || cfg.DebugIoWkrTX {
								log.Infof("TX %d TS worker %d running", txwkr, j)
							}
							continue
						case <-(*cfg.Ctx).Done():
							log.Errorf("Tx TS worker startup error")
							return 
						}
					}
					var txTSBytesSent uint64
					var diff uint64
					var out *outPacket
					txStartDone <- true
					for {
						out = <-(cfg.RunData.rawOutput[i]) // want to send a packet
						if out == nil || len((*out.data).Bytes()) == 0 {
							log.Infof("empty data bad!")
							continue
						}
						if cfg.DebugPrint || cfg.DebugIoWkrTX {
							// debug print
							debugPkt := gopacket.NewPacket((*out.data).Bytes(), layers.LinkTypeEthernet, gopacket.Default)
							log.Debugf("Debug txWkr %d send packet %v", i, debugPkt)
						}
						profiler.Tick()
						if out.getTS {
							// some backpressure, let TXTS worker keep up
							// keep the difference below a certain amount
							for {
								diff = txTSBytesSent - atomic.LoadUint64(&txTSBytesReceived)
								if diff < 15000 {
									break
								}
							}
							n, err := syscall.Write(fdTS, (*out.data).Bytes())
							if err != nil || n == 0 {
								log.Errorf("txWkr %d send packet TS failed, n %v err %v", i, n, err)
							}
							if out.cl != nil {
								out.cl.CountOutgoingPackets++
							}
							atomic.AddUint64(&cfg.Counters.TotalTXTSPacketsSent, 1)
							txTSBytesSent += uint64(len((*out.data).Bytes()))
							diff = txTSBytesSent - atomic.LoadUint64(&txTSBytesReceived)
							if diff > atomic.LoadUint64(&cfg.Counters.MaxTXTSBytesOutstanding) {
								atomic.StoreUint64(&cfg.Counters.MaxTXTSBytesOutstanding, diff)
							}
						} else {
							_, err := syscall.Write(fd, (*out.data).Bytes())
							if err != nil {
								log.Errorf("txWkr %d send packet failed, %v", i, err)
							}
							if out.cl != nil {
								out.cl.CountOutgoingPackets++
								out.sentTS = fastime.Now()
								if out.pktType == pktAnnounceGrantReq {
									out.cl.SentAnnounceGrantReqTime = out.sentTS
								} else if out.pktType == pktSyncGrantReq {
									out.cl.SentlastSyncGrantReqTime = out.sentTS
								} else if out.pktType == pktDelayRespGrantReq {
									out.cl.SentDelayRespGrantReqTime = out.sentTS
								} else if out.pktType == pktDelayReq {
									out.cl.SentDelayReqTime = out.sentTS
								}
							}
							if cfg.DebugPrint || cfg.DebugIoWkrTX {
								log.Debugf("Debug txWkr %d send packet", i)
							}
							if err != nil {
								log.Errorf("Raw socket write packet data failed %v", err)
								doneChan <- fmt.Errorf("Raw socket write packet data failed %v", err)
								return
							}
						}
						atomic.AddUint64(&cfg.Counters.TotalPacketsSent, 1)
						atomic.AddUint64(&cfg.perIOTX[i], 1)
						cfg.RunData.outPacketPool.Put(out)
						profiler.Tock()
					}
				}()
				var err error
				select {
				case <-(*cfg.Ctx).Done():
					log.Infof("TX worker %d cancelling due to context done", i)
					return (*cfg.Ctx).Err()
				case err = <-doneChan:
					return err
				}
			})
		}(txwkr)
		select {
		case <-txStartDone:
			if cfg.DebugPrint || cfg.DebugIoWkrTX {
				log.Debugf("TX worker %d running", txwkr)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Tx worker startup error")
			return 
		}
	}
}
