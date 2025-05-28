# Keith Rausch

import numpy as np
import matplotlib.pyplot as plt
import json, base64
import os

def get_audio_data(file_name="recording.json"):
    import serial
    import re
    import numpy as np
    import time
    import matplotlib.pyplot as plt
    print('starting...')

    # open serial port
    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2.0)
    s = ser.write(b'a')
    time.sleep(1)
    s = ser.read(2**17)
    print(s)
    
    sync = b'xxxxxxxx'
    matches = []
    index = s.find(sync)
    while len(s) > 0:
        index_B = s.find(sync,index+1)
        if index_B < 0:
            index_B = len(s)
        device = s[index+len(sync):index+len(sync)+1]
        device = device.decode()
        sub = s[index+len(sync)+1:index_B]
        matches.append((device, np.frombuffer(sub, dtype=np.int16)))
        index = index_B
        if index_B >= len(s):
            break
        pass

    # Serializing json   
    with open(file_name, "w") as outfile: 
        matches_saveable = [(device, data.tolist()) for device, data in matches]
        json.dump({ "matches": matches_saveable }, outfile)

    ser.close()
        

    n_devices = len(matches)

    plt.figure()
    legend = []
    for i,(device, array) in enumerate(matches):
        # print('len(data)={}'.format(len(array)))
        # plt.subplot(n_devices, 1, i+1)
        # array = np.frombuffer(data, dtype=np.int16)
        # data = array# NOTE OVERWRITE DATA
        # print('len(data)={} len(array)={}'.format(len(data), len(array)))
        n_samples = len(array)
        time = np.linspace(0,n_samples/44100, n_samples)
        plt.plot(time, array)
        legend.append('channel {}'.format(device))
        plt.xlabel('time [s]')
    plt.legend(legend)

    plt.show()
    
    # write a byte

    # read data

if __name__ == "__main__":
    get_audio_data()
    print('finished')