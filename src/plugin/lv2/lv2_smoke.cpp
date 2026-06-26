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
// cleanup) and assert the master output stays finite. This stands in for the
// per-format "instantiate, activate, process silence, no crash" CI check where
// a full LV2 host / lv2lint is not available. Usage: lv2_smoke <hydrogen.so>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/urid/urid.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

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
} // namespace

int main( int argc, char** argv ) {
	if ( argc < 2 ) {
		std::fprintf( stderr, "usage: %s <hydrogen.so>\n", argv[0] );
		return 2;
	}
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
	d->cleanup( inst );

	bool bOk = true;
	for ( uint32_t p = 1; p <= 2; ++p ) {
		for ( float v : audio[p] ) {
			if ( ! std::isfinite( v ) ) {
				bOk = false;
			}
		}
	}
	dlClose( h );

	std::printf( bOk ? "LV2 SMOKE: PASSED\n" : "LV2 SMOKE: FAILED\n" );
	return bOk ? 0 : 1;
}
