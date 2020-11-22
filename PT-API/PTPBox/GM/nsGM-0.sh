#!/bin/bash

GM0_NIC_dev=eth10
GM1_NIC_dev=eth11
GM_ns=GM
GM0_IP_ADDR=192.168.2.100/24
GM1_IP_ADDR=192.168.3.101/24

ip netns add $GM_ns

ip link set dev $GM0_NIC_dev netns $GM_ns
ip link set dev $GM1_NIC_dev netns $GM_ns

ip netns exec $GM_ns ip link set lo up

ip netns exec $GM_ns ip addr add $GM0_IP_ADDR dev $GM0_NIC_dev
ip netns exec $GM_ns ip link set $GM0_NIC_dev up
ip netns exec $GM_ns ip addr add $GM1_IP_ADDR dev $GM1_NIC_dev
ip netns exec $GM_ns ip link set $GM1_NIC_dev up


ip netns exec $GM_ns ptp4l -i $GM0_NIC_dev -f GM-0.cfg -m | tee GM-0.log

