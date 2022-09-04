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
local pix_addr=$(( $(( $2 >> 1 )) + $(( $1 << 6 )) ))
local pix_val=0
local input=$(($3 & 0xFF))
if [ $(( $2 % 2 )) -eq 0 ]; then
  pix_val=$(( $(( ${frameBuffer[$pix_addr]} & 0x0F )) | $(( $input << 4 )) ))
frameBuffer[$pix_addr]=$pix_val
else
  pix_val=$(( $(( ${frameBuffer[$pix_addr]} & 0xF0 )) | $input ))
frameBuffer[$pix_addr]=$pix_val
fi
if [ $4 -eq 1 ]; then
set_cursor $2 $1
i2cset -y $I2CBUS $DEVADDR 0x40 $pix_val i
fi
}

function drawRect(){

local xMax=$(( $1 > $3 ? $1 : $3 ))
local xMin=$(( $1 > $3 ? $3 : $1 ))
local yMax=$(( $2 > $4 ? $2 : $4 ))
local yMin=$(( $2 > $4 ? $4 : $2 ))

for ((j=yMin;j<yMax;j++)) do
    for ((i=xMin;i<xMax;i++)) do
	     drawPixel $i $j $5 $6
    done
done
}

function drawLine(){

local xMax=$(( $1 > $3 ? $1 : $3 ))
local xMin=$(( $1 > $3 ? $3 : $1 ))
local yMax=$(( $2 > $4 ? $2 : $4 ))
local yMin=$(( $2 > $4 ? $4 : $2 ))

local xDelta=$(( $xMax - $xMin ))
local yDelta=$(( $yMax - $yMin ))

local xSign=$(( $1 > $3 ? -1 : 1 ))
local ySign=$(( $2 > $4 ? -1 : 1 ))

if [ $xDelta -gt $yDelta ];then
  for ((t=0;t<xDelta;t++)) do
     if [ $xDelta -ne 0 ]; then 
        drawPixel $(( $1 + $(( $t * $xSign )) )) $(( $2 + $(( $(( $(( $(( $t * $ySign )) *  $yDelta )) / $xDelta  )) )) )) $5 $6
     else
        drawPixel $(( $1 + $(( $t * $xSign )) )) $2 $5 $6
     fi   
  done
else
  for ((t=0;t<yDelta;t++)) do
     if [ $yDelta -ne 0 ]; then 
        drawPixel $(( $1 + $(( $(( $(( $(( $t * $xSign )) *  $xDelta )) / $yDelta  )) )) )) $(( $2 + $(( $t * $ySign )) )) $5 $6
     else
        drawPixel $1 $(( $2 + $(( $t * $ySign )) )) $5 $6
     fi
  done
fi
}


display_off
init_display
display_on

blankBuffer
loadBuffer

#drawRect 10 10 20 100 10 1

drawLine 10 10 100 100 10 1
drawLine 10 10 100 95 10 1
drawLine 10 10 100 90 10 1
drawLine 10 10 100 10 10 1
drawLine 10 10 10 100 10 1
drawLine 100 10 10 100 10 1
drawLine 10 10 95 100 10 1
drawLine 10 10 90 100 10 1


