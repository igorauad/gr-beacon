/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(beacon_sink_c.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(726c105f35ef67f618698d0806d1231b)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/beacon/beacon_sink_c.h>
// pydoc.h is automatically generated in the build directory
#include <beacon_sink_c_pydoc.h>

void bind_beacon_sink_c(py::module& m)
{

    using beacon_sink_c = gr::beacon::beacon_sink_c;


    py::class_<beacon_sink_c,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<beacon_sink_c>>(m, "beacon_sink_c", D(beacon_sink_c))

        .def(py::init(&beacon_sink_c::make),
             py::arg("log_period"),
             py::arg("fft_len"),
             py::arg("alpha"),
             py::arg("samp_rate"),
             D(beacon_sink_c, make))

        .def("get_cnr", &beacon_sink_c::get_cnr, D(beacon_sink_c, get_cnr))

        .def("get_freq", &beacon_sink_c::get_freq, D(beacon_sink_c, get_freq))

        ;
}