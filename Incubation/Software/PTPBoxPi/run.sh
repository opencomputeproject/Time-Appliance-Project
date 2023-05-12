#!/bin/bash

tmux new-session -d -s "PTPBox"

# first screen - OCs
tmux rename-window -t 1 "OCs"
tmux send-keys 'cd ~/PTPBoxPi/BC2' C-m
tmux send-keys './OCp1.sh' C-m
tmux split-window -v -p 70
tmux send-keys 'cd ~/PTPBoxPi/BC2' C-m
tmux send-keys './PHCp1Tp2.sh' C-m 
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBoxPi/BC3' C-m
tmux send-keys './OCp1.sh' C-m

# first screen - GMs
tmux new-window -n "GMs"
tmux send-keys 'cd ~/PTPBoxPi/BC1' C-m
tmux send-keys './GMp2.sh' C-m
tmux split-window -v -p 50
tmux send-keys 'cd ~/PTPBoxPi/BC2' C-m
tmux send-keys './GMp2.sh' C-m

tmux select-window -t "OCs"

