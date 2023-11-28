Here is testptp for configuring the PPS pins on the a NIC

Use the following command for compiling:
```
make
```
or use the full command
```
gcc -Wall -lrt testptp.c -o testptp
```
For installation use:
```
sudo make install
```
or simply use
```
sudo mv testptp /usr/bin/
```
To enabled PPS out for eth0 use the following command:
```
ptp_dev=`ethtool -T eth0 | awk '/PTP Hardware Clock:/ {print $4}'` && testptp -d /dev/ptp$ptp_dev -L 0,1

```
To enable PPS in for eth0 use the following command:
```
ptp_dev=`ethtool -T eth0 | awk '/PTP Hardware Clock:/ {print $4}'` && testptp -d /dev/ptp$ptp_dev -e -1
```
