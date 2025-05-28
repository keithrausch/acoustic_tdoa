# Keith Rausch

import numpy as np
import matplotlib.pyplot as plt
import json, base64
import os
from scipy import signal
from scipy.signal import bode
import warnings

SAMPLE_RATE = 44100
BUF_LENGTH = 128
HALF_BUF_LENGTH = int(BUF_LENGTH / 2)
SPEED_OF_SOUND_MPS = 333
SPEED_OF_SOUND_INPS = SPEED_OF_SOUND_MPS * 39.3701
w = SAMPLE_RATE/BUF_LENGTH

def fftPlot(sig, dt=None, plot=True):
    # Here it's assumes analytic signal (real signal...) - so only half of the axis is required

    if dt is None:
        dt = 1
        t = np.arange(0, sig.shape[-1])
        xLabel = 'samples'
    else:
        t = np.arange(0, sig.shape[-1]) * dt
        xLabel = 'freq [Hz]'

    if sig.shape[0] % 2 != 0:
        warnings.warn("signal preferred to be even in size, autoFixing it...")
        t = t[0:-1]
        sig = sig[0:-1]

    sigFFT = np.fft.fft(sig) / t.shape[0]  # Divided by size t for coherent magnitude

    freq = np.fft.fftfreq(t.shape[0], d=dt)

    # Plot analytic signal - right half of frequence axis needed only...
    firstNegInd = np.argmax(freq < 0)
    freqAxisPos = freq[0:firstNegInd]
    sigFFTPos = 2 * sigFFT[0:firstNegInd]  # *2 because of magnitude of analytic signal

    if plot:
        plt.figure()
        plt.plot(freqAxisPos, np.abs(sigFFTPos))
        plt.xlabel(xLabel)
        plt.ylabel('mag')
        plt.title('Analytic FFT plot')
        # plt.show()

    return sigFFTPos, freqAxisPos

def get_audio_data(file_name="recording.json"):
    import serial
    import re
    import numpy as np
    import time
    import matplotlib.pyplot as plt
    print('starting...')

    with open(file_name, "r") as outfile: 
        data = json.load(outfile)
        matches = data['matches']
        matches = [(device, np.array(data)) for device, data in matches]

    # trim the start of the data
    # avg = np.mean(matches[0][1][:128])
    # index = np.argwhere(matches[0][1] > avg*3)[0][0]
    # matches = [(device, data[index:]) for device, data in matches]
    # matches = [(device, data[int(2.4*128):int(3.5*128)]) for device, data in matches]

    # look ata  specific channel
    # matches= matches[1:]
    # matches= matches[:1]
        

    n_devices = len(matches)

    plt.figure()
    legend = []
    for i,(device, array) in enumerate(matches):
        n_samples = len(array)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        plt.plot(time, array)
        legend.append('channel {}'.format(device))
        plt.xlabel('time [s]')
    plt.legend(legend)
    plt.title('raw')

    reconstructions = [np.zeros_like(data, dtype=float) for _,data in matches]

    w = SAMPLE_RATE / BUF_LENGTH 
    a1 = 0
    b1 = 5*w # w*5*2
    a2 = b1 + w*5
    b2 = a2 + w*5
    N = 2
    for (freq_a, freq_b, bandpass_freqs) in [(a1,b1, (650, 1500)),(a2,b2, (1000, 2000))][:N]:
        offsets = analyze(matches, reconstructions, freq_a=freq_a, freq_b=freq_b, bandpass_freqs=bandpass_freqs)

        tdoa = np.abs(np.diff(offsets)[0])
        # tdoa += 2/ SAMPLE_RATE
        print('TDOA = {}, computed dist = {} in, fuzz dist = {},{}'.format(tdoa, tdoa*SPEED_OF_SOUND_INPS, (tdoa+1/ SAMPLE_RATE)*SPEED_OF_SOUND_INPS, (tdoa+2/ SAMPLE_RATE)*SPEED_OF_SOUND_INPS))
        




    if False:

        def butter_bandpass(lowcut, highcut, fs, order=5):
            return signal.butter(order, [lowcut, highcut], fs=fs, btype='band')

        # Sample rate and desired cutoff frequencies (in Hz).
        fs = 44100.0
        lowcut = b1*0.9
        highcut = b1*1.1

        # Plot the frequency response for a few different orders.
        plt.figure()
        plt.clf()
        for order in [2, 3, 6, 9]:
            bb, aa = butter_bandpass(lowcut, highcut, fs, order=order)
            w, h = signal.freqz(bb, aa, fs=fs, worN=2000)
            plt.plot(w, abs(h), label="order = %d" % order)

        plt.plot([0, 0.5 * fs], [np.sqrt(0.5), np.sqrt(0.5)],
                '--', label='sqrt(0.5)')
        plt.xlabel('Frequency (Hz)')
        plt.ylabel('Gain')
        plt.grid(True)
        plt.legend(loc='best')

    plt.show()
    
    # write a byte

    # read data

