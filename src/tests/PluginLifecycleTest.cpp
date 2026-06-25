/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "PluginLifecycleTest.h"

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/Midi/Midi.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <plugin/HydrogenPlugin.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtCore/QThread>

using namespace H2Core;

namespace {
// Allocate L/R + nBuses stereo buffers of nFrames each, zeroed.
struct PluginBuffers {
	std::vector<float> masterL, masterR;
	std::vector<std::vector<float>> busL, busR;
	std::vector<float*> busLPtr, busRPtr;

	PluginBuffers( unsigned nFrames, int nBuses )
		: masterL( nFrames, 0.0f ), masterR( nFrames, 0.0f ) {
		for ( int ii = 0; ii < nBuses; ++ii ) {
			busL.emplace_back( nFrames, 0.0f );
			busR.emplace_back( nFrames, 0.0f );
		}
		for ( int ii = 0; ii < nBuses; ++ii ) {
			busLPtr.push_back( busL[ ii ].data() );
			busRPtr.push_back( busR[ ii ].data() );
		}
	}
};

bool allFinite( const std::vector<float>& buf, unsigned n ) {
	for ( unsigned i = 0; i < n; ++i ) {
		if ( ! std::isfinite( buf[i] ) ) {
			return false;
		}
	}
	return true;
}

// A headless engine standing in for the editor's read mirror. Caller owns it.
Hydrogen* makeMirrorEngine() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( false );
	auto* pHydrogen = new Hydrogen( pPref, -1 );
	pHydrogen->setGUIState( Hydrogen::GUIState::headless );
	return pHydrogen;
}

// Pump the (main-thread) editor channel via the event loop and yield to the
// plugin's bridge thread, until @a cond holds or @a nTimeoutMs elapses.
bool pumpUntil( std::function<bool()> cond, int nTimeoutMs = 4000 ) {
	QElapsedTimer timer;
	timer.start();
	while ( ! cond() && timer.elapsed() < nTimeoutMs ) {
		QCoreApplication::processEvents( QEventLoop::AllEvents, 10 );
		QThread::msleep( 5 );
	}
	return cond();
}
} // namespace

// A Preferences configured for a headless, host-less secondary instance: fake
// audio, no MIDI, no OSC. create_instance() returns a freshly-owned object
// (ADR 0015), so each instance gets its own.
static std::shared_ptr<Preferences> makePluginPreferences() {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Fake;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->m_nBufferSize = 1024;
	pPref->setOscServerEnabled( false );
	return pPref;
}

