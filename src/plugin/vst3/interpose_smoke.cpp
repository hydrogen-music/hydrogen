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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

// VST3 symbol-isolation smoke test.
//
// A host that has the Hydrogen CLAP binary in the global symbol scope (an
// in-process plugin scan of the CLAP folder, or an RTLD_GLOBAL load - what
// Ardour's and Qtractor's scans do) must not be able to corrupt the VST3
// module. Before the Linux version scripts were added, every plugin module
// exported its whole statically linked interior - thousands of H2Core and Qt
// symbols plus `clap_entry` - so the VST3 module's own references resolved
// into the CLAP binary's copies: clap-wrapper initialized and created
// plugins inside the OTHER .so, and once the host's scanner dlclose()d its
// handle those bindings dangled. The next createInstance/initialize then
// crashed the host (Ardour: garbage call from VST3PI::view(); Qtractor: null
// deref in the insert slot).
//
// This smoke reproduces that exact sequence: load the CLAP RTLD_GLOBAL, load
// the VST3, build the factory, UNLOAD the CLAP, then drive the VST3 through
// createInstance + initialize + terminate. With the version scripts the VST3
// binds its own statically linked entry and is unaffected by the unload;
// without them it dies on the dangling descriptor/factory pointers.
//
// The VST3 ABI is driven raw (IPluginFactory/IPluginBase through the SDK
// headers, a FUnknown-only host context): clap-wrapper's initialize() merely
// queries IHostApplication off the context and treats "not provided" as a
// supported answer (every later use is null-guarded).
//
// Usage: interpose_smoke <Hydrogen.clap> <Hydrogen.so>

#include <dlfcn.h>

#include <cstdio>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"

using Steinberg::FUID;
using Steinberg::FUnknown;
using Steinberg::IPluginBase;
using Steinberg::IPluginFactory;
using Steinberg::PClassInfo;
using Steinberg::TUID;
using Steinberg::kNoInterface;
using Steinberg::kResultOk;
using Steinberg::tresult;
using Steinberg::uint32;

namespace {

// Minimal host context: answering FUnknown only is enough for the wrapper's
// initialize() (see the file comment).
class SmokeContext : public FUnknown
{
public:
	SmokeContext() : m_refCount( 1 ) {}
	tresult queryInterface( const TUID iid, void** obj ) override {
		*obj = nullptr;
		if ( FUID::fromTUID( iid ) == FUnknown::iid ) {
			*obj = static_cast<FUnknown*>( this );
			addRef();
			return kResultOk;
		}
		return kNoInterface;
	}
	uint32 addRef() override { return ++m_refCount; }
	// Stack object - the refcount may drop to zero, but there is nothing to
	// delete.
	uint32 release() override { return --m_refCount; }

private:
	uint32 m_refCount;
};

int fail( const char* what ) {
	std::fprintf( stderr, "FAIL: %s\n", what );
	return 1;
}

} // namespace

int main( int argc, char** argv )
{
	if ( argc != 3 ) {
		std::fprintf( stderr, "usage: %s <Hydrogen.clap> <Hydrogen.so>\n",
					  argv[ 0 ] );
		return 2;
	}
	const char* sClapPath = argv[ 1 ];
	const char* sVst3Path = argv[ 2 ];

	// 1. The polluting load: the CLAP binary enters the GLOBAL scope, exactly
	// like a host's in-process scan of the CLAP folder.
	void* pClap = dlopen( sClapPath, RTLD_GLOBAL | RTLD_NOW );
	if ( pClap == nullptr ) {
		return fail( dlerror() );
	}

	// 2. The host's module load of the VST3 (RTLD_LOCAL - the usual flags).
	void* pVst3 = dlopen( sVst3Path, RTLD_LOCAL | RTLD_NOW );
	if ( pVst3 == nullptr ) {
		return fail( dlerror() );
	}

	// 3. The VST3 Linux module contract, driven exactly like the SDK's
	// hosting code (module_linux.cpp): ModuleEntry, then GetPluginFactory.
	// Without the version scripts the factory build binds the wrapper's
	// `clap_entry` and the plugin descriptors to the CLAP binary's copies;
	// with them everything resolves inside the VST3 module.
	auto pModuleEntry =
		reinterpret_cast<bool ( * )( void* )>( dlsym( pVst3, "ModuleEntry" ) );
	if ( pModuleEntry == nullptr ) {
		return fail( "ModuleEntry not found in the VST3 module" );
	}
	if ( !pModuleEntry( pVst3 ) ) {
		return fail( "ModuleEntry" );
	}
	auto pFactoryFn = reinterpret_cast<IPluginFactory*( *)() >(
		dlsym( pVst3, "GetPluginFactory" ) );
	if ( pFactoryFn == nullptr ) {
		return fail( "GetPluginFactory not found in the VST3 module" );
	}
	IPluginFactory* pFactory = pFactoryFn();
	if ( pFactory == nullptr || pFactory->countClasses() < 1 ) {
		return fail( "the factory exposes no class" );
	}
	PClassInfo info;
	if ( pFactory->getClassInfo( 0, &info ) != kResultOk ) {
		return fail( "getClassInfo(0)" );
	}

	// 4. The host's scanner unloads its CLAP handle. From here on, a
	// non-isolated VST3 module operates on dangling pointers.
	if ( dlclose( pClap ) != 0 ) {
		return fail( "dlclose(clap)" );
	}

	// 5. The insert path: createInstance + initialize must run entirely from
	// the VST3 module's own statically linked entry. This is where a
	// non-isolated module crashes (the dangling descriptor/factory pointers).
	void* pObj = nullptr;
	if ( pFactory->createInstance( info.cid, FUnknown::iid.toTUID(), &pObj )
			 != kResultOk
		 || pObj == nullptr ) {
		return fail( "createInstance" );
	}
	auto pPlugin = static_cast<IPluginBase*>( pObj );
	SmokeContext context;
	if ( pPlugin->initialize( &context ) != kResultOk ) {
		return fail( "initialize" );
	}
	if ( pPlugin->terminate() != kResultOk ) {
		return fail( "terminate" );
	}
	// Exactly one release: whatever the factory handed back plus our implicit
	// ownership. Releasing twice would risk a premature delete if the SDK's
	// choreography differs by a reference; a leak of one reference is
	// harmless in a smoke process (the important teardown - the wrapped
	// plugin and its engine session - already happened in terminate()).
	pPlugin->release();

	// 6. Symbol hygiene from the loading side: the VST3 module must not
	// export the CLAP entry (the nm-based ctest gates check the full export
	// list).
	if ( dlsym( pVst3, "clap_entry" ) != nullptr ) {
		return fail( "the VST3 module exports clap_entry" );
	}

	// The SDK's unload path: ModuleExit before dlclose.
	auto pModuleExit =
		reinterpret_cast<bool ( * )( void )>( dlsym( pVst3, "ModuleExit" ) );
	if ( pModuleExit == nullptr ) {
		return fail( "ModuleExit not found in the VST3 module" );
	}
	if ( !pModuleExit() ) {
		return fail( "ModuleExit" );
	}
	if ( dlclose( pVst3 ) != 0 ) {
		return fail( "dlclose(vst3)" );
	}

	std::printf( "PASS: the VST3 module is self-contained under a CLAP "
				 "global-scope load\n" );
	return 0;
}
