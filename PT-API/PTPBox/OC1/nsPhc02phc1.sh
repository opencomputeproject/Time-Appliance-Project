#!/bin/bash

OC1_0_NIC_dev=eth30
OC1_1_NIC_dev=eth31
OC1_ns=OC1
OC1_0_IP_ADDR=192.168.1.107/24
OC1_1_IP_ADDR=192.168.5.108/24

ip netns add $OC1_ns

ip link set dev $OC1_0_NIC_dev netns $OC1_ns
ip link set dev $OC1_1_NIC_dev netns $OC1_ns

ip netns exec $OC1_ns ip link set lo up

ip netns exec $OC1_ns ip addr add $OC1_0_IP_ADDR dev $OC1_0_NIC_dev
ip netns exec $OC1_ns ip link set $OC1_0_NIC_dev up
ip netns exec $OC1_ns ip addr add $OC1_1_IP_ADDR dev $OC1_1_NIC_dev
ip netns exec $OC1_ns ip link set $OC1_1_NIC_dev up

ip netns exec $OC1_ns phc2sys -w -m  -s $OC1_0_NIC_dev -c $OC1_1_NIC_dev


