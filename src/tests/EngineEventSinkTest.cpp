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

#include "EngineEventSinkTest.h"

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EngineEventSink.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/EngineTelemetry.h>
#include <core/IPC/EngineTelemetryShm.h>
#include <core/Object.h>

#include <plugin/HydrogenPlugin.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace H2Core;

namespace {

/** Thread-safe recording EngineEventSink: the session calls it on the
 * bridge thread, the test reads and drains it on its own. */
class RecordingSink : public EngineEventSink {
public:
	struct RecordedEvent {
		Event::Type type;
		int nValue;
		long nId;
	};

	void onEvent( Event::Type type, int nValue, long nId ) override {
		std::lock_guard<std::mutex> lock( m_mutex );
		m_events.push_back( RecordedEvent{ type, nValue, nId } );
	}

	void onMeterSnapshot( const EngineTelemetrySnapshot& snapshot ) override {
		std::lock_guard<std::mutex> lock( m_mutex );
		m_snapshots.push_back( snapshot );
	}

	void onInfoSnapshot( const EngineInfoSnapshot& snapshot ) override {
		std::lock_guard<std::mutex> lock( m_mutex );
		m_infoSnapshots.push_back( snapshot );
	}

	/** Whether an event of the given (type, value) was recorded
	 * (non-consuming). */
	bool sawEvent( Event::Type type, int nValue ) {
		std::lock_guard<std::mutex> lock( m_mutex );
		for ( const auto& event : m_events ) {
			if ( event.type == type && event.nValue == nValue ) {
				return true;
			}
		}
		return false;
	}

	/** Number of recorded events of @a type (non-consuming). */
	int countEvents( Event::Type type ) {
		std::lock_guard<std::mutex> lock( m_mutex );
		int nCount = 0;
		for ( const auto& event : m_events ) {
			if ( event.type == type ) {
				++nCount;
			}
		}
		return nCount;
	}

	/** Whether some recorded snapshot carried @a fPeak on instrument
	 * @a nInstrument's left meter. */
	bool sawPeakL( int nInstrument, float fPeak ) {
		std::lock_guard<std::mutex> lock( m_mutex );
		for ( const auto& snapshot : m_snapshots ) {
			if ( snapshot.peakL[ nInstrument ] == fPeak ) {
				return true;
			}
		}
		return false;
	}

	/** Number of recorded info snapshots (non-consuming). */
	int countInfoSnapshots() {
		std::lock_guard<std::mutex> lock( m_mutex );
		return m_infoSnapshots.size();
	}

	/** Most recent recorded info snapshot (default-constructed when
	 * none arrived yet). */
	EngineInfoSnapshot lastInfoSnapshot() {
		std::lock_guard<std::mutex> lock( m_mutex );
		if ( m_infoSnapshots.empty() ) {
			return EngineInfoSnapshot();
		}
		return m_infoSnapshots.back();
	}

	/** Swap all recorded events out (test thread). */
	std::vector<RecordedEvent> drainEvents() {
		std::lock_guard<std::mutex> lock( m_mutex );
		auto events = std::move( m_events );
		m_events.clear();
		return events;
	}

private:
	std::mutex m_mutex;
	std::vector<RecordedEvent> m_events;
	std::vector<EngineTelemetrySnapshot> m_snapshots;
	std::vector<EngineInfoSnapshot> m_infoSnapshots;
};

/** Sink that parks inside onMeterSnapshot until released: a slow
 * consumer, so an unregister racing an in-flight callback is
 * observable from the test thread. */
class BlockingSink : public EngineEventSink {
public:
	BlockingSink( std::atomic<int>& rInside, std::atomic<bool>& rRelease )
		: m_rInside( rInside ), m_rRelease( rRelease ) {}

	void onEvent( Event::Type, int, long ) override {}
	void onMeterSnapshot( const EngineTelemetrySnapshot& ) override {
		m_rInside.fetch_add( 1 );
		while ( ! m_rRelease.load() ) {
			std::this_thread::yield();
		}
	}
	void onInfoSnapshot( const EngineInfoSnapshot& ) override {}

private:
	std::atomic<int>& m_rInside;
	std::atomic<bool>& m_rRelease;
};

} // namespace

