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
#include <plugin/ui/PluginUiWindow.h>

#include <core/Midi/Midi.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <QtCore/QFileInfo>
#include <QtCore/QString>

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
	"http://hydrogen-music.org/doc",
	"https://github.com/hydrogen-music/hydrogen/issues",
	"2.0.0",
	"Hydrogen drum machine / sampler",
	kFeatures
};

// Directory containing the loaded .clap, captured in entryInit(plugin_path); used
// to find the bundled editor binary (ADR 0016).
std::string g_clapPluginDir;

struct H2ClapPlugin {
	clap_plugin_t plugin;
	// The host, captured in factoryCreate (this CLAP rev hands it over
	// there, not in init) — the frame timer is registered on it.
	const clap_host_t* host = nullptr;
	HydrogenPlugin* engine = nullptr;
	// The basic-UI window (proposal 0006 UI-5): the embedded child on the
	// host window (guiSetParent) or the self-managed floating top-level
	// (guiShow); null until one of them runs.
	std::unique_ptr<H2Core::PluginUiWindow> pWindow;
	// The disposition chosen in guiCreate; set_parent (embedded) or show
	// (floating) drive the window accordingly.
	bool bEmbedded = false;
	// The frame timer (clap.timer-support); 0 while unregistered.
	clap_id timerId = 0;
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

// ── params extension (VST3-wrap only) ─────────────────────────────────────
// Hydrogen exposes no automatable CLAP parameters (ADR 0021 — the user drives
// the engine via an editor instance over IPC, not via params), so the native
// CLAP/LV2 plugins do NOT advertise this extension. It is compiled in ONLY for
// the clap-wrapper VST3 build (H2_CLAP_FOR_VST3): clap-wrapper sets up its
// MIDI-CC → VST3 mapping (IMidiMapping) inside its setupParameters() path,
// which it skips entirely when the wrapped CLAP plugin has no params extension
// — leaving getMidiControllerAssignment to return an unregistered ParamID 0
// that the Steinberg VST3 validator rejects ("Unknown ParamID [0] returned for
// MIDI Mapping"). Advertising the (empty) extension is what unlocks that path.
//
// We deliberately do NOT expose it for the native CLAP plugin: it is unnecessary
// there (no params, MIDI handled via the note port) and advertising it makes
// clap-validator enable its param/state-reproducibility test suites, which are
// out of scope for the parameterless engine.
#if defined( H2_CLAP_FOR_VST3 )
uint32_t CLAP_ABI paramsCount( const clap_plugin_t* ) {
	return 0;
}
bool CLAP_ABI paramsGetInfo( const clap_plugin_t*, uint32_t, clap_param_info_t* ) {
	return false;
}
bool CLAP_ABI paramsGetValue( const clap_plugin_t*, clap_id, double* ) {
	return false;
}
bool CLAP_ABI paramsValueToText( const clap_plugin_t*, clap_id, double, char*, uint32_t ) {
	return false;
}
bool CLAP_ABI paramsTextToValue( const clap_plugin_t*, clap_id, const char*, double* ) {
	return false;
}
void CLAP_ABI paramsFlush( const clap_plugin_t*, const clap_input_events_t*,
						   const clap_output_events_t* ) {
}
const clap_plugin_params_t kParams = { paramsCount, paramsGetInfo, paramsGetValue,
									   paramsValueToText, paramsTextToValue, paramsFlush };
#endif

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

// ── gui extension (ADR 0035 + proposal 0006 UI-5: the basic UI) ─────────────
// Both dispositions ride the one window seam: embedded (guiSetParent) puts a
// child window on the host's native window; floating (guiShow) opens the
// self-managed top-level basic-UI window (mirroring the LV2 showInterface
// fallback). CLAP's gui extension has no idle method of its own, so the frame
// pump is the clap.timer-support timer (~60 Hz, onTimer below) — registered
// while the UI is shown, unregistered on hide/destroy. Neither disposition
// ever spawns the editor process (TU5.7): the landing view's Open editor
// button owns it, and it dies with the plugin instance (ADR 0035).
const char* preferredWindowApi() {
#if defined( _WIN32 )
	return CLAP_WINDOW_API_WIN32;
#elif defined( __APPLE__ )
	return CLAP_WINDOW_API_COCOA;
#else
	return CLAP_WINDOW_API_X11;
#endif
}

// Registers/unregisters the frame timer; idempotent. Hosts without
// timer-support cannot drive the window — the UI stays static there.
void registerFrameTimer( H2ClapPlugin* p ) {
	if ( p->host == nullptr || p->timerId != 0 ) {
		return;
	}
	auto* timers = static_cast<const clap_host_timer_support_t*>(
		p->host->get_extension( p->host, CLAP_EXT_TIMER_SUPPORT ) );
	if ( timers == nullptr ) {
		return;
	}
	timers->register_timer( p->host, 16, &p->timerId );
}
void unregisterFrameTimer( H2ClapPlugin* p ) {
	if ( p->host == nullptr || p->timerId == 0 ) {
		return;
	}
	auto* timers = static_cast<const clap_host_timer_support_t*>(
		p->host->get_extension( p->host, CLAP_EXT_TIMER_SUPPORT ) );
	if ( timers != nullptr ) {
		timers->unregister_timer( p->host, p->timerId );
	}
	p->timerId = 0;
}
void CLAP_ABI onTimer( const clap_plugin_t* plugin, clap_id timerId ) {
	auto* p = self( plugin );
	if ( p->timerId == timerId && p->pWindow != nullptr ) {
		p->pWindow->idle();
	}
}
const clap_plugin_timer_support_t kTimerSupport = { onTimer };

bool CLAP_ABI guiIsApiSupported( const clap_plugin_t*, const char*,
								 bool /*is_floating*/ ) {
	// Both dispositions (ADR 0035): embedded on the host window, floating
	// as the self-managed top-level.
	return true;
}
bool CLAP_ABI guiGetPreferredApi( const clap_plugin_t*, const char** api,
								  bool* is_floating ) {
	*api = preferredWindowApi();
	// Embedded preferred (ADR 0035's on-contract embedding); floating
	// stays supported for hosts that cannot embed.
	*is_floating = false;
	return true;
}
bool CLAP_ABI guiCreate( const clap_plugin_t* plugin, const char*,
						 bool is_floating ) {
	// Nothing to allocate: the window appears on guiSetParent (embedded)
	// or guiShow (floating); only the disposition is recorded here.
	auto* p = self( plugin );
	p->bEmbedded = ! is_floating;
	return true;
}
void CLAP_ABI guiDestroy( const clap_plugin_t* plugin ) {
	// Tears the UI window and its timer down — and nothing else: the
	// editor is the landing view's business (TU5.7) and dies with the
	// plugin instance (ADR 0035).
	auto* p = self( plugin );
	unregisterFrameTimer( p );
	p->pWindow.reset();
	p->bEmbedded = false;
}
bool CLAP_ABI guiSetScale( const clap_plugin_t*, double ) {
	// The backend reads the native DPI at construction; host-driven
	// rescaling is not wired.
	return false;
}
bool CLAP_ABI guiGetSize( const clap_plugin_t* plugin, uint32_t* width,
						  uint32_t* height ) {
	// Real (TU5.3): the live window size (the host resized the child with
	// its FX window); the initial footprint before the host sized it.
	auto* p = self( plugin );
	if ( width == nullptr || height == nullptr ) {
		return false;
	}
	if ( p->pWindow != nullptr && p->pWindow->isValid() ) {
		int w = 0;
		int h = 0;
		p->pWindow->getSize( w, h );
		*width = static_cast<uint32_t>( w );
		*height = static_cast<uint32_t>( h );
	}
	else {
		*width = HydrogenPlugin::nDefaultWidth;
		*height = HydrogenPlugin::nDefaultHeight;
	}
	return true;
}
bool CLAP_ABI guiCanResize( const clap_plugin_t* ) {
	return true; // the views reflow at any size (UI-4)
}
bool CLAP_ABI guiGetResizeHints( const clap_plugin_t*,
								 clap_gui_resize_hints_t* hints ) {
	// Free resizing both ways, no aspect constraint; the readable floor
	// for the three tabs' content lives in guiAdjustSize.
	if ( hints == nullptr ) {
		return false;
	}
	hints->can_resize_horizontally = true;
	hints->can_resize_vertically = true;
	hints->preserve_aspect_ratio = false;
	hints->aspect_ratio_width = 0;
	hints->aspect_ratio_height = 0;
	return true;
}
bool CLAP_ABI guiAdjustSize( const clap_plugin_t*, uint32_t* width,
							 uint32_t* height ) {
	// Clamp to the readable floor only.
	if ( width == nullptr || height == nullptr ) {
		return false;
	}
	if ( *width < HydrogenPlugin::nMinWidth ) {
		*width = HydrogenPlugin::nMinWidth;
	}
	if ( *height < HydrogenPlugin::nMinHeight ) {
		*height = HydrogenPlugin::nMinHeight;
	}
	return true;
}
bool CLAP_ABI guiSetSize( const clap_plugin_t* plugin, uint32_t width,
						  uint32_t height ) {
	// Real for the embedded path (TU5.3): the host sizes the child with
	// its FX window. Without a window there is nothing to size yet.
	auto* p = self( plugin );
	if ( p->pWindow == nullptr || ! p->pWindow->isValid() ) {
		return false;
	}
	p->pWindow->setSize( static_cast<int>( width ),
						 static_cast<int>( height ) );
	return true;
}
bool CLAP_ABI guiSetParent( const clap_plugin_t* plugin,
							const clap_window_t* window ) {
	// Embedded (ADR 0035): a child window on the host's native window,
	// handed the initial footprint; the host resizes it with its FX
	// window (the backend tracks ConfigureNotify) and the frame timer
	// drives it.
	auto* p = self( plugin );
	if ( p->engine == nullptr || window == nullptr || ! p->bEmbedded ) {
		return false;
	}
	std::uintptr_t parent = 0;
	if ( std::strcmp( window->api, CLAP_WINDOW_API_WIN32 ) == 0 ) {
		parent = reinterpret_cast<std::uintptr_t>( window->win32 );
	}
	else if ( std::strcmp( window->api, CLAP_WINDOW_API_COCOA ) == 0 ) {
		parent = reinterpret_cast<std::uintptr_t>( window->cocoa );
	}
	else if ( std::strcmp( window->api, CLAP_WINDOW_API_X11 ) == 0 ) {
		parent = static_cast<std::uintptr_t>( window->x11 );
	}
	else {
		return false;
	}
	p->pWindow = std::make_unique<H2Core::PluginUiWindow>(
		parent, HydrogenPlugin::nDefaultWidth, HydrogenPlugin::nDefaultHeight,
		p->engine
	);
	if ( !p->pWindow->isValid() ) {
		p->pWindow.reset();
		return false;
	}
	registerFrameTimer( p );
	return true;
}
bool CLAP_ABI guiSetTransient( const clap_plugin_t*, const clap_window_t* ) {
	// WinId-level transient hints are not wired for the fallback window.
	return false;
}
void CLAP_ABI guiSuggestTitle( const clap_plugin_t*, const char* ) {
	// The fallback window's title is the brand name "Hydrogen".
}
bool CLAP_ABI guiShow( const clap_plugin_t* plugin ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr ) {
		return false;
	}
	// Embedded: the child is mapped on the host window already (ADR
	// 0035's on-contract embedding) — only the frame timer is (re)armed.
	// Floating: show() creates the self-managed top-level basic-UI
	// window (UI-5) — never the editor process (TU5.7: the landing
	// view's Open editor button owns that).
	if ( ! p->bEmbedded && p->pWindow == nullptr ) {
		p->pWindow = std::make_unique<H2Core::PluginUiWindow>(
			0, HydrogenPlugin::nDefaultWidth, HydrogenPlugin::nDefaultHeight,
			p->engine
		);
		if ( !p->pWindow->isValid() ) {
			p->pWindow.reset();
			return false;
		}
	}
	registerFrameTimer( p );
	return true;
}
bool CLAP_ABI guiHide( const clap_plugin_t* plugin ) {
	// Stop painting (the timer goes); floating additionally destroys the
	// self-managed window (a later show recreates it), embedded leaves
	// the host-parented child alone. Neither touches the editor (ADR
	// 0035, TU5.7).
	auto* p = self( plugin );
	unregisterFrameTimer( p );
	if ( ! p->bEmbedded ) {
		p->pWindow.reset();
	}
	return true;
}
const clap_plugin_gui_t kGui = {
	guiIsApiSupported, guiGetPreferredApi, guiCreate,	  guiDestroy,
	guiSetScale,	   guiGetSize,		   guiCanResize,	  guiGetResizeHints,
	guiAdjustSize,	   guiSetSize,		   guiSetParent,	  guiSetTransient,
	guiSuggestTitle,   guiShow,			   guiHide
};

