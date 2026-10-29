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

// Minimal LV2 host smoke test (ADR 0014): dlopen the built plugin, drive its
// full lifecycle (instantiate → connect → activate → run silence → deactivate →
// cleanup) and assert the master output stays finite, exercise the state
// interface (save → restore round-trip through a second instance), then drive
// the UI descriptor through its show/hide/idle semantics (proposal 0006,
// phase UI-0).
// This stands in for the per-format "instantiate, activate, process silence,
// no crash" CI check where a full LV2 host / lv2lint is not available.
// Usage: lv2_smoke <hydrogen.so>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/urid/urid.h>
#include <lv2/ui/ui.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/state/state.h>
#include <lv2/time/time.h>

#include <plugin/lv2/Lv2Transport.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtNetwork/QLocalSocket>

// X11 last: Xlib.h #defines Bool/Status/True/False, which poison any Qt
// header included after it.
#if defined( H2_LV2_SMOKE_HAVE_X11 )
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

// Cross-platform dynamic-loading shim: POSIX dlopen on Linux/macOS, the Win32
// loader on Windows (MinGW has no <dlfcn.h>). LV2 is built on all CI platforms,
// so this host must compile everywhere.
#if defined( _WIN32 )
#include <windows.h>
namespace {
void* dlOpen( const char* path ) {
	return reinterpret_cast<void*>( LoadLibraryA( path ) );
}
void* dlSym( void* handle, const char* name ) {
	return reinterpret_cast<void*>(
		GetProcAddress( reinterpret_cast<HMODULE>( handle ), name ) );
}
void dlClose( void* handle ) {
	FreeLibrary( reinterpret_cast<HMODULE>( handle ) );
}
const char* dlErr() { return "LoadLibrary/GetProcAddress failed"; }
} // namespace
#else
#include <dlfcn.h>
namespace {
void* dlOpen( const char* path ) { return dlopen( path, RTLD_NOW | RTLD_LOCAL ); }
void* dlSym( void* handle, const char* name ) { return dlsym( handle, name ); }
void dlClose( void* handle ) { dlclose( handle ); }
const char* dlErr() { return dlerror(); }
} // namespace
#endif

namespace {
std::map<std::string, uint32_t> g_uris;
uint32_t g_next = 1;
LV2_URID mapUri( LV2_URID_Map_Handle, const char* uri ) {
	auto it = g_uris.find( uri );
	if ( it != g_uris.end() ) {
		return it->second;
	}
	const uint32_t id = g_next++;
	g_uris[ uri ] = id;
	return id;
}

// Whether the engine's editor serve loop is alive, observed through the
// QLocalServer endpoint openEditor() binds (HydrogenPlugin.cpp) - the only
// editor-session state visible from outside the module. Connecting is exactly
// how a spawned editor finds the engine on startup.
bool editorEndpointAlive( const QString& sEndpoint ) {
	QLocalSocket socket;
	socket.connectToServer( sEndpoint );
	if ( ! socket.waitForConnected( 1000 ) ) {
		return false;
	}
	// The local socket usually disconnects synchronously; only wait when the
	// close is still in flight (an unconditional wait makes Qt warn about
	// waiting in UnconnectedState).
	socket.disconnectFromServer();
	if ( socket.state() != QLocalSocket::UnconnectedState ) {
		socket.waitForDisconnected( 1000 );
	}
	return true;
}
} // namespace

