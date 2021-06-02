OCP Time Appliance can be used with any NTP/PTP server.
Here are couple of examples and configs:
## NTP
NTP servers usually can work directly with PHC
### Chrony
https://github.com/mlichvar/chrony
```
$ cat /etc/chrony.conf
refclock PHC /dev/ptp1 poll 0 trust
...
allow all
hwtimestamp *
```

## PTP
Keep in mind synchronization of PHC between time card and the Network card is required.
The example shown is for unicast server only.
### Responder
https://github.com/facebookincubator/ptp
```
$ /usr/local/bin/ptpresponder -iface eth0
```

### Linuxptp (ptp4l)
https://sourceforge.net/projects/linuxptp/
```
$ /sbin/ptp4l -f /etc/ptp4l.conf -i eth0 -m"
```
