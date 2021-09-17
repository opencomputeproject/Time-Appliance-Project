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
	"testing"
	"time"

	"golang.org/x/sync/errgroup"
	"github.com/kpango/fastime"
	"github.com/stretchr/testify/require"
)

func Test_constantWorkWithTime(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, true, "ConstantWork")

	// just wait for first valid
	for !profiler.IsValid() {
		profiler.Tick()
		time.Sleep(100 * time.Microsecond)
		profiler.Tock()
	}
	// difficult to guarantee 100% every time
	// allow anything greater than 99%
	require.Greater(t, profiler.GetLastBusy(), 99.0)
	require.LessOrEqual(t, profiler.GetLastBusy(), 100.0)

	f.Stop()
}

func Test_noWorkWithTime(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, true, "NoWork")

	profiler.Tick()
	profiler.Tock()
	for !profiler.IsValid() {
		time.Sleep(10 * time.Microsecond)
	}
	require.Equal(t, float64(0), profiler.GetLastBusy())
	f.Stop()
}

func Test_halfWorkWithTime(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, true, "HalfWork")

	for !profiler.IsValid() {
		profiler.Tick()
		time.Sleep(100000 * time.Microsecond)
		profiler.Tock()
		time.Sleep(100000 * time.Microsecond)
	}
	// difficult to guarantee exactly 50% every time
	// allow an approximate range +/- 5%
	require.GreaterOrEqual(t, profiler.GetLastBusy(), 45.0)
	require.LessOrEqual(t, profiler.GetLastBusy(), 55.0)
	f.Stop()
}

func Test_allWorkCount(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, false, "AllWork")

	for !profiler.IsValid() {
		profiler.CountWork()
		time.Sleep(10 * time.Microsecond)
	}
	require.Equal(t, float64(100), profiler.GetLastBusy())
	f.Stop()
}

func Test_noWorkCount(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, false, "NoWork")

	for !profiler.IsValid() {
		profiler.CountIdle()
		time.Sleep(10 * time.Microsecond)
	}
	require.Equal(t, float64(0), profiler.GetLastBusy())
	f.Stop()
}

func Test_halfWorkCount(t *testing.T) {
	ctx := context.Background()
	errg, ctx := errgroup.WithContext(ctx)
	f := fastime.New().StartTimerD(ctx, 1*time.Microsecond)
	var profiler Profiler
	profiler.Init(errg, &ctx, false, "HalfWork")

	for !profiler.IsValid() {
		profiler.CountIdle()
		profiler.CountWork()
		time.Sleep(10 * time.Microsecond)
	}
	require.Equal(t, float64(50), profiler.GetLastBusy())
	f.Stop()
}
