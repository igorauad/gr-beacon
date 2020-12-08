/* -*- c++ -*- */

#define BEACON_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "beacon_swig_doc.i"

%{
#include "beacon/beacon_sink_c.h"
%}


%include "beacon/beacon_sink_c.h"
GR_SWIG_BLOCK_MAGIC2(beacon, beacon_sink_c);
