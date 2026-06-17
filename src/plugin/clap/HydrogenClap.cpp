/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

// Native CLAP plugin (ADR 0014). A thin C-ABI shim over the format-agnostic
// HydrogenPlugin engine wrapper: it exposes one stereo master output plus
// H2_PLUGIN_OUTPUT_BUSES stereo bus outputs (ADR 0019), a MIDI note input,
// transport following, and state save/load via the .h2project codec.

#include <clap/clap.h>

#include <plugin/HydrogenPlugin.h>

#include <core/Midi/Midi.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

#ifndef H2_PLUGIN_OUTPUT_BUSES
#define H2_PLUGIN_OUTPUT_BUSES 32
#endif

using H2Core::HydrogenPlugin;

namespace {

constexpr int kNumBuses = H2_PLUGIN_OUTPUT_BUSES;
constexpr uint32_t kNumOutputPorts = 1 + kNumBuses; // master + buses

const char* const kFeatures[] = {
	CLAP_PLUGIN_FEATURE_INSTRUMENT,
	CLAP_PLUGIN_FEATURE_DRUM_MACHINE,
	CLAP_PLUGIN_FEATURE_STEREO,
	nullptr
};

const clap_plugin_descriptor_t kDescriptor = {
	CLAP_VERSION_INIT,
	"org.hydrogen-music.hydrogen",
	"Hydrogen",
	"Hydrogen",
	"https://hydrogen-music.org",
	"",
	"",
	"2.0.0",
	"Hydrogen drum machine / sampler",
	kFeatures
};

struct H2ClapPlugin {
	clap_plugin_t plugin;
	HydrogenPlugin* engine = nullptr;
	double sampleRate = 44100.0;
	uint32_t maxFrames = 4096;
};

H2ClapPlugin* self( const clap_plugin_t* plugin ) {
	return static_cast<H2ClapPlugin*>( plugin->plugin_data );
}

// Convert a 0-based MIDI channel (0..15) to Hydrogen's 1-based channel.
int toH2Channel( int nMidiChannel ) {
	return nMidiChannel + 1;
}

// ── audio-ports extension ─────────────────────────────────────────────────
uint32_t CLAP_ABI audioPortsCount( const clap_plugin_t*, bool is_input ) {
	return is_input ? 0 : kNumOutputPorts;
}
bool CLAP_ABI audioPortsGet( const clap_plugin_t*, uint32_t index, bool is_input,
							 clap_audio_port_info_t* info ) {
	if ( is_input || index >= kNumOutputPorts ) {
		return false;
	}
	info->id = index;
	info->channel_count = 2;
	info->flags = ( index == 0 ) ? CLAP_AUDIO_PORT_IS_MAIN : 0;
	info->port_type = CLAP_PORT_STEREO;
	info->in_place_pair = CLAP_INVALID_ID;
	if ( index == 0 ) {
		std::snprintf( info->name, sizeof( info->name ), "Master" );
	} else {
		std::snprintf( info->name, sizeof( info->name ), "Bus %u", index );
	}
	return true;
}
const clap_plugin_audio_ports_t kAudioPorts = { audioPortsCount, audioPortsGet };

// ── note-ports extension ──────────────────────────────────────────────────
uint32_t CLAP_ABI notePortsCount( const clap_plugin_t*, bool is_input ) {
	return is_input ? 1 : 0;
}
bool CLAP_ABI notePortsGet( const clap_plugin_t*, uint32_t index, bool is_input,
							clap_note_port_info_t* info ) {
	if ( ! is_input || index != 0 ) {
		return false;
	}
	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
	info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
	std::snprintf( info->name, sizeof( info->name ), "MIDI In" );
	return true;
}
const clap_plugin_note_ports_t kNotePorts = { notePortsCount, notePortsGet };

// ── state extension ───────────────────────────────────────────────────────
bool CLAP_ABI stateSave( const clap_plugin_t* plugin, const clap_ostream_t* stream ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr ) {
		return false;
	}
	const auto data = p->engine->saveState( /*bEmbedSamples=*/true );
	uint64_t nOffset = 0;
	while ( nOffset < data.size() ) {
		const int64_t nWritten = stream->write(
			stream, data.data() + nOffset, data.size() - nOffset );
		if ( nWritten <= 0 ) {
			return false;
		}
		nOffset += static_cast<uint64_t>( nWritten );
	}
	return true;
}
bool CLAP_ABI stateLoad( const clap_plugin_t* plugin, const clap_istream_t* stream ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr ) {
		return false;
	}
	std::vector<unsigned char> data;
	unsigned char buf[ 65536 ];
	int64_t nRead;
	while ( ( nRead = stream->read( stream, buf, sizeof( buf ) ) ) > 0 ) {
		data.insert( data.end(), buf, buf + nRead );
	}
	if ( nRead < 0 ) {
		return false;
	}
	return p->engine->loadState( data );
}
const clap_plugin_state_t kState = { stateSave, stateLoad };

