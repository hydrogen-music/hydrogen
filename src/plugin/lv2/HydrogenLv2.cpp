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

// Native LV2 plugin (ADR 0014). A thin C-ABI shim over the format-agnostic
// HydrogenPlugin engine wrapper. Ports (must match the generated hydrogen.ttl):
//   0           : MIDI in       (atom sequence input)
//   1, 2        : master out    L, R
//   3, 4        : bus 0 out      L, R
//   ...         : bus i out      L, R   (for i in 0 .. H2_PLUGIN_OUTPUT_BUSES-1)

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/urid/urid.h>
#include <lv2/midi/midi.h>
#include <lv2/ui/ui.h>
#include <lv2/instance-access/instance-access.h>

#include <plugin/HydrogenPlugin.h>
#include <plugin/ui/PluginUiWindow.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

#include <QtCore/QString>

#ifndef H2_PLUGIN_OUTPUT_BUSES
#define H2_PLUGIN_OUTPUT_BUSES 32
#endif

#define H2_LV2_URI "https://hydrogen-music.org/lv2/hydrogen"

using H2Core::HydrogenPlugin;

namespace {

constexpr int kNumBuses = H2_PLUGIN_OUTPUT_BUSES;
constexpr uint32_t kMaxBlock = 8192;

// Port indices.
constexpr uint32_t kPortMidiIn = 0;
constexpr uint32_t kPortMasterL = 1;
constexpr uint32_t kPortMasterR = 2;
constexpr uint32_t kPortBusBase = 3; // bus i: base + 2*i (L), +1 (R)

int toH2Channel( int nMidiChannel ) { return nMidiChannel + 1; }

struct H2Lv2 {
	HydrogenPlugin* engine = nullptr;
	double sampleRate = 44100.0;
	long long nFrame = 0;

	LV2_URID midiEventType = 0;