int main( int argc, char** argv ) {
	if ( argc < 2 ) {
		std::fprintf( stderr, "usage: %s <hydrogen.so>\n", argv[0] );
		return 2;
	}

	// Deterministic editor spawn failure: show() launches the resolved editor
	// binary; point the resolver at a name no PATH will ever resolve so no
	// real editor window can pop up mid-test on a dev machine.
	qputenv( "HYDROGEN_EDITOR_PATH", "hydrogen-editor-smoke-does-not-exist" );

	void* h = dlOpen( argv[1] );
	if ( h == nullptr ) {
		std::fprintf( stderr, "dlopen failed: %s\n", dlErr() );
		return 1;
	}

	using DescFn = const LV2_Descriptor* ( * )( uint32_t );
	auto lv2_descriptor = reinterpret_cast<DescFn>( dlSym( h, "lv2_descriptor" ) );
	if ( lv2_descriptor == nullptr ) {
		std::fprintf( stderr, "no lv2_descriptor symbol\n" );
		return 1;
	}
	const LV2_Descriptor* d = lv2_descriptor( 0 );
	if ( d == nullptr ) {
		std::fprintf( stderr, "lv2_descriptor(0) returned null\n" );
		return 1;
	}
	std::printf( "URI: %s\n", d->URI );

	LV2_URID_Map map{ nullptr, mapUri };
	LV2_Feature mapFeat{ LV2_URID__map, &map };
	const LV2_Feature* features[] = { &mapFeat, nullptr };

	LV2_Handle inst = d->instantiate( d, 44100.0, "/tmp/", features );
	if ( inst == nullptr ) {
		std::fprintf( stderr, "instantiate returned null\n" );
		return 1;
	}

	const uint32_t nFrames = 256;
	// 1 control + 1 MIDI in + 2 master + 2 * H2_PLUGIN_OUTPUT_BUSES; query
	// indices from the descriptor is overkill - we connect a generous, fixed
	// count.
	const uint32_t nPorts = 2 + 2 + 2 * 32;

	// Empty atom sequences on the two input ports (no events, no transport
	// published this block).
	LV2_Atom_Sequence control;
	std::memset( &control, 0, sizeof( control ) );
	control.atom.size = sizeof( LV2_Atom_Sequence_Body );
	control.atom.type = mapUri( nullptr, LV2_ATOM__Sequence );
	d->connect_port( inst, 0, &control );

	LV2_Atom_Sequence midi;
	std::memset( &midi, 0, sizeof( midi ) );
	midi.atom.size = sizeof( LV2_Atom_Sequence_Body );
	midi.atom.type = mapUri( nullptr, LV2_ATOM__Sequence );
	d->connect_port( inst, 1, &midi );

	std::vector<std::vector<float>> audio( nPorts );
	for ( uint32_t p = 2; p < nPorts; ++p ) {
		audio[p].assign( nFrames, 0.0f );
		d->connect_port( inst, p, audio[p].data() );
	}

	if ( d->activate != nullptr ) {
		d->activate( inst );
	}
	for ( int b = 0; b < 8; ++b ) {
		d->run( inst, nFrames );
	}

	// ── Transport checks (time:Position, ADR 0013) ─────────────────────────
	// The latch (Lv2Transport.h) is the module's transport parser, shared
	// source with this smoke: forge host atom sequences and check the parse
	// directly. The engine's following of the published values is covered by
	// the FakePluginHost transport tests; here the end-to-end part only
	// proves the module digests a published transport without disturbing the
	// render.
	bool bTransportOk = true;
	{
		H2Core::Lv2TransportLatch latch;
		latch.mapUrids( &map );

		// Forge a control sequence carrying one time:Position object with
		// the given properties (flags skip frame / bpm, modeling a host
		// that publishes only what it knows).
		uint8_t controlBuf[ 512 ];
		auto forgePosition = [ & ]( float fSpeed, bool bWithFrame,
									long long nFrameArg, bool bWithBpm,
									float fBpmArg ) -> LV2_Atom_Sequence* {
			LV2_Atom_Forge forge;
			lv2_atom_forge_init( &forge, &map );
			lv2_atom_forge_set_buffer( &forge, controlBuf,
									   sizeof( controlBuf ) );
			LV2_Atom_Forge_Frame seq;
			lv2_atom_forge_sequence_head( &forge, &seq, 0 );
			lv2_atom_forge_frame_time( &forge, 0 );
			LV2_Atom_Forge_Frame obj;
			lv2_atom_forge_object( &forge, &obj, 0,
								   mapUri( nullptr, LV2_TIME__Position ) );
			lv2_atom_forge_key( &forge, mapUri( nullptr, LV2_TIME__speed ) );
			lv2_atom_forge_float( &forge, fSpeed );
			if ( bWithFrame ) {
				lv2_atom_forge_key( &forge, mapUri( nullptr, LV2_TIME__frame ) );
				lv2_atom_forge_long( &forge, nFrameArg );
			}
			if ( bWithBpm ) {
				lv2_atom_forge_key( &forge,
									mapUri( nullptr, LV2_TIME__beatsPerMinute ) );
				lv2_atom_forge_float( &forge, fBpmArg );
			}
			lv2_atom_forge_pop( &forge, &obj );
			lv2_atom_forge_pop( &forge, &seq );
			return reinterpret_cast<LV2_Atom_Sequence*>( controlBuf );
		};

		// Rolling at 132 BPM from frame 44100: all three latch.
		latch.read( forgePosition( 1.0f, true, 44100, true, 132.0f ) );
		if ( ! latch.bRolling || latch.fBpm != 132.0 || latch.nFrame != 44100 ) {
			bTransportOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: latch did not adopt a rolling position\n" );
		}

		// An empty control sequence (a host that writes only on change): the
		// latched state stands, and advance() moves the frame while rolling.
		latch.read( &control );
		latch.advance( nFrames );
		if ( ! latch.bRolling || latch.fBpm != 132.0 ||
			 latch.nFrame != 44100 + 256 ) {
			bTransportOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: latch did not hold its state over an empty block\n" );
		}

		// Stopped (speed 0) at a new frame: rolling clears and the frame
		// relocates; the unresent tempo latches on. advance() stands while
		// stopped.
		latch.read( forgePosition( 0.0f, true, 88200, false, 0.0f ) );
		latch.advance( nFrames );
		if ( latch.bRolling || latch.nFrame != 88200 || latch.fBpm != 132.0 ) {
			bTransportOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: latch did not follow a stop\n" );
		}

		// Reverse (speed -1) cannot be rendered: latches as stopped.
		latch.read( forgePosition( -1.0f, false, 0, false, 0.0f ) );
		if ( latch.bRolling ) {
			bTransportOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: reverse transport latched as rolling\n" );
		}

		// A wrong-typed property (frame as Float instead of Long) is
		// skipped: the latched frame survives.
		{
			LV2_Atom_Forge forge;
			lv2_atom_forge_init( &forge, &map );
			lv2_atom_forge_set_buffer( &forge, controlBuf,
									   sizeof( controlBuf ) );
			LV2_Atom_Forge_Frame seq;
			lv2_atom_forge_sequence_head( &forge, &seq, 0 );
			lv2_atom_forge_frame_time( &forge, 0 );
			LV2_Atom_Forge_Frame obj;
			lv2_atom_forge_object( &forge, &obj, 0,
								   mapUri( nullptr, LV2_TIME__Position ) );
			lv2_atom_forge_key( &forge, mapUri( nullptr, LV2_TIME__frame ) );
			lv2_atom_forge_float( &forge, 123.0f ); // spec says Long
			lv2_atom_forge_pop( &forge, &obj );
			lv2_atom_forge_pop( &forge, &seq );
			latch.read( reinterpret_cast<LV2_Atom_Sequence*>( controlBuf ) );
		}
		if ( latch.nFrame != 88200 ) {
			bTransportOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: wrong-typed frame property was not skipped\n" );
		}

		// End-to-end: the module's run() must digest a published transport
		// (rolling, then stopped) without disturbing the render.
		d->connect_port( inst, 0,
			forgePosition( 1.0f, true, 44100, true, 132.0f ) );
		for ( int b = 0; b < 4; ++b ) {
			d->run( inst, nFrames );
		}
		d->connect_port( inst, 0,
			forgePosition( 0.0f, true, 88200, false, 0.0f ) );
		for ( int b = 0; b < 2; ++b ) {
			d->run( inst, nFrames );
		}
		for ( uint32_t p = 2; p <= 3; ++p ) {
			for ( float v : audio[ p ] ) {
				if ( ! std::isfinite( v ) ) {
					bTransportOk = false;
				}
			}
		}
	}

	// ── Plugin state checks (LV2 state extension) ──────────────────────────
	// The state interface must persist the `.h2project` blob as exactly one
	// atom:Chunk property (POD|PORTABLE — the default configuration embeds
	// the samples, so the blob is self-contained) and restore it back.
	// Host-side store/retrieve stand-ins record what save() hands over; a
	// second instance proves the restore path in the host order that
	// matters (instantiate → restore → activate → run, Ardour's sequence).
	bool bStateOk = true;
	const LV2_State_Interface* st = static_cast<const LV2_State_Interface*>(
		d->extension_data( LV2_STATE__interface ) );
	if ( st == nullptr || st->save == nullptr || st->restore == nullptr ) {
		bStateOk = false;
		std::fprintf( stderr, "LV2 SMOKE: no state interface\n" );
	}

	// A host's state map: key URID → stored property.
	struct StoredProperty {
		std::vector<unsigned char> data;
		uint32_t type = 0;
		uint32_t flags = 0;
	};
	using StateMap = std::map<uint32_t, StoredProperty>;
	const auto hostStore = []( LV2_State_Handle handle, uint32_t key,
							   const void* value, size_t size, uint32_t type,
							   uint32_t flags ) -> LV2_State_Status {
		auto& m = *static_cast<StateMap*>( handle );
		const auto* bytes = static_cast<const unsigned char*>( value );
		m[ key ] = { { bytes, bytes + size }, type, flags };
		return LV2_STATE_SUCCESS;
	};
	const auto hostRetrieve = []( LV2_State_Handle handle, uint32_t key,
								  size_t* size, uint32_t* type,
								  uint32_t* flags ) -> const void* {
		const auto& m = *static_cast<const StateMap*>( handle );
		const auto it = m.find( key );
		if ( it == m.end() ) {
			// An absent key: the plugin must fall back to its defaults.
			return nullptr;
		}
		if ( size != nullptr ) { *size = it->second.data.size(); }
		if ( type != nullptr ) { *type = it->second.type; }
		if ( flags != nullptr ) { *flags = it->second.flags; }
		return it->second.data.data();
	};

	// The one property save() must store: the state blob under the plugin's
	// #state key (H2_LV2_URI in HydrogenLv2.cpp).
	const uint32_t stateKey =
		mapUri( nullptr, "https://hydrogen-music.org/lv2/hydrogen#state" );
	const uint32_t chunkType = mapUri( nullptr, LV2_ATOM__Chunk );

	// save() on the live instance: LV2 permits it concurrent with run();
	// the engine lock keeps the snapshot consistent.
	StateMap savedFromA;
	if ( bStateOk ) {
		if ( st->save( inst, hostStore, &savedFromA, 0, nullptr ) !=
			 LV2_STATE_SUCCESS ) {
			bStateOk = false;
			std::fprintf( stderr, "LV2 SMOKE: state save() failed\n" );
		}
	}
	if ( bStateOk ) {
		const auto it = savedFromA.find( stateKey );
		if ( savedFromA.size() != 1 || it == savedFromA.end() ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: save() did not store exactly the #state property\n" );
		} else {
			if ( it->second.type != chunkType ) {
				bStateOk = false;
				std::fprintf( stderr,
					"LV2 SMOKE: state property is not an atom:Chunk\n" );
			}
			if ( it->second.flags !=
				 ( LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE ) ) {
				bStateOk = false;
				std::fprintf( stderr,
					"LV2 SMOKE: state property is not flagged POD|PORTABLE "
					"(pluginStateEmbedSamples disabled in the ambient "
					"config?)\n" );
			}
			// The gzip magic proves the payload is the `.h2project`
			// container, not some other serialization.
			if ( it->second.data.size() < 2 ||
				 it->second.data[ 0 ] != 0x1f || it->second.data[ 1 ] != 0x8b ) {
				bStateOk = false;
				std::fprintf( stderr,
					"LV2 SMOKE: state property is not the gzip'd container\n" );
			}
		}
	}

	// A second instance walks the restore path end to end.
	LV2_Handle instB = nullptr;
	if ( bStateOk ) {
		instB = d->instantiate( d, 44100.0, "/tmp/", features );
		if ( instB == nullptr ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: second instantiate returned null\n" );
		}
	}
	if ( bStateOk ) {
		// An empty state map (a host resetting to defaults): retrieve finds
		// nothing; restore must succeed without touching the engine.
		StateMap empty;
		if ( st->restore( instB, hostRetrieve, &empty, 0, nullptr ) !=
			 LV2_STATE_SUCCESS ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: restore() of an empty map failed\n" );
		}
		// The real thing: A's blob into the fresh instance, before
		// activate() — the restored state must survive activation.
		if ( st->restore( instB, hostRetrieve, &savedFromA, 0, nullptr ) !=
			 LV2_STATE_SUCCESS ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: restore() of the saved state failed\n" );
		}
	}
	if ( bStateOk ) {
		d->connect_port( instB, 0, &control );
		d->connect_port( instB, 1, &midi );
		for ( uint32_t p = 2; p < nPorts; ++p ) {
			d->connect_port( instB, p, audio[ p ].data() );
		}
		if ( d->activate != nullptr ) {
			d->activate( instB );
		}
		for ( int b = 0; b < 4; ++b ) {
			d->run( instB, nFrames );
		}
	}

	// The restored instance must save the same state back: byte-stable
	// (the archive writer is deterministic — no mtimes, fixed entry order)
	// and equal, proving the round-trip neither lost nor altered it.
	StateMap savedFromB;
	if ( bStateOk ) {
		if ( st->save( instB, hostStore, &savedFromB, 0, nullptr ) !=
			 LV2_STATE_SUCCESS ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: state save() on the restored instance failed\n" );
		}
	}
	if ( bStateOk ) {
		const auto a = savedFromA.find( stateKey );
		const auto b = savedFromB.find( stateKey );
		if ( a == savedFromA.end() || b == savedFromB.end() ||
			 a->second.data != b->second.data ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: state blob not stable across save → restore → save\n" );
		}
	}

	// A wrong-typed property must be rejected without touching the engine.
	if ( bStateOk ) {
		StateMap wrongType = savedFromB;
		wrongType[ stateKey ].type = mapUri( nullptr, LV2_ATOM__String );
		if ( st->restore( instB, hostRetrieve, &wrongType, 0, nullptr ) !=
			 LV2_STATE_ERR_BAD_TYPE ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: restore() accepted a non-Chunk property\n" );
		}
	}

	// Hosts also restore into live instances (between run() calls).
	if ( bStateOk ) {
		if ( st->restore( inst, hostRetrieve, &savedFromB, 0, nullptr ) !=
			 LV2_STATE_SUCCESS ) {
			bStateOk = false;
			std::fprintf( stderr,
				"LV2 SMOKE: restore() into the live instance failed\n" );
		}
	}

	if ( instB != nullptr ) {
		if ( d->deactivate != nullptr ) {
			d->deactivate( instB );
		}
		d->cleanup( instB );
	}

	if ( d->deactivate != nullptr ) {
		d->deactivate( inst );
	}

	// ── LV2 UI checks (proposal 0006, phase UI-0) ──────────────────────────
	// The UI lives in the same binary (hydrogen.ttl: ui:binary <hydrogen.so>).
	using UiDescFn = const LV2UI_Descriptor* ( * )( uint32_t );
	auto lv2ui_descriptor =
		reinterpret_cast<UiDescFn>( dlSym( h, "lv2ui_descriptor" ) );
	if ( lv2ui_descriptor == nullptr ) {
		std::fprintf( stderr, "no lv2ui_descriptor symbol\n" );
		d->cleanup( inst );
		dlClose( h );
		return 1;
	}
	const LV2UI_Descriptor* ud = lv2ui_descriptor( 0 );
	if ( ud == nullptr ) {
		std::fprintf( stderr, "lv2ui_descriptor(0) returned null\n" );
		d->cleanup( inst );
		dlClose( h );
		return 1;
	}
	std::printf( "UI URI: %s\n", ud->URI );

	// extension_data must serve both interfaces the ttl declares.
	const auto* showIface = static_cast<const LV2UI_Show_Interface*>(
		ud->extension_data( LV2_UI__showInterface ) );
	const auto* idleIface = static_cast<const LV2UI_Idle_Interface*>(
		ud->extension_data( LV2_UI__idleInterface ) );
	if ( showIface == nullptr || showIface->show == nullptr ||
		 showIface->hide == nullptr ) {
		std::fprintf( stderr, "LV2 UI SMOKE: no showInterface\n" );
		d->cleanup( inst );
		dlClose( h );
		return 1;
	}
	if ( idleIface == nullptr || idleIface->idle == nullptr ) {
		std::fprintf( stderr, "LV2 UI SMOKE: no idleInterface\n" );
		d->cleanup( inst );
		dlClose( h );
		return 1;
	}

	// Semantic checks accumulate; setup failures above returned early.
	bool bUiOk = std::strcmp( ud->URI,
							  "https://hydrogen-music.org/lv2/hydrogen#ui" ) == 0;
	if ( ! bUiOk ) {
		std::fprintf( stderr, "LV2 UI SMOKE: unexpected UI URI\n" );
	}

	// Headless UI instantiate: instance-access hands the UI the DSP instance
	// (same binary, same process). No ui:parent feature, so no embedded
	// widget: the out-param must come back null — without a parent the UI is
	// the showInterface fallback (UI-5, TU5.2), not an embed.
	LV2_Feature instFeat{ LV2_INSTANCE_ACCESS_URI, inst };
	const LV2_Feature* uiFeatures[] = { &mapFeat, &instFeat, nullptr };
	LV2UI_Widget widget = reinterpret_cast<LV2UI_Widget>( 0x1 );
	LV2UI_Handle ui = ud->instantiate( ud, d->URI, "/tmp/", nullptr, nullptr,
									   &widget, uiFeatures );
	if ( ui == nullptr ) {
		std::fprintf( stderr, "LV2 UI SMOKE: uiInstantiate returned null\n" );
		d->cleanup( inst );
		dlClose( h );
		return 1;
	}
	if ( widget != nullptr ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: widget not null without a parent\n" );
	}

	// TU0.2: a host idling a freshly instantiated UI must not be told to
	// destroy it - idle() returns 0 before any show().
	if ( idleIface->idle( ui ) != 0 ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: idle() != 0 before show()\n" );
	}

	// Editor-session observation: the engine session is always-on (UI-3: it
	// starts with the plugin instance and serves for its whole lifetime),
	// binding a QLocalServer named "hydrogen-editor-<pid>-<instance id>"
	// (HydrogenPlugin.cpp). By the time the UI checks run this process hosts
	// exactly one engine instance again (the state section's second instance
	// is cleaned up before them), and its instance log file suffix was
	// allocated first in this fresh process (Hydrogen.cpp), so the id is 0.
	// The editor PROCESS is only
	// ever spawned by the landing view's Open editor button (UI-5, TU5.7)
	// — show() must not spawn it.
	const QString sEndpoint = QString( "hydrogen-editor-%1-%2" )
		.arg( QCoreApplication::applicationPid() ).arg( 0 );

	// The fallback contract (UI-5, TU5.2): show() creates the self-managed
	// top-level window where a display exists; without one (headless CI)
	// it fails gracefully and returns non-zero, so its return value is
	// only asserted where a display exists (the X11 fallback section
	// below). Display-independent: show/hide/cleanup must not disturb the
	// always-on engine session, and idle() must keep returning 0 — a
	// display-less show is a failure, not a close; the host owns the
	// teardown.
	showIface->show( ui );
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: show() disturbed the engine session\n" );
	}
	if ( idleIface->idle( ui ) != 0 ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: idle() != 0 after show()\n" );
	}
	if ( showIface->hide( ui ) != 0 ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: hide() failed\n" );
	}
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: hide() disturbed the engine session\n" );
	}

	// Re-show so a broken cleanup() has a live fallback window to leak;
	// cleanup must tear it down without disturbing the engine session.
	showIface->show( ui );
	ud->cleanup( ui );
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: uiCleanup disturbed the engine session\n" );
	}

	// ── Embedded-widget checks (proposal 0006, UI-2: TU5.1's red pulled
	// forward for the spike): with a ui:parent feature the UI must return
	// a real child window id (ADR 0035's on-contract embedding). X11-only:
	// it needs a display to create the test parent; headless CI compiles
	// the section out.
