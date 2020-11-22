#!/bin/bash
export PTPCLOCK=/dev/ptp4
ip netns exec BC ./clockTest2 192.168.3.101 192.168.4.111 192.168.10.105 192.168.10.107 192.168.10.100 192.168.10.111 | tee BC-all.log
