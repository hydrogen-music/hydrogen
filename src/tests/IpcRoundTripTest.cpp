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
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IPC/EditorSession.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcCoreActionController.h>
#include <core/IPC/IpcEngineAccess.h>
#include <core/IPC/IpcEngineBridge.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>
#include <core/License.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Timeline.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
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

std::shared_ptr<MidiEventMap> makeMidiEventMap() {
	auto pMap = std::make_shared<MidiEventMap>();

	// Events across all families (note/cc/pc/mmc) with actions covering
	// every parameter slot of the legacy string format (pattern,
	// instrument, component+layer, factor, song, and none).
	pMap->registerEvent(
		MidiEvent::Type::Note, Midi::parameterFromIntClamp( 60 ),
		MidiAction::fromQStrings(
			MidiAction::Type::SelectNextPattern, "5", "", "" ),
		Event::Trigger::Suppress, nullptr );
	pMap->registerEvent(
		MidiEvent::Type::Note, Midi::parameterFromIntClamp( 22 ),
		MidiAction::fromQStrings(
			MidiAction::Type::PlaylistSong, "3", "", "" ),
		Event::Trigger::Suppress, nullptr );
	pMap->registerEvent(
		MidiEvent::Type::CC, Midi::parameterFromIntClamp( 7 ),
		MidiAction::fromQStrings(
			MidiAction::Type::StripVolumeAbsolute, "2", "", "" ),
		Event::Trigger::Suppress, nullptr );
	pMap->registerEvent(
		MidiEvent::Type::CC, Midi::parameterFromIntClamp( 74 ),
		MidiAction::fromQStrings(
			MidiAction::Type::BpmCcRelative, "0.5", "", "" ),
		Event::Trigger::Suppress, nullptr );
	pMap->registerEvent(
		MidiEvent::Type::PC, Midi::ParameterInvalid,
		MidiAction::fromQStrings(
			MidiAction::Type::GainLevelAbsolute, "1", "0", "2" ),
		Event::Trigger::Suppress, nullptr );
	pMap->registerEvent(
		MidiEvent::Type::MmcPlay, Midi::ParameterInvalid,
		MidiAction::fromQStrings( MidiAction::Type::Play, "", "", "" ),
		Event::Trigger::Suppress, nullptr );

	return pMap;
}

std::shared_ptr<MidiInstrumentMap> makeMidiInstrumentMap() {
	auto pMap = std::make_shared<MidiInstrumentMap>();

	pMap->setInput( MidiInstrumentMap::Input::Custom );
	pMap->setOutput( MidiInstrumentMap::Output::Constant );
	pMap->setUseGlobalInputChannel( true );
	pMap->setGlobalInputChannel( Midi::channelFromInt( 3 ) );
	pMap->setUseGlobalOutputChannel( true );
	pMap->setGlobalOutputChannel( Midi::channelFromInt( 9 ) );

	// A typed instrument lands in the type-based map, a typeless one in
	// the id-based one.
	auto pTypedInstrument = std::make_shared<Instrument>();
	pTypedInstrument->setType( "Kick" );
	pMap->insertCustomInputMapping(
		pTypedInstrument, Midi::noteFromInt( 60 ),
		Midi::channelFromInt( 5 ) );

	auto pTypelessInstrument =
		std::make_shared<Instrument>( Instrument::Id( 7 ) );
	pMap->insertCustomInputMapping(
		pTypelessInstrument, Midi::noteFromInt( 23 ),
		Midi::channelFromInt( 13 ) );

	return pMap;
}

} // namespace

// ── Test methods ────────────────────────────────────────────────────

