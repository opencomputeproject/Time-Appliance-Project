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

package protocol

import (
	"fmt"
	"net"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestTimeIntervalNanoseconds(t *testing.T) {
	tests := []struct {
		in   TimeInterval
		want float64
	}{
		{
			in:   13697024,
			want: 209,
		},
		{
			in:   0x0000000000028000,
			want: 2.5,
		},
		{
			in:   -9240576,
			want: -141,
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("TimeInterval.Nanoseconds t=%d", tt.in), func(t *testing.T) {
			// first, convert from TimeInterval to time.Time
			got := tt.in.Nanoseconds()
			require.Equal(t, tt.want, got)
			// then convert time.Time we just got back to Timestamp
			gotTI := NewTimeInterval(got)
			assert.Equal(t, tt.in, gotTI)
		})
	}
}

func TestTimestamp(t *testing.T) {
	tests := []struct {
		in   Timestamp
		want time.Time
	}{
		{
			in: Timestamp{
				Seconds:     [6]byte{0x0, 0x0, 0x0, 0x0, 0x0, 0x02},
				Nanoseconds: 1,
			},
			want: time.Unix(2, 1),
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("Timestamp t=%d", tt.in), func(t *testing.T) {
			// first, convert from Timestamp to time.Time
			got := tt.in.Time()
			require.Equal(t, tt.want, got)
			// then convert time.Time we just got back to Timestamp
			gotTS := NewTimestamp(got)
			assert.Equal(t, tt.in, gotTS)
		})
	}
}

func TestLogInterval(t *testing.T) {
	tests := []struct {
		in   LogInterval
		want float64 // seconds
	}{
		{
			in:   0,
			want: 1,
		},
		{
			in:   1,
			want: 2,
		},
		{
			in:   5,
			want: 32,
		},
		{
			in:   -1,
			want: 0.5,
		},
		{
			in:   -7,
			want: 0.0078125,
		},
	}
	for _, tt := range tests {
		t.Run(fmt.Sprintf("LogInterval t=%d", tt.in), func(t *testing.T) {
			// first, convert from LogInterval to Seconds
			gotDuration := tt.in.Duration()
			require.Equal(t, tt.want, gotDuration.Seconds())
			// then convert time.Duration we just got back to LogInterval
			gotLI, err := NewLogInterval(gotDuration)
			require.Nil(t, err)
			assert.Equal(t, tt.in, gotLI)
		})
	}
}

func TestClockIdentity(t *testing.T) {
	macStr := "0c:42:a1:6d:7c:a6"
	mac, err := net.ParseMAC(macStr)
	require.Nil(t, err)
	got, err := NewClockIdentity(mac)
	require.Nil(t, err)
	want := ClockIdentity(0xc42a1fffe6d7ca6)
	assert.Equal(t, want, got)
	wantStr := "0c42a1.fffe.6d7ca6"
	assert.Equal(t, wantStr, got.String())
}

func TestPTPText(t *testing.T) {
	tests := []struct {
		name    string
		in      []byte
		want    string
		wantErr bool
	}{
		{
			name:    "no data",
			in:      []byte{},
			want:    "",
			wantErr: true,
		},
		{
			name:    "empty",
			in:      []byte{0},
			want:    "",
			wantErr: false,
		},
		{
			name:    "some text",
			in:      []byte{4, 65, 108, 101, 120},
			want:    "Alex",
			wantErr: false,
		},
		{
			name:    "padding",
			in:      []byte{3, 120, 101, 108, 0},
			want:    "xel",
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			var text PTPText
			err := text.UnmarshalBinary(tt.in)
			if tt.wantErr {
				assert.Error(t, err)
			} else {
				require.Nil(t, err)
				assert.Equal(t, tt.want, string(text))

				gotBytes, err := text.MarshalBinary()
				require.Nil(t, err)
				assert.Equal(t, tt.in, gotBytes)
			}
		})
	}
}
