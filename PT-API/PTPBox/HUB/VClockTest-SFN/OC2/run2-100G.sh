#!/bin/bash
export PTPCLOCK=/dev/ptp6
ip netns exec OC2 ./clockTest2 192.168.5.108 192.168.4.108 | tee OC2-100G.log
