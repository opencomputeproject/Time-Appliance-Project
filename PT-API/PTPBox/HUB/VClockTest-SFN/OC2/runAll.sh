#!/bin/bash
export PTPCLOCK=/dev/ptp3
ip netns exec OC2 ./clockTest2 192.168.5.108 192.168.4.108 192.168.10.105 192.168.10.107 192.168.10.109 192.168.10.100 | tee OC2-all.log
