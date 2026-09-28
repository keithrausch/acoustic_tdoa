import numpy as np
from functools import partial
from scipy import optimize
import re
from pathlib import Path
import os, shutil

import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from matplotlib.backends.backend_pdf import PdfPages

sinc = lambda t: np.sin(t) / t if t != 0 else 1.0
sinc2 = lambda t: np.sin(t)*np.sin(t) / (t*t) if t != 0 else 1.0

def create_time(sample_period, duration_s=1.0):
    return np.arange(0.0, duration_s, sample_period)

def create_wave_impl(sample_period, duration_s=1.0, 
                     N_sinusoids=None, 
                     sinusoid_amp_freq_list=None, 
                     sinc_amplitude=None, 
                     sinc_center_s=None, 
                     sinc_freq=None, 
                     sinc_func=sinc):
    
    tt = create_time(sample_period, duration_s)
    yy = np.zeros_like(tt)

    if sinc_amplitude is not None:
        yy = yy + sinc_amplitude * np.array([ sinc_func(sinc_freq * 2*np.pi * (t-sinc_center_s)) for t in tt])

    rng = np.random.default_rng(seed=9)
    if N_sinusoids is not None:
        for i in range(N_sinusoids):
            amplitude = rng.random()
            freq = 10*rng.random()
            phase = rng.random()
            yy = yy + amplitude*np.sin(freq * 2 * np.pi*tt + phase)
    else:
        for amp, freq in sinusoid_amp_freq_list:
            phase = rng.random()
            yy = yy + amp*np.sin(freq * 2 * np.pi*tt + phase)

    return tt, yy

def create_sinc(sample_period, 
                duration_s=1.0, 
                center_s=0.0, 
                amplitude=1.0, 
                freq=1, 
                func=sinc):
    tt = create_time(sample_period, duration_s)
    yy = amplitude * np.array([ func(freq * 2*np.pi * (t-center_s)) for t in tt])
    return tt, yy


def correlate_impl(A, B, f, duration_s, tau, derivative_order=0, is_normalized=False):
    A = A[:, np.newaxis]
    B = B[:, np.newaxis]
    f = f[:, np.newaxis]
    # N_coeff = float(len(freqs))
    normalization = 1.0 / (len(f)*len(f)) if not is_normalized else 1.0
    return duration_s * np.power(2.j*np.pi, derivative_order) * normalization * \
        np.sum(np.power(f,derivative_order) * np.conjugate(A)*B*np.exp(2.j*np.pi*np.outer(f, tau)), axis=0)

def correlate_ifft_impl(A, B, f, duration_s, derivative_order=0, is_normalized=False):
    normalization = 1.0 / (len(f)*len(f)) if not is_normalized else 1.0
    mult = np.power(f,derivative_order) * np.conj(A) * B
    # NOTE: setting normalization to 'forward' so that it doesnt do anything here. 
    # we are manually normalizing. unfortunantly there is no 'none' options
    return duration_s * np.power(2.j*np.pi, derivative_order) * normalization * \
        np.fft.fftshift(np.fft.ifft(np.fft.ifftshift(mult), norm='forward'))

