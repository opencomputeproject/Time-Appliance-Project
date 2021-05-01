#!/bin/bash

# code written by Ahmad Byagowi for demonstration purposes of the SI7020 chip over the i2c bus

I2CBUS=10
DEVADDR=0x40

SI7020_A20_MEAS_RH_HOLD=0xE5 # Measure Relative Humidity, Hold Master Mode
SI7020_A20_MEAS_RH_NOHOLD=0xF5 # Measure Relative Humidity, No Hold Master Mode
SI7020_A20_MEAS_TEMP_HOLD=0xE3 # Measure Temperature, Hold Master Mode
SI7020_A20_MEAS_TEMP_NOHOLD=0xF3 # Measure Temperature, No Hold Master Mode
SI7020_A20_READ_PREV_TEMP=0xE0 # Read Temperature Value from Previous RH Measurement
SI7020_A20_RESET=0xFE # Reset
SI7020_A20_WRITERHT_REG=0xE6 # Write RH/T User Register 1
SI7020_A20_READRHT_REG=0xE7 # Read RH/T User Register 1
SI7020_A20_WRITEHEATER_RE=0x51 # Write Heater Control Register
SI7020_A20_READHEATER_REG=0x11 # Read Heater Control Register

function humidity(){

i2cset -y $I2CBUS $DEVADDR 0xE5 0x01
local raw_humidity=$(i2cget -y $I2CBUS $DEVADDR 0xE5 w)

i2cset -y $I2CBUS $DEVADDR 0xE3 0x01
local raw_temp=$(i2cget -y $I2CBUS $DEVADDR 0xE3 w)

humidity=$(($(($raw_humidity*124))-$((65536*6))))

Temperature=$(($(($raw_temp*17572))-$((65536*4685))))

printf "Humidity = %.3f%% RH\n" $(echo $humidity / 65536 | bc -l)

printf "Temperature = %.3f C\n" $(echo $Temperature / 6553600 | bc -l)

}


humidity
