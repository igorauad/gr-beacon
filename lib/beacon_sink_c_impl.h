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

#ifndef INCLUDED_BEACON_BEACON_SINK_C_IMPL_H
#define INCLUDED_BEACON_BEACON_SINK_C_IMPL_H

#include <gnuradio/fft/fft.h>
#include <beacon/beacon_sink_c.h>
#include <chrono>

namespace gr {
namespace beacon {

struct block_res {
    float cnr;
    float freq;
};

class beacon_sink_c_impl : public beacon_sink_c
{
private:
    /* Input Parameters */
    float d_log_period;
    int d_fft_len;
    int d_half_fft_len;
    float d_alpha;
    float d_beta;
    float d_samp_rate;

    float d_cnr;
    float d_freq;
    fft::fft_complex* d_fft;
    std::chrono::time_point<std::chrono::system_clock> d_t_last_print;

    /* Volk Buffers */
    float* d_mag_buffer;
    float* d_avg_buffer;
    uint32_t* d_i_max_buffer;
    float* d_accum_buffer;

    std::vector<float> d_window;
    float d_win_enbw; // Window equivalent noise bandwidth (ENBW) in dB

    struct block_res process_block(const gr_complex* in);

public:
    beacon_sink_c_impl(float log_period, int fft_len, float alpha, float samp_rate);
    ~beacon_sink_c_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);

    float get_cnr() { return d_cnr; };
    float get_freq() { return d_freq; };
};

} // namespace beacon
} // namespace gr

#endif /* INCLUDED_BEACON_BEACON_SINK_C_IMPL_H */
