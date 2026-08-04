/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#include "IpcRoundTripTest.h"

#include "assertions/RoundTripAssertions.h"
#include "TestHelper.h"

#include <core/Basics/Adsr.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcCoreActionController.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/Timeline.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>

#include <functional>
#include <memory>

using namespace H2Core;

// ── Test infrastructure ─────────────────────────────────────────────

namespace {

License makeLicense( License::LicenseType type ) {
	License lic( "", "" );
	lic.setType( type );
	return lic;
}

} // namespace

// ── Non-trivial factories ───────────────────────────────────────────

namespace {

std::shared_ptr<InstrumentLayer> makeLayer( Hydrogen* pHydrogen ) {
	auto pSample =
		std::make_shared<Sample>( H2TEST_FILE( "/drumkits/baseKit/snare.wav" ),
								  makeLicense( License::CC_BY_NC )
		);

	std::vector<EnvelopePoint> panEnvelope;
	panEnvelope.push_back( EnvelopePoint( 123, 653 ) );
	panEnvelope.push_back( EnvelopePoint( 124, 652 ) );
	pSample->setPanEnvelope( panEnvelope );

	std::vector<EnvelopePoint> velocityEnvelope;
	velocityEnvelope.push_back( EnvelopePoint( 13, 53 ) );
	velocityEnvelope.push_back( EnvelopePoint( 14, 52 ) );
	pSample->setVelocityEnvelope( velocityEnvelope );

	Sample::Loops loops;
	loops.nStartFrame = 100;
	loops.nLoopFrame = 150;
	loops.nEndFrame = 200;
	loops.nCount = 2;
	loops.mode = Sample::Loops::Mode::PingPong;
	pSample->setLoops( loops );

	Sample::Rubberband rubberband;
#ifdef H2CORE_HAVE_RUBBERBAND
	rubberband.bUse = true;
#else
	rubberband.bUse = false;
#endif
	rubberband.fLengthInBeats = 10.5;
	rubberband.fSemitonesToShift = 0.45;
	rubberband.nCrispness = 2;
	pSample->setRubberband( rubberband );
	pSample->setIsModified( true );

	auto pLayer = std::make_shared<InstrumentLayer>( pSample );
	pLayer->setStartVelocity( 0.1f );
	pLayer->setEndVelocity( 0.9f );
	pLayer->setPitchOffset( 5.0f );
	pLayer->setGain( 0.8f );
	pLayer->setIsMuted( false );
	pLayer->setIsSoloed( true );

	// Fallback paths are handled differently depending of whether the sample is
	// located within the drumkit the instrument is associated with or not. This
	// is determined during XML serialization. As this variable is only used for
	// sample retrieval on file load operations, it is save to overwrite it in
	// here as we only care about IPC and in-process serialization of loaded
	// data.
	pLayer->setFallbackSampleFileName( pSample->getFilePath() );

	return pLayer;
}

std::shared_ptr<InstrumentComponent> makeComponent() {
	auto pComp = std::make_shared<InstrumentComponent>( "TestComp", 0.7f );
	pComp->setIsMuted( true );
	pComp->setIsSoloed( false );
	pComp->setSelection( InstrumentComponent::Selection::RoundRobin );

	// Layers are added via Instrument::addLayer (friend).
	return pComp;
}

std::shared_ptr<Instrument> makeInstrument( Hydrogen* pHydrogen ) {
	auto pAdsr = std::make_shared<ADSR>( 100, 200, 0.5f, 300 );
	auto pInstr = std::make_shared<Instrument>(
		Instrument::Id( 42 ), "TestInstr", pAdsr
	);
	pInstr->setType( "Kick" );
	pInstr->setVolume( 0.6f );
	pInstr->setMuted( false );
	pInstr->setSoloed( true );
	pInstr->setPan( 0.3f );
	pInstr->setPitchOffset( -2.0f );
	pInstr->setRandomPitchFactor( 0.4f );
	pInstr->setGain( 1.2f );
	pInstr->setApplyVelocity( true );
	pInstr->setFilterActive( true );
	pInstr->setFilterCutoff( 0.5f );
	pInstr->setFilterResonance( 0.3f );
	pInstr->setMuteGroup( 3 );
	pInstr->setMidiOutChannel( Midi::channelFromInt( 5 ) );
	pInstr->setMidiOutNote( Midi::Note( 60 ) );
	pInstr->setStopNotes( true );
	pInstr->setHihatGrp( 1 );
	pInstr->setLowerCc( Midi::Parameter( 10 ) );
	pInstr->setHigherCc( Midi::Parameter( 20 ) );
	pInstr->setIsPreviewInstrument( true );
	pInstr->setDrumkitPath( "/test/path" );
	pInstr->setDrumkitName( "TestKit" );
	auto pComponent = makeComponent();
	pInstr->addComponent( pComponent );

	auto pLayer = makeLayer( pHydrogen );
	pInstr->addLayer(
		pComponent, pLayer, 0, Event::Trigger::Default, pHydrogen
	);
	pInstr->loadSamples( 157.3, pHydrogen->getPreferences().get() );
	return pInstr;
}

std::shared_ptr<Drumkit> makeDrumkit( Hydrogen* pHydrogen ) {
	auto pKit = std::make_shared<Drumkit>();
	pKit->setContext( Filesystem::Context::Song );
	pKit->setPath( "/path/to/RoundTripKit" );
	pKit->setName( "RoundTripKit" );
	pKit->setVersion( 2 );
	pKit->setAuthor( "Test Author" );
	pKit->setInfo( "Non-trivial drumkit for round-trip test" );
	auto license = makeLicense( License::CC_0 );
	license.setCopyrightHolder( pKit->getAuthor() );
	pKit->setLicense( license );
	pKit->setTags( QStringList() << "tag1" << "tag2" );
	pKit->setImage( "kit.png" );
	auto imageLicense = makeLicense( License::Other );
	imageLicense.setCopyrightHolder( pKit->getAuthor() );
	pKit->setImageLicense( imageLicense );
	pKit->setIsModified( true );

	auto pInstrs = std::make_shared<InstrumentList>();
	pInstrs->add( makeInstrument( pHydrogen ) );
	pKit->setInstruments( pInstrs );
	return pKit;
}

std::shared_ptr<Pattern> makePattern( std::shared_ptr<Drumkit> pKit )
{
	auto pPattern = std::make_shared<Pattern>();
	pPattern->setVersion( 1 );
	pPattern->setName( "RoundTripPattern" );
	pPattern->setPath( "/path/to/Pattern" );
	pPattern->setDrumkitName( pKit->getExportName() );
	pPattern->setAuthor( "Pattern Author" );
	pPattern->setInfo( "Non-trivial pattern" );
	pPattern->setLicense( makeLicense( License::CC_0 ) );
	pPattern->setLength( 192 );
	pPattern->setDenominator( 4 );
	pPattern->setIsModified( true );
	pPattern->setTags( QStringList() << "ptag1" << "ptag2" );

	// Add a note with non-default properties
	if ( pKit != nullptr && pKit->getInstruments()->size() > 0 ) {
		auto pInstr = pKit->getInstruments()->get( 0 );
		auto pNote = std::make_shared<Note>( pInstr, 16, 0.8f, 0.2f, 48 );

		pNote->setNoteOff( true );
		pNote->setProbability( 0.9f );
		pNote->setKey( Note::Key::C );
		pNote->setOctave( Note::Octave::P8C );
		pNote->setType( "Kick" );
		pNote->setInstrumentId( Instrument::Id( 8 ) );
		pNote->setHumanizeDelay( 14 );
		pNote->setMidiNoteOffOffsetFrame( 56 );
		pNote->setMidiNoteOnSentFrame( 54 );
		pNote->setMidiNoteOffTimePoint( TimePoint() );
		pPattern->insertNote( pNote );
	}

	pPattern->mapToDrumkit( pKit );

	return pPattern;
}

std::shared_ptr<Song> makeSong( Hydrogen* pHydrogen ) {
	auto pSong = Song::getEmptySong( pHydrogen );
	pSong->setPath( "/path/to/RoundTripSong" );
	pSong->setName( "RoundTripSong" );
	pSong->setAuthor( "Song Author" );
	pSong->setNotes( "Non-trivial song for round-trip test" );
	auto license = makeLicense( License::CC_0 );
	license.setCopyrightHolder( pSong->getAuthor() );
	pSong->setLicense( license );
	pSong->setBpm( 140.0f );
	pSong->setVolume( 0.7f );
	pSong->setIsMuted( true );
	pSong->setVersion( 3 );
	pSong->setTags( QStringList() << "stag1" << "stag2" );
	pSong->setLoopMode( Song::LoopMode::Enabled );
	pSong->setPatternMode( Song::PatternMode::Selected );
	pSong->setMode( Song::Mode::Song );
	pSong->setActionMode( Song::ActionMode::drawMode );
	pSong->setIsPatternEditorLocked( true );
	pSong->setIsTimelineActivated( true );
	pSong->setIsModified( true );
	pSong->setPanLawType( 1 );
	pSong->setPanLawKNorm( 3.5f );
	pSong->setHumanizeTimeValue( 0.3f );
	pSong->setHumanizeVelocityValue( 0.4f );
	pSong->setSwingFactor( 0.2f );
	pSong->setLastLoadedDrumkitPath( "/test/drumkit/path" );
	pSong->setWasAskedAboutMissingSamples( true );

	// Replace drumkit with our non-trivial one
	auto pKit = makeDrumkit( pHydrogen );
	pSong->setDrumkit( pKit );

	// Add a non-trivial pattern
	auto pPattern = makePattern( pKit );
	pSong->getPatternList()->add( pPattern );
	pSong->getPatternList()->mapToDrumkit( pKit );

	auto pPatternGroupVector =
		std::make_shared<std::vector<std::shared_ptr<PatternList>>>();
	pPatternGroupVector->push_back( pSong->getPatternList() );
	pPatternGroupVector->push_back( pSong->getPatternList() );
	pSong->setPatternGroupVector( pPatternGroupVector );

	// Add a tempo marker and tag to the timeline
	pSong->getTimeline()->addTempoMarker( 2, 130.0f );
	pSong->getTimeline()->addTag( 4, "TestTag" );

	// Add an automation path point
	pSong->getAutomationPath()->addPoint( 1.0f, 0.5f );

	auto pPlaybackInstrument = makeInstrument( pHydrogen );
	pPlaybackInstrument->setId( Instrument::PlaybackTrackId );
	pPlaybackInstrument->setName( "PlaybackTrack" );
	pPlaybackInstrument->loadSamples( 120, pHydrogen->getPreferences().get() );
	pSong->setPlaybackTrackInstrument( pPlaybackInstrument );

	return pSong;
}

std::shared_ptr<Playlist> makePlaylist() {
	auto pPlaylist = std::make_shared<Playlist>();
	auto pEntry1 = std::make_shared<PlaylistEntry>( "/song1.h2song",
													 "/script1.sh", true );
	auto pEntry2 = std::make_shared<PlaylistEntry>( "/song2.h2song",
													 "", false );
	pPlaylist->setPath( "/path/to/playlist" );
	pPlaylist->add( pEntry1 );
	pPlaylist->add( pEntry2 );
	pPlaylist->setActiveSongNumber( 1 );
	pPlaylist->setIsModified( true );
	return pPlaylist;
}

std::shared_ptr<PlaylistEntry> makePlaylistEntry() {
	return std::make_shared<PlaylistEntry>( "/roundtrip/song.h2song",
											"/roundtrip/script.sh", true );
}

} // namespace