void IpcRoundTripTest::testSongRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();
	
	// ── Serialization level: toXmlBuffer → fromXmlBuffer ──
	auto pSongA = makeSong( pEngine );
	const auto xml = pSongA->toXmlBuffer(
		Xml::Flag::KeepMissingSamples | Xml::Flag::Ipc, false );
	auto pSongB = Song::fromXmlBuffer( xml, Xml::Flag::Ipc, false, pEngine );
	CPPUNIT_ASSERT( pSongB != nullptr );
	RoundTripAssertions::assertSongEqual( pSongA, pSongB );

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

	RoundTripAssertions::assertSongEqual( pSong, pEngine->getSong() );

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
	const auto xml = pKitA->toXmlBuffer(
		Xml::Flag::SongKit | Xml::Flag::KeepMissingSamples );
	auto pKitB =
		Drumkit::fromXmlBuffer( xml, "", Xml::Flag::SongKit, false, pEngine );
	CPPUNIT_ASSERT( pKitB != nullptr );
	RoundTripAssertions::assertDrumkitEqual( pKitA, pKitB );

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
		pKit, pEngine->getSong()->getDrumkit() );

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
	const auto xml = pInstrA->toXmlBuffer(
		Xml::Flag::SongKit | Xml::Flag::KeepMissingSamples, false );
	auto pInstrB =
		Instrument::fromXmlBuffer( xml, Xml::Flag::SongKit, false, pEngine );
	CPPUNIT_ASSERT( pInstrB != nullptr );
	RoundTripAssertions::assertInstrumentEqual( pInstrA, pInstrB );

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
		pNewInstr, pEngine->getSong()->getDrumkit()->getInstruments()->get( 0 ) );

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
	RoundTripAssertions::assertPatternEqual( pPatternA, pPatternB );

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
		pPattern, pEngine->getSong()->getPatternList()->get( 0 ) );

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
	RoundTripAssertions::assertPlaylistEqual( pPlaylistA, pPlaylistB );

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
		pPlaylist, pEngine->getPlaylist() );

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
	RoundTripAssertions::assertPlaylistEntryEqual( pEntryA, pEntryB );

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
		pEntry, pEngine->getPlaylist()->get( 0 ) );

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

	RoundTripAssertions::assertCorePreferencesEqual( pPrefA, pPrefB );

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
		pMirrorPref, pEngine->getPreferences() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;
}