def correlate_impl(a, b, freq_a=None, freq_b=None, bandpass_freqs=None):
    # a = (a - np.mean(a)) / (np.std(a) * len(a))
    # b = (b - np.mean(b)) / (np.std(b))
    # c = np.correlate(a, b, 'same')

    f = freq_b + freq_a
    

    sos = signal.butter(2, bandpass_freqs, 'bandpass', fs=44100, output='sos')
    c = signal.sosfiltfilt(sos, a)
    return c

def correlate_white_hot_black_hot(time, sigA, sigB, freq_a=None, freq_b=None, bandpass_freqs=None):
    surface = correlate_impl(sigA, sigB, freq_a=freq_a, freq_b=freq_b, bandpass_freqs=bandpass_freqs)

    # f = freq_b + freq_a
    # sos = signal.butter(2, (f-50, f+50), 'bandpass', fs=44100, output='ba')
    # omega, mag, phase = bode(sos, w=f*2*np.pi)
    # time_offset = (phase*np.pi/180) / (f*2*np.pi)
    # index_offset = time_offset * (SAMPLE_RATE)

    # surface = surface * np.linspace(0.1, 1.0, len(surface)) ####################################################
    surface_abs = np.abs(surface)
    surface_argmax = np.argmax(surface_abs)
    # multiplier = 1
    # multiplier *= sigA[surface_argmax]

    N_LEFT = 1
    N_RIGHT = 1
    surface_argmax = np.argmax(np.abs(sigA[surface_argmax-N_LEFT:surface_argmax+N_RIGHT+1])) + surface_argmax - N_LEFT

    # if np.abs(sigA[surface_argmax-1]) > np.abs(sigA[surface_argmax+1]):
    #     N_LEFT += 1
    # else:
    #     N_RIGHT += 1

    # t1 = -1.0 / SAMPLE_RATE
    # t2 =  0.0 / SAMPLE_RATE
    # t3 = +1.0 / SAMPLE_RATE
    b_vandermonde = sigA[surface_argmax-N_LEFT:surface_argmax+N_RIGHT+1]

    # A = [[t1*t1, t1, 1],
    #      [t2*t2, t2, 1],
    #      [t3*t3, t3, 1]]
    A_vandermonde = np.array([[t*t, t, 1] for t in np.linspace(-N_LEFT, N_RIGHT, N_LEFT+N_RIGHT+1)])

    abc = np.linalg.lstsq(A_vandermonde, b_vandermonde, rcond=None)[0]
    a,b,c= abc.ravel()
    t_optimum_descrete = -b / (2*a)
    t_optimum = t_optimum_descrete / SAMPLE_RATE
    t_optimum = t_optimum + time[surface_argmax]

    ###############################
    ttt = np.linspace(-N_LEFT, N_RIGHT, 100)
    curve_fit = lambda t: a*t*t + b*t+c
    yyy = curve_fit(ttt)
    ttt = ttt / SAMPLE_RATE
    ttt = ttt + time[surface_argmax]

    multiplier = curve_fit(t_optimum_descrete)


    return surface, multiplier, t_optimum, (yyy,ttt)


