#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the TSL2575 chip over the i2c bus

I2CBUS=10
DEVADDR=0x39

TSL2572_GAIN_1X=1
TSL2572_GAIN_8X=8
TSL2572_GAIN_16X=16
TSL2572_GAIN_120X=120

TSL2572_GAIN=1

TSL2572_CMDS_ENABLE=0x80
TSL2572_CMDS_ALS_TIMING=0x81
TSL2572_CMDS_WAIT_TIME=0x83
TSL2572_CMDS_PERSISTANCE=0x8c
TSL2572_CMDS_CONFIG=0x8d
TSL2572_CMDS_CONTROL=0x8f
TSL2572_CMDS_WHOAMI=0x92
TSL2572_CMDS_STATUS=0x93

#initialize
function init_light(){
i2cget -y $I2CBUS $DEVADDR $TSL2572_CMDS_WHOAMI
}

#enable sensor
function enable_light(){
TSL2572_GAIN=$1
if [ "$1" -eq "$TSL2572_GAIN_1X" ];then
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_CONTROL 0x00
fi
if [ "$1" -eq "$TSL2572_GAIN_8X" ];then
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_CONTROL 0x01
fi
if [ "$1" -eq "$TSL2572_GAIN_16X" ];then
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_CONTROL 0x10
fi
if [ "$1" -eq "$TSL2572_GAIN_120X" ];then
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_CONTROL 0x11
fi
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_ALS_TIMING 0xed #51.87ms
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_ENABLE 0x03
if [ "$1" -eq "$TSL2572_GAIN_1X" ] || [ "$1" -eq "$TSL2572_GAIN_8X" ]; then
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_CONFIG 0x04
fi
}

function disable_lightsensor(){
i2cset -y $I2CBUS $DEVADDR $TSL2572_CMDS_ENABLE 0
}

function light(){

i2cset -y $I2CBUS $DEVADDR 0xb4 0x1
local c0l=$(($(i2cget -y $I2CBUS $DEVADDR 0x14 w)))
i2cset -y $I2CBUS $DEVADDR 0x00 0x1
local c0h=$(($(i2cget -y $I2CBUS $DEVADDR 0x15 w)))
i2cset -y $I2CBUS $DEVADDR 0x00 0x1
local c1l=$(($(i2cget -y $I2CBUS $DEVADDR 0x16 w)))
i2cset -y $I2CBUS $DEVADDR 0x00 0x1
local c1h=$(($(i2cget -y $I2CBUS $DEVADDR 0x17 w)))

local c0=$(($(($(($c0h<<8))|$c0l))))
local c1=$(($(($(($c1h<<8))|$c1l))))

local cpl=$(($(($((5187*$TSL2572_GAIN))/60))))

if [ "$TSL2572_GAIN" -eq "$TSL2572_GAIN_1X" ] || [ "$TSL2572_GAIN" -eq "$TSL2572_GAIN_8X" ]; then
cpl=$(($cpl/6))
fi
lux1=$((-1*$(($(($c0*100))-$((187 *$c1))))))
lux2=$((-1*$(($((63*$c0))-$((c1*100))))))
val=$lux1
if [ "$lux2" -gt "$val" ]; then
val=$lux2
fi
if [ "0" -gt "$val" ]; then
val=0
fi

printf "Light = %.3f Lux\n" $(echo $val / $cpl | bc -l)

}


enable_light 120
light
