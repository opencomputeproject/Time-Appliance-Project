---
sidebar_position: 3
---

# Usage

Time Card is interfaced via `sysfs`.

### Sysfs

To view all the `sysfs` parameters exposed,

```
ls /sys/class/timecard/ocp0/
```

Additional Time Card devices will enumerate as `ocpN` where `N` is the device order.

### Serial ports and PHC number

Serial ports and PHC number can be found via `dmesg` or `ls -l`

## Using Time Card IO

### Ouputs

All possible outputs are available via `available_sma_outputs`,

```
cat available_sma_outputs
```

Will display all possible output types such as,

```
10Mhz PHC MAC GNSS1 GNSS2 IRIG DCF GEN1 GEN2 GEN3 GEN4 GND VCC
```

A few examples are shown below for configuring the output signals,

1. Output FPGA PPS on SMA

```
echo OUT: PHC >> sma1
```

2. Output Atomic clock PPS on SMA

```
echo OUT: MAC >> sma1
```

3. Output GPS Module's PPS on SMA

```
echo OUT: GNSS1 >> sma1
```

### Inputs

All possible inputs are available via `available_sma_inputs`.

```
cat available_sma_inputs
```

Will display all possible inputs types such as,

```
10Mhz PPS1 PPS2 TS1 TS2 IRIG DCF TS3 TS4 FREQ1 FREQ2 FREQ3 FREQ4 None
```

A few examples are shown below for configuring the input signals,

#### Configure SMA ports

1. Use a port for time-stamping incoming signals, this configured `SMA1` with timestamper-1 `TS1`

```
echo IN: TS1 >> sma1
```

To read back the timestamped signals, use `testptp`.

#### Download `testptp` source

Source available at these locations

1. https://www.mjmwired.net/kernel/Documentation/ptp/testptp.c
2. https://github.com/torvalds/linux/blob/master/tools/testing/selftests/ptp/testptp.c

#### Read timestamped signals

**Note: ** `-1` for inifite readings, use a positive number for fixed event count, from `Timestamper 1` `-i 1` from the Time Card `/dev/ptp1`

```
./testptp -d /dev/ptp1 -e -1 -i 1
```
