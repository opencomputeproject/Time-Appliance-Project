#!/bin/bash
export PTPCLOCK=/dev/ptp2
ip netns exec BC ./clockTest2 192.168.10.105 192.168.10.107 192.168.10.100 192.168.10.111 | tee BC.log
