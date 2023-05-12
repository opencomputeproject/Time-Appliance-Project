import pyaudio
import numpy as np
import time

p = pyaudio.PyAudio()

volume = 0.5     # range [0.0, 1.0]
fs = 44100       # sampling rate, Hz, must be integer
duration = 1.0   # in seconds, may be float
f = 440.0        # sine frequency, Hz, may be float

# generate samples, note conversion to float32 array
samples = (np.sin(2*np.pi*np.arange(fs*duration)*f/fs)).astype(np.float32)

info = p.get_host_api_info_by_index(0)
numdevices = info.get('deviceCount')


for i in range (0,numdevices):
        if p.get_device_info_by_host_api_device_index(0,i).get('maxInputChannels')>0:
                print("Input Device id ", i, " - ", p.get_device_info_by_host_api_device_index(0,i).get('name'))

        if p.get_device_info_by_host_api_device_index(0,i).get('maxOutputChannels')>0:
                print("Output Device id ", i, " - ", p.get_device_info_by_host_api_device_index(0,i).get('name'))
                # for paFloat32 sample values must be in range [-1.0, 1.0]
                stream = p.open(format=pyaudio.paFloat32,
                    channels=2,
                    rate=fs,
                    output_device_index = i,
                    output=True)

                # play. May repeat with different volume values (if done interactively) 
                stream.write(volume*samples)
                time.sleep(duration)
                stream.stop_stream()
                stream.close()








p.terminate()
