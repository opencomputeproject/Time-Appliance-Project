#!/bin/bash
export PTPCLOCK=/dev/ptp8
ip netns exec GM ./clockTest2 192.168.10.105 192.168.10.107 192.168.10.109 192.168.10.111 | tee GM.log