void PluginLifecycleTest::testRepeatedLifecycle() {
	___INFOLOG( "" );

	// A plugin host instantiates and destroys engine instances repeatedly within
	// one process; doing so must leave no residual global state (ADR 0015). The
	// harness's own Hydrogen is alive throughout, so the alive-object count after
	// each destroyed instance must return to this baseline.
	const int nBaseline = Base::getAliveObjectCount();
	const int nIterations = 5;

	for ( int ii = 0; ii < nIterations; ++ii ) {
		// Each instance owns its own OSC server / NSM client (per-instance, not a
		// singleton — ADR 0015); with OSC disabled they never touch the network.
		auto* pHydrogen = new Hydrogen( makePluginPreferences(), -1 );

		CPPUNIT_ASSERT( pHydrogen->getAudioEngine() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getPreferences() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getEventQueue() != nullptr );
		CPPUNIT_ASSERT( pHydrogen->getSong() != nullptr );

		// Use it.
		const float fBpm = 123.0f + static_cast<float>( ii );
		pHydrogen->getSong()->setBpm( fBpm );
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
			fBpm, pHydrogen->getSong()->getBpm(), 0.001 );

		delete pHydrogen;

		// No residual global state: every object the instance constructed has
		// been destructed, so we are back to the baseline.
		CPPUNIT_ASSERT_EQUAL( nBaseline, Base::getAliveObjectCount() );
	}

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testPluginProcess() {
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	const int nBuses = 4;

	HydrogenPlugin plugin( nSampleRate, nBlock, nBuses );
	plugin.activate( nSampleRate, nBlock );
	CPPUNIT_ASSERT_EQUAL( nBuses, plugin.getBusCount() );

	PluginBuffers buf( nBlock, nBuses );

	// Idle (not rolling): output stays finite and silent.
	for ( int b = 0; b < 4; ++b ) {
		plugin.process( nBlock, buf.masterL.data(), buf.masterR.data(),
						buf.busLPtr, buf.busRPtr,
						/*bRolling=*/false, 120.0, 0 );
		CPPUNIT_ASSERT( allFinite( buf.masterL, nBlock ) );
		CPPUNIT_ASSERT( allFinite( buf.masterR, nBlock ) );
	}

	// Rolling + a note: still finite, no NaN/denormals leaking to the host.
	plugin.noteOn( 36, 100, static_cast<int>( Midi::ChannelDefault ) );
	long long nFrame = 0;
	for ( int b = 0; b < 8; ++b ) {
		plugin.process( nBlock, buf.masterL.data(), buf.masterR.data(),
						buf.busLPtr, buf.busRPtr,
						/*bRolling=*/true, 130.0, nFrame );
		CPPUNIT_ASSERT( allFinite( buf.masterL, nBlock ) );
		CPPUNIT_ASSERT( allFinite( buf.masterR, nBlock ) );
		for ( int bus = 0; bus < nBuses; ++bus ) {
			CPPUNIT_ASSERT( allFinite( buf.busL[ bus ], nBlock ) );
			CPPUNIT_ASSERT( allFinite( buf.busR[ bus ], nBlock ) );
		}
		nFrame += nBlock;
	}

	plugin.deactivate();

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testPluginStateRoundTrip() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 512, 4 );

	// Embedded state is a portable .h2project bundle.
	const auto state = plugin.saveState( /*bEmbedSamples=*/true );
	CPPUNIT_ASSERT( ! state.empty() );
	CPPUNIT_ASSERT( H2Project::looksLikeArchive( state ) );

	// A fresh plugin instance restores it.
	HydrogenPlugin other( 48000, 256, 2 );
	CPPUNIT_ASSERT( other.loadState( state ) );
	CPPUNIT_ASSERT( other.getHydrogen()->getSong() != nullptr );

	// Song-only state round-trips too.
	const auto songOnly = plugin.saveState( /*bEmbedSamples=*/false );
	CPPUNIT_ASSERT( ! songOnly.empty() );
	CPPUNIT_ASSERT( ! H2Project::looksLikeArchive( songOnly ) );
	CPPUNIT_ASSERT( other.loadState( songOnly ) );

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testPluginRepeatedLifecycle() {
	___INFOLOG( "" );

	// A host instantiates and destroys plugin instances repeatedly within one
	// process; doing so must leave no residual global state (ADR 0015).
	const int nBaseline = Base::getAliveObjectCount();

	for ( int ii = 0; ii < 4; ++ii ) {
		auto* pPlugin = new HydrogenPlugin( 44100, 256, 2 );
		PluginBuffers buf( 256, 2 );
		pPlugin->process( 256, buf.masterL.data(), buf.masterR.data(),
						  buf.busLPtr, buf.busRPtr, false, 120.0, 0 );
		delete pPlugin;
		CPPUNIT_ASSERT_EQUAL( nBaseline, Base::getAliveObjectCount() );
	}

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testEditorOpenServesEngine() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 512, 0 );
	CPPUNIT_ASSERT( plugin.getHydrogen() != nullptr );
	CPPUNIT_ASSERT( plugin.getHydrogen()->getSong() != nullptr );
	plugin.getHydrogen()->getSong()->setName( "PLUGINSONG" );

	// Open the editor without spawning the GUI process — this test acts as the
	// editor itself by attaching an EditorSession to the served endpoint.
	CPPUNIT_ASSERT( plugin.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( plugin.isEditorOpen() );
	CPPUNIT_ASSERT( ! plugin.getEditorEndpoint().isEmpty() );
	const QString sEndpoint = plugin.getEditorEndpoint();

	auto* pMirror = makeMirrorEngine();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	// The plugin's engine sends its song as the initial snapshot.
	CPPUNIT_ASSERT( pumpUntil( [&]() {
		return pMirror->getSong() != nullptr &&
			pMirror->getSong()->getName() == QString( "PLUGINSONG" );
	} ) );

	// Closing tears down the serve loop: the endpoint is no longer served.
	pEditor.reset();
	plugin.closeEditor();
	CPPUNIT_ASSERT( ! plugin.isEditorOpen() );
	CPPUNIT_ASSERT( plugin.getEditorEndpoint().isEmpty() );

	auto* pMirror2 = makeMirrorEngine();
	auto pLate = EditorSession::connect( sEndpoint, pMirror2, 300 );
	CPPUNIT_ASSERT( pLate == nullptr ); // nothing listening anymore

	delete pMirror2;
	delete pMirror;

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testEditorCommandReachesEngine() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 512, 0 );
	CPPUNIT_ASSERT( plugin.openEditor( /*bLaunchProcess=*/false ) );

	auto* pMirror = makeMirrorEngine();
	auto pEditor = EditorSession::connect( plugin.getEditorEndpoint(), pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	auto pAccess = pEditor->createEngineAccess();
	CPPUNIT_ASSERT( pAccess != nullptr );
	CPPUNIT_ASSERT( pAccess->getCoreActionController() != nullptr );

	// A command on the editor flows over IPC to the plugin's engine. (Use master
	// volume, not BPM: under a plugin host the engine cedes tempo to the host, so
	// setBpm is intentionally a no-op there.)
	pAccess->getCoreActionController()->setMasterVolume( 0.25f );
	CPPUNIT_ASSERT( pumpUntil( [&]() {
		return std::abs(
			plugin.getHydrogen()->getSong()->getVolume() - 0.25f ) < 0.001f;
	} ) );

	pEditor.reset();
	plugin.closeEditor();
	delete pMirror;

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testEditorReopen() {
	___INFOLOG( "" );

	HydrogenPlugin plugin( 44100, 512, 0 );

	CPPUNIT_ASSERT( plugin.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( plugin.isEditorOpen() );
	// Idempotent: a second open while already open is a no-op success.
	CPPUNIT_ASSERT( plugin.openEditor( /*bLaunchProcess=*/false ) );

	plugin.closeEditor();
	CPPUNIT_ASSERT( ! plugin.isEditorOpen() );
	// Double close is harmless.
	plugin.closeEditor();

	// Re-openable after close (a fresh endpoint + serve loop).
	CPPUNIT_ASSERT( plugin.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( plugin.isEditorOpen() );

	auto* pMirror = makeMirrorEngine();
	auto pEditor = EditorSession::connect( plugin.getEditorEndpoint(), pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	pEditor.reset();
	plugin.closeEditor();
	delete pMirror;

	___INFOLOG( "passed" );
}

void PluginLifecycleTest::testEditorBinaryDiscovery() {
	___INFOLOG( "" );

	qunsetenv( "HYDROGEN_EDITOR_PATH" );

	// Explicit override always wins.
	CPPUNIT_ASSERT_EQUAL(
		std::string( "/opt/h/hydrogen" ),
		HydrogenPlugin::resolveEditorBinary( "/opt/h/hydrogen", "" )
			.toStdString() );

	// No override and nothing bundled → PATH fallback (a bare name).
	const QString sFallback =
		HydrogenPlugin::resolveEditorBinary( "", "/no/such/plugin/dir" );
	CPPUNIT_ASSERT( sFallback == QStringLiteral( "hydrogen" ) ||
					sFallback == QStringLiteral( "hydrogen.exe" ) );

	// An executable bundled next to the plugin is discovered.
	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
#if defined( _WIN32 )
	const QString sName = QStringLiteral( "hydrogen.exe" );
#else
	const QString sName = QStringLiteral( "hydrogen" );
#endif
	const QString sExe = QDir( tmp.path() ).filePath( sName );
	{
		QFile f( sExe );
		CPPUNIT_ASSERT( f.open( QIODevice::WriteOnly ) );
		f.write( "#!/bin/sh\n" );
	}
	QFile::setPermissions( sExe, QFile::permissions( sExe ) |
								  QFileDevice::ExeOwner | QFileDevice::ExeUser );
	CPPUNIT_ASSERT_EQUAL(
		QFileInfo( sExe ).absoluteFilePath().toStdString(),
		HydrogenPlugin::resolveEditorBinary( "", tmp.path() ).toStdString() );

	___INFOLOG( "passed" );
}
