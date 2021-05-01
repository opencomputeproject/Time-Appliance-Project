#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the SSD1306 OLED module over the i2c bus

I2CBUS=10
DEVADDR=0x3C

function display_off() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAE  # Display OFF (sleep mode)
sleep 0.1
}

function init_display() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xA8  # Set Multiplex Ratio
i2cset -y $I2CBUS $DEVADDR 0x00 0x3F  # value
i2cset -y $I2CBUS $DEVADDR 0x00 0xD3  # Set Display Offset
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  # no vertical shift
i2cset -y $I2CBUS $DEVADDR 0x00 0x40  # Set Display Start Line to 000000b
i2cset -y $I2CBUS $DEVADDR 0x00 0xA1  # Set Segment Re-map, column address 127 ismapped to SEG0
i2cset -y $I2CBUS $DEVADDR 0x00 0xC8  # Set COM Output Scan Direction, remapped mode. Scan from COM7 to COM0
#i2cset -y $I2CBUS $DEVADDR 0x00 0xC0  # Set COM Output Scan Direction, remapped mode. Scan from COM7 to COM0
i2cset -y $I2CBUS $DEVADDR 0x00 0xDA  # Set COM Pins Hardware Configuration
#i2cset -y $I2CBUS $DEVADDR 0x00 0x12  # Alternative COM pin configuration, Disable COM Left/Right remap
#i2cset -y $I2CBUS $DEVADDR 0x00 0x02  # Sequential COM pin configuration,  Disable COM Left/Right remap
#i2cset -y $I2CBUS $DEVADDR 0x00 0x22  # Sequential COM pin configuration,  Enable Left/Right remap  (8pixels height)
i2cset -y $I2CBUS $DEVADDR 0x00 0x32  # Alternative COM pin configuration, Enable Left/Right remap   (4pixels height)
#i2cset -y $I2CBUS $DEVADDR 0x00 0x81  # Set Contrast Control
#i2cset -y $I2CBUS $DEVADDR 0x00 0xCF  # value, 0x7F max.
i2cset -y $I2CBUS $DEVADDR 0x00 0xA4  # display RAM content
i2cset -y $I2CBUS $DEVADDR 0x00 0xA6  # non-inverting display mode - black dots on white background
i2cset -y $I2CBUS $DEVADDR 0x00 0xD5  # Set Display Clock (Divide Ratio/Oscillator Frequency)
i2cset -y $I2CBUS $DEVADDR 0x00 0x80  # max fequency, no divide ratio
i2cset -y $I2CBUS $DEVADDR 0x00 0x8D  # Charge Pump Setting
i2cset -y $I2CBUS $DEVADDR 0x00 0x14  # enable charge pump
i2cset -y $I2CBUS $DEVADDR 0x00 0x20  # page addressing mode
i2cset -y $I2CBUS $DEVADDR 0x00 0x20  # horizontal addressing mode
#i2cset -y $I2CBUS $DEVADDR 0x00 0x21  # vertical addressing mode
#i2cset -y $I2CBUS $DEVADDR 0x00 0x22  # page addressing mode
}

function display_on() {
i2cset -y $I2CBUS $DEVADDR 0x00 0xAF  # Display ON (normal mode)
sleep 0.001
}

function reset_cursor() {
i2cset -y $I2CBUS $DEVADDR 0x00 0x21  # set column address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set start address
i2cset -y $I2CBUS $DEVADDR 0x00 0x7F  #   set end address (127 max)
i2cset -y $I2CBUS $DEVADDR 0x00 0x22  # set page address
i2cset -y $I2CBUS $DEVADDR 0x00 0x00  #   set start address
i2cset -y $I2CBUS $DEVADDR 0x00 0x07  #   set end address (7 max)
}

display_off
init_display
display_on
reset_cursor

# fill screen
for i in $(seq 1024)
do
   i2cset -y $I2CBUS $DEVADDR 0x40 0xff
done

reset_cursor

# clear screen
for i in $(seq 1024)
do
   i2cset -y $I2CBUS $DEVADDR 0x40 0x0
done

reset_cursor

# draw a pattern
for i in $(seq 146)
do
    for i in 1 4 16 64 16 4 1
    do
        i2cset -y $I2CBUS $DEVADDR 0x40 $i
    done
done