def build_morlet_freq_control(amp, freq_a, freq_b):
  
    np.array([BUF_LENGTH])
    i = np.arange(BUF_LENGTH)
    t = (i-BUF_LENGTH/2) * 1.0/SAMPLE_RATE # convert index to time and center it in the window

    arg = np.pi*t*(freq_b-freq_a)
    sinc = np.sin(arg)/ arg
    val = amp * np.cos(np.pi*t*(freq_b+freq_a)) *sinc * sinc
    val[np.isnan(val)] = amp
    return val, t

def analyze(matches, reconstructions, freq=None, freq_a=None, freq_b=None, bandpass_freqs=None):
    wavelet, wavelet_time = build_morlet_freq_control(1, freq_a=freq_a, freq_b=freq_b)
    plt.figure()
    plt.plot(wavelet_time, wavelet)
    plt.title('freq_a={}, freq_b={} wavelet'.format(freq_a, freq_b))


    # reconstruct(matches, reconstructions)
    fftPlot(wavelet, dt=1/44100, plot=True)


    time_received = []

    plt.figure()
    plt.title('correlation between raw and wavelet ({}, {})'.format(freq_a, freq_b))
    legend = []
    for i,(device, data) in enumerate(matches):
        n_samples = len(data)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        surface, multiplier, _, _ = correlate_white_hot_black_hot(time, data, wavelet, freq_a=freq_a, freq_b=freq_b, bandpass_freqs=bandpass_freqs)
        # offset_index = np.argmax(np.abs(surface))
        # offset_time = time[offset_index]
        plt.plot(time, np.abs(surface))
        legend.append('channel {}'.format(device))
        plt.xlabel('time [s]')
    plt.legend(legend)


    plt.figure()
    plt.title('raw data with wavelet ({}, {})'.format(freq_a, freq_b))
    legend = []
    for i,(device, data) in enumerate(matches):
        n_samples = len(data)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        plt.plot(time, data)
        legend.append('channel {}'.format(device))
        plt.xlabel('time [s]')
    for i,(device, data) in enumerate(matches):
        n_samples = len(data)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        _, multiplier, time_of_peak, (yyy, ttt) = correlate_white_hot_black_hot(time, data, wavelet, freq_a=freq_a, freq_b=freq_b, bandpass_freqs=bandpass_freqs)
        # offset_index = np.argmax(np.abs(data))
        # offset_time = time[offset_index]
        time_received.append(time_of_peak)
        plt.plot(wavelet_time+time_of_peak, wavelet*multiplier)
        plt.plot(ttt, yyy)
        # reconstructions[i][offset_index-int(len(wavelet)/2):offset_index-int(len(wavelet)/2)+len(wavelet)] += wavelet*5000*np.sign(multiplier)
    plt.legend(legend)

    # plt.figure()
    # plt.title('bandpass')
    # legend = []
    # for i,(device, data) in enumerate(matches):
    #     n_samples = len(data)
    #     time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
    #     sos = signal.butter(2, (freq_a, freq_b), 'bandpass', fs=44100, output='sos')
    #     filtered = signal.sosfilt(sos, data)
    #     plt.plot(time, filtered)
    #     legend.append('channel {}'.format(device))
    #     plt.xlabel('time [s]')

    return time_received

def reconstruct(matches, reconstructions):
    plt.figure()
    plt.title('reconstruction')
    legend = []
    for i,(device, data) in enumerate(matches):
        n_samples = len(data)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        plt.plot(time, data)
        legend.append('channel {}'.format(device))
        plt.xlabel('time [s]')
    for i,((device, data), reconstruction) in enumerate(zip(matches, reconstructions)):
        n_samples = len(data)
        time = np.linspace(0,n_samples/SAMPLE_RATE, n_samples)
        reconstruction = reconstruction / np.max(np.abs(reconstruction)) * np.max(np.abs(data))
        plt.plot(time, reconstruction)
    plt.legend(legend)

if __name__ == "__main__":
    get_audio_data()
    print('finished')