// ── plugin vtable ─────────────────────────────────────────────────────────
bool CLAP_ABI pluginInit( const clap_plugin_t* plugin ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr ) {
		p->engine = new ( std::nothrow )
			HydrogenPlugin( p->sampleRate, p->maxFrames, kNumBuses );
	}
	return p->engine != nullptr;
}
void CLAP_ABI pluginDestroy( const clap_plugin_t* plugin ) {
	auto* p = self( plugin );
	delete p->engine;
	delete p;
}
bool CLAP_ABI pluginActivate( const clap_plugin_t* plugin, double sample_rate,
							  uint32_t /*min_frames*/, uint32_t max_frames ) {
	auto* p = self( plugin );
	p->sampleRate = sample_rate;
	p->maxFrames = max_frames;
	if ( p->engine == nullptr ) {
		p->engine = new ( std::nothrow )
			HydrogenPlugin( sample_rate, max_frames, kNumBuses );
	}
	if ( p->engine == nullptr ) {
		return false;
	}
	p->engine->activate( sample_rate, max_frames );
	return true;
}
void CLAP_ABI pluginDeactivate( const clap_plugin_t* plugin ) {
	auto* p = self( plugin );
	if ( p->engine != nullptr ) {
		p->engine->deactivate();
	}
}
bool CLAP_ABI pluginStartProcessing( const clap_plugin_t* ) { return true; }
void CLAP_ABI pluginStopProcessing( const clap_plugin_t* ) {}
void CLAP_ABI pluginReset( const clap_plugin_t* ) {}

void handleEvent( H2ClapPlugin* p, const clap_event_header_t* hdr ) {
	if ( hdr->space_id != CLAP_CORE_EVENT_SPACE_ID ) {
		return;
	}
	const int nOffset = static_cast<int>( hdr->time );
	switch ( hdr->type ) {
	case CLAP_EVENT_NOTE_ON: {
		auto* ev = reinterpret_cast<const clap_event_note_t*>( hdr );
		p->engine->noteOn( ev->key,
						   static_cast<int>( std::lround( ev->velocity * 127.0 ) ),
						   toH2Channel( ev->channel < 0 ? 9 : ev->channel ),
						   nOffset );
		break;
	}
	case CLAP_EVENT_NOTE_OFF: {
		auto* ev = reinterpret_cast<const clap_event_note_t*>( hdr );
		p->engine->noteOff( ev->key,
							toH2Channel( ev->channel < 0 ? 9 : ev->channel ),
							nOffset );
		break;
	}
	case CLAP_EVENT_MIDI: {
		auto* ev = reinterpret_cast<const clap_event_midi_t*>( hdr );
		const uint8_t nStatus = ev->data[0] & 0xF0;
		const int nChannel = toH2Channel( ev->data[0] & 0x0F );
		if ( nStatus == 0x90 && ev->data[2] > 0 ) {
			p->engine->noteOn( ev->data[1], ev->data[2], nChannel, nOffset );
		} else if ( nStatus == 0x80 ||
					( nStatus == 0x90 && ev->data[2] == 0 ) ) {
			p->engine->noteOff( ev->data[1], nChannel, nOffset );
		} else if ( nStatus == 0xB0 ) {
			p->engine->controlChange( ev->data[1], ev->data[2], nChannel,
									  nOffset );
		}
		break;
	}
	default:
		break;
	}
}