	const LV2_Atom_Sequence* midiIn = nullptr;
	float* masterL = nullptr;
	float* masterR = nullptr;
	std::vector<float*> busL;
	std::vector<float*> busR;
};

LV2_Handle instantiate( const LV2_Descriptor*, double sample_rate,
						const char* bundle_path,
						const LV2_Feature* const* features ) {
	LV2_URID_Map* map = nullptr;
	for ( int ii = 0; features != nullptr && features[ii] != nullptr; ++ii ) {
		if ( std::strcmp( features[ii]->URI, LV2_URID__map ) == 0 ) {
			map = static_cast<LV2_URID_Map*>( features[ii]->data );
		}
	}
	if ( map == nullptr ) {
		return nullptr; // urid:map is required to read MIDI atoms
	}

	auto* p = new ( std::nothrow ) H2Lv2();
	if ( p == nullptr ) {
		return nullptr;
	}
	p->sampleRate = sample_rate;
	p->midiEventType = map->map( map->handle, LV2_MIDI__MidiEvent );
	p->busL.assign( kNumBuses, nullptr );
	p->busR.assign( kNumBuses, nullptr );
	p->engine = new ( std::nothrow )
		HydrogenPlugin( sample_rate, kMaxBlock, kNumBuses );
	if ( p->engine == nullptr ) {
		delete p;
		return nullptr;
	}
	if ( bundle_path != nullptr && bundle_path[0] != '\0' ) {
		// The LV2 bundle directory is where packaging may place the editor
		// binary (ADR 0016); search it before falling back to PATH.
		p->engine->setEditorSearchDir( QString::fromUtf8( bundle_path ) );
	}
	return static_cast<LV2_Handle>( p );
}

void connect_port( LV2_Handle instance, uint32_t port, void* data ) {
	auto* p = static_cast<H2Lv2*>( instance );
	if ( port == kPortMidiIn ) {
		p->midiIn = static_cast<const LV2_Atom_Sequence*>( data );
	} else if ( port == kPortMasterL ) {
		p->masterL = static_cast<float*>( data );
	} else if ( port == kPortMasterR ) {
		p->masterR = static_cast<float*>( data );
	} else if ( port >= kPortBusBase ) {
		const uint32_t rel = port - kPortBusBase;
		const uint32_t bus = rel / 2;
		if ( bus < static_cast<uint32_t>( kNumBuses ) ) {
			if ( ( rel % 2 ) == 0 ) {
				p->busL[ bus ] = static_cast<float*>( data );
			} else {
				p->busR[ bus ] = static_cast<float*>( data );
			}
		}
	}
}

void activate( LV2_Handle instance ) {
	auto* p = static_cast<H2Lv2*>( instance );
	if ( p->engine != nullptr ) {
		p->engine->activate( p->sampleRate, kMaxBlock );
	}
}

void run( LV2_Handle instance, uint32_t sample_count ) {
	auto* p = static_cast<H2Lv2*>( instance );
	if ( p->engine == nullptr || p->masterL == nullptr ||
		 p->masterR == nullptr ) {
		return;
	}

	// Deliver MIDI for this block.
	if ( p->midiIn != nullptr ) {
		LV2_ATOM_SEQUENCE_FOREACH( p->midiIn, ev ) {
			if ( ev->body.type != p->midiEventType ) {
				continue;
			}
			const uint8_t* msg =
				reinterpret_cast<const uint8_t*>( LV2_ATOM_BODY_CONST( &ev->body ) );
			const int nOffset = static_cast<int>( ev->time.frames );
			const uint8_t nStatus = msg[0] & 0xF0;
			const int nChannel = toH2Channel( msg[0] & 0x0F );
			if ( nStatus == 0x90 && ev->body.size >= 3 && msg[2] > 0 ) {
				p->engine->noteOn( msg[1], msg[2], nChannel, nOffset );
			} else if ( nStatus == 0x80 ||
						( nStatus == 0x90 && ev->body.size >= 3 && msg[2] == 0 ) ) {
				p->engine->noteOff( msg[1], nChannel, nOffset );
			} else if ( nStatus == 0xB0 && ev->body.size >= 3 ) {
				p->engine->controlChange( msg[1], msg[2], nChannel, nOffset );
			}
		}
	}

	// LV2 transport (time:Position) sync is a follow-up; notes trigger
	// regardless of transport, which is the primary drum-machine use.
	p->engine->process( sample_count, p->masterL, p->masterR, p->busL, p->busR,
						/*bRolling=*/false, /*fBpm=*/0.0, p->nFrame );
	p->nFrame += sample_count;
}

void deactivate( LV2_Handle instance ) {
	auto* p = static_cast<H2Lv2*>( instance );
	if ( p->engine != nullptr ) {
		p->engine->deactivate();
	}
}

void cleanup( LV2_Handle instance ) {
	auto* p = static_cast<H2Lv2*>( instance );
	delete p->engine;
	delete p;
}

const void* extension_data( const char* ) { return nullptr; }

const LV2_Descriptor kDescriptor = {
	H2_LV2_URI, instantiate, connect_port, activate,
	run, deactivate, cleanup, extension_data
};

// ── LV2 UI (ADR 0016 + ADR 0035: editor via ui:showInterface) ──────────────
// Two coexisting contracts until proposal 0006 UI-5 lands: with a
// ui:parent feature the UI embeds a real child window (ADR 0035's
// on-contract embedding, the basic UI's window seam) which the host idles;
// without one the external editor process opens via ui:showInterface (the
// legacy contract). The UI obtains the DSP instance through the
// instance-access feature. The plugin instance owns the external editor
// lifecycle (ADR 0035): only show() drives it - hide(), cleanup() and
// idle() leave the editor alone, and it dies with the DSP instance.
#define H2_LV2_UI_URI H2_LV2_URI "#ui"

struct H2Lv2Ui {
	HydrogenPlugin* engine = nullptr;
	// The embedded child window (ADR 0035); null in external-editor mode.
	std::unique_ptr<H2Core::PluginUiWindow> pWindow;
};