// ── plugin vtable ─────────────────────────────────────────────────────────
bool CLAP_ABI pluginInit( const clap_plugin_t* plugin ) {
	auto* p = self( plugin );
	if ( p->engine == nullptr ) {
		p->engine = new ( std::nothrow )
			HydrogenPlugin( p->sampleRate, p->maxFrames, kNumBuses );
	}
	if ( p->engine != nullptr && ! g_clapPluginDir.empty() ) {
		// Find the editor binary next to the installed plugin (ADR 0016).
		p->engine->setEditorSearchDir(
			QString::fromStdString( g_clapPluginDir ) );
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
		p->engine->noteOn(
			ev->key, static_cast<int>( std::lround( ev->velocity * 127.0 ) ),
			toH2Channel(
				ev->channel < 0
					? static_cast<int>( H2Core::Midi::ChannelDefault ) - 1
					: ev->channel
			),
			nOffset
		);
		break;
	}
	case CLAP_EVENT_NOTE_OFF: {
		auto* ev = reinterpret_cast<const clap_event_note_t*>( hdr );
		p->engine->noteOff(
			ev->key,
			toH2Channel(
				ev->channel < 0
					? static_cast<int>( H2Core::Midi::ChannelDefault ) - 1
					: ev->channel
			),
			nOffset
		);
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
		// free running host
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
#if defined( H2_CLAP_FOR_VST3 )
	if ( std::strcmp( id, CLAP_EXT_PARAMS ) == 0 ) {
		return &kParams;
	}
#endif
	if ( std::strcmp( id, CLAP_EXT_GUI ) == 0 ) {
		return &kGui;
	}
	if ( std::strcmp( id, CLAP_EXT_TIMER_SUPPORT ) == 0 ) {
		return &kTimerSupport;
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
											 const clap_host_t* host,
											 const char* plugin_id ) {
	if ( plugin_id == nullptr ||
		 std::strcmp( plugin_id, kDescriptor.id ) != 0 ) {
		return nullptr;
	}
	auto* p = new ( std::nothrow ) H2ClapPlugin();
	if ( p == nullptr ) {
		return nullptr;
	}
	// This CLAP rev hands the host over here (not in init); the frame
	// timer is registered on it while the UI is shown.
	p->host = host;
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
bool CLAP_ABI entryInit( const char* plugin_path ) {
	if ( plugin_path != nullptr && plugin_path[0] != '\0' ) {
		g_clapPluginDir =
			QFileInfo( QString::fromUtf8( plugin_path ) ).absolutePath()
				.toStdString();
	}
	return true;
}
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
