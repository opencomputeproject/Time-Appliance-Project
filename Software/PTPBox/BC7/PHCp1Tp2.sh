#!/bin/bash

source ../configs

source_port=1
destination_port=2

#gets extrated based on the number in the folder (BCx->x)
node_number=$(basename "$PWD"| tr -dc '0-9')

ip netns add ${node[$node_number,0]}

ip link set dev ${node[$node_number,1]} netns ${node[$node_number,0]}
ip link set dev ${node[$node_number,3]} netns ${node[$node_number,0]}

ip netns exec ${node[$node_number,0]} ip link set lo up

ip netns exec ${node[$node_number,0]} ip addr add ${node[$node_number,2]} dev ${node[$node_number,1]}
ip netns exec ${node[$node_number,0]} ip link set ${node[$node_number,1]} up
ip netns exec ${node[$node_number,0]} ip addr add ${node[$node_number,4]} dev ${node[$node_number,3]}
ip netns exec ${node[$node_number,0]} ip link set ${node[$node_number,3]} up

ip netns exec ${node[$node_number,0]} phc2sys -O 0 -m -s ${node[$node_number,$(($(($(($source_port-1))*2))+1))]} -c ${node[$node_number,$(($(($(($destination_port-1))*2))+1))]} | tee $(basename "$PWD")-PHC$source_port\to$destination_port.log