	LV2UI_Handle uiInstantiate( const LV2UI_Descriptor*, const char* /*plugin_uri*/,
							const char* /*bundle_path*/,
							LV2UI_Write_Function /*write*/,
							LV2UI_Controller /*controller*/,
							LV2UI_Widget* widget,
							const LV2_Feature* const* features ) {
	// The editor needs the engine living in the DSP instance; LV2 hands it over
	// via the instance-access feature (UI and DSP share this process).
	HydrogenPlugin* engine = nullptr;
	std::uintptr_t parentHandle = 0;
	for ( int ii = 0; features != nullptr && features[ ii ] != nullptr; ++ii ) {
		if ( std::strcmp( features[ ii ]->URI, LV2_INSTANCE_ACCESS_URI ) == 0 ) {
			auto* p = static_cast<H2Lv2*>( features[ ii ]->data );
			if ( p != nullptr ) {
				engine = p->engine;
			}
		}
		else if ( std::strcmp( features[ ii ]->URI, LV2_UI__parent ) == 0 ) {
			parentHandle = reinterpret_cast<std::uintptr_t>(
				features[ ii ]->data );
		}
	}
	if ( engine == nullptr ) {
		return nullptr; // can't drive the editor without the DSP instance
	}

	auto* ui = new ( std::nothrow ) H2Lv2Ui();
	if ( ui == nullptr ) {
		return nullptr;
	}
	ui->engine = engine;
	if ( parentHandle != 0 ) {
		// Embedded mode (proposal 0006 UI-2): a child window on the host
		// parent, handed back as the widget for the host to idle. The
		// initial footprint fits the three tabs' content (the mixer's
		// strip columns, the mapping tables); the host resizes the
		// child with its FX window (the backend tracks ConfigureNotify).
		ui->pWindow = std::make_unique<H2Core::PluginUiWindow>(
			parentHandle, 960, 600, engine );
		if ( ! ui->pWindow->isValid() ) {
			delete ui;
			return nullptr;
		}
		if ( widget != nullptr ) {
			*widget = reinterpret_cast<LV2UI_Widget>(
				ui->pWindow->nativeHandle() );
		}
	}
	else if ( widget != nullptr ) {
		*widget = nullptr; // no embedded widget; shown via ui:showInterface
	}
	return static_cast<LV2UI_Handle>( ui );
}

void uiCleanup( LV2UI_Handle handle ) {
	auto* ui = static_cast<H2Lv2Ui*>( handle );
	// The editor is owned by the plugin instance, not the UI (ADR 0035): it
	// dies with the DSP instance (the HydrogenPlugin destructor closes it).
	delete ui;
}

void uiPortEvent( LV2UI_Handle, uint32_t, uint32_t, uint32_t, const void* ) {}

int uiShow( LV2UI_Handle handle ) {
	auto* ui = static_cast<H2Lv2Ui*>( handle );
	if ( ui == nullptr || ui->engine == nullptr ) {
		return 1;
	}
	// Embedded mode: the child window is already mapped on the host parent
	// (ADR 0035's embedding contract); show() has nothing to drive. UI-5
	// (proposal 0006) retires the external-editor path entirely.
	if ( ui->pWindow != nullptr ) {
		return 0;
	}
	return ui->engine->openEditor() ? 0 : 1;
}

int uiHide( LV2UI_Handle ) {
	// Interim no-op (ADR 0035): the plugin instance owns the editor, so hiding
	// the UI must not close it. UI-5 (proposal 0006) turns this into hiding
	// the embedded basic-UI window.
	return 0;
}

int uiIdle( LV2UI_Handle handle ) {
	auto* ui = static_cast<H2Lv2Ui*>( handle );
	if ( ui == nullptr || ui->engine == nullptr ) {
		return 1;
	}
	// Embedded mode: one frame per idle (ADR 0035: the host drives
	// rendering). Interim rule (proposal 0006, TU0.2): idle() returns 0 for
	// the lifetime of the handle so no host tears the UI down while the
	// plugin instance lives. UI-5 (TU5.2) replaces this with window-driven
	// semantics.
	if ( ui->pWindow != nullptr ) {
		ui->pWindow->idle();
	}
	return 0;
}

const LV2UI_Show_Interface kShowInterface = { uiShow, uiHide };
const LV2UI_Idle_Interface kIdleInterface = { uiIdle };

const void* uiExtensionData( const char* uri ) {
	if ( std::strcmp( uri, LV2_UI__showInterface ) == 0 ) {
		return &kShowInterface;
	}
	if ( std::strcmp( uri, LV2_UI__idleInterface ) == 0 ) {
		return &kIdleInterface;
	}
	return nullptr;
}

const LV2UI_Descriptor kUiDescriptor = {
	H2_LV2_UI_URI, uiInstantiate, uiCleanup, uiPortEvent, uiExtensionData
};

} // namespace

extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor( uint32_t index ) {
	return index == 0 ? &kDescriptor : nullptr;
}

extern "C" LV2_SYMBOL_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor( uint32_t index ) {
	return index == 0 ? &kUiDescriptor : nullptr;
}
