#!/bin/bash

ip netns exec GM  ip route add 192.168.1.0/24 via 192.168.2.105
ip netns exec GM  ip route add 192.168.5.0/24 via 192.168.2.105
ip netns exec GM  ip route add 192.168.4.0/24 via 192.168.2.105

ip netns exec TC  ip route add 192.168.5.0/24 via 192.168.1.107
ip netns exec TC  ip route add 192.168.4.0/24 via 192.168.1.107

ip netns exec OC1 ip route add 192.168.4.0/24 via 192.168.5.112

ip netns exec BC  ip route add 192.168.5.0/24 via 192.168.4.111
ip netns exec BC  ip route add 192.168.1.0/24 via 192.168.4.111
ip netns exec BC  ip route add 192.168.2.0/24 via 192.168.4.111

ip netns exec OC2 ip route add 192.168.1.0/24 via 192.168.5.108
ip netns exec OC2 ip route add 192.168.2.0/24 via 192.168.5.108

ip netns exec OC1 ip route add 192.168.2.0/24 via 192.168.1.104
