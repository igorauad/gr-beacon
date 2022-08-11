/* -*- c++ -*- */
/*
 * Copyright 2022 Blockstream Corp..
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_BEACON_BEACON_SINK_C_IMPL_H
#define INCLUDED_BEACON_BEACON_SINK_C_IMPL_H

#include <gnuradio/beacon/beacon_sink_c.h>
#include <gnuradio/fft/fft.h>
#include <iomanip>

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
    uint32_t d_half_fft_len;
    float d_alpha;
    float d_beta;
    float d_samp_rate;

    float d_cnr;
    float d_freq;
    fft::fft_complex_fwd* d_fft;
    std::chrono::time_point<std::chrono::system_clock> d_t_last_print;

    /* Volk Buffers */
    volk::vector<float> d_mag_buffer;
    volk::vector<float> d_avg_buffer;
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
