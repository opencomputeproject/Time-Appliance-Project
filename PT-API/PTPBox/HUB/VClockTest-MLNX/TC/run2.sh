#!/bin/bash
export PTPCLOCK=/dev/ptp2
ip netns exec TC ./clockTest2 192.168.10.100 192.168.10.107 192.168.10.109 192.168.10.111 | tee TC.log