void IpcRoundTripTest::testNoteRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();

	// Engine needs a song with a drumkit containing an instrument
	pEngine->getCoreActionController()->setSong(
		Song::getEmptySong( pEngine ) );
	auto pEngineSong = pEngine->getSong();
	auto pEngineKit = pEngineSong->getDrumkit();
	CPPUNIT_ASSERT( pEngineKit != nullptr );
	CPPUNIT_ASSERT( pEngineKit->getInstruments()->size() > 0 );
	auto pEngineInstr = pEngineKit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pEngineInstr != nullptr );

	// ── Serialization level: toXmlBuffer → fromXmlBuffer ──
	auto pNoteA = std::make_shared<Note>( pEngineInstr, 16, 0.8f, 0.2f, 48 );
	pNoteA->setNoteOff( false );
	pNoteA->setProbability( 0.9f );
	pNoteA->setKey( Note::Key::C );
	pNoteA->setOctave( Note::Octave::P8C );
	pNoteA->setType( "Kick" );
	pNoteA->setHumanizeDelay( 14 );

	// Set SelectedLayerInfo if the instrument has components/layers
	CPPUNIT_ASSERT( pEngineInstr->getComponents()->size() > 0 );
	auto pComponent = pEngineInstr->getComponent( 0 );
	CPPUNIT_ASSERT( pComponent != nullptr && pComponent->getLayers().size() > 0 );
	auto pSelInfo = std::make_shared<SelectedLayerInfo>();
	pSelInfo->pLayer = pComponent->getLayer( 0 );
	pNoteA->setSelectedLayerInfo( pSelInfo, pComponent );

	const auto xml = pNoteA->toXmlBuffer();
	auto pNoteB = Note::fromXmlBuffer( xml, false, pEngine );
	CPPUNIT_ASSERT( pNoteB != nullptr );
	CPPUNIT_ASSERT( pNoteB->getSelectedLayerInfo( pComponent ) != nullptr );
	RoundTripAssertions::assertNoteEqual( pNoteA, pNoteB );
	RoundTripAssertions::assertLayerEqual(
		pSelInfo->pLayer, pNoteB->getSelectedLayerInfo( pComponent )->pLayer );

	// ── IPC level: noteOn via IpcCoreActionController ──
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

	// Build a note in the mirror context using the mirror's drumkit instrument
	auto pMirrorInstr =
		pMirrorSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pMirrorInstr != nullptr );
	auto pNote = std::make_shared<Note>( pMirrorInstr, 16, 0.8f );
	CPPUNIT_ASSERT( pEngine->getAudioEngine()->getSampler()
			->getPlayingNotesNumber() == 0 );
	pController->noteOn( pNote );

	// Pump until the note arrives in the engine's sampler playing queue
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getAudioEngine()->getSampler()
			->getPlayingNotesNumber() > 0;
	} ) );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testMidiEventMapRoundTrip()
{
	___INFOLOG( "" );

	// ── Serialization level: toXmlBuffer → fromXmlBuffer ──
	auto pMapA = makeMidiEventMap();
	const auto xml = pMapA->toXmlBuffer();
	auto pMapB = MidiEventMap::fromXmlBuffer( xml );
	CPPUNIT_ASSERT( pMapB != nullptr );
	RoundTripAssertions::assertMidiEventMapEqual( pMapA, pMapB );

	// ── IPC level: setMidiEventMap via IpcCoreActionController ──
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

	auto pMap = makeMidiEventMap();
	CPPUNIT_ASSERT( pController->setMidiEventMap( pMap ) );

	// The engine boots with a default map of its own (loaded from the
	// test preferences) which happens to have the same event count —
	// waiting on the size alone could pass before our install lands.
	// Wait for content equality instead.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		const auto pEngineMap = pEngine->getPreferences()->getMidiEventMap();
		return pEngineMap != nullptr &&
			   pEngineMap->toXmlBuffer() == pMap->toXmlBuffer();
	} ) );

	RoundTripAssertions::assertMidiEventMapEqual(
		pMap, pEngine->getPreferences()->getMidiEventMap() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testMidiInstrumentMapRoundTrip()
{
	___INFOLOG( "" );

	// ── Serialization level: toXmlBuffer → fromXmlBuffer ──
	auto pMapA = makeMidiInstrumentMap();
	const auto xml = pMapA->toXmlBuffer();
	auto pMapB = MidiInstrumentMap::fromXmlBuffer( xml );
	CPPUNIT_ASSERT( pMapB != nullptr );
	RoundTripAssertions::assertMidiInstrumentMapEqual( pMapA, pMapB );

	// ── IPC level: setMidiInstrumentMap via IpcCoreActionController ──
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

	auto pMap = makeMidiInstrumentMap();
	CPPUNIT_ASSERT( pController->setMidiInstrumentMap( pMap ) );

	// The engine boots with a default map of its own (loaded from the
	// test preferences) — wait for content equality, not just presence.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		const auto pEngineMap =
			pEngine->getPreferences()->getMidiInstrumentMap();
		return pEngineMap != nullptr &&
			   pEngineMap->toXmlBuffer() == pMap->toXmlBuffer();
	} ) );

	RoundTripAssertions::assertMidiInstrumentMapEqual(
		pMap, pEngine->getPreferences()->getMidiInstrumentMap() );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testLastMidiEventRoundTrip()
{
	___INFOLOG( "" );

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

	// ── Query level: the poll reads the authoritative engine ──
	// Seed the channel like an incoming MIDI event would — on the
	// engine, and differently on the mirror: the engine's MIDI input
	// is the only writer, so a stale mirror value must not shadow it.
	pEngine->setLastMidiEvent( MidiEvent::Type::CC );
	pEngine->setLastMidiEventParameter( Midi::Parameter( 74 ) );
	pMirror->setLastMidiEvent( MidiEvent::Type::PC );
	pMirror->setLastMidiEventParameter( Midi::Parameter( 3 ) );

	const auto lastMidiEvent = pAccess->getLastMidiEvent();
	CPPUNIT_ASSERT( lastMidiEvent.type == MidiEvent::Type::CC );
	CPPUNIT_ASSERT( lastMidiEvent.parameter == Midi::Parameter( 74 ) );

	// ── Command level: the editor's reset crosses to the engine ──
	CPPUNIT_ASSERT( pController->setLastMidiEvent(
		MidiEvent::Type::Null, Midi::ParameterInvalid ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getLastMidiEvent() == MidiEvent::Type::Null &&
			pEngine->getLastMidiEventParameter() == Midi::ParameterInvalid;
	} ) );

	// The reset is visible through the query — what the widget polls
	// while no event arrives. The query is FIFO-ordered after the
	// reset command; the pump above is defensive.
	const auto resetEvent = pAccess->getLastMidiEvent();
	CPPUNIT_ASSERT( resetEvent.type == MidiEvent::Type::Null );
	CPPUNIT_ASSERT( resetEvent.parameter == Midi::ParameterInvalid );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testCustomLibraryDirsRoundTrip()
{
	___INFOLOG( "" );

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

	// A real kit in a temporary dir — proves the engine's database
	// rescans the registered dir, not just its Preferences list.
	QTemporaryDir tmpDir( Filesystem::tmpDir() +
						  "custom-lib-dirs-ipc-test-XXXXXX" );
	CPPUNIT_ASSERT( tmpDir.isValid() );
	auto pKit = Drumkit::load( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml" ),
							   false, nullptr, false, pMirror );
	CPPUNIT_ASSERT( pKit != nullptr );
	CPPUNIT_ASSERT( pKit->save(
		Filesystem::drumkitPathFromDir( tmpDir.path() ), false ) );

	const auto dbHasTmpKit = [&]( Hydrogen* pHydrogen ) {
		for ( const auto& it : pHydrogen->getSoundLibraryDatabase()
					->getDrumkitDatabase() ) {
			if ( it.first.contains( "custom-lib-dirs-ipc-test" ) ) {
				return true;
			}
		}
		return false;
	};

	// ── Command level: the add crosses to the engine ──
	CPPUNIT_ASSERT( pController->addCustomSoundLibraryDir( tmpDir.path() ) );

	// The engine's Preferences pick up the dir ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getPreferences()->getCustomSoundLibraryDirs()
			.contains( tmpDir.path() );
	} ) );
	// ... and its database rescans it — the part a plain
	// SetPreferences sync would miss.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return dbHasTmpKit( pEngine );
	} ) );

	// The base call keeps the mirror coherent — synchronously, as
	// the dual-apply runs on the caller thread.
	CPPUNIT_ASSERT( pMirror->getPreferences()->getCustomSoundLibraryDirs()
					.contains( tmpDir.path() ) );
	CPPUNIT_ASSERT( dbHasTmpKit( pMirror ) );

	// ── The remove crosses too ──
	CPPUNIT_ASSERT( pController->removeCustomSoundLibraryDir(
		tmpDir.path() ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getPreferences()->getCustomSoundLibraryDirs()
			.contains( tmpDir.path() );
	} ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! dbHasTmpKit( pEngine );
	} ) );
	CPPUNIT_ASSERT( ! pMirror->getPreferences()->getCustomSoundLibraryDirs()
					.contains( tmpDir.path() ) );
	CPPUNIT_ASSERT( ! dbHasTmpKit( pMirror ) );

	// An empty path is rejected before anything is sent.
	CPPUNIT_ASSERT( ! pController->addCustomSoundLibraryDir( "" ) );
	CPPUNIT_ASSERT( ! pController->removeCustomSoundLibraryDir( "" ) );

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testExportSongRoundTrip()
{
	___INFOLOG( "" );

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

	// A song with notes, so the render produces audio.
	auto pSong = Song::load(
		QString( H2TEST_FILE( "functional/test_adsr.h2song" ) ), false,
		pMirror );
	CPPUNIT_ASSERT( pSong != nullptr );
	const QString sNewSongName( "IPC_EXPORT_SONG_TEST" );
	pSong->setName( sNewSongName );
	pController->setSong( pSong );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getSong() != nullptr &&
			   pEngine->getSong()->getName() == sNewSongName;
	} ) );

	// The mirror never renders: no disk writer before, during, or
	// after the export.
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pMirror->getAudioDriver() ) == nullptr );

	QTemporaryDir tmpDir( Filesystem::tmpDir() +
						  "export-song-ipc-test-XXXXXX" );
	CPPUNIT_ASSERT( tmpDir.isValid() );
	const QString sFile = tmpDir.path() + "/export.wav";

	// One-shot plan: a single render with a per-instrument exclusion,
	// exercising the full marshalled payload.
	auto pExcluded = pSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pExcluded != nullptr );
	std::vector<ExportRender> renders;
	renders.push_back( ExportRender{ sFile, { pExcluded->getUuid() } } );
	CPPUNIT_ASSERT( pController->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		renders ) );

	// The session opens on the engine ...
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getIsExportSessionActive();
	} ) );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pEngine->getAudioDriver() ) != nullptr );
	// ... but never on the mirror.
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pMirror->getAudioDriver() ) == nullptr );

	// The final progress event crosses to the mirror.
	CPPUNIT_ASSERT( TestHelper::pumpUntilEvent(
		pMirror, Event::Type::AudioExportProgress, 100, 30000 ) );

	// The engine tears the session down itself and the file holds
	// audio.
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getIsExportSessionActive();
	}, 30000 ) );
	CPPUNIT_ASSERT( QFileInfo( sFile ).size() > 0 );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pEngine->getAudioDriver() ) == nullptr );
	CPPUNIT_ASSERT( std::dynamic_pointer_cast<DiskWriterDriver>(
		pMirror->getAudioDriver() ) == nullptr );

	// The failure query reads the engine's writer — which did not
	// fail — not the mirror's (nonexistent) one.
	CPPUNIT_ASSERT( ! pAccess->isExportWritingFailed() );

	// Stopping without an active session is an idempotent no-op.
	pController->stopExportSession();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getIsExportSessionActive();
	} ) );

	// Multi-render phase, modeling the reported failure: the
	// transport was rolling when the export started, several files
	// render in one plan, and afterwards the editor must not roll on
	// and the toolbar's stop must still take.
	pAccess->sequencerPlay();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getAudioEngine()->getState() ==
			AudioEngine::State::Playing;
	}, 5000 ) );

	const QString sFile2 = tmpDir.path() + "/export2.wav";
	const QString sFile3 = tmpDir.path() + "/export3.wav";
	std::vector<ExportRender> renders2;
	renders2.push_back( ExportRender{ sFile2, {} } );
	renders2.push_back( ExportRender{ sFile3, { pExcluded->getUuid() } } );
	CPPUNIT_ASSERT( pController->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		renders2 ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getIsExportSessionActive();
	}, 30000 ) );
	CPPUNIT_ASSERT( QFileInfo( sFile2 ).size() > 0 );
	CPPUNIT_ASSERT( QFileInfo( sFile3 ).size() > 0 );

	// Both sides must be at rest afterwards: the engine parks and
	// restores its own transport, the export command parks the
	// mirror's, and the telemetry play-follow must not roll the
	// mirror along with the engine's background renders.
	CPPUNIT_ASSERT( pEngine->getAudioEngine()->getState() !=
					AudioEngine::State::Playing );
	CPPUNIT_ASSERT( pEngine->getAudioEngine()->getNextState() !=
					AudioEngine::State::Playing );
	CPPUNIT_ASSERT_MESSAGE(
		QString( "mirror left rolling: state [%1] next [%2] "
				 "(2=Initialized 3=Prepared 4=Ready 5=CountIn 6=Playing)" )
			.arg( static_cast<int>( pMirror->getAudioEngine()->getState() ) )
			.arg( static_cast<int>( pMirror->getAudioEngine()->getNextState() ) )
			.toStdString(),
		pMirror->getAudioEngine()->getState() != AudioEngine::State::Playing );
	CPPUNIT_ASSERT( pMirror->getAudioEngine()->getNextState() !=
					AudioEngine::State::Playing );

	// The toolbar's stop path must still take: the command crosses
	// the channel and both engines settle at rest.
	pAccess->sequencerStop();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getAudioEngine()->getState() !=
				AudioEngine::State::Playing &&
			   pMirror->getAudioEngine()->getState() !=
				AudioEngine::State::Playing;
	}, 5000 ) );

	// A stop while a plan is still running cancels it: no later
	// render may call play() over the user's stop. Arm a
	// six-render plan and stop right away — the engine must tear
	// the session down, leave the transport at rest, and never run
	// the queued renders.
	std::vector<ExportRender> renders3;
	for ( int ii = 0; ii < 6; ++ii ) {
		renders3.push_back( ExportRender{
			QString( "%1/export-cancel-%2.wav" )
				.arg( tmpDir.path() ).arg( ii ),
			{} } );
	}
	CPPUNIT_ASSERT( pController->exportSong(
		48000, 16, 0.0, Interpolation::InterpolateMode::Linear, false,
		renders3 ) );
	pAccess->sequencerStop();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return ! pEngine->getIsExportSessionActive();
	}, 5000 ) );
	CPPUNIT_ASSERT( pEngine->getAudioEngine()->getState() !=
					AudioEngine::State::Playing );
	CPPUNIT_ASSERT( pMirror->getAudioEngine()->getState() !=
					AudioEngine::State::Playing );
	{
		int nFiles = 0;
		for ( int ii = 0; ii < 6; ++ii ) {
			if ( QFileInfo( QString( "%1/export-cancel-%2.wav" )
							 .arg( tmpDir.path() ).arg( ii ) ).size() > 0 ) {
				++nFiles;
			}
		}
		// The stop landed while the first render was still running —
		// the plan must have been cancelled, not run to completion.
		CPPUNIT_ASSERT( nFiles < 6 );
	}

	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}

