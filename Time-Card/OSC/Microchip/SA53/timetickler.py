# Copyright (c) Facebook, Inc. and its affiliates. All rights reserved.

# This source code is licensed under the license found in the LICENSE file in the root directory of this source tree.

import argparse
import itertools
import logging
import os
import sys
 
import serial
 
BASE_SYS_TIMECARD_PATH = "/sys/class/timecard"
DEFAULT_BAUDRATE = 57600
 
MAC_COMMANDS_GET = [
    "DisciplineLocked",
    "DisciplineThresholdPps0",
    "PpsInDetected",
    "LockProgress",
    "PpsSource",
    "LastCorrection",
    "DigitalTuning",
    "Phase",
    "Temperature",
    "serial",
    "Locked",
    "PhaseLimit",
    "EffectiveTuning",
]
MAC_COMMANDS_SET = ["PpsWidth", "TauPps0", "PpsSource", "PpsOffset", "Disciplining", "DisciplineThresholdPps0", "PhaseMetering", "DigitalTuning", "CableDelay", "latch"]
 
log = logging.getLogger(__name__)

def query_system_timecards():
    timecards = os.listdir(BASE_SYS_TIMECARD_PATH)
    return timecards
 

def list_timecard(timecard):
    timecard_sys_class = os.listdir(os.path.join(BASE_SYS_TIMECARD_PATH, timecard))
    print(f"{timecard} sys class info:")
    for entity in timecard_sys_class:
        path = os.path.join(BASE_SYS_TIMECARD_PATH, timecard, entity)
        if os.path.islink(path):
            print("\t" + entity + " -> " + os.readlink(path))
        else:
            print("\t" + entity)
 

def get_mac_tty(timecard):
    mac_path = os.path.join(BASE_SYS_TIMECARD_PATH, timecard, "ttyMAC")
    mac_tty = os.path.basename(os.readlink(mac_path))
    return os.path.join(os.sep, "dev", mac_tty)
 

def query_timecard_mac(tty, fields, n):
    with serial.Serial(tty, baudrate=DEFAULT_BAUDRATE, timeout=1) as tty_dev:
        for i in itertools.count(start=0):
            if i == n:
                break
            for field in fields:
                cmd_str = "\{get," + field + "}"
                tty_dev.write(cmd_str.encode())
                field_val = tty_dev.readline().decode().strip()
                print(f"{field}, {field_val}")
 

def set_timecard_mac(tty, field, value, store=False):
    with serial.Serial(tty, baudrate=DEFAULT_BAUDRATE, timeout=1) as tty_dev:
        if field == "latch":
            cmd_str = "\{latch}"
        else:
            cmd_str = "\{set," + field + "," + value + "}"
        tty_dev.write(cmd_str.encode())
        if store:
            cmd_str = "\{store}"
            tty_dev.write(cmd_str.encode())
        ret_val = tty_dev.readline().decode().strip()
        if field == "latch":
            print(f"Set: {field}, stored: {store}, returned: {ret_val}")
        else:
            print(f"Set: {field} to: {value}, stored: {store}, returned: {ret_val}")
 

def main():
    parser = argparse.ArgumentParser("timetickler")
    subparsers = parser.add_subparsers(dest="command")
    get_parser = subparsers.add_parser("get", help="obtain value from field")
    set_parser = subparsers.add_parser("set", help="write value to field")
    list_parser = subparsers.add_parser("list", help="list timecard details on system")
 
    get_parser.add_argument(
        "field",
        nargs="+",
        choices=MAC_COMMANDS_GET + MAC_COMMANDS_SET,
    )
    get_parser.add_argument(
        "-n",
        default=1,
        type=int,
        help="query field 'n' times, or '-1' until terminated by user",
    )
 
    set_parser.add_argument("field", choices=MAC_COMMANDS_SET)
    set_parser.add_argument("value")
    set_parser.add_argument(
        "--store",
        "-s",
        help="store value into mac eeprom",
        action="store_true",
    )
 
    parser.add_argument(
        "timecard",
        nargs="?",
        default=query_system_timecards()[0],
        choices=query_system_timecards(),
        help="specify which timecard to list all info",
    )
 
    args = parser.parse_args()
    # print(args)
 
    if args.command == "get":
        query_timecard_mac(get_mac_tty(args.timecard), args.field, args.n)
    elif args.command == "set":
        set_timecard_mac(get_mac_tty(args.timecard), args.field, args.value, args.store)
    elif args.command == "list":
        list_timecard(args.timecard)
    else:
        parser.print_help()
 

if __name__ == "__main__":
    sys.exit(main())
