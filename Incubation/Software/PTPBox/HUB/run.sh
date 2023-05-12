#!/bin/bash

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


