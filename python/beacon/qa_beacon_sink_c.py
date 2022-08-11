#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2022 Blockstream Corp..
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

from math import sqrt
from gnuradio import gr, gr_unittest
from gnuradio import analog, blocks
try:
    from gnuradio.beacon import beacon_sink_c
except ImportError:
    import os
    import sys
    dirname, filename = os.path.split(os.path.abspath(__file__))
    sys.path.append(os.path.join(dirname, "bindings"))
    from gnuradio.beacon import beacon_sink_c


class qa_beacon_sink_c(gr_unittest.TestCase):

    def setUp(self):
        self.tb = gr.top_block()

    def tearDown(self):
        self.tb = None

    def _test_sine_wave(self,
                        cnr_db,
                        freq,
                        samp_rate,
                        Nfft,
                        alpha,
                        A=1.0,
                        n_blocks=1000,
                        tol=0.5):
        """Test CNR estimation for a "complex wave", i.e., exp(j2*pi*F*n)

        Args:
            cnr_db    : Target CNR in dB
            freq      : Sine-wave frequency in Hz
            samp_rate : Sample rate in Hz
            Nfft      : FFT length
            alpha     : Exponentially-weighted moving average coefficient
            A         : Amplitude
            n_blocks  : Number of FFT windows to simulate (determines
                        the duration or number of samples in the simulation)
            tol       : CNR measurement error tolerance in dB
        """
        # 1) Define the noise floor to achieve a target CNR
        #
        # There are multiple possibilities for the power spectral density (PSD)
        # magnitude seen on the carrier frequency, depending on how the PSD is
        # computed. Assuming wave "A * exp(j*2*pi*f*t)" has frequency "f" that
        # is perfectly matched to an FFT bin (an integer multiple of the FFT
        # frequency spacing) and that there is no leakage, the FFT magnitude
        # (i.e., |FFT|) seen on the FFT bin corresponding to the carrier
        # frequency becomes "A*Nfft". This holds for the standard
        # (non-orthonormal) FFT computation. Correspondingly, the PSD
        # computations below lead to:
        #
        # 1) |FFT|^2 -> Magnitude "(A*Nfft)^2" on the carrier bin.
        # 2) |FFT|^2 / Nfft -> Magnitude "(A^2)*Nfft" on the carrier bin.
        # 3) |FFT|^2 / Nfft^2 -> Magnitude "A^2" on the carrier bin.
        #
        # The beacon sink module uses computation #1. Hence, it follows that:
        carrier_psd = (A * Nfft)**2
        noise_floor = carrier_psd / (10**(cnr_db / 10))

        # For each PSD computaion above, there is a corresponding way to
        # compute the total power, summarized below:
        #
        # 1) P[k] = |FFT[k]|^2 ->  sum( P[k] / Nfft^2 )  for all k
        # 2) P[k] = |FFT[k]|^2 / Nfft -> sum( P[k] / Nfft )  for all k
        # 3) P[k] = |FFT[k]|^2 / Nfft^2 -> sum( P[k] )  for all k
        #
        # Given that we use computation #1, the noise power for a flat white
        # noise ("P[k] = noise_floor" for all k) becomes:
        noise_power = noise_floor / Nfft
        noise_std = sqrt(noise_power)

        # Beacon sink block parameters
        log_period = 0

        # Time-limit the sine-wave to a target number of FFT blocks
        n_samples = n_blocks * Nfft

        # Flowgraph
        src = analog.sig_source_c(samp_rate, analog.GR_SIN_WAVE, freq, A)
        noise = analog.noise_source_c(analog.GR_GAUSSIAN, noise_std, 0)
        nadder = blocks.add_cc()
        head = blocks.head(gr.sizeof_gr_complex, int(n_samples))
        beacon_sink = beacon_sink_c(log_period, Nfft, alpha, samp_rate)
        self.tb.connect(src, head, (nadder, 0))
        self.tb.connect(noise, (nadder, 1))
        self.tb.connect(nadder, beacon_sink)
        self.tb.run()

        # The beacon sink block finds the carrier peak using an FFT. Hence, the
        # observed frequency will depend on the FFT frequency spacing.
        delta_f = samp_rate / Nfft
        expected_freq = round(freq / delta_f) * delta_f

        # Due to the flat-top window, the power level seen on the adjacent bins
        # around the peak can be similar to the actual peak, causing errors
        # within +-delta_f on the frequency estimate. This is an acceptable
        # price to be paid for using the flat-top window, which allows for
        # better CNR estimates avoiding scalloping loss effects.
        self.assertAlmostEqual(beacon_sink.get_freq(),
                               expected_freq,
                               delta=delta_f)

        # Check CNR estimate
        self.assertAlmostEqual(beacon_sink.get_cnr(), cnr_db, delta=tol)

    def test_fft_aligned_cw_frequency(self):
        # Generate a frequency aligned with an FFT bin
        alpha = 0.0005
        samp_rate = 240e3
        Nfft = 512
        deltaf = samp_rate / Nfft
        freq = round(0.1 * Nfft) * deltaf
        n_blocks = int(1 / alpha)  # generate enough FFT blocks to average them
        for cnr_db in range(10, 40):
            with self.subTest(cnr_db=cnr_db):
                self._test_sine_wave(cnr_db,
                                     freq,
                                     samp_rate,
                                     Nfft,
                                     alpha,
                                     n_blocks=n_blocks)

    def test_non_fft_aligned_cw_frequency(self):
        # Generate a frequency falling halfway between two FFT bins
        alpha = 0.0005
        samp_rate = 240e3
        Nfft = 512
        deltaf = samp_rate / Nfft
        aligned_freq = round(0.1 * Nfft) * deltaf
        freq = aligned_freq + (deltaf / 2.1)
        # NOTE: freq is almost halfway, but slightly before the midpoint to
        # avoid rounding ambiguities
        n_blocks = int(1 / alpha)  # generate enough FFT blocks to average them
        for cnr_db in range(10, 40):
            with self.subTest(cnr_db=cnr_db):
                self._test_sine_wave(cnr_db,
                                     freq,
                                     samp_rate,
                                     Nfft,
                                     alpha,
                                     n_blocks=n_blocks)


if __name__ == '__main__':
    gr_unittest.run(qa_beacon_sink_c)
