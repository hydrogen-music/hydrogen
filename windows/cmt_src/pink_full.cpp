/* pink_full.cpp

   A full-frequency pink noise generator.

   (c) 2002 Nathaniel Virgo

   Part of the Computer Music Toolkit - a library of LADSPA plugins. 
   The Computer Music Toolkit is Copyright (C) 2000-2002 
   Richard W.E. Furse.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public Licence as
   published by the Free Software Foundation; either version 2 of the
   Licence, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA. */

/*****************************************************************************/

#include <cstdlib>

/*****************************************************************************/

#include "cmt.h"

#include "pinknoise.h"
#include "utils.h"

/*****************************************************************************/

namespace pink_full {

    enum {
	port_output      = 0,

	n_ports          = 1
    };
    
    static void activate(LADSPA_Handle instance);
    static void run(LADSPA_Handle instance,
                    unsigned long sample_count);	

  /** This plugin generates a signal which approximates pink noise, using the
	Voss-McCartney algorithm described at www.firstpr.com.au/dsp/pink-noise/ */
    class Plugin : public CMT_PluginInstance {
    private:
	PinkNoise noise_source;
    public:
	
        Plugin(const LADSPA_Descriptor *,
	       unsigned long) : 
            CMT_PluginInstance(n_ports) {
	}
	
	~Plugin() {
	}
	
	friend void activate(LADSPA_Handle instance);
	
	friend void run(LADSPA_Handle instance,
			unsigned long sample_count);	
    };
    
    static void activate(LADSPA_Handle instance) {
	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;
	
	p.noise_source.reset();
    }

    static void run(LADSPA_Handle instance,
                    unsigned long sample_count) {

	Plugin *pp = (Plugin *) instance;
	Plugin &p  = *pp;

	LADSPA_Data * out       =  pp->m_ppfPorts[port_output];

	for (unsigned long i=0; i<sample_count; ++i)
	    *(out++) = p.noise_source.getValue2();
    }

    void initialise() {
	CMT_Descriptor * d = new CMT_Descriptor
	    (1844,
	     "pink_full_frequency",
	     0,
	     "Pink Noise (full frequency range)",
	     CMT_MAKER("Nathaniel Virgo"),
	     CMT_COPYRIGHT("2002", "Nathaniel Virgo"),
	     NULL,
	     CMT_Instantiate<Plugin>,
	     activate,
	     run,
	     NULL,
	     NULL,
	     NULL);
	d->addPort
	    (LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	     "Output");
	registerNewPluginDescriptor(d);
    }

} // end of namespace

/*****************************************************************************/

/* EOF */
