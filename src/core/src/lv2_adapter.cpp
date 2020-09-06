/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <hydrogen/object.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/midi_map.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define H2_URI "http://lv2plug.in/plugins/h2"

typedef enum {
	AMP_GAIN   = 0,
	AMP_INPUT  = 1,
	AMP_OUTPUT = 2
} PortIndex;

typedef struct {
	// Port buffers
	const float* gain;
	const float* input;
	float*       output;
} Amp;


static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Logger* logger = H2Core::Logger::get_instance();
	H2Core::Object::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );

	QCoreApplication a();

	H2Core::Filesystem::bootstrap( logger );

	QString filename = "/hla";

	MidiMap::create_instance();
	H2Core::Preferences::create_instance();
	H2Core::Hydrogen::create_instance();
	H2Core::Preferences *preferences = H2Core::Preferences::get_instance();

	/*
	H2Core::Song *pSong = H2Core::Song::load( filename );
	if (pSong == nullptr) {
		cout << "Error loading song!" << endl;
		exit(2);
	}*/

	H2Core::Hydrogen *hydrogen = H2Core::Hydrogen::get_instance();
	
	return (LV2_Handle) hydrogen;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	/*
	Amp* amp = (Amp*)instance;

	switch ((PortIndex)port) {
	case AMP_GAIN:
		amp->gain = (const float*)data;
		break;
	case AMP_INPUT:
		amp->input = (const float*)data;
		break;
	case AMP_OUTPUT:
		amp->output = (float*)data;
		break;
	}
	*/
}

static void
activate(LV2_Handle instance)
{
}

static void
deactivate(LV2_Handle instance)
{
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	/*
	const Amp* amp = (const Amp*)instance;

	const float        gain   = *(amp->gain);
	const float* const input  = amp->input;
	float* const       output = amp->output;

	const float coef = DB_CO(gain);

	for (uint32_t pos = 0; pos < n_samples; pos++) {
		output[pos] = input[pos] * coef;
	}
	*/
}

static void
cleanup(LV2_Handle instance)
{
	//free(instance);
}

static const void*
extension_data(const char* uri)
{
	return nullptr;
}

static const LV2_Descriptor descriptor = {
	H2_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return nullptr;
	}
}
