#!/bin/bash

if [ $(ip netns | wc -l) -ne 0 ]; then
  ip netns delete n0
  ip netns delete n1
fi

D0_0=enp3s0f0

D0_1=enp3s0f1
D1_0=enp9s0f0

D1_1=enp9s0f1
D2_0=enp10s0f0

D2_1=enp10s0f1
D3_0=enp7s0f0

D3_1=enp7s0f1
D4_1=enp8s0f0

D4_1=enp8s0f1

#ip link show

ip netns add n0
ip link set dev $D0_0 netns n0
ip link set dev $D0_1 netns n0
ip netns exec n0 ip addr add 192.168.0.100/24 dev $D0_0
ip netns exec n0 ip addr add 192.168.1.101/24 dev $D0_1
ip netns exec n0 ip link set lo up
ip netns exec n0 ip link set $D0_1 up

ip netns add n1
ip link set dev $D1_0 netns n1
ip link set dev $D1_1 netns n1
ip netns exec n1 ip addr add 192.168.1.102/24 dev $D1_0
ip netns exec n1 ip addr add 192.168.2.101/24 dev $D1_1
ip netns exec n1 ip link set lo up
ip netns exec n1 ip link set $D1_0 up

exit

# Setup GM
ip netns exec n0 ptp4l -i $D0_1 -f GM-1.cfg -m

# Setup Sys to GM
ip netns exec n0 phc2sys -q -w -m -s CLOCK_REALTIME -c $D0_1

# Setup OC
ip netns exec n1 ptp4l -i $D1_0 -f OC-0.cfg -s -m -A 

exit 


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

