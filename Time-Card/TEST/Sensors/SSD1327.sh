#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the SSD1327 OLED module over the i2c bus

I2CBUS=2
DEVADDR=0x3D

declare -A frameBuffer

declare -A tempBuff

function display_off() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0x00 # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAE # Display OFF (sleep mode)
sleep 0.1
}

function init_display() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xFD 0x01 0x12 i # Unlock
i2cset -y $I2CBUS $DEVADDR 0x00 0xAE 0x00 i  # Display off
i2cset -y $I2CBUS $DEVADDR 0x00 0x15 0x00 0x3F i  # Set Column address
i2cset -y $I2CBUS $DEVADDR 0x00 0x75 0x00 0x7F i  # Set Row address
i2cset -y $I2CBUS $DEVADDR 0x00 0xA1 0x00  i  # Set Start line
i2cset -y $I2CBUS $DEVADDR 0x00 0xA2 0x00 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xA0 0x14 0x11 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xA8 0x7F i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB 0x01 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xB1 0xE2 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xB3 0x91 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xBC 0x08 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xBE 0x07 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xB6 0x01 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xD5 0x62 i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xb8 0x0f 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x10 0x18 0x20 0x2f 0x38 0x3f i  
i2cset -y $I2CBUS $DEVADDR 0x00 0xB9  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0x81 0x7F i  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xA4 # 
i2cset -y $I2CBUS $DEVADDR 0x00 0x2E  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xAF  # 
i2cset -y $I2CBUS $DEVADDR 0x00 0xCA 0x3F i  # S
i2cset -y $I2CBUS $DEVADDR 0x00 0xA0 0x51 0x42 i  # 

}

function display_on() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB  # Display ON (normal mode)
i2cset -y $I2CBUS $DEVADDR 0x00 0x01  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAF  # Set Display offset

sleep 0.001
}

function reset_cursor() {
   i2cset -y $I2CBUS $DEVADDR 0x00 0x15 0x00 0x3F 0x75 0x00 0x7F i 
}

function set_cursor() {
	i2cset -y $I2CBUS $DEVADDR 0x00 0x15 $(( ${1} >> 1 ))  0x3F 0x75 ${2} 0x7F i 
}

function set_WriteZone() {
	i2cset -y $I2CBUS $DEVADDR 0x00 0x15 ${1} ${2} 0x75 ${3} ${4} i 
}

function frameToOLED(){
	echo $(( $(( {$1}/2 )) + $(( {$2}*64 )) ))
}


function ord() {
  # Get ASCII Value from Character
  local chardec=$(LC_CTYPE=C printf '%d' "'$1")
  [ "${chardec}" -eq 0 ] && chardec=32             # Manual Mod for " " (Space)
  echo ${chardec}
  #printf '%d' "'$1"
  #LC_CTYPE=C printf '%d' "'$1"
}

function showtext() {
  local a=0; local b=0; local achar=0; local charp=0; local charout="";
  local text=${1}
  local textlen=${#text}
  #echo "Textlen: ${textlen}"
  for (( a=0; a<${textlen}; a++ )); do
    achar="`ord ${text:${a}:1}`"               # get the ASCII Code
    let charp=(achar-32)*${font_width}         # calculate first byte in font array
    charout=""
    for (( b=0; b<${font_width}; b++ )); do    # character loop
      charout="${charout} ${font[charp+b]}"    # build character out of single values
    done
    # echo "${a}: ${text:${a}:1} -> ${achar} -> ${charp} -> ${charout}"
  i2cset -y $I2CBUS $DEVADDR 0x40 ${charout} i                      # send character bytes to display
  done
}

function loadBuffer(){
i2cset -y $I2CBUS $DEVADDR 0x00 0x15 0x00 0x3F 0x75 0x00 0x7F i 
for ((i=0;i<256;i++)) do
	for ((j=0;j<32;j++)) do
		local pointer=$(( $j+(($i<<5)) ))
		tempBuff[$j]=${frameBuffer[$pointer]}
	done
   i2cset -y $I2CBUS $DEVADDR 0x40 ${tempBuff[0]} ${tempBuff[1]} ${tempBuff[2]} ${tempBuff[3]} ${tempBuff[4]} ${tempBuff[5]} ${tempBuff[6]} ${tempBuff[7]} ${tempBuff[8]} ${tempBuff[9]} ${tempBuff[10]} ${tempBuff[11]} ${tempBuff[12]} ${tempBuff[13]} ${tempBuff[14]} ${tempBuff[15]} ${tempBuff[16]} ${tempBuff[17]} ${tempBuff[18]} ${tempBuff[19]} ${tempBuff[20]} ${tempBuff[21]} ${tempBuff[22]} ${tempBuff[23]} ${tempBuff[24]} ${tempBuff[25]} ${tempBuff[26]} ${tempBuff[27]} ${tempBuff[28]} ${tempBuff[29]} ${tempBuff[30]} ${tempBuff[31]} i

done
}

function blankBuffer(){
for ((i=0;i<64;i++)) do
    for ((j=0;j<128;j++)) do
	local pointer=$(( $i+(($j<<6)) ))
        frameBuffer[$pointer]=0x00
    done
done
}


function fillBuffer(){
for ((i=0;i<64;i++)) do
    for ((j=0;j<128;j++)) do
	local pointer=$(( $i+(($j<<6)) ))
        frameBuffer[$pointer]=0xFF
    done
done
}

function drawPixel(){
local pix_addr=$(( $(( $1 >> 1 )) + $(( $2 << 6 )) ))
if [ $(( $1 % 2 )) -eq 0 ]; then
frameBuffer[$pix_addr]=$(( $(( ${frameBuffer[$pix_addr]} & 0x0F )) | $(( $3 << 4 )) ))
else
frameBuffer[$pix_addr]=$(( $(( ${frameBuffer[$pix_addr]} & 0xF0 )) | $3 ))
fi
}

function drawRect(){

local xMax=$(( $1 > $3 ? $1 : $3 ))
local xMin=$(( $1 > $3 ? $3 : $1 ))
local yMax=$(( $2 > $4 ? $2 : $4 ))
local yMin=$(( $2 > $4 ? $4 : $2 ))

for ((i=yMin;i<yMax;i++)) do
    for ((j=xMin;j<xMax;j++)) do
	drawPixel $i $j $5
    done
done


}


display_off
init_display
display_on

blankBuffer

drawRect 10 10 118 118 10

drawRect 20 20 108 108 8

drawRect 30 30 98 98 6

drawRect 40 40 88 88 4

drawRect 50 50 78 78 2

drawRect 60 60 68 68 1


loadBuffer





