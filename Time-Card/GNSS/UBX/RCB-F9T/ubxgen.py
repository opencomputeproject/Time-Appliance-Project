#!/usr/bin/python3

import sys
import binascii

cs0 = 0
cs1 = 0

ubx = r"\xb5\x62"

for d in sys.argv[1:]:
    i = int(d, base=16)
    c = "0x{:02x}".format(i)
    ubx = r"{}\x{}".format(ubx, c[2:])
    cs0 += i
    cs0 &= 255
    cs1 += cs0
    cs1 &= 255

ubx = r"{}\x{}\x{}".format(ubx, "0x{:02x}".format(cs0)[2:], "0x{:02x}".format(cs1)[2:])
print(ubx)
