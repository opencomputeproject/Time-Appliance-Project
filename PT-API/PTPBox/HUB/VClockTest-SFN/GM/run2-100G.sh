#!/bin/bash
export PTPCLOCK=/dev/ptp8
ip netns exec GM ./clockTest2 192.168.2.105 192.168.3.109 | tee GM-100G.log
