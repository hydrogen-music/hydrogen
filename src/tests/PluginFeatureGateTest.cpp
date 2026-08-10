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

#include "PluginFeatureGateTest.h"

#include <core/config.h>

#include <memory>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#ifdef H2CORE_HAVE_OSC
#include <core/OscServer.h>
#endif

#include "TestHelper.h"
#include "utils/FakePluginHost.h"

using namespace H2Core;

// A plugin-mode instance with OSC explicitly enabled in its preferences - the
// plugin predicate must still keep OSC off. Owned by the caller (ADR 0015).
static Hydrogen* makePluginInstanceWithOscEnabled() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Plugin;
	pPref->m_midiDriver = Preferences::MidiDriver::Plugin;
	pPref->m_nBufferSize = 1024;
	pPref->setOscServerEnabled( true );
	auto* pHydrogen =
		new Hydrogen( pPref, Hydrogen::ProcessMode::Headless, -1 );
	pHydrogen->setFullyOperational( true );
	return pHydrogen;
}

// The single predicate distinguishes plugin from standalone instances.
void PluginFeatureGateTest::testPredicate() {
	___INFOLOG( "" );

	FakePluginHost host;
	CPPUNIT_ASSERT( host.getHydrogen()->isUnderPluginHost() );

	// The standalone test engine (fake driver) is not under a plugin host.
	CPPUNIT_ASSERT( ! pTestHydrogen()->isUnderPluginHost() );

	___INFOLOG( "passed" );
}

// Under a plugin host the tempo source is the host transport, which structurally
// disables Tap Tempo, Beat Counter, MIDI-clock-in and Timeline tempo (all of
// which only act when the source is Song / their own).
void PluginFeatureGateTest::testTempoSourceIsPlugin() {
	___INFOLOG( "" );

	FakePluginHost host;
	CPPUNIT_ASSERT_EQUAL( Hydrogen::Tempo::Plugin,
						  host.getHydrogen()->getTempoSource() );

	___INFOLOG( "passed" );
}

// Beat Counter is inert under a plugin host (it requires the Song tempo source).
void PluginFeatureGateTest::testBeatCounterInert() {
	___INFOLOG( "" );

	FakePluginHost host;
	// Two taps that would otherwise begin accumulating a tempo estimate.
	CPPUNIT_ASSERT( ! host.getHydrogen()->handleBeatCounter() );
	CPPUNIT_ASSERT( ! host.getHydrogen()->handleBeatCounter() );

	___INFOLOG( "passed" );
}

// Playlist navigation actions are filtered out under a plugin host. The action
// type IS registered, so without the gate the async path would accept it.
void PluginFeatureGateTest::testPlaylistMidiActionsDisabled() {
	___INFOLOG( "" );

	FakePluginHost host;
	auto pManager = host.getHydrogen()->getMidiActionManager();

	for ( const auto type : { MidiAction::Type::PlaylistSong,
							  MidiAction::Type::PlaylistNextSong,
							  MidiAction::Type::PlaylistPrevSong } ) {
		auto pAction = std::make_shared<MidiAction>( type );
		CPPUNIT_ASSERT( ! pManager->handleMidiActionAsync( pAction ) );
		CPPUNIT_ASSERT( ! pManager->handleMidiActionSync( pAction ) );
	}

	// A non-playlist action is still accepted by the async path (sanity check
	// that we only gate playlist actions, not everything).
	auto pPlay = std::make_shared<MidiAction>( MidiAction::Type::Play );
	CPPUNIT_ASSERT( pManager->handleMidiActionAsync( pPlay ) );

	___INFOLOG( "passed" );
}

// Loop is effectively forced on: the engine never reports the end of the song,
// so transport never auto-stops - the host owns position and looping.
void PluginFeatureGateTest::testLoopForcedOn() {
	___INFOLOG( "" );

	FakePluginHost host;
	auto* pEngine = host.getHydrogen()->getAudioEngine();

	CPPUNIT_ASSERT( ! pEngine->isEndOfSongReached( pEngine->getPlayhead() ) );

	___INFOLOG( "passed" );
}

// OSC must not be started under a plugin host even when enabled in preferences.
void PluginFeatureGateTest::testOscServerNotStarted() {
	___INFOLOG( "" );

	auto* pHydrogen = makePluginInstanceWithOscEnabled();

	CPPUNIT_ASSERT( pHydrogen->isUnderPluginHost() );

	// Enabling OSC explicitly must remain a no-op under a plugin host.
	pHydrogen->toggleOscServer( true );

#ifdef H2CORE_HAVE_OSC
	CPPUNIT_ASSERT( pHydrogen->getOscServer() != nullptr );
	// The server thread is only created by start(); under a plugin host start()
	// is never reached, so it stays null (no port is bound).
	CPPUNIT_ASSERT( pHydrogen->getOscServer()->getServerThread() == nullptr );
#endif

	delete pHydrogen;

	___INFOLOG( "passed" );
}
