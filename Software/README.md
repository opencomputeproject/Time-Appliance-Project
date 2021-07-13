## Table of Contents
1. [Driver](#Driver)
1. [NTP](#Ntp)
   1. [Chrony](#Chrony)
   1. [PTP](#PTP)
      1. [ptp4u](#ptp4u)
      1. [linuxptp](#linuxptp)
1. [License](#License)


## Driver
This repository contains the [ocp_ptp driver](https://github.com/opencomputeproject/Time-Appliance-Project/tree/master/Time-Card/DRV) (included in Linux kernel 5.12 and newer). Driver may require vt-d CPU flag enabled in BIOS.
To compile the driver manually just run `./remake` and then load it with `modprobe ptp_ocp`.

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
### ptp4u
https://github.com/facebookincubator/ptp
```
$ /usr/local/bin/ptp4u -iface eth1
```

### linuxptp
https://sourceforge.net/projects/linuxptp/
```
$ /sbin/ptp4l -f /etc/ptp4l.conf -i eth0 -m
```

# License
OCP encourages participants to share their proposals, specifications and designs with the community. This is to promote openness and encourage continuous and open feedback. It is important to remember that by providing feedback for any such documents, whether in written or verbal form, that the contributor or the contributor's organization grants OCP and its members irrevocable right to use this feedback for any purpose without any further obligation. 

It is acknowledged that any such documentation and any ancillary materials that are provided to OCP in connection with this document, including without limitation any white papers, articles, photographs, studies, diagrams, contact information (together, “Materials”) are made available under the Creative Commons Attribution-ShareAlike 4.0 International License found here: https://creativecommons.org/licenses/by-sa/4.0/, or any later version, and without limiting the foregoing, OCP may make the Materials available under such terms.

As a contributor to this document, all members represent that they have the authority to grant the rights and licenses herein.  They further represent and warrant that the Materials do not and will not violate the copyrights or misappropriate the trade secret rights of any third party, including without limitation rights in intellectual property.  The contributor(s) also represent that, to the extent the Materials include materials protected by copyright or trade secret rights that are owned or created by any third-party, they have obtained permission for its use consistent with the foregoing.  They will provide OCP evidence of such permission upon OCP’s request. This document and any "Materials" are published on the respective project's wiki page and are open to the public in accordance with OCP's Bylaws and IP Policy. This can be found at http://www.opencompute.org/participate/legal-documents/.  If you have any questions please contact OCP.

This work is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/).
