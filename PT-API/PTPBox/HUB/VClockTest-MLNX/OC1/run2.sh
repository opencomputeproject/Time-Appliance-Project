#!/bin/bash
export PTPCLOCK=/dev/ptp0
ip netns exec OC1 ./clockTest2 192.168.10.105 192.168.10.100 192.168.10.109 192.168.10.111 | tee OC1.log
