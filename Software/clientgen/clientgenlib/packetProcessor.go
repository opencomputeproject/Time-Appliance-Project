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

	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	log "github.com/sirupsen/logrus"
)

func handleICMPv6Incoming(cfg *ClientGenConfig, in *PktDecoder) {
	ipv6 := &in.ip6
	icmpv6 := &in.icmpv6
	// sanity check, make sure it's coming to Broadcast
	if !(ipv6.DstIP.IsMulticast() ||
		ipv6.DstIP.IsLinkLocalMulticast() ||
		ipv6.DstIP.IsInterfaceLocalMulticast()) {
		log.Infof("Handle ICMPv6 incoming not multicast!")
		return
	}

	// handle neighbor solitication, others maybe later
	if icmpv6.TypeCode.Type() != layers.ICMPv6TypeNeighborSolicitation {
		log.Infof("Handle ICMPv6 incoming not Neighbor Solicitation!")
		return
	}
	// need to look at
	icmpv6ns := &in.icmpv6ns

	// https://blog.apnic.net/2019/10/18/how-to-ipv6-neighbor-discovery/
	// make sure this ns is for a client IP I'm running
	cl, err := getClientFromIP(cfg, icmpv6ns.TargetAddress)
	if err != nil || cl == nil {
		log.Infof("Handle ICMPv6 incoming not my client!")
		return
	}

	// provide a neighbor announcement response
	// inefficient , need to cache this but get MAC address of interface here
	ethLayer := &in.eth
	eth := layers.Ethernet{
		SrcMAC:       cfg.srcMAC,
		DstMAC:       ethLayer.SrcMAC, // could also use ICMPv6OptSourceAddress
		EthernetType: layers.EthernetTypeIPv6,
	}
	ip := layers.IPv6{
		SrcIP:      icmpv6ns.TargetAddress,
		DstIP:      ipv6.SrcIP,
		Version:    6,
		HopLimit:   255,
		NextHeader: layers.IPProtocolICMPv6,
	}
	icmp := layers.ICMPv6{
		TypeCode: layers.CreateICMPv6TypeCode(layers.ICMPv6TypeNeighborAdvertisement, 0),
	}
	icmpna := layers.ICMPv6NeighborAdvertisement{
		Flags:         0x40 | 0x20, // solicited (0x40) and override (0x20)
		TargetAddress: icmpv6ns.TargetAddress,
		Options: []layers.ICMPv6Option{
			{Type: layers.ICMPv6OptTargetAddress,
				Data: cfg.srcMAC,
			},
		},
	}

	out := cfg.RunData.outPacketPool.Get().(*outPacket)
	out.cl = nil
	out.getTS = false
	out.pktType = pktIgnore

	err = icmp.SetNetworkLayerForChecksum(&ip)
	if err != nil {
		log.Errorf("icmp SetNetworkLayerForChecksum failed! %v", err)
	}
	err = gopacket.SerializeLayers(*out.data,
		cfg.RunData.commonSerializeOp, &eth, &ip, &icmp, &icmpna)
	if err != nil {
		log.Errorf("Serialize failed! %v", err)
	}
	cfg.RunData.rawOutput[getTxChanNumToUse(cfg)] <- out

}

func handleARPIncoming(cfg *ClientGenConfig, in *PktDecoder) {
	ethLayer := &in.eth
	arpLayer := &in.arp

	if arpLayer.Operation == 1 { // only care about requests
		// craft the ARP packet by layer, ethernet then ARP
		eth := layers.Ethernet{
			SrcMAC:       cfg.srcMAC,
			DstMAC:       ethLayer.SrcMAC,
			EthernetType: layers.EthernetTypeARP,
		}
		arpResponse := *arpLayer
		arpResponse.Operation = 2                // sending response
		arpResponse.SourceHwAddress = cfg.srcMAC // from my interface

		// need to validate if the IP falls in my pseudo client range, but for now just reply

		arpResponse.SourceProtAddress = arpLayer.DstProtAddress
		arpResponse.DstHwAddress = arpLayer.SourceHwAddress
		arpResponse.DstProtAddress = arpLayer.SourceProtAddress

		out := cfg.RunData.outPacketPool.Get().(*outPacket)
		err := gopacket.SerializeLayers(*out.data,
			cfg.RunData.commonSerializeOp, &eth, &arpResponse)
		if err != nil {
			fmt.Errorf("Handle arp incoming seriallayers err %v", err)
		}
		out.cl = nil
		out.pktType = pktIgnore
		out.getTS = false

		if cfg.DebugPrint {
			debugPacket := gopacket.NewPacket((*out.data).Bytes(), layers.LinkTypeEthernet, gopacket.Default)
			log.Debugf("handleARPIncoming debug arp response raw %v", debugPacket)
		}
		cfg.RunData.rawOutput[getTxChanNumToUse(cfg)] <- out
	}

}

func startPacketProcessor(cfg *ClientGenConfig) {
	packetProcStartDone := make(chan bool)
	for proc := 0; proc < cfg.NumPacketProcessors; proc++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				var profiler Profiler
				var err error
				profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("PacketProcessor %d", i))
				cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)
				doneChan := make(chan error, 1)
				go func() {
					var data *PktDecoder
					var index int
					packetProcStartDone <- true
					for {
						data = <-cfg.RunData.pktToProc[i]
						profiler.Tick()
						if cfg.DebugPrint {
							log.Debugf("Packet processor %d got packet %+v", i, data)
						}
						// loop over the layers
						for index = range data.layers {
							if data.layers[index] == layers.LayerTypeARP {
								if cfg.DebugPrint {
									log.Debugf("PacketProcessor %d handleARPIncoming", i)
								}
								if !data.fromTX {
									handleARPIncoming(cfg, data)
								}
								break
							} else if data.layers[index] == layers.LayerTypeICMPv6 {
								// similar to ARP
								if cfg.DebugPrint {
									log.Debugf("Handle %d ICMPv6 Incoming", i)
								}
								if !data.fromTX {
									handleICMPv6Incoming(cfg, data)
								}
								break
							} else if data.layers[index] == layers.LayerTypeUDP {
								// could be ipv6 or ipv4
								if cfg.DebugPrint {
									log.Debugf("PacketProcessor %d handleUDPIncoming", i)
								}
								handleUDPIncoming(cfg, data)
								break
							}
						}
						if data.fromTX {
							// need to put back the raw byte slice also
							cfg.RunData.bytePool.Put(data.rawData)
						}
						cfg.RunData.goPacketDecoderPool.Put(data)
						profiler.Tock()
					}
				}()
				select {
				case <-(*cfg.Ctx).Done():
					log.Infof("packet Processor %d ending", i)
					return (*cfg.Ctx).Err()
				case err = <-doneChan:
					return err
				}
			})
		}(proc)
		select {
		case <-packetProcStartDone:
			if cfg.DebugPrint {
				log.Infof("Packet processor %d running", proc)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Packet processor startup error")
			return 
		}
	}
}
