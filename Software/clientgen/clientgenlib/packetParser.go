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

	log "github.com/sirupsen/logrus"
)

func startPacketParser(cfg *ClientGenConfig) {
	parserStartDone := make(chan bool)
	for parser := 0; parser < cfg.NumPacketParsers; parser++ {
		func(i int) {
			cfg.Eg.Go(func() error {
				doneChan := make(chan error, 1)
				var err error
				var profiler Profiler
				profiler.Init(cfg.Eg, cfg.Ctx, true, fmt.Sprintf("PacketParser %d", i))
				cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)

				go func() {
					var pkt *inPacket
					var decoder *PktDecoder
					var err error
					parserStartDone <- true
					for {
						for pkt = range cfg.RunData.rawInput[i] {
							profiler.Tick()
							decoder = cfg.RunData.goPacketDecoderPool.Get().(*PktDecoder)
							err = decoder.parser.DecodeLayers(pkt.data, &decoder.layers)
							if err != nil && (cfg.DebugPrint || cfg.DebugIoWkrRX) {
								log.Errorf("Decoder parser error %v", err)
							}
							decoder.Timestamp = pkt.ts
							decoder.fromTX = pkt.fromTX
							decoder.rawData = pkt.data

							// return this back to the pool
							cfg.RunData.inPacketPool.Put(pkt)

							if cfg.DebugPrint || cfg.DebugIoWkrRX {
								log.Debugf("PacketParser %d parsed: %+v", i, decoder)
							}
							cfg.RunData.pktToProc[getPacketProcChanNumToUse(cfg)] <- decoder
							profiler.Tock()
						}
					}
				}()
				select {
				case <-(*cfg.Ctx).Done():
					log.Infof("Packet parser %d cancelling", i)
					return (*cfg.Ctx).Err()
				case err = <-doneChan:
					log.Infof("Packet parser %d donechan", i)
					return err
				}
			})
		}(parser)
		select {
		case <-parserStartDone:
			if cfg.DebugPrint || cfg.DebugIoWkrRX {
				log.Infof("Packet Parser %d running", parser)
			}
			continue
		case <-(*cfg.Ctx).Done():
			log.Errorf("Packet Parser startup error")
			return 
		}
	}

}