def perform_fft_and_shift_and_normalize(period, wave):
    N = len(wave)
    freqs = np.fft.fftfreq(N, d=period)
    # freqs = np.fft.fftshift(freqs)
    freqs = freqs[:N//2]

    coeffs = np.fft.fft(wave)
    # coeffs = np.fft.fftshift(coeffs)
    coeffs = coeffs[:N//2]
    coeffs_normalized = coeffs / float(N)

    return freqs, coeffs_normalized

def multipage(folder_name, prefix, figs=None, dpi=200):
    script_dir = Path(__file__).resolve().parent

    dir_path = os.path.join(script_dir, folder_name)
    if os.path.exists(dir_path) and os.path.isdir(dir_path):
        shutil.rmtree(dir_path) # clear out old files else the naming and numbering gets weird
    os.makedirs(dir_path, exist_ok=True)

    if figs is None:
        figs = [plt.figure(n) for n in plt.get_fignums()]

    for i,fig in enumerate(figs):
        raw_title = fig.axes[0].get_title()
        if not raw_title:
            raw_title = 'unnamed_plot'
        sanitized_filename = re.sub(r'[^\w\s-]', '', raw_title).strip().lower()
        sanitized_filename = re.sub(r'[-\s]+', '_', sanitized_filename)

        pp = PdfPages(f'{script_dir}/{folder_name}/{prefix}{i:02d}_{sanitized_filename}.pdf')
        fig.savefig(pp, format='pdf', bbox_inches='tight')
        pp.close()

def show_sinc(freq=1):

    N = 2**7
    duration_s = 1.0
    sample_period = duration_s / float(N)
    tt, yy = create_sinc(sample_period, duration_s=duration_s, freq=freq, center_s=duration_s*0.5)

    coeffs = np.fft.fft(yy)
    mag = np.absolute(coeffs)
    phase = np.angle(coeffs)
    freqs = np.fft.fftfreq(N, d=sample_period)

    freqs_plot = np.fft.fftshift(freqs)
    mag_plot = np.fft.fftshift(mag)
    phase_plot = np.fft.fftshift(phase)

    plt.figure()
    plt.plot(tt - duration_s*0.5, yy, '.-')
    plt.xlabel('time [s]')
    plt.title(f'sinc(t) {freq}Hz')

    plt.figure()
    plt.plot(freqs_plot, mag_plot, '.-')
    plt.xlabel('freq [Hz?]')
    plt.title(f'freq mag of sinc(t) {freq}Hz')


def test_reconstruction():

    duration_s=3.0
    N_sinusoids=5
    create_wave = partial(create_wave_impl, duration_s=duration_s, N_sinusoids=N_sinusoids)


    N_true = int(2**10)
    N_sample = int(2**8)
    N_interp = int(2**10/2*3)
    period_true = duration_s / float(N_true)
    period_sample = duration_s / float(N_sample)
    period_interp = duration_s / float(N_interp)

    tt_true, yy_true = create_wave(period_true)
    tt_sample, yy_sample = create_wave(period_sample)

    coeffs = np.fft.fft(yy_sample)
    mag = np.absolute(coeffs)
    phase = np.angle(coeffs)
    freqs = np.fft.fftfreq(N_sample, d=period_sample)

    freqs_plot = np.fft.fftshift(freqs)
    mag_plot = np.fft.fftshift(mag)
    phase_plot = np.fft.fftshift(phase)

    plt.figure()
    plt.plot(freqs_plot, mag_plot, '.-')
    plt.xlabel('freq [Hz?]')
    plt.title('freq mag of example waveform')

    # plt.figure()
    # plt.plot(freqs_plot, phase_plot)
    # plt.title('phase')

    # reconstruction
    yy_recon = np.array([ np.sum([x_m*np.exp(2.j*np.pi*k*m/N_sample) for m, x_m in enumerate(coeffs)]) for k in range(N_sample) ]) / float(N_sample)

    def fft_interp(coeffs, freqs, t):
        # 1D FFT interpolate for arbitrary x
        # assume x is periodic on [0,1] interval
        # size = len(coeffs)
        # kn = np.fft.fftfreq(size)
        # eikx = np.exp(2.j*np.pi*t*size*kn)
        eikx = np.exp(2.j*np.pi* t * freqs)
        return np.dot(coeffs, eikx) / len(coeffs)


    tt_interp, _ = create_wave(period_interp)
    yy_interp = [fft_interp(coeffs, freqs, t) for t in tt_interp ]

    plt.figure()
    plt.plot(tt_true, yy_true, '-', label='true waveform')
    plt.plot(tt_sample, yy_recon, 'o', fillstyle='none', label='reconstruction from samples')
    plt.plot(tt_interp, yy_interp, '.', fillstyle='none', label='interpolation from samples')
    plt.xlabel('time [s]')
    plt.title('example waveform reconstruction')
    plt.legend()


def test_correlation():

    duration_s=3.0
    N_sinusoids=5
    create_wave = partial(create_wave_impl, duration_s=duration_s, N_sinusoids=N_sinusoids)

    N_true = int(2**10)
    N_sample = int(2**9)
    N_interp = int(2**12)
    period_true = duration_s / float(N_true)
    period_sample = duration_s / float(N_sample)
    period_interp = duration_s / float(N_interp)

    # create sinc
    sinc_center_s = 0.75 +0.01# duration_s*0.5 - 0.1
    sinc_amplitude = 3
    sinc_freq = 2
    _, sinc_sample = create_sinc(period_sample, 
                                 duration_s=duration_s, 
                                 center_s=0.5*duration_s, 
                                 amplitude=sinc_amplitude, 
                                 freq=sinc_freq)
    
    # create signal without sinc
    _, wave_sample_sans_sinc = create_wave(period_sample, 
                                         sinc_amplitude=None, 
                                         sinc_center_s=None, 
                                         sinc_freq=None)

    # create signal
    tt_sample, wave_sample = create_wave(period_sample, 
                                         sinc_amplitude=sinc_amplitude, 
                                         sinc_center_s=sinc_center_s, 
                                         sinc_freq=sinc_freq)

    # get coefficients of both signals and shift so negative freqs come first
    freqs = np.fft.fftfreq(N_sample, d=period_sample)
    freqs = np.fft.fftshift(freqs)

    wave_coeffs = np.fft.fft(wave_sample)
    wave_coeffs = np.fft.fftshift(wave_coeffs)

    sinc_coeffs = np.fft.fft(sinc_sample)
    sinc_coeffs = np.fft.fftshift(sinc_coeffs)

    correlate = partial(correlate_impl, 
                        wave_coeffs, 
                        sinc_coeffs, 
                        freqs, 
                        duration_s)


    plt.figure()
    plt.plot(tt_sample, sinc_sample, '.-', fillstyle='none', label='sinc(t)')
    plt.axvline(duration_s*0.5, color='r', linestyle='--', label='peak location')
    plt.xlabel('time [s]')
    plt.title('example sinc(t)')

    plt.figure()
    plt.plot(tt_sample, wave_sample_sans_sinc, '.-', fillstyle='none', label='env(t)')
    plt.plot(tt_sample, wave_sample, '.-', fillstyle='none', label='env(t)+sinc(t)')
    plt.axvline(sinc_center_s, color='r', linestyle='--', label='expected peak')
    plt.xlabel('time [s]')
    plt.title('example env(t) + sinc(t)')
    plt.legend()

    # create correlation surface
    for shrink_factor in [0.5, 0.01]:
        tt_true = duration_s*0.5 - sinc_center_s # exact solution

        tt_interp = np.linspace(tt_true + -duration_s*shrink_factor, 
                                tt_true +duration_s*shrink_factor, 
                                N_interp)
        corr_interp = [correlate(t) for t in tt_interp ]
        corr_interp_mag = corr_interp # np.absolute(corr_interp)
        corr_interp_angle = np.angle(corr_interp)


        tt_a = tt_true + -duration_s*shrink_factor
        tt_b = tt_true + duration_s*shrink_factor
        tt_sample = np.arange(N_sample) * period_sample - duration_s * 0.25 # all timestamps
        tt_sample = tt_sample[(tt_sample >= tt_a) & (tt_sample <= tt_b)]
        corr_sample = np.array([correlate(t) for t in tt_sample ])
        corr_sample_mag = corr_sample # np.absolute(corr_interp)
        corr_sample_angle = np.angle(corr_sample)

        # mask = (tt_sample <= np.max(tt_interp)) & (tt_sample >= np.min(tt_interp))

        plt.figure()
        plt.plot(tt_interp, corr_interp_mag, '-', fillstyle='none', label=r'corr$(\tau)$')
        plt.plot(tt_sample, corr_sample_mag, 'b.', fillstyle='none')
        plt.axvline(tt_true, color='r', linestyle='--', label='expected peak')
        plt.title(rf'example correlation surface around expected peak (+/-{shrink_factor*100}% of block period)')
        plt.xlabel('time [s]')
        plt.legend()

    # plt.figure()
    # plt.plot(tt_interp, corr_interp_angle, '-', fillstyle='none')
    # plt.title('correlation surface angle')


def test_correlation_realistic_numbers():

    block_size = 128
    cd_sample_freq = 44100
    duration_s=block_size/cd_sample_freq
    N_sinusoids=None
    sinusoid_amp_freq_list = [(1.0,10.987E3), (0.5,7.54E3), (0.7,7.23E3)]
    sinc_func = sinc
    create_wave = partial(create_wave_impl, 
                          duration_s=duration_s, 
                          sinusoid_amp_freq_list=sinusoid_amp_freq_list, 
                          sinc_func=sinc_func)

    N_sample = block_size
    N_interp = N_sample*100

    period_sample = duration_s / float(N_sample)


    # create sinc
    sinc_center_offset_index = 8.1
    sinc_center_s = (block_size*0.5+ sinc_center_offset_index)*period_sample
    sinc_amplitude = 3
    sinc_freq = 2E3
    _, sinc_sample = create_sinc(period_sample, 
                                 duration_s=duration_s, 
                                 center_s=0.5*duration_s, 
                                 amplitude=sinc_amplitude, 
                                 freq=sinc_freq, 
                                 func=sinc_func)

    # create signal without sinc
    tt_sample, wave_sample_sans_sinc = create_wave(period_sample, 
                                         sinc_amplitude=None, 
                                         sinc_center_s=None, 
                                         sinc_freq=None)
    
    # create signal
    tt_sample, wave_sample = create_wave(period_sample, 
                                         sinc_amplitude=sinc_amplitude, 
                                         sinc_center_s=sinc_center_s, 
                                         sinc_freq=sinc_freq)

    # get coefficients of both signals and shift so negative freqs come first
    freqs = np.fft.fftfreq(N_sample, d=period_sample)
    freqs = np.fft.fftshift(freqs)

    wave_coeffs = np.fft.fft(wave_sample)
    wave_coeffs = np.fft.fftshift(wave_coeffs)

    sinc_coeffs = np.fft.fft(sinc_sample)
    sinc_coeffs = np.fft.fftshift(sinc_coeffs)

    plt.figure()
    plt.plot(tt_sample, sinc_sample, '.-', fillstyle='none', label='sinc(t)')
    plt.axvline(duration_s*0.5, color='r', linestyle='--', label='peak location')
    plt.xlabel('time [s]')
    plt.title('realistic sinc(t)')
    plt.legend()

    plt.figure()
    plt.plot(tt_sample, wave_sample_sans_sinc, '.-', fillstyle='none', label='env(t)')
    plt.plot(tt_sample, wave_sample, '.-', fillstyle='none', label='env(t)+sinc(t)')
    plt.axvline(sinc_center_s, color='r', linestyle='--', label='expected peak')
    plt.xlabel('time [s]')
    plt.title('realistic env(t) + sinc(t)')
    plt.legend()

    tt_true = duration_s*0.5 - sinc_center_s # exact solution

    # create correlation surface
    for shrink_factor in [0.5, 0.01]:

        correlate = partial(correlate_impl, wave_coeffs, sinc_coeffs, freqs, duration_s)
        correlate_ifft = partial(correlate_ifft_impl, wave_coeffs, sinc_coeffs, freqs, duration_s)

        tt_a = tt_true + -duration_s*shrink_factor
        tt_b = tt_true + duration_s*shrink_factor
        # tt_sample = np.arange(tt_a, tt_b, period_sample)
        # tt_sample = tt_sample - tt_sample[len(tt_sample)//2] + tt_true # middle point is at 0
        tt_sample = np.arange(N_sample) * period_sample - duration_s * 0.5 # all timestamps
        tt_sample = tt_sample[(tt_sample >= tt_a) & (tt_sample <= tt_b)]
        tt_ifft = np.arange(N_sample) * period_sample - duration_s * 0.5 # all timestamps
        tt_ifft_mask = (tt_ifft >= tt_a) & (tt_ifft <= tt_b)
        
        tt_interp = np.linspace(tt_true + -duration_s*shrink_factor, 
                                tt_true +duration_s*shrink_factor, 
                                N_interp)
        
        corr_sample = [correlate(t) for t in tt_sample ]
        corr_sample_real = np.real(corr_sample)
        corr_interp = [correlate(t) for t in tt_interp ]
        corr_interp_real = np.real(corr_interp)
        corr_ifft_real = np.real(correlate_ifft())

        # compute derivative
        fprime = lambda tau: correlate(tau, derivative_order=1)
        corr_deriv1_sample = [fprime(t) for t in tt_sample ]
        corr_deriv1_sample_real = np.real(corr_deriv1_sample)
        corr_deriv1_interp = [fprime(t) for t in tt_interp ]
        corr_deriv1_interp_real = np.real(corr_deriv1_interp)
        corr_deriv1_ifft_real = np.real(correlate_ifft(derivative_order=1))

        fprime2 = lambda tau: correlate(tau, derivative_order=2)
        corr_deriv2_sample = [fprime2(t) for t in tt_sample ]
        corr_deriv2_sample_real = np.real(corr_deriv2_sample)
        corr_deriv2_interp = [fprime2(t) for t in tt_interp ]
        corr_deriv2_interp_real = np.real(corr_deriv2_interp)
        corr_deriv2_ifft_real = np.real(correlate_ifft(derivative_order=2))

        # find true maximum according to correlation surface
        optimal_tt_s = optimize.newton(fprime, tt_true, fprime=fprime2, maxiter=5)
        tt_residual_s = optimal_tt_s - tt_true
        print(f'tt_residual_s (magnitude) = {np.absolute(tt_residual_s)}')


        plt.figure()
        plt.axvline(tt_true, color='r', linestyle='--', label='expected peak')
        plt.plot(tt_interp, corr_interp_real, 'b-', fillstyle='none', label=r'corr$(\tau)$')
        plt.plot(tt_sample, corr_sample_real, 'b.', fillstyle='none')
        plt.plot(tt_ifft[tt_ifft_mask], corr_ifft_real[tt_ifft_mask], 'r.', fillstyle='none', label='ifft', markersize=18)
        plt.xlabel('time [s]')
        plt.title(f'realistic correlation surface around expected peak (+/-{shrink_factor*100}% of block period)')
        plt.legend()

        plt.figure()
        plt.axvline(tt_true, color='r', linestyle='--', label='expected peak')
        plt.plot(tt_interp, corr_deriv1_interp_real, 'g-', fillstyle='none', label=r'$\frac{d}{d\tau}$ corr$(\tau)$')
        plt.plot(tt_sample, corr_deriv1_sample_real, 'g.', fillstyle='none')
        plt.plot(tt_ifft[tt_ifft_mask], corr_deriv1_ifft_real[tt_ifft_mask], 'r.', fillstyle='none', label='ifft', markersize=18)
        plt.title(rf'realistic $\frac{{d}}{{d\tau}}$ correlation surface around expected peak (+/-{shrink_factor*100}% of block period)')
        plt.xlabel('time [s]')
        plt.legend()

        plt.figure()
        plt.axvline(tt_true, color='r', linestyle='--', label='expected peak')
        plt.plot(tt_interp, corr_deriv2_interp_real, 'k-', fillstyle='none', label=r'$\frac{d^2}{d\tau^2}$ corr$(\tau)$')
        plt.plot(tt_sample, corr_deriv2_sample_real, 'k.', fillstyle='none')
        plt.plot(tt_ifft[tt_ifft_mask], corr_deriv2_ifft_real[tt_ifft_mask], 'r.', fillstyle='none', label='ifft', markersize=18)
        plt.title(rf'realistic $\frac{{d^2}}{{d\tau^2}}$ correlation surface around expected peak (+/-{shrink_factor*100}% of block period)')
        plt.xlabel('time [s]')
        plt.legend()

    plt.figure()
    plt.plot(freqs, np.absolute(wave_coeffs), 'r.-', label='coeff env(t)+sinc(t)')
    plt.plot(freqs, np.absolute(sinc_coeffs), 'b.-', label='coeff sinc(t)')
    plt.xlabel('freq [Hz?]')
    plt.legend()
    plt.title('realistic frequency coefficient magnitude')


def make_performance_plots():

    def get_tau_residual(args):

        N_sample = args['block_size']
        cd_sample_freq = args['cd_sample_freq']
        sinusoid_amp_freq_list = args['sinusoid_amp_freq_list']

        sinc_func = args['sinc_func']
        sinc_center_offset_index = args['sinc_center_offset_index']
        sinc_amplitude = args['sinc_amplitude']
        sinc_freq = args['sinc_freq']

        use_tau_hint = args['use_tau_hint']

        # compute other args
        duration_s = N_sample/cd_sample_freq
        period_sample = 1.0 / cd_sample_freq

        # create sinc
        sinc_center_s = (N_sample*0.5 + sinc_center_offset_index) * period_sample
        _, sinc_sample = create_sinc(period_sample, 
                                     duration_s=duration_s, 
                                     center_s=0.5*duration_s, 
                                     amplitude=sinc_amplitude, 
                                     freq=sinc_freq, 
                                     func=sinc_func)

        # create signal
        tt_sample, wave_sample = create_wave_impl(period_sample, 
                                                  duration_s=duration_s, 
                                                  sinusoid_amp_freq_list=sinusoid_amp_freq_list, 
                                                  sinc_func=sinc_func,
                                                  sinc_amplitude=sinc_amplitude, 
                                                  sinc_center_s=sinc_center_s, 
                                                  sinc_freq=sinc_freq)

        # get coefficients of both signals and shift so negative freqs come first
        freqs, wave_coeffs = perform_fft_and_shift_and_normalize(period_sample, wave_sample)
        _, sinc_coeffs = perform_fft_and_shift_and_normalize(period_sample, sinc_sample)

        # find true maximum according to correlation surface
        correlate = partial(correlate_impl, wave_coeffs, sinc_coeffs, freqs, duration_s, is_normalized=True)
        fprime = lambda tau: np.real(correlate(tau, derivative_order=1))
        fprime2 = lambda tau: np.real(correlate(tau, derivative_order=2))

        tau_true = duration_s*0.5 - sinc_center_s # exact solution

        if use_tau_hint:
            tau_guess = tau_true
        else:
            taus_brute_force = np.arange(-N_sample/2*period_sample, 
                                         N_sample/2*period_sample,
                                         period_sample*1)
            
            corr = np.real(correlate(taus_brute_force))
            tau_guess = taus_brute_force[np.argmax(corr)]

            # print(f'tau_true = {tau_true}')
            # print(f'tau_guess = {tau_guess}')

        max_iterations = 10
        optimal_tau_s, results = optimize.newton(fprime, tau_guess, 
                                                 fprime=fprime2, 
                                                 maxiter=max_iterations, 
                                                 full_output=True, disp=False)
        optimal_tau_s = optimal_tau_s[0]
        if not results.converged or results.iterations==max_iterations:
            optimal_tau_s = np.nan
        tau_residual_s = optimal_tau_s - tau_true
        if np.abs(tau_residual_s) > period_sample:
            tau_residual_s = np.nan

        if np.isnan(tau_residual_s):

            if args['plot_first_failure']:
                plt.figure()
                plt.plot(tt_sample, sinc_sample, '-', fillstyle='none')
                plt.axvline(duration_s*0.5, color='r', linestyle='--')
                plt.title('sinc(t)')

                plt.figure()
                plt.plot(tt_sample, wave_sample, '-', fillstyle='none')
                plt.axvline(sinc_center_s, color='r', linestyle='--')
                plt.title('wave(t) + sinc(t)')
                
                plt.figure()
                plt.axvline(tau_true, color='r', linestyle='--')
                plt.axvline(tau_guess, color='k', linestyle='-')
                plt.axvline(optimal_tau_s, color='g', linestyle='--')
                plt.plot(taus_brute_force, corr, 'b-', fillstyle='none')
                plt.title(f'correlation surface brute force search')
                plt.legend(['tau_true', 'tau_guess', 'tau_optimal', r'$surface(\tau)$'])
                plt.show()
            pass
        
        tau_residual_normalized = tau_residual_s / period_sample
        results = {'tau_residual_s': tau_residual_s, 'tau_residual_normalized': tau_residual_normalized}
        return results


    def run_sweep(result_dict_name, title, default_args, arg_name, sweep_values, arg2_name=None, sweep2_values=None):

        # plt.figure()
        fig, ax1 = plt.subplots(figsize=(12.8, 9.6))

        title_string = f'{title} - block_size={default_args['block_size']}'

        period_sample = 1.0 / default_args['cd_sample_freq']

        if arg2_name is not None:
            # form colormap
            n_colors = len(sweep2_values)

            color_A = (0, 0, 0)
            color_B = (1, 0, 0)
            # cm = plt.cm.jet
            cm = LinearSegmentedColormap.from_list("Custom", [color_A, color_B], N=n_colors)
            colors = cm(np.linspace(0,1,n_colors))

            # plot 1 line for each secondary parameter
            legend_strings = []
            for val2, color in zip(sweep2_values, colors):
                sweep_results = [get_tau_residual({**default_args, arg_name: val, arg2_name: val2})[result_dict_name] for val in sweep_values]
                if any(np.isnan(sweep_results)):
                    print(f'{title_string}:')
                    print(f'failure at {arg2_name}={val2}, {arg_name}={np.array(sweep_values)[np.isnan(sweep_results)]}')
                # for result, val1 in zip(sweep_results, sweep_values):
                legend_strings.append(f'{arg2_name}={val2} ({np.sum(np.isnan(sweep_results))} failures)')
                ax1.plot(sweep_values, sweep_results, color=color)

            # apply legend
            plt.legend(legend_strings, loc='upper right')

        else:
            sweep_results = [get_tau_residual({**default_args, arg_name: val})[result_dict_name] for val in sweep_values]
            ax1.plot(sweep_values, sweep_results)

        # try to plot this variable with distance units
        delta_tau_to_delta_dist = None
        if result_dict_name == 'tau_residual_s':
            delta_tau_to_delta_dist = 343*1E3 # convert delta time to delta distance
        elif result_dict_name == 'tau_residual_normalized':
            delta_tau_to_delta_dist = period_sample * 343*1E3 # convert delta time to delta distance
        if delta_tau_to_delta_dist is not None:
            ax2 = ax1.twinx()
            y1, y2 = ax1.get_ylim()
            ax2.set_ylim(y1*delta_tau_to_delta_dist, y2*delta_tau_to_delta_dist)
            ax2.set_ylabel('distance residual [mm]')

        ax1.set_xlabel(arg_name)
        ax1.set_ylabel(result_dict_name)
        plt.title(title_string)


    default_args = {
        'block_size':             128*2,
        'cd_sample_freq':         44100,
        'sinusoid_amp_freq_list': [(1.0,10.987E3), (0.5,7.54E3), (0.7,7.23E3)],
        'sinc_func':              sinc,
        'sinc_center_offset_index': 8.1, # swept
        'sinc_amplitude': 3,
        'sinc_freq': 2E3, # swept
        'use_tau_hint': False,
        'plot_first_failure': False,
    }

    run_sweep('tau_residual_normalized', 'test', default_args, 'sinc_center_offset_index', np.linspace(-3.5, +3.5, 100))
    # run_sweep('tau_residual_normalized', default_args, 'sinc_center_offset_index', np.linspace(-3.5, +3.5, 100), 'sinc_freq', np.linspace(2E3, 12E3, 10*2+1))
    run_sweep('tau_residual_normalized', 'wave_freqs_manually_set', default_args, 'sinc_center_offset_index', np.linspace(-64, +64, 1000), 'sinc_freq', np.linspace(2E3, 7E3, 10*1+1))
    run_sweep('tau_residual_normalized', 'wave_freqs_manually_set', default_args, 'sinc_center_offset_index', np.linspace(-64, +64, 1000), 'sinc_freq', np.linspace(7E3, 12E3, 5*1+1))


    rng = np.random.default_rng(seed=9)
    default_args = {
        'block_size':             128*2,
        'cd_sample_freq':         44100,
        'sinusoid_amp_freq_list': [(rng.random(),15*rng.random()) for _ in range(20)],
        'sinc_func':              sinc,
        'sinc_center_offset_index': 8.1, # swept
        'sinc_amplitude': 3,
        'sinc_freq': 2E3, # swept
        'use_tau_hint': False,
        'plot_first_failure': False,
    }
    run_sweep('tau_residual_normalized', 'wave_freqs_randomly_set', default_args, 'sinc_center_offset_index', np.linspace(-64, +64, 1000), 'sinc_freq', np.linspace(2E3, 7E3, 10*1+1))
    run_sweep('tau_residual_normalized', 'wave_freqs_randomly_set', default_args, 'sinc_center_offset_index', np.linspace(-64, +64, 1000), 'sinc_freq', np.linspace(7E3, 12E3, 5*1+1))


    plt.figure()
    sinc_freqs = np.linspace(2E3, 12E3, 10*1+1)
    sinc_coeffs_list = []
    for sinc_freq in sinc_freqs:

        # compute other args
        sinc_amplitude = 1.0
        sinc_func = sinc
        N_sample = 128*2
        duration_s = N_sample/default_args['cd_sample_freq']
        period_sample = 1.0 / default_args['cd_sample_freq']

        # create sinc
        _, sinc_sample = create_sinc(period_sample, 
                                     duration_s=duration_s, 
                                     center_s=0.5*duration_s, 
                                     amplitude=sinc_amplitude, 
                                     freq=sinc_freq, 
                                     func=sinc_func)

        # get coefficients of both signals and shift so negative freqs come first
        _, sinc_coeffs = perform_fft_and_shift_and_normalize(period_sample, sinc_sample)
        sinc_coeffs_list.append(sinc_coeffs)

        plt.plot(np.absolute(sinc_coeffs))
    plt.xlabel('fourier frequency')
    plt.ylabel('coefficient magnitude')
    plt.legend([f'sinc_freq={sinc_freq}' for sinc_freq in sinc_freqs])



    plt.figure()
    N_audio_channels = 2
    N_bytes_per_coeff = 4*2
    seconds_per_block = period_sample*128
    byte_rates = np.array([np.sum(sinc_coeffs > 1E-3) * N_bytes_per_coeff * N_audio_channels / seconds_per_block for sinc_coeffs in sinc_coeffs_list])
    plt.plot(sinc_freqs, byte_rates *1E-3)
    plt.xlabel('sinc freq [Hz]')
    plt.ylabel('byte rate [KB/s]')
    plt.title('required byte rate for different sinc frequencies (2 channel audio)')
        


if __name__ == "__main__":

    show_sinc(freq=10)
    show_sinc(freq=20)
    test_reconstruction()
    test_correlation()
    test_correlation_realistic_numbers()
    make_performance_plots()


    multipage(folder_name='saved_figs', prefix='')
    plt.show()
