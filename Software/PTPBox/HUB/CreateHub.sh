#!/bin/bash

source ../configs

number_nodes=7

ip netns add hub
ip netns exec hub ip link add name NUC_hub type bridge
ip netns exec hub ip link set NUC_hub up

for ((node_number = 1 ; node_number <= $number_nodes ; node_number++)); do
   ip netns add ${node[$node_number,0]}
   ip link add oob$node_number type veth peer name hub_oob$node_number
   ip link set hub_oob$node_number netns hub
   ip link set oob$node_number netns ${node[$node_number,0]}
   ip netns exec ${node[$node_number,0]} ip addr add 192.168.10.10$node_number/24 dev oob$node_number
   ip netns exec hub ip link set hub_oob$node_number up
   ip netns exec ${node[$node_number,0]}  ip link set oob$node_number up
   ip netns exec hub ip link set hub_oob$node_number master NUC_hub
done

ip netns exec hub bridge link show NUC_hub

ip netns exec hub ip addr add 192.168.10.100/24 brd + dev NUC_hub