// ── Test methods ────────────────────────────────────────────────────

void IpcRoundTripTest::testSongRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();
	
	// ── Serialization level: toXmlBuffer → fromXmlBuffer ──
	auto pSongA = makeSong( pEngine );
	const auto xml = pSongA->toXmlBuffer( true, false );
	auto pSongB = Song::fromXmlBuffer( xml, false, pEngine );
	CPPUNIT_ASSERT( pSongB != nullptr );
	RoundTripAssertions::assertSongEqual( *pSongA, *pSongB );

	// ── IPC level: IpcCoreActionController → engine ──
	auto pMirror = TestHelper::makeMirror();
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	// Set a song with a distinctive name so we can detect arrival
	auto pSong = makeSong( pMirror );
	const QString sNewSongName( "IPC_SONG_TEST" );
	pSong->setName( sNewSongName );
	CPPUNIT_ASSERT( pEngine->getSong()->getName() != sNewSongName );
	pController->setSong( pSong );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong() != nullptr &&
			   pEngine->getSong()->getName() == sNewSongName;
	} ) );

	RoundTripAssertions::assertSongEqual( *pSong, *pEngine->getSong() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testDrumkitRoundTrip()
{
	___INFOLOG( "" );
	
	auto pEngine = TestHelper::makeEngine();

	// ── Serialization level ──
	auto pKitA = makeDrumkit( pEngine );
	const auto xml = pKitA->toXmlBuffer( true /* bSongKit */ );
	auto pKitB = Drumkit::fromXmlBuffer( xml, "", true, false, pEngine );
	CPPUNIT_ASSERT( pKitB != nullptr );
	RoundTripAssertions::assertDrumkitEqual( *pKitA, *pKitB );

	// ── IPC level ──
	auto pMirror = TestHelper::makeMirror();
	// Engine needs a song to hold the drumkit
	pEngine->getCoreActionController()->setSong(
		Song::getEmptySong( pEngine ) );
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	// Mirror also needs a song
	pMirror->getCoreActionController()->setSong(
		Song::getEmptySong( pMirror ) );

	auto pKit = makeDrumkit( pMirror );
	pKit->setName( "IPC_KIT_TEST" );
	pController->setDrumkit( pKit );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong() != nullptr &&
			   pEngine->getSong()->getDrumkit() != nullptr &&
			   pEngine->getSong()->getDrumkit()->getName() ==
				   QString( "IPC_KIT_TEST" );
	} ) );
	pKit->loadSamples(
		pEngine->getSong()
			->getDrumkit()
			->getInstruments()
			->get( 0 )
			->getLastSampleLoadBpm(),
		pEngine->getPreferences().get()
	);

	RoundTripAssertions::assertDrumkitEqual(
		*pKit, *pEngine->getSong()->getDrumkit() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testInstrumentRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();

	// ── Serialization level ──
	auto pInstrA = makeInstrument( pEngine );
	const auto xml = pInstrA->toXmlBuffer( true /* bSongKit */, true, false );
	auto pInstrB =
		Instrument::fromXmlBuffer( xml, true /* bSongKit */, false, pEngine );
	CPPUNIT_ASSERT( pInstrB != nullptr );
	RoundTripAssertions::assertInstrumentEqual( *pInstrA, *pInstrB );

	// ── IPC level: replaceInstrument ──
	auto pMirror = TestHelper::makeMirror();

	// Both need a song with a drumkit containing an instrument to replace
	auto pEngineSong = Song::getEmptySong( pEngine );
	pEngine->getCoreActionController()->setSong( pEngineSong );
	auto pMirrorSong = Song::getEmptySong( pMirror );
	pMirror->getCoreActionController()->setSong( pMirrorSong );
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	// Get the old instrument from the mirror's drumkit
	auto pOldInstr = pMirrorSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pOldInstr != nullptr );

	auto pNewInstr = makeInstrument( pMirror );
	pNewInstr->setName( "IPC_INSTR_TEST" );
	pController->replaceInstrument( pNewInstr, pOldInstr );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		auto pInstrs = pEngine->getSong()->getDrumkit()->getInstruments();
		return pInstrs->size() > 0 &&
			   pInstrs->get( 0 )->getName() == QString( "IPC_INSTR_TEST" );
	} ) );

	pNewInstr->loadSamples(
		pEngine->getSong()
			->getDrumkit()
			->getInstruments()
			->get( 0 )
			->getLastSampleLoadBpm(),
		pEngine->getPreferences().get()
	);

	RoundTripAssertions::assertInstrumentEqual(
		*pNewInstr, *pEngine->getSong()->getDrumkit()->getInstruments()->get( 0 ) );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testPatternRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();

	auto pEngineSong = Song::getEmptySong( pEngine );
	pEngine->getCoreActionController()->setSong( pEngineSong );

	// ── Serialization level ──
	auto pKit = makeDrumkit( pEngine );
	auto pPatternA = makePattern( pKit );
	const auto xml = pPatternA->toXmlBuffer( pKit );
	auto pPatternB = Pattern::fromXmlBuffer(
		xml, pKit, false, pEngine->getSoundLibraryDatabase()
	);
	CPPUNIT_ASSERT( pPatternB != nullptr );
	RoundTripAssertions::assertPatternEqual( *pPatternA, *pPatternB );

	// ── IPC level ──
	auto pMirror = TestHelper::makeMirror();
	auto pMirrorSong = Song::getEmptySong( pMirror );
	pMirror->getCoreActionController()->setSong( pMirrorSong );
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	// Ensure the pattern is registered into the right song.
	pController->setSong( pMirrorSong );
	auto pPattern = makePattern( pMirrorSong->getDrumkit() );
	pPattern->setName( "IPC_PATTERN_TEST" );
	pPattern->mapToDrumkit( pMirrorSong->getDrumkit() );
	pController->setPattern( pPattern, 0, true );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		auto pList = pEngine->getSong()->getPatternList();
		return pList->size() > 0 &&
			   pList->get( 0 )->getName() == QString( "IPC_PATTERN_TEST" );
	} ) );

	RoundTripAssertions::assertPatternEqual(
		*pPattern, *pEngine->getSong()->getPatternList()->get( 0 ) );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testPlaylistRoundTrip()
{
	___INFOLOG( "" );

	// ── Serialization level ──
	auto pPlaylistA = makePlaylist();
	const auto xml = pPlaylistA->toXmlBuffer();
	auto pPlaylistB = Playlist::fromXmlBuffer( xml, "" );
	CPPUNIT_ASSERT( pPlaylistB != nullptr );
	RoundTripAssertions::assertPlaylistEqual( *pPlaylistA, *pPlaylistB );

	// ── IPC level ──
	auto pEngine = TestHelper::makeEngine();
	auto pMirror = TestHelper::makeMirror();
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	auto pPlaylist = makePlaylist();
	pController->setPlaylist( pPlaylist );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPlaylist() != nullptr &&
			   pEngine->getPlaylist()->size() == 2;
	} ) );

	RoundTripAssertions::assertPlaylistEqual(
		*pPlaylist, *pEngine->getPlaylist() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testPlaylistEntryRoundTrip()
{
	___INFOLOG( "" );

	// ── Serialization level: toMimeText → fromMimeText ──
	auto pEntryA = makePlaylistEntry();
	const auto mime = pEntryA->toMimeText();
	auto pEntryB = PlaylistEntry::fromMimeText( mime );
	CPPUNIT_ASSERT( pEntryB != nullptr );
	RoundTripAssertions::assertPlaylistEntryEqual( *pEntryA, *pEntryB );

	// ── IPC level: addToPlaylist ──
	auto pEngine = TestHelper::makeEngine();
	auto pMirror = TestHelper::makeMirror();
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	// Both need an empty playlist
	pEngine->setPlaylist( std::make_shared<Playlist>() );
	pMirror->setPlaylist( std::make_shared<Playlist>() );

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	auto pEntry = makePlaylistEntry();
	pController->addToPlaylist( pEntry, 0 );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPlaylist() != nullptr &&
			   pEngine->getPlaylist()->size() == 1;
	} ) );

	RoundTripAssertions::assertPlaylistEntryEqual(
		*pEntry, *pEngine->getPlaylist()->get( 0 ) );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testPreferencesRoundTrip()
{
	___INFOLOG( "" );

	// ── Serialization level: corePropsToXml → applyCorePropsFromXml ──
	auto pPrefA = Preferences::create_instance();
	pPrefA->setMaxBars( 500 );
	pPrefA->setHearNewNotes( false );
	pPrefA->setQuantizeEvents( true );
	pPrefA->m_fMetronomeVolume = 0.65f;
	pPrefA->m_nMaxNotes = 32;
	pPrefA->m_nBufferSize = 256;
	pPrefA->m_nSampleRate = 48000;
	pPrefA->setCountIn( true );
	pPrefA->m_bUseMetronome = true;

	const auto xml = pPrefA->corePropsToXml();

	auto pPrefB = Preferences::create_instance();
	pPrefB->applyCorePropsFromXml( xml );

	RoundTripAssertions::assertCorePreferencesEqual( *pPrefA, *pPrefB );

	// ── IPC level: setPreferences via IpcCoreActionController ──
	auto pEngine = TestHelper::makeEngine();
	auto pMirror = TestHelper::makeMirror();
	const QString sEndpoint = TestHelper::uniqueEndpoint();

	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );
	auto pAccess = pEditorSession->createEngineAccess();
	auto pController = std::dynamic_pointer_cast<IpcCoreActionController>(
		pAccess->getCoreActionController() );
	CPPUNIT_ASSERT( pController != nullptr );

	// Set non-default core preferences on the mirror
	auto pMirrorPref = pMirror->getPreferences();
	pMirrorPref->setMaxBars( 777 );
	pMirrorPref->setHearNewNotes( false );
	pMirrorPref->setQuantizeEvents( true );
	pMirrorPref->m_fMetronomeVolume = 0.42f;
	pMirrorPref->m_nMaxNotes = 16;
	pMirrorPref->m_nBufferSize = 512;
	pMirrorPref->m_nSampleRate = 96000;
	pMirrorPref->setCountIn( true );
	pMirrorPref->m_bUseMetronome = true;

	pController->setPreferences( pMirrorPref );

	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		auto pEnginePref = pEngine->getPreferences();
		return pEnginePref->getMaxBars() == 777;
	} ) );

	RoundTripAssertions::assertCorePreferencesEqual(
		*pMirrorPref, *pEngine->getPreferences() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}
