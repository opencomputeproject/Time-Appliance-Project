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
	"context"
	"sync"
	"time"

	"golang.org/x/sync/errgroup"
	"github.com/kpango/fastime"
)

// A busy profiler based on time between Tick/Tock or Counting Idle versus Work
// by default, averages each busy calculated via tick tock tick pattern
// per second
type Profiler struct {
	Name           string
	lastSecondBusy float64
	busyValid      bool

	totalTime     time.Duration
	totalWorkTime time.Duration

	// used for adding to work time
	startWork time.Time

	// used for computing totalWorkTime
	startAccumulation time.Time

	useTimeMode bool

	mu  sync.Mutex
	eg  *errgroup.Group
	ctx *context.Context
}

// Initializes a busy profiler with error group and context with a name and as using time or counting
func (p *Profiler) Init(eg *errgroup.Group, ctx *context.Context, useTime bool, n string) {
	p.Name = n
	p.eg = eg
	p.ctx = ctx
	ticker := time.NewTicker(1 * time.Second)
	p.startAccumulation = fastime.Now()
	p.useTimeMode = useTime
	p.busyValid = false
	p.eg.Go(func() error {
		go func() {
			for {
				<-ticker.C
				// calculate 1 second average
				// make sure array isn't changing
				p.mu.Lock()
				// simple average
				if p.useTimeMode {
					p.totalTime = fastime.Now().Sub(p.startAccumulation)
				}
				p.lastSecondBusy = (float64(p.totalWorkTime) / float64(p.totalTime)) * 100
				p.totalWorkTime = 0
				p.totalTime = 0
				if p.useTimeMode {
					p.startAccumulation = fastime.Now()
				}
				p.busyValid = true
				p.mu.Unlock()
			}
		}()

		<-(*p.ctx).Done()
		return nil
	})
}

// GetLastBusy returns the percentage the profiler calculated it was busy in the last second
func (p *Profiler) GetLastBusy() float64 {
	p.mu.Lock()
	toReturn := p.lastSecondBusy
	p.mu.Unlock()
	return toReturn
}

// Tick stores the current time right as work starts
func (p *Profiler) Tick() {
	p.startWork = fastime.Now()
}

// Tock stores the difference between Tick and Now as how long work took
func (p *Profiler) Tock() {
	p.totalWorkTime += fastime.Now().Sub(p.startWork)
}

// CountIdle for non-time based , counts when no work was done
func (p *Profiler) CountIdle() {
	// add to totalTime
	p.totalTime += 1
}

// CountWork for non-time based, counts when work was done
func (p *Profiler) CountWork() {
	// add to totalTime and WorkTime
	p.totalTime += 1
	p.totalWorkTime += 1
}

// IsValid provides status when the lastSecondBusy is available
func (p *Profiler) IsValid() bool {
	return p.busyValid
}