// TU3.1 (editor case): one drained event fans out to BOTH the attached
// editor and a registered in-process sink, and one telemetry build feeds
// BOTH the SHM block and the sink — a double consume (one pass per
// consumer) would starve one of the two observers.
void EngineEventSinkTest::testFanoutToEditorAndSink() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	RecordingSink sink;
	pServer->setEventSink( &sink );

	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );

	// Let the initial handshake/state settle first.
	TestHelper::pumpUntil( [&]() { return pMirror->getSong() != nullptr; } );

	// One engine event surfaces on the editor's mirror AND the sink.
	pEngine->getEventQueue()->pushEvent( Event::Type::Metronome, 7 );
	CPPUNIT_ASSERT(
		TestHelper::pumpUntilEvent( pMirror, Event::Type::Metronome, 7 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.sawEvent( Event::Type::Metronome, 7 ); } ) );

	// One telemetry build feeds both consumers: the sink's snapshot and the
	// SHM block a telemetry reader (like the editor's mirror) attaches to.
	// The bridge thread builds consume-style (read + reset, ADR 0027), so
	// seed instrument 0's left peak and keep re-seeding under the engine
	// lock while pumping — every ~50 ms build then carries 0.75 to both. A
	// build that consumed the peaks twice (once per consumer) would leave
	// one of the two seeing 0.0.
	auto pInstrument =
		pEngine->getSong()->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pAudioEngine = pEngine->getAudioEngine();
	const float fSeedPeak = 0.75f;

	// The session creates the segment on its bridge thread right after the
	// listen binds — which can race this attach, so retry until it lands.
	EngineTelemetryShm reader;
	bool bAttached = false;
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		if ( ! bAttached ) {
			bAttached = reader.attach(
				EngineTelemetryShm::keyForEndpoint( sEndpoint ) );
		}
		return bAttached;
	} ) );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		pAudioEngine->lock( RIGHT_HERE );
		pInstrument->setPeak_L( fSeedPeak );
		pAudioEngine->unlock();
		if ( ! sink.sawPeakL( 0, fSeedPeak ) ) {
			return false;
		}
		EngineTelemetrySnapshot shmSnapshot;
		return reader.load( shmSnapshot ) &&
			shmSnapshot.peakL[ 0 ] == fSeedPeak;
	} ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// TU3.1 (no-editor case) + TU3.5: with NO editor ever attached, a
// registered sink still receives the engine's events — the session drains
// the queue on its own. Errors are additionally retained for the ADR 0026
// replay, so a late editor still sees them; the sink fan-out must not
// consume the replay buffer.
void EngineEventSinkTest::testFanoutWithoutEditor() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	RecordingSink sink;
	pServer->setEventSink( &sink );

	// No editor is attached (nor will be, in this first phase).
	pEngine->getEventQueue()->pushEvent( Event::Type::Metronome, 7 );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.sawEvent( Event::Type::Metronome, 7 ); } ) );

	// Errors reach the sink live AND are retained for the replay.
	pEngine->getEventQueue()->pushEvent( Event::Type::Error, 42 );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.sawEvent( Event::Type::Error, 42 ); } ) );

	// A late editor still gets the retained error replayed (ADR 0026
	// point 9).
	auto* pMirror = TestHelper::makeMirror();
	auto pEditor = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditor != nullptr );
	CPPUNIT_ASSERT(
		TestHelper::pumpUntilEvent( pMirror, Event::Type::Error, 42 ) );

	pEditor.reset();
	pServer->stop();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

// TU3.2: an observing sink (one that does not consume the EventQueue
// itself — the bridge thread stays its sole drainer) sees a burst of
// events complete and in order while the session idles with no editor
// attached; unregistering stops delivery without loss or duplication.
void EngineEventSinkTest::testSinkQueueDrainedByIdle() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	RecordingSink sink;
	pServer->setEventSink( &sink );

	// A burst well below the EventQueue cap (nMaxEvents): the idle session
	// must drain it to the sink completely and in order.
	const int nBurst = 8;
	for ( int ii = 0; ii < nBurst; ++ii ) {
		pEngine->getEventQueue()->pushEvent( Event::Type::Metronome, ii );
	}
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.countEvents( Event::Type::Metronome ) >= nBurst; } ) );
	{
		const auto events = sink.drainEvents();
		std::vector<int> metronomeValues;
		for ( const auto& event : events ) {
			if ( event.type == Event::Type::Metronome ) {
				metronomeValues.push_back( event.nValue );
			}
		}
		CPPUNIT_ASSERT_EQUAL( static_cast<size_t>( nBurst ),
							  metronomeValues.size() );
		for ( int ii = 0; ii < nBurst; ++ii ) {
			CPPUNIT_ASSERT_EQUAL( ii, metronomeValues[ ii ] );
		}
	}

	// Unregistering stops delivery: a later event is not fanned out to
	// the unregistered sink. One poll cycle (50 ms) plus margin.
	pServer->setEventSink( nullptr );
	pEngine->getEventQueue()->pushEvent( Event::Type::Metronome, 99 );
	TestHelper::pumpUntil( []() { return false; }, 200 );
	CPPUNIT_ASSERT( sink.drainEvents().empty() );

	// The unregister-before-destroy contract: stop the session (joining
	// the bridge thread) before the sink goes out of scope.
	pServer->stop();
	delete pEngine;

	___INFOLOG( "passed" );
}

