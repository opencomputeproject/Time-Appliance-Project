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

package main

import (
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"runtime/pprof"
	"time"

	genlib "clientgenlib"
	log "github.com/sirupsen/logrus"
	"golang.org/x/sync/errgroup"
)

func main() {

	var cpuProfile string
	var configFile string
	var logFile *os.File
	var err error

	flag.Usage = func() {
		fmt.Printf("Usage of clientgen:\n")
		fmt.Printf("Please refer to the README.md for the configuration file syntax\n")
	}
	flag.StringVar(&cpuProfile, "profilelog", "", "File to write CPU Profiling data to")
	flag.StringVar(&configFile, "config", "clientgen_config.json", "JSON Configuration file")
	flag.Parse()

	if cpuProfile != "" {
		logFile, err = os.Create(cpuProfile)
		if err != nil {
			fmt.Printf("Could not create cpu profile %v\n", err)
			return
		}
		defer logFile.Close()
		pprof.StartCPUProfile(logFile)
		defer pprof.StopCPUProfile()
	}

	testConfig := &genlib.ClientGenConfig{}

	log.Infof("Loading config from file %s", configFile)
	file, err := os.Open(configFile)
	defer file.Close()
	if err != nil {
		log.Errorf("Failed to open config file %s err %v", configFile, err)
		return
	}
	decoder := json.NewDecoder(file)
	err = decoder.Decode(&testConfig)
	if err != nil {
		log.Errorf("Failed to decode config file err %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(float64(time.Second)*testConfig.TimeoutSec))
	defer cancel()
	errg, ctx := errgroup.WithContext(ctx)

	testConfig.Eg = errg
	testConfig.Ctx = &ctx

	log.Infof("Starting clientgen!")
	genlib.StartClientGen(testConfig)
	fmt.Println("Done!")
}