clap_process_status CLAP_ABI pluginProcess( const clap_plugin_t* plugin,
											const clap_process_t* process ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr || process == nullptr ) {
		return CLAP_PROCESS_ERROR;
	}

	// Deliver this block's MIDI before rendering.
	if ( process->in_events != nullptr ) {
		const uint32_t nEvents = process->in_events->size( process->in_events );
		for ( uint32_t ii = 0; ii < nEvents; ++ii ) {
			handleEvent( p, process->in_events->get( process->in_events, ii ) );
		}
	}

	// Transport.
	bool bRolling = false;
	double fBpm = 0.0;
	long long nFrame = 0;
	if ( process->transport != nullptr ) {
		const auto* t = process->transport;
		bRolling = ( t->flags & CLAP_TRANSPORT_IS_PLAYING ) != 0;
		fBpm = t->tempo;
		const double fSeconds =
			static_cast<double>( t->song_pos_seconds ) / CLAP_SECTIME_FACTOR;
		nFrame = static_cast<long long>( fSeconds * p->sampleRate );
	} else if ( process->steady_time >= 0 ) {
		nFrame = process->steady_time;
	}

	// Map output ports: port 0 = master, ports 1.. = buses.
	float* pMasterL = nullptr;
	float* pMasterR = nullptr;
	std::vector<float*> busL, busR;
	if ( process->audio_outputs_count > 0 &&
		 process->audio_outputs[0].channel_count >= 2 &&
		 process->audio_outputs[0].data32 != nullptr ) {
		pMasterL = process->audio_outputs[0].data32[0];
		pMasterR = process->audio_outputs[0].data32[1];
	}
	for ( uint32_t port = 1; port < process->audio_outputs_count; ++port ) {
		auto& ab = process->audio_outputs[port];
		if ( ab.channel_count >= 2 && ab.data32 != nullptr ) {
			busL.push_back( ab.data32[0] );
			busR.push_back( ab.data32[1] );
		} else {
			busL.push_back( nullptr );
			busR.push_back( nullptr );
		}
	}

	if ( pMasterL == nullptr || pMasterR == nullptr ) {
		return CLAP_PROCESS_ERROR;
	}

	p->engine->process( process->frames_count, pMasterL, pMasterR, busL, busR,
						bRolling, fBpm, nFrame );

	return CLAP_PROCESS_CONTINUE;
}

const void* CLAP_ABI pluginGetExtension( const clap_plugin_t*, const char* id ) {
	if ( std::strcmp( id, CLAP_EXT_AUDIO_PORTS ) == 0 ) {
		return &kAudioPorts;
	}
	if ( std::strcmp( id, CLAP_EXT_NOTE_PORTS ) == 0 ) {
		return &kNotePorts;
	}
	if ( std::strcmp( id, CLAP_EXT_STATE ) == 0 ) {
		return &kState;
	}
	return nullptr;
}
void CLAP_ABI pluginOnMainThread( const clap_plugin_t* ) {}

// ── factory ───────────────────────────────────────────────────────────────
uint32_t CLAP_ABI factoryGetPluginCount( const clap_plugin_factory_t* ) {
	return 1;
}
const clap_plugin_descriptor_t* CLAP_ABI factoryGetDescriptor(
	const clap_plugin_factory_t*, uint32_t index ) {
	return index == 0 ? &kDescriptor : nullptr;
}
const clap_plugin_t* CLAP_ABI factoryCreate( const clap_plugin_factory_t*,
											 const clap_host_t* /*host*/,
											 const char* plugin_id ) {
	if ( plugin_id == nullptr ||
		 std::strcmp( plugin_id, kDescriptor.id ) != 0 ) {
		return nullptr;
	}
	auto* p = new ( std::nothrow ) H2ClapPlugin();
	if ( p == nullptr ) {
		return nullptr;
	}
	p->plugin.desc = &kDescriptor;
	p->plugin.plugin_data = p;
	p->plugin.init = pluginInit;
	p->plugin.destroy = pluginDestroy;
	p->plugin.activate = pluginActivate;
	p->plugin.deactivate = pluginDeactivate;
	p->plugin.start_processing = pluginStartProcessing;
	p->plugin.stop_processing = pluginStopProcessing;
	p->plugin.reset = pluginReset;
	p->plugin.process = pluginProcess;
	p->plugin.get_extension = pluginGetExtension;
	p->plugin.on_main_thread = pluginOnMainThread;
	return &p->plugin;
}
const clap_plugin_factory_t kFactory = {
	factoryGetPluginCount, factoryGetDescriptor, factoryCreate
};

// ── entry ─────────────────────────────────────────────────────────────────
bool CLAP_ABI entryInit( const char* ) { return true; }
void CLAP_ABI entryDeinit( void ) {}
const void* CLAP_ABI entryGetFactory( const char* factory_id ) {
	if ( std::strcmp( factory_id, CLAP_PLUGIN_FACTORY_ID ) == 0 ) {
		return &kFactory;
	}
	return nullptr;
}

} // namespace

extern "C" const CLAP_EXPORT clap_plugin_entry_t clap_entry = {
	CLAP_VERSION_INIT, entryInit, entryDeinit, entryGetFactory
};
