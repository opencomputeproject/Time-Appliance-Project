#!/bin/bash
export PTPCLOCK=/dev/ptp0
ip netns exec OC1 ./clockTest2 192.168.1.104 192.168.5.112 | tee OC1-100G.log
