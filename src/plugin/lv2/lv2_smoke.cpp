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
// cleanup) and assert the master output stays finite, then drive the UI
// descriptor through its show/hide/idle semantics (proposal 0006, phase UI-0).
// This stands in for the per-format "instantiate, activate, process silence,
// no crash" CI check where a full LV2 host / lv2lint is not available.
// Usage: lv2_smoke <hydrogen.so>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/urid/urid.h>
#include <lv2/ui/ui.h>
#include <lv2/instance-access/instance-access.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtNetwork/QLocalSocket>

// X11 last: Xlib.h #defines Bool/Status/True/False, which poison any Qt
// header included after it.
#if defined( H2_LV2_SMOKE_HAVE_X11 )
#include <X11/Xlib.h>
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
	// 1 MIDI in + 2 master + 2 * H2_PLUGIN_OUTPUT_BUSES; query indices from the
	// descriptor is overkill - we connect a generous, fixed count.
	const uint32_t nPorts = 1 + 2 + 2 * 32;

	// An empty MIDI sequence on port 0 (no events).
	LV2_Atom_Sequence midi;
	std::memset( &midi, 0, sizeof( midi ) );
	midi.atom.size = sizeof( LV2_Atom_Sequence_Body );
	midi.atom.type = mapUri( nullptr, "http://lv2plug.in/ns/ext/atom#Sequence" );
	d->connect_port( inst, 0, &midi );

	std::vector<std::vector<float>> audio( nPorts );
	for ( uint32_t p = 1; p < nPorts; ++p ) {
		audio[p].assign( nFrames, 0.0f );
		d->connect_port( inst, p, audio[p].data() );
	}

	if ( d->activate != nullptr ) {
		d->activate( inst );
	}
	for ( int b = 0; b < 8; ++b ) {
		d->run( inst, nFrames );
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
	// (same binary, same process). No ui:parent feature, so no window is
	// created and the widget out-param must come back null (the current
	// no-embed contract; the basic UI, proposal 0006 UI-5, replaces this with
	// a real child window).
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

	// Editor-session observation: openEditor() binds a QLocalServer named
	// "hydrogen-editor-<pid>-<instance id>" (HydrogenPlugin.cpp). This process
	// hosts exactly one engine instance, and the per-instance counter starts
	// at 0 (Hydrogen.cpp), so the id is 0.
	const QString sEndpoint = QString( "hydrogen-editor-%1-%2" )
		.arg( QCoreApplication::applicationPid() ).arg( 0 );

	// TU0.3: hide() must not close the editor - the plugin instance owns the
	// editor lifecycle (ADR 0035); the editor dies with the DSP instance, not
	// with the UI.
	if ( showIface->show( ui ) != 0 ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: show() failed\n" );
	}
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: editor session not serving after show()\n" );
	}
	// The interim rule is stronger than the pre-show case alone: idle()
	// returns 0 for the lifetime of the handle (superseded at UI-5, TU5.2).
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
		std::fprintf( stderr, "LV2 UI SMOKE: hide() closed the editor\n" );
	}

	// TU0.4: cleanup() must not close the editor either. Re-show first so a
	// broken cleanup() has a live session to destroy.
	if ( showIface->show( ui ) != 0 ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: re-show failed\n" );
	}
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: editor session not serving after re-show\n" );
	}
	ud->cleanup( ui );
	if ( ! editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: uiCleanup closed the editor\n" );
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
#endif

	d->cleanup( inst );

	// The editor dies with the DSP instance (the HydrogenPlugin destructor
	// closes it), not with the UI.
	if ( editorEndpointAlive( sEndpoint ) ) {
		bUiOk = false;
		std::fprintf( stderr, "LV2 UI SMOKE: editor session outlived the DSP instance\n" );
	}

	bool bOk = true;
	for ( uint32_t p = 1; p <= 2; ++p ) {
		for ( float v : audio[p] ) {
			if ( ! std::isfinite( v ) ) {
				bOk = false;
			}
		}
	}
	dlClose( h );

	const bool bPassed = bOk && bUiOk;
	std::printf( bPassed ? "LV2 SMOKE: PASSED\n" : "LV2 SMOKE: FAILED\n" );
	return bPassed ? 0 : 1;
}
