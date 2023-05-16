#!/usr/bin/python3

# Inspired by https://portal.u-blox.com/s/question/0D52p00008HKCh8CAH/ublox-8-configuration-using-hex-commands
# ex: ./ubxgen.py -c ./Configs/ZED-F9T-00B-01/timingCardUBXG9_PPS_NetTimeLogic-PassiveAntenna115200.txt -t /dev/ttyS2 | bash -x
import argparse
import binascii
import sys

parser = argparse.ArgumentParser(
    description="Generate ublox 8+ commands from u-center config file"
)
parser.add_argument("--cfg", "-c", type=str, required=True, help="Ublox config file")
parser.add_argument("--raw", "-r", action="store_true", help="Print raw string. Useful for debugging")
parser.add_argument("--lay", "-l", type=str, default="RAM", help="Target layer to configure")
parser.add_argument(
    "--tty",
    "-t",
    type=str,
    default="",
    help="Serial device of the ublox receiver. eq /dev/ttyS2",
)
args = parser.parse_args()

with open(args.cfg, "r") as f:
    for line in f.readlines():
        linesplit = line.split(" - ")
        if len(linesplit) != 2:
            continue
        cmd = linesplit[1]
        cs0 = 0
        cs1 = 0
        ubx = r"\xb5\x62"

        if linesplit[0] == "CFG-VALGET":
            cmd = cmd.replace("06 8B", "06 8A", 1)

        cmdsplit = cmd.split(" ")
        if cmdsplit[5] == "00":
            if args.lay == "RAM":
                cmdsplit[5] = "01"
            if args.lay == "BBR":
                cmdsplit[5] = "02"
            if args.lay == "Flash":
                cmdsplit[5] = "04"

        for d in cmdsplit:
            i = int(d, base=16)
            c = "0x{:02x}".format(i)
            ubx = r"{}\x{}".format(ubx, c[2:])
            cs0 += i
            cs0 &= 255
            cs1 += cs0
            cs1 &= 255

        ubx = r"{}\x{}\x{}".format(
            ubx, "0x{:02x}".format(cs0)[2:], "0x{:02x}".format(cs1)[2:]
        )
        if args.raw:
            print(ubx)
        else:
            # print(ubx.encode().decode('unicode-escape'), end='')
            # Print in Python is producing a different result than echo -e:
            # $ python3 -c 'print("\xb5\x62\x06\x8b\x40", end="")' | md5sum
            # 73351b7a3ad7ebf8446816eb4cb7a34d  -
            # $ echo -e -n '\xb5\x62\x06\x8b\x40' | md5sum
            # e057e08cb6bc9c193aa0b27903140a37  -
            # TODO: figure out the print diff
            redirect = ""
            if args.tty:
                redirect = " > {}".format(args.tty)
            print("echo -n -e '{}'{}".format(ubx, redirect))

