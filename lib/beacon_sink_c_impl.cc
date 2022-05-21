/* -*- c++ -*- */
/*
 * Copyright 2020 Blockstream Corp..
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "beacon_sink_c_impl.h"
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>
#include <volk/volk.h>
#include <iostream>

namespace gr {
namespace beacon {

/**
 * @brief Compute the equivalent noise bandwidth (ENBW) of a window.
 * @param window Vector with the window taps.
 * @param Nfft FFT length.
 * @return (float) ENBW in dB.
 */
static float calc_enbw(std::vector<float>& window, int Nfft)
{
    float sum_abs_sq = 0;
    float abs_sum_sq = 0;
    for (auto& tap : window) {
        sum_abs_sq += fabs(tap * tap);
        abs_sum_sq += tap;
    }
    abs_sum_sq = fabs(abs_sum_sq * abs_sum_sq);
    return 10 * log10(Nfft * sum_abs_sq / abs_sum_sq);
}

/**
 * @brief Print FFT results for debugging.
 * @param p_fft pointer to FFT buffer.
 * @param N FFT length.
 * @return Void.
 */
static void print_fft(float* p_fft, int N)
{
    std::cout << "[";
    for (int i = 0; i < (N - 1); i++) {
        std::cout << p_fft[i] << ", ";
    }
    std::cout << p_fft[N - 1] << "]" << std::endl;
}

beacon_sink_c::sptr
beacon_sink_c::make(float log_period, int fft_len, float alpha, float samp_rate)
{
    return gnuradio::get_initial_sptr(
        new beacon_sink_c_impl(log_period, fft_len, alpha, samp_rate));
}

/*
 * The private constructor
 */
beacon_sink_c_impl::beacon_sink_c_impl(float log_period,
                                       int fft_len,
                                       float alpha,
                                       float samp_rate)
    : gr::sync_block("beacon_sink_c",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(0, 0, 0)),
      d_log_period(log_period),
      d_fft_len(fft_len),
      d_half_fft_len(fft_len / 2),
      d_alpha(alpha),
      d_beta(1 - alpha),
      d_samp_rate(samp_rate),
      d_cnr(0),
      d_freq(0)
{
    set_output_multiple(fft_len);
    d_fft = new fft::fft_complex(fft_len, true);
    d_mag_buffer = (float*)volk_malloc(fft_len * sizeof(float), volk_get_alignment());
    d_avg_buffer = (float*)volk_malloc(fft_len * sizeof(float), volk_get_alignment());
    d_i_max_buffer = (uint32_t*)volk_malloc(sizeof(uint32_t), volk_get_alignment());
    d_accum_buffer = (float*)volk_malloc(sizeof(float), volk_get_alignment());
    memset(d_avg_buffer, 0, fft_len * sizeof(float));

    // Use a flat-top window given that it is one of the few windows that
    // presents negligible scalloping loss. This window has a relatively wide
    // main lobe and, therefore, offers a poor frequency resolution. However,
    // this limitation is not a big concern here given that the goal is to
    // estimate the power of a single sinusoid (the CW beacon). Meanwhile, the
    // flat-top window's peak side lobe level is around -86 dB, which is good
    // enough for measuring practical beacon CNR levels. More importantly, its
    // scalloping loss is negligible, which allows for measuring the beacon
    // power level well even if the beacon frequency does not align with the
    // frequency of an FFT bin.
    d_window = filter::firdes::window(
        filter::firdes::WIN_FLATTOP, fft_len, 0 /* unused param */);

    // Compute the window's ENBW in dB, which is used to compensate for the
    // windowing-induced increase in noise floor.
    d_win_enbw = calc_enbw(d_window, fft_len);
}

/*
 * Our virtual destructor.
 */
beacon_sink_c_impl::~beacon_sink_c_impl()
{
    volk_free(d_mag_buffer);
    volk_free(d_avg_buffer);
    volk_free(d_i_max_buffer);
    volk_free(d_accum_buffer);
}

struct block_res beacon_sink_c_impl::process_block(const gr_complex* in)
{
    /* Windowing */
    volk_32fc_32f_multiply_32fc(d_fft->get_inbuf(), in, &d_window.front(), d_fft_len);

    /* FFT */
    d_fft->execute();

