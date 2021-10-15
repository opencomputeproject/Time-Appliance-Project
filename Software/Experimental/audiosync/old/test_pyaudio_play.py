#!/usr/bin/env python3
# -*- mode: python; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-

# Simple test script that plays (some) wav files

import sys
import wave
import getopt
import time
import pyaudio
import linuxfd,signal,select
import math

def play(device, f):
    print(f"Using device {device}")
    p = pyaudio.PyAudio()
    if device == -1:
        stream = p.open(
                format = p.get_format_from_width(f.getsampwidth()),
                channels = f.getnchannels(),
                rate = f.getframerate(),
                output = True,
            )
    else:
        stream = p.open(
                format = p.get_format_from_width(f.getsampwidth()),
                channels = f.getnchannels(),
                rate = f.getframerate(),
                output = True,
                output_device_index = device
            )

    data = f.readframes(1024)


    # wait to start at a specific time
    # use linuxfd epoll
    tfd = linuxfd.timerfd(rtc=True, nonBlocking=True, closeOnExec=True)

    start_time = time.clock_gettime(time.CLOCK_REALTIME)
    start_time_floor = math.floor(start_time)

    seconds = start_time_floor % 30
    diff = 30 - seconds
    start_time_floor += diff


    print(f"Start time {start_time} floor {start_time_floor} sec {seconds}")

    tfd.settime(value=start_time_floor, interval=0, absolute=True)
    signal.pthread_sigmask(signal.SIG_SETMASK,{signal.SIGINT})
    epl = select.epoll()
    epl.register(tfd.fileno(),select.EPOLLIN)
    events = epl.poll(-1)
    first = True

    while data != '':
        stream.write(data)
        data = f.readframes(1024)
        if first:
            signal.pthread_sigmask(signal.SIG_SETMASK,{})
            print(f"Starting at time {time.clock_gettime(time.CLOCK_REALTIME)}")
            first = False


def usage():
    print('usage: playwav.py [-d <device>] <file>', file=sys.stderr)

    p = pyaudio.PyAudio()

    info = p.get_host_api_info_by_index(0)
    numdevices = info.get('deviceCount')
    #for each audio device, determine if is an input or an output and add it to the appropriate list and dictionary

    print("################# DEVICES ##################")
    for i in range (0,numdevices):
	    if p.get_device_info_by_host_api_device_index(0,i).get('maxInputChannels')>0:
		    print("Input Device id ", i, " - ", p.get_device_info_by_host_api_device_index(0,i).get('name'))

	    if p.get_device_info_by_host_api_device_index(0,i).get('maxOutputChannels')>0:
		    print("Output Device id ", i, " - ", p.get_device_info_by_host_api_device_index(0,i).get('name'))


    sys.exit(2)


if __name__ == '__main__':

    device = '-1'

    opts, args = getopt.getopt(sys.argv[1:], 'd:')
    if not args:
        usage()
    if opts:
        device = opts[0][1]

    with wave.open(args[0], 'rb') as f:
        play( int(device) ,f)
    
