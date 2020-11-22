#!/bin/bash

OC2_0_NIC_dev=eth50
OC2_1_NIC_dev=eth51
OC2_ns=OC2
OC2_0_IP_ADDR=192.168.4.111/24
OC2_1_IP_ADDR=192.168.5.112/24

ip netns add $OC2_ns

ip link set dev $OC2_0_NIC_dev netns $OC2_ns
ip link set dev $OC2_1_NIC_dev netns $OC2_ns

ip netns exec $OC2_ns ip link set lo up

ip netns exec $OC2_ns ip addr add $OC2_0_IP_ADDR dev $OC2_0_NIC_dev
ip netns exec $OC2_ns ip link set $OC2_0_NIC_dev up
ip netns exec $OC2_ns ip addr add $OC2_1_IP_ADDR dev $OC2_1_NIC_dev
ip netns exec $OC2_ns ip link set $OC2_1_NIC_dev up

ip netns exec $OC2_ns phc2sys -w -m  -s $OC2_0_NIC_dev -c $OC2_1_NIC_dev


