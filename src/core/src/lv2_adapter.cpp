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

#include "hydrogen/config.h"

#ifdef H2CORE_HAVE_LV2

#include <hydrogen/lv2_uris.h>
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

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/worker/worker.h"
#include "lv2/core/lv2_util.h"

typedef struct {
		LV2_Log_Logger				logger;
		LV2_Worker_Schedule*		schedule;
		// Port buffers
		const LV2_Atom_Sequence*	control;
		const LV2_Atom_Sequence*	notify;
		LV2_Atom_Forge_Frame		notify_frame; 
		LV2_Atom_Forge				forge;
		float*						out_L;
		float*						out_R;

		// Features
		LV2_URID_Map* map;

		SamplerURIs uris;
} H2Lv2Adapter;

/**
   Do work in a non-realtime thread.

   This is called for every piece of work scheduled in the audio thread using
   self->schedule->schedule_work().  A reply can be sent back to the audio
   thread using the provided `respond` function.
*/
static LV2_Worker_Status
work(LV2_Handle                  instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle   handle,
     uint32_t                    size,
     const void*                 data)
{
	H2Lv2Adapter* pH2Lv2Adapter = (H2Lv2Adapter*)instance;
	
	const LV2_Atom* atom = (const LV2_Atom*)data;
	
	if (atom->type == pH2Lv2Adapter->forge.Object) {
		// Handle set message (load sample).
		const LV2_Atom_Object* obj  = (const LV2_Atom_Object*)data;
		const char*            path = readSetDrumkit(&pH2Lv2Adapter->uris, obj);
		if (!path) {
			lv2_log_error(&pH2Lv2Adapter->logger, "Malformed set file request\n");
			return LV2_WORKER_ERR_UNKNOWN;
		}

		// Load drumkit.
		const char* pDrumkitName = readSetDrumkit(&pH2Lv2Adapter->uris,obj);
		
		QString sDrumkitName = pDrumkitName;
		H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
		H2Core::Drumkit* pDrumkitInfo = H2Core::Drumkit::load_by_name( sDrumkitName , true );
		
		if( pDrumkitInfo ) {
			pHydrogen->loadDrumkit( pDrumkitInfo );
		}
		
	}

	return LV2_WORKER_SUCCESS;
}

