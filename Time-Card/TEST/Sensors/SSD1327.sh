#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the SSD1327 OLED module over the i2c bus

I2CBUS=2
DEVADDR=0x3D

function display_off() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0x00 # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAE # Display OFF (sleep mode)
sleep 0.1
}

function init_display() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xFD 0x12 i # Unlock
i2cset -y $I2CBUS $DEVADDR 0x00 0xAE  # Display off
i2cset -y $I2CBUS $DEVADDR 0x00 0x15 0x00 0x3F i  # Set Column address
i2cset -y $I2CBUS $DEVADDR 0x00 0x75 0x00 0x7F i  # Set Row address
i2cset -y $I2CBUS $DEVADDR 0x00 0xA1 0x00  i  # Set Start line
i2cset -y $I2CBUS $DEVADDR 0x00 0xA2 0x00 i  # Set Display offset



i2cset -y $I2CBUS $DEVADDR 0x00 0xA0 0x51 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xA8 0x7F i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB 0x01 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xB1 0x51 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xB3 0x01 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xBC 0x08 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xBE 0x07 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xB6 0x01 i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xD5 0x62 i  # Set Display offset

i2cset -y $I2CBUS $DEVADDR 0x00 0xB9  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0x81 0x7F i  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xA4 # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0x2E  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAF  # Set Display offset

}

function display_on() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAB  # Display ON (normal mode)
i2cset -y $I2CBUS $DEVADDR 0x00 0x01  # Set Display offset
i2cset -y $I2CBUS $DEVADDR 0x00 0xAF  # Set Display offset

sleep 0.001
}

function reset_cursor() {
i2cset -y $I2CBUS $DEVADDR 0x00 0x15  # set column address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set start address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set end address (127 max)
i2cset -y $I2CBUS $DEVADDR 0x00 0x75  # set page address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set start address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set end address (7 max)
}

display_off
init_display
display_on
reset_cursor

# fill screen
for i in $(seq 8192)
do
   i2cset -y $I2CBUS $DEVADDR 0x40 0xff
done

reset_cursor

# clear screen
for i in $(seq 8192)
do
   i2cset -y $I2CBUS $DEVADDR 0x40 0x0
done

reset_cursor

# draw a pattern
for i in $(seq 1)
do
    for i in 1 4 16 64 16 4 1
    do
        i2cset -y $I2CBUS $DEVADDR 0x40 $i
    done
done
