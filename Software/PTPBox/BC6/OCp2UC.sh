#!/bin/bash

source ../configs

#port number (1 or 2)
port_number=2
#port role (master or ordinary)
role='ordinary'

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

if [ "$role" == "ordinary" ]; then
        echo "Ordinary role"	

	echo '[global]' > ptp4l.conf
	echo 'unicast_req_duration 60' >> ptp4l.conf
	echo '[unicast_master_table]' >> ptp4l.conf
	echo 'table_id 1' >> ptp4l.conf
	echo "UDPv4 ${node[$(($node_number-1)),2]%/24}" >> ptp4l.conf
	echo "[${node[$node_number,$(($(($(($port_number-1))*2))+1))]}]" >> ptp4l.conf
	echo 'unicast_master_table 1' >> ptp4l.conf


	ip netns exec ${node[$node_number,0]} ptp4l -i ${node[$node_number,$(($(($(($port_number-1))*2))+1))]} --slaveOnly 1 -f ptp4l.conf -m | tee $(basename "$PWD")-OC-$port_number.log
else
	echo "Master role"
	ip netns exec ${node[$node_number,0]} ptp4l -i ${node[$node_number,$(($(($(($port_number-1))*2))+1))]} -m | tee $(basename "$PWD")-GM-$port_number.log
fi
