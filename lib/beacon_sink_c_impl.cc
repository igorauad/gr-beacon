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
#include <gnuradio/io_signature.h>
#include <volk/volk.h>
#include <iostream>

namespace gr {
namespace beacon {

beacon_sink_c::sptr beacon_sink_c::make(
    float log_period, float ref_ampl, float ref_cnr, int fft_len, float alpha)
{
    return gnuradio::get_initial_sptr(
        new beacon_sink_c_impl(log_period, ref_ampl, ref_cnr, fft_len, alpha));
}

/*
 * The private constructor
 */
beacon_sink_c_impl::beacon_sink_c_impl(
    float log_period, float ref_ampl, float ref_cnr, int fft_len, float alpha)
    : gr::sync_block("beacon_sink_c",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(0, 0, 0)),
      d_log_period(log_period),
      d_ref_ampl(ref_ampl),
      d_ref_cnr(ref_cnr),
      d_fft_len(fft_len),
      d_alpha(alpha),
      d_beta(1 - alpha),
      d_avg_ampl(0),
      d_cnr(0)
{
    set_output_multiple(fft_len);
    d_fft = new fft::fft_complex(fft_len, true);
    d_mag_buffer = (float*)volk_malloc(fft_len * sizeof(float), volk_get_alignment());
    d_avg_buffer = (float*)volk_malloc(fft_len * sizeof(float), volk_get_alignment());
    d_i_max_buffer = (uint32_t*)volk_malloc(sizeof(uint32_t), volk_get_alignment());
    d_accum_buffer = (float*)volk_malloc(sizeof(float), volk_get_alignment());
    memset(d_avg_buffer, 0, fft_len * sizeof(float));
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

float beacon_sink_c_impl::process_block(const gr_complex* in)
{
    /* FFT */
    memcpy(d_fft->get_inbuf(), in, sizeof(gr_complex) * d_fft_len);
    d_fft->execute();

    /* Magnitude and average magnitude */
    volk_32fc_magnitude_squared_32f(d_mag_buffer, d_fft->get_outbuf(), d_fft_len);
    volk_32f_s32f_multiply_32f(d_mag_buffer, d_mag_buffer, d_alpha, d_fft_len);
    volk_32f_s32f_multiply_32f(d_avg_buffer, d_avg_buffer, d_beta, d_fft_len);
    volk_32f_x2_add_32f(d_avg_buffer, d_avg_buffer, d_mag_buffer, d_fft_len);

    /* Peak detection */
    volk_32f_index_max_32u(d_i_max_buffer, d_avg_buffer, d_fft_len);
    uint32_t i_max = *d_i_max_buffer;

    /* Assign an range of index belonging to the peak CW */
    uint32_t N = 32;
    // NOTE: to facilitate the indexing, consider that i_s and i_e are exclusive
    // (they are outside the peak region already). Hence, assume there are N-1
    // samples on each side of the peak, with a total of 2*N -1 samples
    // composing the peak region.
    uint32_t i_s = (i_max - N) % d_fft_len;
    uint32_t i_e = (i_max + N) % d_fft_len;

    /* Obtain the noise floor by averaging all FFT mag values out of the peak
     * region. Sum values before and after the end of the peak region. */
    float noise_accum = 0;
    uint32_t n_points;
    if (i_s > i_e) {
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

    // CNR
    float cnr = 10 * log10(d_avg_buffer[i_max] / noise_floor);
    return cnr;
}

int beacon_sink_c_impl::work(int noutput_items,
                             gr_vector_const_void_star& input_items,
                             gr_vector_void_star& output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    int n_blocks = noutput_items / d_fft_len;

    // Process one FFT block at a time
    float cnr;
    for (int i_block = 0; i_block < n_blocks; i_block++) {
        cnr = process_block(in);
    }

    // Find average amplitude
    for (int i = 0; i < noutput_items; i++) {
        d_avg_ampl = (d_beta * d_avg_ampl) + (d_alpha * fabs(in[0]));
    }

    /* Print wrapped index periodically if so desired */
    std::chrono::time_point<std::chrono::system_clock> t_now =
        std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = t_now - d_t_last_print;
    if (d_log_period > 0 && elapsed.count() > d_log_period) {
        std::time_t t_c = std::chrono::system_clock::to_time_t(t_now);
        std::cout << std::put_time(std::gmtime(&t_c), "%F %T") << " "
                  << "Average amplitude: " << d_avg_ampl << "\tCNR: " << cnr << std::endl;
        d_t_last_print = t_now;
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace beacon */
} /* namespace gr */