#if defined( H2_LV2_SMOKE_HAVE_X11 )
	{
		Display* pDisplay = XOpenDisplay( nullptr );
		if ( pDisplay == nullptr ) {
			std::printf( "LV2 UI SMOKE: SKIP embedded section — no DISPLAY\n" );
		} else {
			const Window parent = XCreateSimpleWindow(
				pDisplay, DefaultRootWindow( pDisplay ), 0, 0, 300, 200, 0,
				BlackPixel( pDisplay, DefaultScreen( pDisplay ) ),
				WhitePixel( pDisplay, DefaultScreen( pDisplay ) ) );
			XMapWindow( pDisplay, parent );
			XFlush( pDisplay );

		LV2_Feature parentFeat{ LV2_UI__parent,
			reinterpret_cast<void*>( parent ) };
			const LV2_Feature* embedFeatures[] =
				{ &mapFeat, &instFeat, &parentFeat, nullptr };
			LV2UI_Widget embedWidget = nullptr;
			LV2UI_Handle embedUi = ud->instantiate( ud, d->URI, "/tmp/",
				nullptr, nullptr, &embedWidget, embedFeatures );
			bool bEmbedOk = embedUi != nullptr;
			if ( ! bEmbedOk ) {
				std::fprintf( stderr,
					"LV2 UI SMOKE: uiInstantiate returned null with a parent\n" );
			} else {
				if ( embedWidget == nullptr ) {
					bEmbedOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: widget null with a parent (TU5.1)\n" );
				} else {
					// The widget must be a mapped child of the test parent.
					const Window child =
						reinterpret_cast<Window>( embedWidget );
					XWindowAttributes attr = {};
					XGetWindowAttributes( pDisplay, child, &attr );
					if ( attr.map_state != IsViewable ) {
						bEmbedOk = false;
						std::fprintf( stderr,
							"LV2 UI SMOKE: embedded child not mapped\n" );
					}
					Window root = None;
					Window parentOfChild = None;
					Window* pChildren = nullptr;
					unsigned nChildren = 0;
					XQueryTree( pDisplay, child, &root, &parentOfChild,
						&pChildren, &nChildren );
					if ( parentOfChild != parent ) {
						bEmbedOk = false;
						std::fprintf( stderr,
							"LV2 UI SMOKE: embedded child not parented on the host parent\n" );
					}
					if ( pChildren != nullptr ) {
						XFree( pChildren );
					}
				}
				// Idle frames must not crash and must not ask for teardown.
				for ( int i = 0; i < 5; ++i ) {
					if ( idleIface->idle( embedUi ) != 0 ) {
						bEmbedOk = false;
						std::fprintf( stderr,
							"LV2 UI SMOKE: idle() != 0 on the embedded UI\n" );
					}
				}
				// Teardown releases the child window with the UI handle.
				ud->cleanup( embedUi );
				XSync( pDisplay, false );
				Window root = None;
				Window parentOfParent = None;
				Window* pChildren = nullptr;
				unsigned nChildren = 0;
				XQueryTree( pDisplay, parent, &root, &parentOfParent,
					&pChildren, &nChildren );
				if ( nChildren != 0 ) {
					bEmbedOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: child window outlived uiCleanup\n" );
				}
				if ( pChildren != nullptr ) {
					XFree( pChildren );
				}
			}
			bUiOk = bUiOk && bEmbedOk;
			XDestroyWindow( pDisplay, parent );
			XCloseDisplay( pDisplay );
		}
	}

	// ── Fallback-window checks (proposal 0006, UI-5: TU5.2): without a
	// ui:parent feature show() must create the self-managed top-level
	// basic-UI window (never the editor process — TU5.7), idle() must
	// drive it and return non-zero once the user closed it, hide() must
	// destroy it, and uiCleanup must tear a recreated one down.
	{
		Display* pDisplay = XOpenDisplay( nullptr );
		if ( pDisplay == nullptr ) {
			std::printf( "LV2 UI SMOKE: SKIP fallback section — no DISPLAY\n" );
		} else {
			auto rootChildren = [ pDisplay ]() -> std::vector<Window> {
				Window root = None;
				Window parentOfRoot = None;
				Window* pChildren = nullptr;
				unsigned nChildren = 0;
				XQueryTree( pDisplay, DefaultRootWindow( pDisplay ), &root,
							&parentOfRoot, &pChildren, &nChildren );
				std::vector<Window> children( pChildren, pChildren + nChildren );
				if ( pChildren != nullptr ) {
					XFree( pChildren );
				}
				return children;
			};
			// Finds our fallback window among the root's new children: a
			// window manager may wrap it in a frame, so the search covers
			// the new subtree (depth <= 2) and matches the WM_CLASS
			// "Hydrogen" the backend sets on the top-level window.
			auto findFallbackWindow = [ pDisplay, rootChildren ](
				const std::vector<Window>& before ) -> Window {
				for ( Window candidate : rootChildren() ) {
					if ( std::find( before.begin(), before.end(), candidate )
							!= before.end() ) {
						continue;
					}
					Window root = None;
					Window parentOfCandidate = None;
					Window* pChildren = nullptr;
					unsigned nChildren = 0;
					XQueryTree( pDisplay, candidate, &root,
								&parentOfCandidate, &pChildren, &nChildren );
					std::vector<Window> subtree{ candidate };
					subtree.insert( subtree.end(), pChildren,
						pChildren + nChildren );
					if ( pChildren != nullptr ) {
						XFree( pChildren );
					}
					for ( Window w : subtree ) {
						XClassHint hint = {};
						if ( XGetClassHint( pDisplay, w, &hint ) != 0 ) {
							const bool bOurs = hint.res_class != nullptr
								&& std::strcmp( hint.res_class, "Hydrogen" ) == 0;
							XFree( hint.res_name );
							XFree( hint.res_class );
							if ( bOurs ) {
								return w;
							}
						}
					}
				}
				return None;
			};
			// The WM destroys its frame asynchronously after our window
			// goes away; poll for the subtree to disappear.
			auto windowGone = [ pDisplay, &findFallbackWindow ](
				const std::vector<Window>& before ) {
				for ( int i = 0; i < 50; ++i ) {
					XSync( pDisplay, false );
					if ( findFallbackWindow( before ) == None ) {
						return true;
					}
					std::this_thread::sleep_for(
						std::chrono::milliseconds( 20 ) );
				}
				return false;
			};

			LV2UI_Widget fallbackWidget = nullptr;
			LV2UI_Handle fallbackUi = ud->instantiate( ud, d->URI, "/tmp/",
				nullptr, nullptr, &fallbackWidget, uiFeatures );
			bool bFallbackOk = fallbackUi != nullptr;
			if ( ! bFallbackOk ) {
				std::fprintf( stderr,
					"LV2 UI SMOKE: fallback uiInstantiate returned null\n" );
			} else {
				if ( fallbackWidget != nullptr ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: fallback widget not null\n" );
				}
				const std::vector<Window> before = rootChildren();
				if ( showIface->show( fallbackUi ) != 0 ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: fallback show() failed (TU5.2)\n" );
				}
				XSync( pDisplay, false );
				const Window fallbackWindow = findFallbackWindow( before );
				if ( fallbackWindow == None ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: show() created no fallback window (TU5.2)\n" );
				} else {
					if ( idleIface->idle( fallbackUi ) != 0 ) {
						bFallbackOk = false;
						std::fprintf( stderr,
							"LV2 UI SMOKE: idle() != 0 while the fallback window is alive (TU5.2)\n" );
					}
					// The user's close button: WM_DELETE_WINDOW through the
					// real pump — idle() must tell the host to tear down.
					XEvent ev = {};
					ev.xclient.type = ClientMessage;
					ev.xclient.window = fallbackWindow;
					ev.xclient.message_type =
						XInternAtom( pDisplay, "WM_PROTOCOLS", False );
					ev.xclient.format = 32;
					ev.xclient.data.l[ 0 ] =
						XInternAtom( pDisplay, "WM_DELETE_WINDOW", False );
					XSendEvent( pDisplay, fallbackWindow, False, 0, &ev );
					XSync( pDisplay, false );
					if ( idleIface->idle( fallbackUi ) == 0 ) {
						bFallbackOk = false;
						std::fprintf( stderr,
							"LV2 UI SMOKE: idle() == 0 after the fallback window closed (TU5.2)\n" );
					}
				}
				if ( showIface->hide( fallbackUi ) != 0 ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: fallback hide() failed (TU5.2)\n" );
				}
				if ( ! windowGone( before ) ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: fallback window outlived hide() (TU5.2)\n" );
				}
				// Re-show, then cleanup must tear the recreated window down.
				showIface->show( fallbackUi );
				XSync( pDisplay, false );
				if ( findFallbackWindow( before ) == None ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: re-show created no fallback window (TU5.2)\n" );
				}
				ud->cleanup( fallbackUi );
				if ( ! windowGone( before ) ) {
					bFallbackOk = false;
					std::fprintf( stderr,
						"LV2 UI SMOKE: fallback window outlived uiCleanup (TU5.2)\n" );
				}
			}
			bUiOk = bUiOk && bFallbackOk;
			XCloseDisplay( pDisplay );
		}
	}
#endif

	d->cleanup( inst );

	// The editor dies with the DSP instance (the HydrogenPlugin destructor
	// closes it), not with the UI.
	if ( editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: editor session outlived the DSP instance\n" );
	}

	bool bOk = true;
	for ( uint32_t p = 2; p <= 3; ++p ) {
		for ( float v : audio[p] ) {
			if ( ! std::isfinite( v ) ) {
				bOk = false;
			}
		}
	}
	dlClose( h );

	const bool bPassed = bOk && bUiOk && bTransportOk && bStateOk;
	std::printf( bPassed ? "LV2 SMOKE: PASSED\n" : "LV2 SMOKE: FAILED\n" );
	return bPassed ? 0 : 1;
}