// TU3.5: a plugin that never opens its editor still runs its engine
// session for its whole lifetime — a sink registered on the plugin
// receives the engine's events with no editor ever existing (the
// basic-UI-only mode an embedding host stays in).
void EngineEventSinkTest::testPluginFanoutWithoutEditor() {
	___INFOLOG( "" );

	// The sink must outlive the plugin (whose session outlives this
	// test's body): declare it first so it destructs last.
	RecordingSink sink;
	HydrogenPlugin plugin( 44100, 512, 0 );
	CPPUNIT_ASSERT( plugin.getHydrogen() != nullptr );

	plugin.setEventSink( &sink );

	// No editor is ever opened.
	plugin.getHydrogen()->getEventQueue()->pushEvent(
		Event::Type::Metronome, 7 );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.sawEvent( Event::Type::Metronome, 7 ); } ) );

	___INFOLOG( "passed" );
}

// The publish cycle delivers an info snapshot to a registered sink
// (ADR 0035 UI-3): the same song-level data the landing info section
// shows — identity, mode, counts, playback position — built on the
// bridge thread, so the UI never reads the song itself.
void EngineEventSinkTest::testInfoSnapshotFanout() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	auto pSong = Song::load(
		H2TEST_FILE( "song/midiExport_timelineAndTimeSignatures.h2song" ),
		false, pEngine );
	CPPUNIT_ASSERT( pSong != nullptr );
	pSong->setMode( Song::Mode::Song );
	pEngine->setSong( pSong );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	RecordingSink sink;
	pServer->setEventSink( &sink );

	// No editor attached: the publish cycle alone feeds the sink.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sink.countInfoSnapshots() > 0; } ) );
	const auto info = sink.lastInfoSnapshot();

	// The snapshot mirrors the song's own lists (the timeline fixture:
	// ten patterns across six columns) and the idle transport.
	CPPUNIT_ASSERT( info.sSongName == pSong->getName() );
	if ( pSong->getDrumkit() != nullptr ) {
		CPPUNIT_ASSERT( info.sDrumkitName ==
						pSong->getDrumkit()->getName() );
	}
	else {
		CPPUNIT_ASSERT( info.sDrumkitName.isEmpty() );
	}
	CPPUNIT_ASSERT( info.mode == Song::Mode::Song );
	CPPUNIT_ASSERT_EQUAL( 10, info.nPatternCount );
	CPPUNIT_ASSERT_EQUAL( 6, info.nColumnCount );
	CPPUNIT_ASSERT( info.playingPatterns.empty() );
	pEngine->getAudioEngine()->lock( RIGHT_HERE );
	const int nPlayheadColumn =
		pEngine->getAudioEngine()->getPlayhead()->getColumn();
	pEngine->getAudioEngine()->unlock();
	CPPUNIT_ASSERT( info.nCurrentColumn == nPlayheadColumn );

	pServer->stop();
	delete pEngine;

	___INFOLOG( "passed" );
}

