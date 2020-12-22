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

#ifndef INCLUDED_BEACON_BEACON_SINK_C_H
#define INCLUDED_BEACON_BEACON_SINK_C_H

#include <gnuradio/sync_block.h>
#include <beacon/api.h>

namespace gr {
namespace beacon {

/*!
 * \brief Beacon receiver
 * \ingroup beacon
 *
 */
class BEACON_API beacon_sink_c : virtual public gr::sync_block
{
public:
    typedef boost::shared_ptr<beacon_sink_c> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of beacon::beacon_sink_c.
     * \param log_period Period to log amplitude and CNR measurements.
     * \param fft_len FFT length used to compute the power spectral density (PSD).
     * \param alpha Coefficient for exponentially-weighted moving average of PSD
     * measurements.
     */
    static sptr make(float log_period, int fft_len, float alpha);
};

} // namespace beacon
} // namespace gr

#endif /* INCLUDED_BEACON_BEACON_SINK_C_H */
