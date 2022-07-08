#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the AD7414 chip over the i2c bus

I2CBUS=2
DEVADDR=0x4f

AD7414_VAL=0x00
AD7414_CONF=0x01
AD7414_T_HIGH=0x02
AD7414_T_LOW=0x03

AD7414_DEF_CONF=0x40

#set default configuration
function init_default_config(){
i2cset -y $I2CBUS $DEVADDR $AD7414_CONF $AD7414_DEF_CONF
}

#read temperature value register 
function read_temperature(){
local raw_val=$(($(i2cget -y $I2CBUS $DEVADDR $AD7414_VAL w)))
local lo_val=$(("$raw_val">>14))
local hi_val=$(("$raw_val"&0xff))
local code_val=$(("$lo_val"|$(("$hi_val"<<2))))
if [ $(( "$hi_val" & 0x80)) -ne 0 ]; then
	code_val=$(($code_val - 512))
fi
printf "Temperature = %.2f C\n" $(echo $code_val / 4 | bc -l)
}


init_default_config
read_temperature
