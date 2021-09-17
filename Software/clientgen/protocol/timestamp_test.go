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

	"github.com/stretchr/testify/require"
	"golang.org/x/sys/unix"
)

// Testing conversion so if Packet structure changes we notice
func Test_byteToTime(t *testing.T) {
	timeb := []byte{63, 155, 21, 96, 0, 0, 0, 0, 52, 156, 191, 42, 0, 0, 0, 0}
	res, err := byteToTime(timeb)
	require.Nil(t, err)

	require.Equal(t, int64(1612028735717200436), res.UnixNano())
}

func Test_ReadTXtimestamp(t *testing.T) {
	conn, err := net.ListenUDP("udp", &net.UDPAddr{IP: net.ParseIP("127.0.0.1"), Port: 0})
	require.Nil(t, err)
	defer conn.Close()

	connFd, err := ConnFd(conn)
	require.Nil(t, err)

	txts, attempts, err := ReadTXtimestamp(connFd)
	require.Equal(t, time.Time{}, txts)
	require.Equal(t, maxTXTS, attempts)
	require.Equal(t, fmt.Errorf("no TX timestamp found after %d tries", maxTXTS), err)

	err = EnableSWTimestampsSocket(connFd)
	require.Nil(t, err)

	addr := &net.UDPAddr{IP: net.ParseIP("127.0.0.1"), Port: 12345}
	_, err = conn.WriteTo([]byte{}, addr)
	require.Nil(t, err)
	txts, attempts, err = ReadTXtimestamp(connFd)

	require.NotEqual(t, time.Time{}, txts)
	require.Equal(t, 1, attempts)
	require.Nil(t, err)

}

func Test_scmDataToTime(t *testing.T) {
	hwData := []byte{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		63, 155, 21, 96, 0, 0, 0, 0, 52, 156, 191, 42, 0, 0, 0, 0,
	}
	swData := []byte{
		63, 155, 21, 96, 0, 0, 0, 0, 52, 156, 191, 42, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	}
	noData := []byte{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	}

	tests := []struct {
		name    string
		data    []byte
		want    int64
		wantErr bool
	}{
		{
			name:    "hardware timestamp",
			data:    hwData,
			want:    1612028735717200436,
			wantErr: false,
		},
		{
			name:    "software timestamp",
			data:    swData,
			want:    1612028735717200436,
			wantErr: false,
		},
		{
			name:    "zero timestamp",
			data:    noData,
			want:    0,
			wantErr: true,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			res, err := scmDataToTime(tt.data)
			if tt.wantErr {
				require.Error(t, err)
			} else {
				require.Nil(t, err)
				require.Equal(t, tt.want, res.UnixNano())
			}
		})
	}
}

func TestIPToSockaddr(t *testing.T) {
	ip4 := net.ParseIP("127.0.0.1")
	ip6 := net.ParseIP("::1")
	port := 123

	expectedSA4 := &unix.SockaddrInet4{Port: port}
	copy(expectedSA4.Addr[:], ip4.To4())

	expectedSA6 := &unix.SockaddrInet6{Port: port}
	copy(expectedSA6.Addr[:], ip6.To16())

	sa4 := IPToSockaddr(ip4, port)
	sa6 := IPToSockaddr(ip6, port)

	require.Equal(t, expectedSA4, sa4)
	require.Equal(t, expectedSA6, sa6)
}

func TestSockaddrToIP(t *testing.T) {
	ip4 := net.ParseIP("127.0.0.1")
	ip6 := net.ParseIP("::1")
	port := 123

	sa4 := IPToSockaddr(ip4, port)
	sa6 := IPToSockaddr(ip6, port)

	require.Equal(t, ip4.String(), SockaddrToIP(sa4).String())
	require.Equal(t, ip6.String(), SockaddrToIP(sa6).String())
}

/*
o: [60 0 0 0 0 0 0 0 41 0 0 0 25 0 0 0 42 0 0 0 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 64 0 0 0 0 0 0 0 1 0 0 0 65 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 230 180 10 97 0 0 0 0 239 83 199 39 0 0 0 0 ]
Data [{Header:{Len:60 Level:41 Type:25} Data:[42 0 0 0 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]}, {Header:{Len:64 Level:1 Type:65} Data:[0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 230 180 10 97 0 0 0 0 239 83 199 39 0 0 0 0]}]
*/
func TestSocketControlMessageTimestamp(t *testing.T) {
	if timestamping != unix.SO_TIMESTAMPING_NEW {
		t.Skip("This test supports SO_TIMESTAMPING_NEW only. No sample of SO_TIMESTAMPING")
	}

	b := []byte{60, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 25, 0, 0, 0, 42, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 65, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 230, 180, 10, 97, 0, 0, 0, 0, 239, 83, 199, 39, 0, 0, 0, 0}
	ts, err := SocketControlMessageTimestamp(b)
	require.NoError(t, err)
	require.Equal(t, int64(1628091622667374575), ts.UnixNano())

}
