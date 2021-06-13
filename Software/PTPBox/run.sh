#!/bin/bash

REAL_TIME=1
PPS_IN=1
TS2PHC_BC1=0

tmux new-session -d -s "PTPBox"

# first screen - GMs
tmux rename-window -t 1 "GMs"
#tmux setw synchronize-panes
tmux send-keys 'cd ~/PTPBox/BC1' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 90
tmux send-keys 'cd ~/PTPBox/BC2' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 80
tmux send-keys 'cd ~/PTPBox/BC3' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 70
tmux send-keys 'cd ~/PTPBox/BC4' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 60
tmux send-keys 'cd ~/PTPBox/BC5' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBox/BC6' C-m
tmux send-keys './GMp2.sh' C-m

# second screen - OCs
tmux new-window -n "OCs"
tmux send-keys 'cd ~/PTPBox/BC2' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 90
tmux send-keys 'cd ~/PTPBox/BC3' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 80
tmux send-keys 'cd ~/PTPBox/BC4' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 70
tmux send-keys 'cd ~/PTPBox/BC5' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 60
tmux send-keys 'cd ~/PTPBox/BC6' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBox/BC7' C-m
tmux send-keys './OCp1.sh' C-m

# third screen - PHCs
tmux new-window -n "PHCs"
tmux send-keys 'cd ~/PTPBox/BC2' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi
tmux split-window -v -p 90
tmux send-keys 'cd ~/PTPBox/BC3' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi
tmux split-window -v -p 80
tmux send-keys 'cd ~/PTPBox/BC4' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi
tmux split-window -v -p 70
tmux send-keys 'cd ~/PTPBox/BC5' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi
tmux split-window -v -p 60
tmux send-keys 'cd ~/PTPBox/BC6' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBox/BC7' C-m
if [[ $REAL_TIME == 0 ]]; then tmux send-keys './PHCp1Tp2.sh' C-m ; fi

# fourth screen - PPS input
tmux new-window -n "PPSs"
tmux send-keys 'cd ~/PTPBox/BC2' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC2 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 2' C-m ; fi
tmux split-window -v -p 90
tmux send-keys 'cd ~/PTPBox/BC3' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC3 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 3' C-m ; fi
tmux split-window -v -p 80
tmux send-keys 'cd ~/PTPBox/BC4' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC4 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 4' C-m ; fi
tmux split-window -v -p 70
tmux send-keys 'cd ~/PTPBox/BC5' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC5 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 5' C-m ; fi
tmux split-window -v -p 60
tmux send-keys 'cd ~/PTPBox/BC6' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC6 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m 
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 6' C-m ; fi
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBox/BC7' C-m
if [[ $PPS_IN == 1 ]]; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC7 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m 
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m 
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 7' C-m ; fi

tmux new-window -n "PPSHub"
tmux send-keys 'cd ~/PTPBox/BC1' C-m
if [[ $PPS_IN == 1 ]] ; then
tmux send-keys 'PHC_NUM=$(ip netns exec BC1 ~/PTPBox/tools/ptpmap | grep -m 1 ptp | cut -b 9-10)' C-m 
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -L0,1' C-m

if [[ $TS2PHC_BC1 == 1 ]]; then
tmux send-keys 'ip netns exec BC1 ~/PTPBox/tools/ts2phc -c eth1p1 -s generic -m -l 7 | grep offset' C-m
else
tmux send-keys '~/PTPBox/tools/ptptool -d $PHC_NUM -e -1 -E 127.0.0.1 -n 1' C-m
fi 
fi

tmux split-window -v -p 90
if [[ $PPS_IN == 1 ]]; then
tmux send-keys '~/PTPBox/tools/ptptool -G | tee PPS_all.log' C-m ; fi





