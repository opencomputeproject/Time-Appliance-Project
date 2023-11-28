Here is testptp for configuring the PPS pins on the a NIC

Use the following command for compiling:
```
gcc -Wall -lrt testptp.c -o testptp
```
For installation use:
```
mv testptp /root/bin/
```
To enabled PPS out for eth0 use the following command:
```
ptp_dev=`ethtool -T eth0 | awk '/PTP Hardware Clock:/ {print $4}'` && testptp -d /dev/ptp$ptp_dev -L 0,1

```
To enable PPS in for eth0 use the following command:
```
ptp_dev=`ethtool -T eth0 | awk '/PTP Hardware Clock:/ {print $4}'` && testptp -d /dev/ptp$ptp_dev -e -1
```
