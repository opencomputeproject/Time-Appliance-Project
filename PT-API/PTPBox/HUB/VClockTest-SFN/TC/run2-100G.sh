#!/bin/bash
export PTPCLOCK=/dev/ptp2
ip netns exec TC ./clockTest2 192.168.2.100 192.168.1.107 | tee TC-100G.log
