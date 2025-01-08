#!/bin/bash

# I2C bus and PCA9548 mux address
I2C_BUS="3"
MUX_ADDR=0x70

# TCA6408 register addresses
TCA6408_CONFIG_REG=0x03
TCA6408_OUTPUT_REG=0x01

# Function to configure a TCA6408
configure_tca6408() {
    local bus=$1
    local addr=$2
    local output_value=$3

    # Set all GPIOs as outputs (write 0x00 to the config register)
    i2cset -y "$bus" "$addr" "$TCA6408_CONFIG_REG" 0x00

    # Set output values
    i2cset -y "$bus" "$addr" "$TCA6408_OUTPUT_REG" "$output_value"
}


#Psemi 4259-63 truth table
# CTRL = 1, RFC to RF1 ; CTRL = 0, RFC to RF2

# Configure MUX output 3 (TCA6408 at 0x21)
# SubG attenuator control, set all to high
echo "Configuring SubGhz path"
i2cset -y "$I2C_BUS" "$MUX_ADDR" 0x08
configure_tca6408 "$I2C_BUS" 0x21 0xFF  # Set all GPIOs to high (0xFF)

# Configure MUX output 6 (TCA6408 at 0x21)
# GNSS switches and SubGhz switch
# bit 0 = GNSS LNA1 EN for Antenna -> SFP path, 1 for enable , default 0
# bit 1 = GNSS SW1 , RF2 to LNA1, RF1 direct to switch 2 (default), default 1
# bit 2 = GNSS SW2 , RF1 to LNA1, RF2 direct to switch 1 (default), default 0
# bit 3 = GNSS SW3 , RF1 to LNA2, RF2 direct to switch 4 (default), default 0
# bit 4 = GNSS SW4 , RF2 to LNA2, RF1 direct to switch 3 (default), default 1
# bit 5 = GNSS LNA2 EN for SFP -> SMA path, 1 for enable , default 0
# bit 6 = SubGhz SW1, RF1 to attenuator path, RF2 direct to switch 2 (default), default 0
# bit 7 = SubGhz SW2, RF2 to attenuator path, RF1 direct to switch 1 (default), default 1
# 0x92 for default direct path config above
# For GPS LNA only on Antenna -> SFP path => 0x95
# For GPS LNA only on SFP -> SMA path => 0xAA
# For GPS LNA on both Antenna -> SFP and SFP -> SMA path => 0xAD
echo "Configuring GNSS path"
i2cset -y "$I2C_BUS" "$MUX_ADDR" 0x40
configure_tca6408 "$I2C_BUS" 0x21 0xAD

# Configure MUX output 4 (TCA6408 at 0x20 and 0x21)
# 0x21 is attenuator control, set all to high
# 0x20 is switch
# bit 0 = WIFI SW1 CTRL , RF1 to attenuator path, RF2 for direct to switch 2 (default), default 0
# bit 1 = WIFI SW2 CTRL , RF2 to attenuator path, RF1 for direct to switch 1 (default), default 1
echo "Configuring WiFi path"
i2cset -y "$I2C_BUS" "$MUX_ADDR" 0x10
configure_tca6408 "$I2C_BUS" 0x20 0x02  # Example: 0xAA = 10101010
configure_tca6408 "$I2C_BUS" 0x21 0xFF  # Example: 0x55 = 01010101

echo "I2C GPIO expanders configured successfully."