void IpcRoundTripTest::testMidiNoteRecordingRoundTrip()
{
	___INFOLOG( "" );

	auto pEngine = TestHelper::makeEngine();

	// The empty song ships a drumkit with at least one instrument and ten
	// patterns (the first one active) in pattern mode.
	pEngine->getCoreActionController()->setSong(
		Song::getEmptySong( pEngine ) );
	auto pEngineSong = pEngine->getSong();
	CPPUNIT_ASSERT( pEngineSong != nullptr );
	CPPUNIT_ASSERT( pEngineSong->getPatternList()->size() > 0 );
	auto pEngineInstr = pEngineSong->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pEngineInstr != nullptr );
	const auto engineInstrId = pEngineInstr->getId();

	auto pMirror = TestHelper::makeMirror();
	pMirror->getCoreActionController()->setSong(
		Song::getEmptySong( pMirror ) );

	const QString sEndpoint = TestHelper::uniqueEndpoint();
	auto pSession = EngineSession::start( pEngine, sEndpoint );
	CPPUNIT_ASSERT( pSession != nullptr );

	auto pEditorSession = EditorSession::connect( sEndpoint, pMirror );
	CPPUNIT_ASSERT( pEditorSession != nullptr );

	// Recording only happens while the audio engine is actually Playing
	// (Hydrogen::addRealtimeNote) and lands in the selected pattern.
	pEngine->setSelectedPatternNumber( 0 );
	pEngine->setRecordEnabled( true );
	pEngine->sequencerPlay();
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pEngine->getAudioEngine()->getState() ==
			AudioEngine::State::Playing;
	} ) );

	// Simulate a MIDI note-on arriving at the headless engine (engines own
	// all control surfaces, ADR 0016). NoteInvalid keeps key/octave at
	// their defaults regardless of the MIDI instrument map mode.
	CPPUNIT_ASSERT( pEngine->getEventQueue()->m_addMidiNoteVector.empty() );
	CPPUNIT_ASSERT(
		pEngine->addRealtimeNote( 0, 0.8f, false, Midi::NoteInvalid ) );

	// The note must cross the split: the bridge drains the engine's
	// m_addMidiNoteVector and the editor's mirror re-queues it for
	// HydrogenApp::onEventQueueTimer (which turns it into an undoable
	// pattern edit).
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return pMirror->getEventQueue()->m_addMidiNoteVector.size() == 1;
	} ) );

	const auto& noteAction =
		pMirror->getEventQueue()->m_addMidiNoteVector[ 0 ];
	CPPUNIT_ASSERT( noteAction.id == engineInstrId );
	CPPUNIT_ASSERT( noteAction.nPattern == 0 );
	CPPUNIT_ASSERT( noteAction.fVelocity == 0.8f );
	CPPUNIT_ASSERT( noteAction.fPan == 0.f );
	CPPUNIT_ASSERT( noteAction.nLength == -1 );
	CPPUNIT_ASSERT( noteAction.key == Note::KeyDefault );
	CPPUNIT_ASSERT( noteAction.octave == Note::OctaveDefault );
	CPPUNIT_ASSERT( noteAction.nColumn >= 0 );
	CPPUNIT_ASSERT( noteAction.nColumn <
					pEngineSong->getPatternList()->get( 0 )->getLength() );

	// The engine-side vector must be drained — nothing else consumes it in
	// the split, an undrained vector would grow unboundedly.
	CPPUNIT_ASSERT( pEngine->getEventQueue()->m_addMidiNoteVector.empty() );

	pEngine->sequencerStop();
	pSession->stop();
	pEditorSession->disconnect();
	delete pMirror;
	delete pEngine;

	___INFOLOG( "passed" );
}
