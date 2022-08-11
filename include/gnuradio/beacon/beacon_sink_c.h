/* -*- c++ -*- */
/*
 * Copyright 2022 Blockstream Corp..
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_BEACON_BEACON_SINK_C_H
#define INCLUDED_BEACON_BEACON_SINK_C_H

#include <gnuradio/beacon/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace beacon {

/*!
 * \brief <+description of block+>
 * \ingroup beacon
 *
 */
class BEACON_API beacon_sink_c : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<beacon_sink_c> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of beacon::beacon_sink_c.
     * \param log_period Period to log amplitude and CNR measurements.
     * \param fft_len FFT length used to compute the power spectral density (PSD).
     * \param alpha Coefficient for exponentially-weighted moving average of PSD
     * measurements.
     * \param samp_rate Sampling rate.
     */
    static sptr make(float log_period, int fft_len, float alpha, float samp_rate);

    /*!
     * \brief Get the most recent CNR measurement
     * \return (float) CNR
     */
    virtual float get_cnr() = 0;

    /*!
     * \brief Get the most recent carrier (or CW beacon) frequency measurement
     * \return (float) Frequency in Hz
     */
    virtual float get_freq() = 0;
};

} // namespace beacon
} // namespace gr

#endif /* INCLUDED_BEACON_BEACON_SINK_C_H */
