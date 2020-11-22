#!/bin/bash

GM_dev0=enp3s0f0
GM_dev0_new=eth10
GM_dev1=enp3s0f1
GM_dev1_new=eth11

TC_dev0=enp9s0f0
TC_dev0_new=eth20
TC_dev1=enp9s0f1
TC_dev1_new=eth21

OC1_dev0=enp10s0f0
OC1_dev0_new=eth30
OC1_dev1=enp10s0f1
OC1_dev1_new=eth31

BC_dev0=enp7s0f0
BC_dev0_new=eth40
BC_dev1=enp7s0f1
BC_dev1_new=eth41

OC2_dev0=enp8s0f0
OC2_dev0_new=eth50
OC2_dev1=enp8s0f1
OC2_dev1_new=eth51

ip link set $GM_dev0  down
ip link set $GM_dev1  down
ip link set $TC_dev0  down
ip link set $TC_dev1  down
ip link set $OC1_dev0 down
ip link set $OC1_dev1 down
ip link set $BC_dev0  down
ip link set $BC_dev1  down
ip link set $OC2_dev0 down
ip link set $OC2_dev1 down

ip link set $GM_dev0  name $GM_dev0_new
ip link set $GM_dev1  name $GM_dev1_new
ip link set $TC_dev0  name $TC_dev0_new
ip link set $TC_dev1  name $TC_dev1_new
ip link set $OC1_dev0 name $OC1_dev0_new
ip link set $OC1_dev1 name $OC1_dev1_new
ip link set $BC_dev0  name $BC_dev0_new
ip link set $BC_dev1  name $BC_dev1_new
ip link set $OC2_dev0 name $OC2_dev0_new
ip link set $OC2_dev1 name $OC2_dev1_new

ip link set $GM_dev0_new  up
ip link set $GM_dev1_new  up
ip link set $TC_dev0_new  up
ip link set $TC_dev1_new  up
ip link set $OC1_dev0_new up
ip link set $OC1_dev1_new up
ip link set $BC_dev0_new  up
ip link set $BC_dev1_new  up
ip link set $OC2_dev0_new up
ip link set $OC2_dev1_new up

ip link list

