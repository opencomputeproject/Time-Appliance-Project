#!/bin/bash
export PTPCLOCK=/dev/ptp4
ip netns exec BC ./clockTest2 192.168.3.101 192.168.4.111 | tee BC-100G.log