/**
   Handle a response from work() in the audio thread.

   When running normally, this will be called by the host after run().  When
   freewheeling, this will be called immediately at the point the work was
   scheduled.
*/
static LV2_Worker_Status
work_response(LV2_Handle  instance,
              uint32_t    size,
              const void* data)
{
	std::cout << "work_response!! " << std::endl;
	
	/*
	H2Lv2Adapter* pH2Lv2Adapter = (H2Lv2Adapter*)instance;
	Sample*  old_sample = self->sample;
	Sample*  new_sample = *(Sample*const*)data;

	// Install the new sample
	self->sample = *(Sample*const*)data;

	// Schedule work to free the old sample
	SampleMessage msg = { { sizeof(Sample*), self->uris.eg_freeSample },
	                      old_sample };
	self->schedule->schedule_work(self->schedule->handle, sizeof(msg), &msg);

	// Send a notification that we're using a new sample
	lv2_atom_forge_frame_time(&self->forge, self->frame_offset);
	write_set_file(&self->forge, &self->uris,
		       new_sample->path,
		       new_sample->path_len);
	*/

	return LV2_WORKER_SUCCESS;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	H2Lv2Adapter* pH2Lv2Adapter = (H2Lv2Adapter*) calloc(1, sizeof(H2Lv2Adapter));
	
	// Get host features
	const char* missing = lv2_features_query(
	  features,
	  LV2_LOG__log,         &pH2Lv2Adapter->logger.log, false,
	  LV2_URID__map,        &pH2Lv2Adapter->map,        true,
	  LV2_WORKER__schedule, &pH2Lv2Adapter->schedule,   true,
	  nullptr);
  
	lv2_log_logger_set_map(&pH2Lv2Adapter->logger, pH2Lv2Adapter->map);
	if (missing) {
	  lv2_log_error(&pH2Lv2Adapter->logger, "Missing feature <%s>\n", missing);
	  free(pH2Lv2Adapter);
	  return nullptr;
	}
		
	mapSamplerUris(pH2Lv2Adapter->map, &pH2Lv2Adapter->uris);
	lv2_atom_forge_init(&pH2Lv2Adapter->forge, pH2Lv2Adapter->map);
	
	unsigned logLevelOpt = H2Core::Logger::Error;
	H2Core::Logger::create_instance();
	H2Core::Logger::set_bit_mask( logLevelOpt );
	H2Core::Logger* logger = H2Core::Logger::get_instance();
	H2Core::Object::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );
	H2Core::Filesystem::bootstrap( logger );

	MidiMap::create_instance();
	H2Core::Preferences::create_instance();
	
	//Misuse the central config here to store our LV2 driver specific properties
	H2Core::Preferences* pPref = H2Core::Preferences::get_instance();
	pPref->m_sAudioDriver = "LV2";
	pPref->m_nSampleRate = rate;
	
	H2Core::Hydrogen::create_instance();
	
	H2Core::Song *pSong = H2Core::Song::load( H2Core::Filesystem::empty_song_path() );
	
	H2Core::Hydrogen *pHydrogen = H2Core::Hydrogen::get_instance();
	pHydrogen->setSong( pSong );
	
	return (LV2_Handle) pH2Lv2Adapter;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	H2Lv2Adapter* pH2Lv2Adapter = (H2Lv2Adapter*)instance;

	switch ((PortIndex)port) {
		case H2_PORT_OUTPUT_L:
			pH2Lv2Adapter->out_L = (float*)data;
			break;
		case H2_PORT_OUTPUT_R:
			pH2Lv2Adapter->out_R = (float*)data;
			break;
		case H2_PORT_CONTROL:
			pH2Lv2Adapter->control = (const LV2_Atom_Sequence*)data;
			break;
		case H2_PORT_NOTIFY:
			pH2Lv2Adapter->notify = (const LV2_Atom_Sequence*)data;
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
	H2Core::LV2MidiDriver* pLV2MidiDriver = dynamic_cast< H2Core::LV2MidiDriver* >( pHydrogen->getMidiInput() );
	H2Core::LV2AudioDriver* pLV2AudioDriver = dynamic_cast< H2Core::LV2AudioDriver* >( pHydrogen->getAudioOutput() );
	H2Lv2Adapter* self = (H2Lv2Adapter*) instance;
	
	assert(pLV2AudioDriver);
	assert(pLV2MidiDriver);
	
	// Set up forge to write directly to notify output port.
	const uint32_t notify_capacity = self->notify->atom.size;
	lv2_atom_forge_set_buffer(&self->forge,
							  (uint8_t*)self->notify,
							  notify_capacity);
	lv2_atom_forge_sequence_head(&self->forge, &self->notify_frame, 0);
	
	LV2_ATOM_SEQUENCE_FOREACH(self->control, ev) {
		
		if (ev->body.type == self->uris.midi_Event) 
		{
			H2Core::MidiMessage midiMsg;
			
			lv2_log_error(&self->logger, "incoming midi message\n");
			
			const uint8_t* const pRawLv2Msg = (const uint8_t*)(ev + 1);
			switch (lv2_midi_message_type(pRawLv2Msg)) {
				case LV2_MIDI_MSG_NOTE_ON:
					midiMsg.m_type = H2Core::MidiMessage::NOTE_ON;
					midiMsg.m_nData1 =pRawLv2Msg[1];
					midiMsg.m_nData2 = pRawLv2Msg[2];
					midiMsg.m_nChannel = pRawLv2Msg[0];
					
					pLV2MidiDriver->forwardMidiMessage(midiMsg);
					
					break;
				case LV2_MIDI_MSG_NOTE_OFF:
					break;
					
				default: break;
			}
		} else if (lv2_atom_forge_is_object_type(&self->forge, ev->body.type)) {
			const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
			if (obj->body.otype == self->uris.patch_Get) {
				
				lv2_log_error(&self->logger, "incoming patch get\n");
				
				
				QString DrumkitName = pHydrogen->getCurrentDrumkitname();
				QByteArray byteArray = DrumkitName.toLocal8Bit();
				const char* pDrumkitName = byteArray.data();
				
				lv2_atom_forge_frame_time(&self->forge, 0);
				writeSetDrumkit(&self->forge, &self->uris,
								pDrumkitName,
								strlen(pDrumkitName));
			} else if (obj->body.otype == self->uris.patch_Set) {
				//const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&obj->body;
				lv2_log_error(&self->logger, "incoming patch set\n");
								
				// Get the property and value of the set message
				const LV2_Atom* property = nullptr;
				const LV2_Atom* value    = nullptr;
				lv2_atom_object_get(obj,
									self->uris.patch_property, &property,
									self->uris.patch_value,    &value,
									0);
				if (!property) {
					lv2_log_error(&self->logger, "Set message with no property\n");
					return;
				} else if (property->type != self->uris.atom_URID) {
					lv2_log_error(&self->logger, "Set property is not a URID\n");
					return;
				}
				
				const uint32_t key = ((const LV2_Atom_URID*)property)->body;
				if (key == self->uris.h2_drumkit) {
					// Sample change, send it to the worker.
					lv2_log_trace(&self->logger, "Scheduling sample change\n");
					self->schedule->schedule_work(self->schedule->handle,
												  lv2_atom_total_size(&ev->body),
												  &ev->body);
				}
			}
		}
	}

	pLV2AudioDriver->handleData(n_samples, self->out_L, self->out_R);
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
	//static const LV2_State_Interface  state  = {save, restore};
	static const LV2_Worker_Interface worker = {work, work_response, NULL};
	
	/*if (!strcmp(uri, LV2_STATE__interface)) {
	  return &state;
	} else */
		
	if (!strcmp(uri, LV2_WORKER__interface)) {
	  return &worker;
	}
	return NULL;
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
