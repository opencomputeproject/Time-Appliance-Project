#!/usr/bin/env python3
# -*- mode: python; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-

# Simple test script that plays (some) wav files

from __future__ import print_function

import sys
import wave
import getopt
import alsaaudio
import time
from multiprocessing import Process, Lock
import multiprocessing as mp

def play(device, f, queue):

    format = None

    # 8bit is unsigned in wav files
    if f.getsampwidth() == 1:
        format = alsaaudio.PCM_FORMAT_U8
    # Otherwise we assume signed data, little endian
    elif f.getsampwidth() == 2:
        format = alsaaudio.PCM_FORMAT_S16_LE
    elif f.getsampwidth() == 3:
        format = alsaaudio.PCM_FORMAT_S24_3LE
    elif f.getsampwidth() == 4:
        format = alsaaudio.PCM_FORMAT_S32_LE
    else:
        raise ValueError('Unsupported format')

    # 100ms period
    periodsize = f.getframerate() // 10
    
    #compute 10ms period size
    #periodsize = f.getframerate()
    #periodsize = 1

    print('%d channels, %d sampling rate, format %d, periodsize %d\n' %
          (f.getnchannels(), f.getframerate(), format, periodsize))

    device = alsaaudio.PCM(
        channels=f.getnchannels(),
        rate=f.getframerate(),
        format=format,
        periodsize=periodsize,
        device=device)

    preload_length = 5

    for i in range(preload_length):
        data = f.readframes(periodsize)
        device.write(data)

    # floating point value in seconds
    while data:
        #queue.get() # block until timer give me something
        device.write(data)
        data = f.readframes(periodsize)
        wrote_time = time.clock_gettime(time.CLOCK_REALTIME)
        #write to queue
        #queue.put(1)

def lock_timer(queue):
    while 1:
        start_time = time.clock_gettime(time.CLOCK_REALTIME)
        while time.clock_gettime(time.CLOCK_REALTIME) - start_time < (0.001):
            pass
        release_time = time.clock_gettime(time.CLOCK_REALTIME)

        

def usage():
    print('usage: playwav.py [-d <device>] <file>', file=sys.stderr)
    print(f"Devices: {alsaaudio.pcms()}")

    sys.exit(2)


if __name__ == '__main__':

    device = 'default'

    opts, args = getopt.getopt(sys.argv[1:], 'd:')
    for o, a in opts:
        if o == '-d':
            device = a

    if not args:
        usage()

    queue = mp.Queue() 
    with wave.open(args[0], 'rb') as f:
        Process(target=play, args=(device,f,queue)).start()
    Process(target=lock_timer, args=(queue,)).start()
    
