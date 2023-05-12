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
	"reflect"
	"strings"
	"time"

	"github.com/jamiealquiza/tachymeter"
	"github.com/kpango/fastime"
	log "github.com/sirupsen/logrus"
)

func printSliceHistogram(cfg *ClientGenConfig, data []uint64, heapName string) {
	max := data[0]
	min := data[0]
	distinctValues := make([]uint64, 1)
	distinctValuesCount := make([]uint64, 1)
	foundDistinct := false
	for i := 0; i < len(data); i++ {
		if data[i] > max {
			max = data[i]
		}
		if data[i] < min {
			min = data[i]
		}
		foundDistinct = false
		for j := 0; j < len(distinctValues); j++ {
			if distinctValues[j] == data[i] {
				distinctValuesCount[j]++
				foundDistinct = true
				break
			}
		}
		if !foundDistinct {
			distinctValues = append(distinctValues, data[i])
			distinctValuesCount = append(distinctValuesCount, 1)
		}
	}
	fmt.Printf("Total: %d, Max: %d , Min: %d\n", len(data), max, min)
	for i := 0; i < len(distinctValues); i++ {
		fmt.Printf("%d:%d,", distinctValues[i], distinctValuesCount[i])
	}
	fmt.Printf("\n")
}

func startCounterProcessor(cfg *ClientGenConfig) {
	if cfg.CounterPrintIntervalSecs == 0 {
		return
	}
	// for now just print the counters every second
	cfg.Eg.Go(func() error {
		var data GlobalStatistics
		var prevData GlobalStatistics
		var profiler Profiler
		var startTime time.Time
		var clientAnnounceGrantReq []uint64
		var clientSyncGrantReq []uint64
		var clientDelayRespGrantReq []uint64
		var clientDelayReq []uint64
		var clientStates []uint64
		var clientLatencyHistogram *tachymeter.Tachymeter

		profiler.Init(cfg.Eg, cfg.Ctx, true, "CounterProcessor")
		cfg.PerfProfilers = append(cfg.PerfProfilers, &profiler)
		doneChan := make(chan error, 1)
		startTime = fastime.Now()
		if cfg.PrintClientReqData {
			clientAnnounceGrantReq = make([]uint64, len(cfg.RunData.clients))
			clientSyncGrantReq = make([]uint64, len(cfg.RunData.clients))
			clientDelayRespGrantReq = make([]uint64, len(cfg.RunData.clients))
			clientDelayReq = make([]uint64, len(cfg.RunData.clients))
		}
		if cfg.PrintClientData {
			clientStates = make([]uint64, len(cfg.RunData.clients))
		}

		if cfg.PrintLatencyData {
			clientLatencyHistogram = tachymeter.New(&tachymeter.Config{Size: len(cfg.RunData.clients)})
		}

		go func() {
			for {
				profiler.Tick()
				data = cfg.Counters // local copy

				s := reflect.ValueOf(&data).Elem()
				oldS := reflect.ValueOf(&prevData).Elem()

				typeOfData := s.Type()

				fmt.Printf("========================Statistics after %v============\n", fastime.Now().Sub(startTime))
				if cfg.PrintClientData {
					fmt.Printf("==ClientData=============\n")
					for i := 0; i < s.NumField(); i++ {
						f := s.Field(i)
						oldF := oldS.Field(i)
						fmt.Printf("%d: %s = %v, rate %v\n", i,
							typeOfData.Field(i).Name,
							f.Interface(),
							(f.Interface().(uint64)-oldF.Interface().(uint64))/
								uint64(cfg.CounterPrintIntervalSecs))
					}
					for i := 0; i < len(cfg.RunData.clients); i++ {
						clientStates[i] = uint64(cfg.RunData.clients[i].state)
					}
					fmt.Printf("Client states\n")
					printSliceHistogram(cfg, clientStates, "ClientStates")
				}

				if cfg.PrintTxRxCounts {
					fmt.Printf("==Tx Rx Counters=============\n")
					for i := 0; i < len(cfg.perIOTX); i++ {
						fmt.Printf("TX worker %d pkt send: %v\n",
							i, cfg.perIOTX[i])
					}
					for i := 0; i < len(cfg.perIORX); i++ {
						fmt.Printf("RX worker %d pkt recv: %v\n",
							i, cfg.perIORX[i])
					}
				}
				prevData = data

				if cfg.PrintClientReqData {
					// look at the four types of requests sent for each client
					// create a histogram of it

					fmt.Printf("==Client Request Data============\n")
					for i := 0; i < len(cfg.RunData.clients); i++ {
						clientAnnounceGrantReq[i] = cfg.RunData.clients[i].CountAnnounceGrantReq
						clientSyncGrantReq[i] = cfg.RunData.clients[i].CountSyncGrantReq
						clientDelayRespGrantReq[i] = cfg.RunData.clients[i].CountDelayRespGrantReq
						clientDelayReq[i] = cfg.RunData.clients[i].CountDelayReq
					}
					fmt.Printf("Announce Grant Requests sent\n")
					printSliceHistogram(cfg, clientAnnounceGrantReq, "AnnounceGrantReqs")
					fmt.Printf("Sync Grant Requests sent\n")
					printSliceHistogram(cfg, clientSyncGrantReq, "SyncGrantReqs")
					fmt.Printf("Delay Resp Grant Requests sent\n")
					printSliceHistogram(cfg, clientDelayRespGrantReq, "DelayRespGrantReqs")
					fmt.Printf("Delay Requests sent\n")
					printSliceHistogram(cfg, clientDelayReq, "DelayReqs")

				}
				if cfg.PrintLatencyData {
					clientLatencyHistogram.Reset() // reset histogram
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						if !cl.GotAnnounceGrantReqTime.IsZero() &&
							!cl.SentAnnounceGrantReqTime.IsZero() {
							latency := cl.GotAnnounceGrantReqTime.Sub(cl.SentAnnounceGrantReqTime)
							if latency > 0 {
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Announce Grant Latency\n", clientLatencyHistogram.Calc())

					clientLatencyHistogram.Reset() // reset histogram
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						if !cl.GotlastSyncGrantReqTime.IsZero() &&
							!cl.SentlastSyncGrantReqTime.IsZero() {
							latency := cl.GotlastSyncGrantReqTime.Sub(cl.SentlastSyncGrantReqTime)
							if latency > 0 {
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Sync Grant Latency\n", clientLatencyHistogram.Calc())

					clientLatencyHistogram.Reset() // reset histogram
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						if !cl.GotDelayRespGrantReqTime.IsZero() &&
							!cl.SentDelayRespGrantReqTime.IsZero() {
							latency := cl.GotDelayRespGrantReqTime.Sub(cl.SentDelayRespGrantReqTime)
							if latency > 0 {
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Delay Resp Grant Latency\n", clientLatencyHistogram.Calc())

					clientLatencyHistogram.Reset() // reset histogram
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						if !cl.GotDelayRespTime.IsZero() &&
							!cl.SentDelayReqTime.IsZero() {
							latency := cl.GotDelayRespTime.Sub(cl.SentDelayReqTime)
							if latency > 0 { // sometimes see this, not sure why
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Delay Req Latency\n", clientLatencyHistogram.Calc())

					clientLatencyHistogram.Reset()
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						for i := 0; i < latencyMeasCount-1; i++ {
							if cl.lastSyncTimes[i].IsZero() ||
								cl.lastSyncTimes[i+1].IsZero() {
								//fmt.Printf("Debug lastSyncTimes %v\n", cl.lastSyncTimes)
								break
							}
							latency := cl.lastSyncTimes[i].Sub(cl.lastSyncTimes[i+1])
							if latency > 0 {
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Time Between Syncs\n", clientLatencyHistogram.Calc())

					clientLatencyHistogram.Reset()
					for i := 0; i < len(cfg.RunData.clients); i++ {
						cl := &cfg.RunData.clients[i]
						for i := 0; i < latencyMeasCount-1; i++ {
							if cl.lastFollowupTimes[i].IsZero() ||
								cl.lastFollowupTimes[i+1].IsZero() {
								break
							}
							latency := cl.lastFollowupTimes[i].Sub(cl.lastFollowupTimes[i+1])
							if latency > 0 {
								clientLatencyHistogram.AddTime(latency)
							}
						}
					}
					fmt.Println("Time Between Follow-ups\n", clientLatencyHistogram.Calc())

				}

				profiler.Tock()
				if cfg.PrintPerformance {
					fmt.Printf("==Software Performance=============\n")
					for index := range cfg.PerfProfilers {
						name := cfg.PerfProfilers[index].Name
						if (strings.Contains(name, "syscallwrite") ||
							strings.Contains(name, "readTxTimestamp")) &&
							!cfg.DebugDetailPerf {
							continue
						}

						fmt.Printf("Profiler %s last busy %.2f%%\n",
							cfg.PerfProfilers[index].Name,
							cfg.PerfProfilers[index].GetLastBusy())
						if cfg.DebugProfilers {
							log.Infof("Debug %v", cfg.PerfProfilers[index])
						}
					}
				}
				fmt.Printf("========================Statistics end at %v============\n", fastime.Now().Sub(startTime))
				time.Sleep(time.Duration(cfg.CounterPrintIntervalSecs) * time.Second)
			}
		}()
		select {
		case <-(*cfg.Ctx).Done():
			log.Infof("Counter processor done due to context")
			return (*cfg.Ctx).Err()
		case err := <-doneChan:
			log.Errorf("Counter processor done due to err %v", err)
			return err
		}

	})
}
