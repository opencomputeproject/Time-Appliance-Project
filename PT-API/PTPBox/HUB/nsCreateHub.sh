#!/bin/bash

ip netns add hub

ip netns add GM
ip link add oob1 type veth peer name hub_oob1
ip link set hub_oob1 netns hub
ip link set oob1 netns GM
ip netns exec GM  ip addr add 192.168.10.101/24 dev oob1

ip netns add TC
ip link add oob2 type veth peer name hub_oob2
ip link set hub_oob2 netns hub
ip link set oob2 netns TC
ip netns exec TC  ip addr add 192.168.10.102/24 dev oob2

ip netns add OC1
ip link add oob3 type veth peer name hub_oob3
ip link set hub_oob3 netns hub
ip link set oob3 netns OC1
ip netns exec OC1 ip addr add 192.168.10.103/24 dev oob3

ip netns add BC
ip link add oob4 type veth peer name hub_oob4
ip link set hub_oob4 netns hub
ip link set oob4 netns BC
ip netns exec BC  ip addr add 192.168.10.104/24 dev oob4

ip netns add OC2
ip link add oob5 type veth peer name hub_oob5
ip link set hub_oob5 netns hub
ip link set oob5 netns OC2
ip netns exec OC2 ip addr add 192.168.10.105/24 dev oob5

ip netns exec hub ip link add name NUC_hub type bridge
ip netns exec hub ip link set NUC_hub up
ip netns exec hub ip link set hub_oob1 up
ip netns exec hub ip link set hub_oob2 up
ip netns exec hub ip link set hub_oob3 up
ip netns exec hub ip link set hub_oob4 up
ip netns exec hub ip link set hub_oob5 up

ip netns exec GM  ip link set oob1 up
ip netns exec TC  ip link set oob2 up
ip netns exec OC1 ip link set oob3 up
ip netns exec BC  ip link set oob4 up
ip netns exec OC2 ip link set oob5 up

ip netns exec hub ip link set hub_oob1 master NUC_hub
ip netns exec hub ip link set hub_oob2 master NUC_hub
ip netns exec hub ip link set hub_oob3 master NUC_hub
ip netns exec hub ip link set hub_oob4 master NUC_hub
ip netns exec hub ip link set hub_oob5 master NUC_hub

ip netns exec hub bridge link show NUC_hub

ip netns exec hub ip addr add 192.168.10.1/24 brd + dev NUC_hub

ip netns list














