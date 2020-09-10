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

#ifdef H2CORE_HAVE_LV2

#include <iostream>
#include "hydrogen/logger.h"
#include <hydrogen/object.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/midi_map.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/IO/LV2MidiDriver.h>
#include <hydrogen/IO/LV2AudioDriver.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define H2_URI "http://hydrogen-music.org/plugins/hydrogen"

typedef enum {
	H2_OUTPUT_L = 0,
	H2_OUTPUT_R = 1,
	H2_CONTROL = 2
} PortIndex;

typedef struct {
		// Port buffers
		const LV2_Atom_Sequence* control;
		float*                   out_L;
		float*                   out_R;

		// Features
		LV2_URID_Map* map;

		struct {
			LV2_URID midi_MidiEvent;
		} uris;
} H2Lv2Adapter;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	LV2_URID_Map* map = nullptr;
	for (int i = 0; features[i]; ++i) {
			if (!strcmp(features[i]->URI, LV2_URID__map)) {
					map = (LV2_URID_Map*)features[i]->data;
					break;
			}
	}
	if (!map) {
		return nullptr;
	}
	
	H2Lv2Adapter* self = (H2Lv2Adapter*)calloc(1, sizeof(H2Lv2Adapter));
	self->map = map;
	self->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
	
	
	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Logger* logger = H2Core::Logger::get_instance();
	H2Core::Object::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );
	H2Core::Filesystem::bootstrap( logger );

	MidiMap::create_instance();
	H2Core::Preferences::create_instance();
	H2Core::Hydrogen::create_instance();
	
	H2Core::Song *pSong = H2Core::Song::load( H2Core::Filesystem::empty_song_path() );

	//Misuse the central config here to store our LV2 driver specific properties
	H2Core::Preferences* pPref = H2Core::Preferences::get_instance();
	pPref->m_nSampleRate = rate;
	
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	pHydrogen->setSong( pSong );
	
	return (LV2_Handle) self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{

	H2Lv2Adapter* pH2Lv2Adapter = (H2Lv2Adapter*)instance;

	switch ((PortIndex)port) {
		case H2_OUTPUT_L:
			pH2Lv2Adapter->out_L = (float*)data;
			break;
		case H2_OUTPUT_R:
			pH2Lv2Adapter->out_R = (float*)data;
			break;
		case H2_CONTROL:
			pH2Lv2Adapter->control = (const LV2_Atom_Sequence*)data;
			break;
	}
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
	H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
	H2Core::LV2MidiDriver *lv2MidiDriver = dynamic_cast< H2Core::LV2MidiDriver* >( pHydrogen->getMidiInput() );
	H2Core::LV2AudioDriver *lv2AudioDriver = dynamic_cast< H2Core::LV2AudioDriver* >( pHydrogen->getAudioOutput() );
	H2Lv2Adapter* self   = (H2Lv2Adapter*) instance;

	LV2_ATOM_SEQUENCE_FOREACH(self->control, ev) {
			
			if (ev->body.type == self->uris.midi_MidiEvent) {
					
					H2Core::MidiMessage midiMsg;

					const uint8_t* const pRawLv2Msg = (const uint8_t*)(ev + 1);
					switch (lv2_midi_message_type(pRawLv2Msg)) {
						case LV2_MIDI_MSG_NOTE_ON:
									midiMsg.m_type = H2Core::MidiMessage::NOTE_ON;
									midiMsg.m_nData1 =pRawLv2Msg[1];
									midiMsg.m_nData2 = pRawLv2Msg[2];
									midiMsg.m_nChannel = pRawLv2Msg[0];
							
									lv2MidiDriver->forwardMidiMessage(midiMsg);
								
									break;
						case LV2_MIDI_MSG_NOTE_OFF:
								break;
							
						default: break;
					}
			}
	}

	lv2AudioDriver->handleData(n_samples, self->out_L, self->out_R);
}

static void
cleanup(LV2_Handle instance)
{
	H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
	
	pHydrogen->sequencer_stop();

	delete pHydrogen;
	delete H2Core::EventQueue::get_instance();
	delete H2Core::AudioEngine::get_instance();
	delete H2Core::Preferences::get_instance();
	delete H2Core::Logger::get_instance();
	
	free(instance);
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

#endif // H2CORE_HAVE_LV2
