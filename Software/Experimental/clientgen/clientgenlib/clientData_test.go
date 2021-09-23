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
	"net"
	"testing"

	"github.com/stretchr/testify/require"
)

func Test_ipSubtract(t *testing.T) {
	ip4_a := net.ParseIP("192.168.1.1")
	ip4_b := net.ParseIP("192.168.1.2")

	ip6_a := net.ParseIP("fe80::55de:a728:3bc6:7fae")
	ip6_b := net.ParseIP("fe80::55de:a728:3bc6:7faf")

	pos_val, err := ipSubtract(ip4_b, ip4_a)
	require.Equal(t, uint64(1), pos_val)
	require.Nil(t, err)

	neg_val, err := ipSubtract(ip4_a, ip4_b)
	require.Equal(t, uint64(0), neg_val)
	require.Error(t, err)

	pos_val, err = ipSubtract(ip6_b, ip6_a)
	require.Equal(t, uint64(1), pos_val)
	require.Nil(t, err)

	neg_val, err = ipSubtract(ip6_a, ip6_b)
	require.Equal(t, uint64(0), neg_val)
	require.Error(t, err)
}

func Test_nextIP(t *testing.T) {
	ip4_a := net.ParseIP("192.168.1.1")

	next := NextIP(ip4_a, 2)
	require.Equal(t, true, next.Equal(net.ParseIP("192.168.1.3")))

	next = NextIP(ip4_a, 256)
	require.Equal(t, true, next.Equal(net.ParseIP("192.168.2.1")))

	next = NextIP(ip4_a, 65536)
	require.Equal(t, true, next.Equal(net.ParseIP("192.169.1.1")))

	next = NextIP(ip4_a, 16777216)
	require.Equal(t, true, next.Equal(net.ParseIP("193.168.1.1")))

	ip6_a := net.ParseIP("fe80::55de:a728:3bc6:7fae")

	next = NextIP(ip6_a, 1)
	require.Equal(t, true, next.Equal(net.ParseIP("fe80::55de:a728:3bc6:7faf")))

	next = NextIP(ip6_a, 256)
	require.Equal(t, true, next.Equal(net.ParseIP("fe80::55de:a728:3bc6:80ae")))

	next = NextIP(ip6_a, 65536)
	require.Equal(t, true, next.Equal(net.ParseIP("fe80::55de:a728:3bc7:7fae")))

	next = NextIP(ip6_a, 16777216)
	require.Equal(t, true, next.Equal(net.ParseIP("fe80::55de:a728:3cc6:7fae")))

}

func Test_ipBetween(t *testing.T) {

	ip4_start := net.ParseIP("192.168.1.1")
	ip4_end := net.ParseIP("192.168.1.10")

	ip4_test := net.ParseIP("192.168.1.3")
	require.Equal(t, true, IpBetween(ip4_start, ip4_end, ip4_test))

	ip4_test = net.ParseIP("192.168.1.1")
	require.Equal(t, true, IpBetween(ip4_start, ip4_end, ip4_test))

	ip4_test = net.ParseIP("192.168.1.10")
	require.Equal(t, true, IpBetween(ip4_start, ip4_end, ip4_test))

	ip4_test = net.ParseIP("192.168.1.11")
	require.Equal(t, false, IpBetween(ip4_start, ip4_end, ip4_test))

	ip4_test = net.ParseIP("192.167.1.1")
	require.Equal(t, false, IpBetween(ip4_start, ip4_end, ip4_test))

	ip4_test = net.ParseIP("10.1.1.1")
	require.Equal(t, false, IpBetween(ip4_start, ip4_end, ip4_test))

	ip6_start := net.ParseIP("fe80::55de:a728:3bc6:7fae")
	ip6_end := net.ParseIP("fe80::55de:a728:3bc6:8000")

	ip6_test := net.ParseIP("fe80::55de:a728:3bc6:7fae")
	require.Equal(t, true, IpBetween(ip6_start, ip6_end, ip6_test))

	ip6_test = net.ParseIP("fe80::55de:a728:3bc6:8000")
	require.Equal(t, true, IpBetween(ip6_start, ip6_end, ip6_test))

	ip6_test = net.ParseIP("fe80::55de:a728:3bc6:8001")
	require.Equal(t, false, IpBetween(ip6_start, ip6_end, ip6_test))

	ip6_test = net.ParseIP("fe80::55de:a728:3bc6:7faa")
	require.Equal(t, false, IpBetween(ip6_start, ip6_end, ip6_test))

	require.Equal(t, false, IpBetween(ip4_start, ip6_start, ip4_test))
	require.Equal(t, false, IpBetween(ip4_start, ip6_end, ip4_test))
	require.Equal(t, false, IpBetween(ip4_end, ip6_start, ip4_test))
	require.Equal(t, false, IpBetween(ip4_end, ip6_end, ip4_test))

	require.Equal(t, false, IpBetween(ip4_start, ip6_start, ip6_test))
	require.Equal(t, false, IpBetween(ip4_start, ip6_end, ip6_test))
	require.Equal(t, false, IpBetween(ip4_end, ip6_start, ip6_test))
	require.Equal(t, false, IpBetween(ip4_end, ip6_end, ip6_test))
}