    /* Average |FFT|^2 (squared FFT magnitude) */
    volk_32fc_magnitude_squared_32f(d_mag_buffer, d_fft->get_outbuf(), d_fft_len);
    volk_32f_s32f_multiply_32f(d_mag_buffer, d_mag_buffer, d_alpha, d_fft_len);
    volk_32f_s32f_multiply_32f(d_avg_buffer, d_avg_buffer, d_beta, d_fft_len);
    volk_32f_x2_add_32f(d_avg_buffer, d_avg_buffer, d_mag_buffer, d_fft_len);

    /* Peak detection */
    volk_32f_index_max_32u(d_i_max_buffer, d_avg_buffer, d_fft_len);
    uint32_t i_max = *d_i_max_buffer;

    /* Skip a range of indexes around the CW peak to measure the noise floor */
    uint32_t N = 8;
    // NOTE 1: to facilitate the indexing, consider that i_s and i_e are
    // exclusive (they are outside the peak region already). Hence, assume there
    // are N-1 samples on each side of the peak, with a total of 2*N -1 samples
    // composing the peak region.
    //
    // NOTE 2: the peak is expected to decay rapidly with proper
    // windowing. Hence, consider only a few neighbor indexes.
    uint32_t i_s = (i_max - N) % d_fft_len;
    uint32_t i_e = (i_max + N) % d_fft_len;

    /* Obtain the noise floor by averaging all FFT mag values **out** of the
     * peak region. Sum values before and after the end of the peak region. */
    float noise_accum = 0;
    uint32_t n_points;
    if (i_s > i_e) {
        // When i_s is beyond i_e, it is because i_s wrapped around
        // Accumulate range [i_e to i_s]
        volk_32f_accumulator_s32f(d_accum_buffer, d_avg_buffer + i_e, (i_s - i_e));
        noise_accum += *d_accum_buffer;
        assert((i_s - i_e) == (d_fft_len - (2 * N - 1)));
    } else {
        // Accumulate range [i_e, fft_len)
        volk_32f_accumulator_s32f(d_accum_buffer, d_avg_buffer + i_e, (d_fft_len - i_e));
        noise_accum += *d_accum_buffer;
        // Accumulate range [0, i_s]
        volk_32f_accumulator_s32f(d_accum_buffer, d_avg_buffer, i_s);
        noise_accum += *d_accum_buffer;
        assert(((d_fft_len - i_e) + i_s) == (d_fft_len - (2 * N - 1)));
    }
    float noise_floor = noise_accum / (d_fft_len - (2 * N - 1));

    // Final CNR measurement
    //
    // NOTE: the observed peak level includes noise. That is, it represents
    // "C+N", not just "C". Hence, compute the CNR as follows:
    //
    //   C/N = (C+N)/N - 1
    float cnr_lin = (d_avg_buffer[i_max] / noise_floor) - 1;

    // On the final C/N level in dB, compensate for the increase in noise floor
    // induced by the non-rectangular window. The increase in noise floor is
    // equivalent to the window's ENBW. Hence, the measured C/N in db inherently
    // has a -ENBW term (minus due to noise floor being in the denominator). We
    // add ENBW back to compensate for this term.
    float cnr = 10 * log10(cnr_lin) + d_win_enbw;

    // Beacon/carrier frequency
    int i_max_wrapped = (i_max > d_half_fft_len) ? i_max - d_fft_len : i_max;
    float freq = i_max_wrapped * (d_samp_rate / d_fft_len);

    return { cnr, freq };
}

int beacon_sink_c_impl::work(int noutput_items,
                             gr_vector_const_void_star& input_items,
                             gr_vector_void_star& output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    int n_blocks = noutput_items / d_fft_len;

    // Process one FFT block at a time
    for (int i_block = 0; i_block < n_blocks; i_block++) {
        const auto res = process_block(in);
        // Given that the FFT is already averaged inside `process_block()`, the
        // resulting CNR and frequency measurements are averaged results.
        d_cnr = res.cnr;
        d_freq = res.freq;
    }

    /* Print wrapped index periodically if so desired */
    std::chrono::time_point<std::chrono::system_clock> t_now =
        std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = t_now - d_t_last_print;
    if (d_log_period > 0 && elapsed.count() > d_log_period) {
        std::time_t t_c = std::chrono::system_clock::to_time_t(t_now);
        std::cout << std::put_time(std::gmtime(&t_c), "%F %T") << " "
                  << " Freq: " << d_freq << " Hz"
                  << " CNR: " << d_cnr << " dB" << std::endl;
        d_t_last_print = t_now;
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace beacon */
} /* namespace gr */
