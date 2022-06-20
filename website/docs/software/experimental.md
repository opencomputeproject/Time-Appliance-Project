---
sidebar_position: 3
---

# Experimental Software

## DiffPHC

DiffPHC is a tool that allows you to measure the difference between two or multiple PHCs of the system.

## Linearizible clock test

This is a vanilla program that tests the true clocks. It is by no means mature, yet. It only depends on the C++11 features and no extra packages are needed(e.g. thrift, etc.) The OSS socket library from https://cs.baylor.edu/~donahoo/practical/CSockets/practical/ are copied and modified for our needs.

**Note:** the original socket library does not support IPv6. Currently, efforts are only made to support IPv6 for UDP sockets.

## Open time instrument

This software is a modified version of testptp. It uses the timestamper inputs of the Time Card to read PPS timestamps from the Time Card, and outputs TIE measurements in a format recognized by Calnex CAT.

### Usage

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
