#!/bin/bash
export PTPCLOCK=/dev/ptp1
ip netns exec TC ./clockTest2 192.168.2.100 192.168.1.107 192.168.10.100 192.168.10.107 192.168.10.109 192.168.10.111 | tee TC-all.log