// A degraded cycle (the engine lock stays contended for the builder's
// small try-lock budget) skips info delivery entirely — nullopt, never
// a blank snapshot that would read as "no song". Uncontended, the
// build delivers the song's shape.
void EngineEventSinkTest::testBuildInfoSnapshotDegradedSkips() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );
	auto pAudioEngine = pEngine->getAudioEngine();
	CPPUNIT_ASSERT( pAudioEngine != nullptr );

	// Hold the engine lock on another thread, as the audio thread
	// would during a buffer render.
	std::atomic<bool> bLockHeld{ false };
	std::atomic<bool> bRelease{ false };
	std::thread lockHolder( [&]() {
		pAudioEngine->lock( RIGHT_HERE );
		bLockHeld.store( true );
		while ( ! bRelease.load() ) {
			std::this_thread::yield();
		}
		pAudioEngine->unlock();
	} );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return bLockHeld.load(); } ) );

	const auto degraded = EngineSession::buildInfoSnapshot( pEngine );
	CPPUNIT_ASSERT( ! degraded.has_value() );

	bRelease.store( true );
	lockHolder.join();

	// Uncontended: the build delivers the empty song's Pattern-mode
	// shape (the song-mode-only counts stay at their defaults).
	const auto snapshot = EngineSession::buildInfoSnapshot( pEngine );
	CPPUNIT_ASSERT( snapshot.has_value() );
	CPPUNIT_ASSERT( snapshot->mode == Song::Mode::Pattern );
	CPPUNIT_ASSERT( snapshot->sSongName ==
					pEngine->getSong()->getName() );
	CPPUNIT_ASSERT_EQUAL( 0, snapshot->nPatternCount );
	CPPUNIT_ASSERT_EQUAL( 0, snapshot->nColumnCount );
	CPPUNIT_ASSERT( snapshot->nCurrentColumn == -1 );
	CPPUNIT_ASSERT( snapshot->playingPatterns.empty() );
	const int nSelected = pEngine->getSelectedPatternNumber();
	CPPUNIT_ASSERT( snapshot->sPatternName ==
					pEngine->getSong()->getPatternList()
						->get( nSelected )->getName() );

	delete pEngine;

	___INFOLOG( "passed" );
}

// The unregister-before-destroy contract: setEventSink( nullptr )
// drains in-flight callbacks before returning — it must not return
// while the bridge thread is still inside a sink callback, or the
// caller could destroy the sink out from under the bridge.
void EngineEventSinkTest::testUnregisterQuiescesInFlight() {
	___INFOLOG( "" );

	auto* pEngine = TestHelper::makeEngine();
	pEngine->setSong( Song::getEmptySong( pEngine ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pServer = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pServer != nullptr );

	std::atomic<int> nInside{ 0 };
	std::atomic<bool> bRelease{ false };
	BlockingSink sink( nInside, bRelease );
	pServer->setEventSink( &sink );

	// Park the bridge thread inside the sink's meter callback.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return nInside.load() > 0; } ) );

	// Unregister concurrently: without the drain it returns immediately
	// — while the bridge is still inside the sink. The window spans
	// several publish cycles, and the sink stays parked for all of it,
	// so a correct drain deterministically overruns the window.
	std::atomic<bool> bReturned{ false };
	std::thread unregisterer( [&]() {
		pServer->setEventSink( nullptr );
		bReturned.store( true );
	} );
	CPPUNIT_ASSERT( ! TestHelper::pumpUntil(
		[&]() { return bReturned.load(); }, 300 ) );

	// Releasing the sink lets the callback finish, the drain observe
	// it, and the unregister return.
	bRelease.store( true );
	unregisterer.join();
	CPPUNIT_ASSERT( bReturned.load() );
	CPPUNIT_ASSERT( nInside.load() > 0 );

	pServer->stop();
	delete pEngine;

	___INFOLOG( "passed" );
}

// HydrogenPlugin::clearEventSink is conditional: it unregisters only
// the sink still registered — a displaced sink's teardown must not
// kill its successor.
void EngineEventSinkTest::testClearEventSinkKeepsSuccessor() {
	___INFOLOG( "" );

	// Sinks must outlive the plugin (whose session outlives this
	// test's body): declare them first so they destruct last.
	RecordingSink sinkA;
	RecordingSink sinkB;
	HydrogenPlugin plugin( 44100, 512, 0 );
	CPPUNIT_ASSERT( plugin.getHydrogen() != nullptr );

	plugin.setEventSink( &sinkA );
	plugin.setEventSink( &sinkB );
	// A's teardown clears only if A is still registered — it is not.
	plugin.clearEventSink( &sinkA );

	// B still receives the engine's events.
	plugin.getHydrogen()->getEventQueue()->pushEvent(
		Event::Type::Metronome, 7 );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return sinkB.sawEvent( Event::Type::Metronome, 7 ); } ) );
	CPPUNIT_ASSERT( ! sinkA.sawEvent( Event::Type::Metronome, 7 ) );

	// Clearing the registered pointer itself does unregister.
	plugin.clearEventSink( &sinkB );
	plugin.getHydrogen()->getEventQueue()->pushEvent(
		Event::Type::Metronome, 8 );
	TestHelper::pumpUntil( []() { return false; }, 200 );
	CPPUNIT_ASSERT( ! sinkB.sawEvent( Event::Type::Metronome, 8 ) );

	___INFOLOG( "passed" );
}
