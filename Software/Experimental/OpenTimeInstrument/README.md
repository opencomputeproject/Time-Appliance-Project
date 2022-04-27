# Open Time Instrument
This software is a modified version of testptp.
It uses the timestamper inputs of the Time Card to read PPS timestamps from the Time Card, and outputs TIE measurements in a format recognized by Calnex CAT.

## Usage
1. Enable the Time Card SMAs as Timestamper inputs
```
echo IN: TS1 >> /sys/class/timecard/ocp0/sma1
echo IN: TS2 >> /sys/class/timecard/ocp0/sma2
echo IN: TS3 >> /sys/class/timecard/ocp0/sma3
echo IN: TS4 >> /sys/class/timecard/ocp0/sma4
```
2. Build this tool
```
make
```
3. Run this application using the ptp device for the Time Card
```
./OpenTimeInstrument -d /dev/ptp1 -e -1 
```
4. Log results are made as .log files, one log per channel.
