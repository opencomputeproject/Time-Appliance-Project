#!/bin/bash

TC0_NIC_dev=eth20
TC1_NIC_dev=eth21
TC_ns=TC
TC0_IP_ADDR=192.168.2.105/24
TC1_IP_ADDR=192.168.1.104/24

ip netns add $TC_ns

ip link set dev $TC0_NIC_dev netns $TC_ns
ip link set dev $TC1_NIC_dev netns $TC_ns

ip netns exec $TC_ns ip link set lo up

ip netns exec $TC_ns ip addr add $TC0_IP_ADDR dev $TC0_NIC_dev
ip netns exec $TC_ns ip link set $TC0_NIC_dev up
ip netns exec $TC_ns ip addr add $TC1_IP_ADDR dev $TC1_NIC_dev
ip netns exec $TC_ns ip link set $TC1_NIC_dev up

ip netns exec $TC_ns phc2sys -w -m  -s $TC0_NIC_dev -c $TC1_NIC_dev

