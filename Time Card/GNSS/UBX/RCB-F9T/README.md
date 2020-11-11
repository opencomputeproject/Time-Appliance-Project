# UBX RCB-F9T GNSS module configuration

Please use the UBX U-Center configurator.
First choose the right COM port and select the baud rate 115200 (for stock units)
Then from the menu Tools -> Receiver Configuration... Select the following file and press Transfer file -> GNSS while the "store configuration inBBR/Flash has been selected.
The configuration will go half way with to problem until it starts to fail.
Change the baud rate to 4800 and repreat the configuration again. This time the configuration will go all the way through. 

## Programming the GNSS module

To program the GNSS module you can either use the UBX provided hardware or a simple FTDI-TTL3v3 cable (please note, you have to use the 3v3 version oppose to the 5v). Here is a simple wiring digram to program the GNSS module.

![UBX GNSS simple programmer](prog.png)
