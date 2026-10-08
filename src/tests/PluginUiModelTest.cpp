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

#include "PluginUiModelTest.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>

#include <plugin/HydrogenPlugin.h>
#include <plugin/ui/PluginUiLandingModel.h>
#include <plugin/ui/PluginUiMappingModel.h>
#include <plugin/ui/PluginUiMixerModel.h>
#include <plugin/ui/PluginUiStrings.h>

#include <QtCore/QFile>

#include "TestHelper.h"

using namespace H2Core;

void PluginUiModelTest::testLandingLoadCommands()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiLandingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.loadSong(
		H2TEST_FILE( "functional/test_adsr.h2song" ) ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// Song: load + install — a new song object replaces the empty one and
	// setSong loads the installed kit's samples (ASSERT_SONG; the fixture's
	// kit resolves its samples via its drumkitPath elements).
	auto pSongBefore = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSongBefore != nullptr );
	CPPUNIT_ASSERT( model.loadSong(
		H2TEST_FILE( "functional/test_adsr.h2song" ) ) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong != pSongBefore );
	ASSERT_SONG( pSong );

	// An unloadable path leaves the current song untouched.
	CPPUNIT_ASSERT( ! model.loadSong( "/nonexistent/nope.h2song" ) );
	CPPUNIT_ASSERT( pHydrogen->getSong() == pSong );

	// Pattern: load + install at position 0 of the current song.
	auto pPatternList = pSong->getPatternList();
	CPPUNIT_ASSERT( pPatternList != nullptr );
	const int nPatternsBefore = pPatternList->size();
	CPPUNIT_ASSERT( model.loadPattern(
		H2TEST_FILE( "pattern/pattern.h2pattern" ), 0, false ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );
	CPPUNIT_ASSERT( pPatternList->get( 0 ) != nullptr );
	CPPUNIT_ASSERT( pPatternList->get( 0 )->getName() == "pat" );

	// A second load replacing position 0 swaps the pattern in place.
	CPPUNIT_ASSERT( model.loadPattern(
		H2TEST_FILE( "pattern/empty.h2pattern" ), 0, true ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );
	CPPUNIT_ASSERT( pPatternList->get( 0 ) != nullptr );
	CPPUNIT_ASSERT( pPatternList->get( 0 )->getName() == "Pattern" );

	// An unloadable path leaves the pattern list untouched.
	CPPUNIT_ASSERT( ! model.loadPattern( "/nonexistent/nope.h2pattern",
										 0, false ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );

	// Drumkit: load + install into the current song.
	CPPUNIT_ASSERT( model.loadDrumkit(
		TestHelper::get_instance()->getDataDir() + "/drumkits/GMRockKit" ) );
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getName() == "GMRockKit" );

	// An unloadable path leaves the current kit untouched.
	CPPUNIT_ASSERT( ! model.loadDrumkit( "/nonexistent/nope" ) );
	CPPUNIT_ASSERT( pHydrogen->getSong()->getDrumkit() == pDrumkit );

	// Editor button: forwards to the plugin's editor seam. No process
	// spawn in the unit test — the always-on session (UI-3) serves the
	// endpoint already.
	CPPUNIT_ASSERT( model.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( ! plugin.getEditorEndpoint().isEmpty() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testLandingMidiConfigNoWriteBack()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// The plugin never persists its runtime config: whatever the model
	// applies, the user config file must stay byte-identical (or stay
	// absent). Snapshot after construction so bootstrap noise cannot
	// mask the pin.
	const QString sConfigPath = Filesystem::userConfigPath();
	QByteArray sConfigBefore;
	bool bConfigExists = false;
	{
		QFile file( sConfigPath );
		if ( file.exists() ) {
			CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly ) );
			sConfigBefore = file.readAll();
			file.close();
			bConfigExists = true;
		}
	}

	// Scalar MIDI control settings: applied to the plugin's engine
	// preferences (ADR 0022 plugin-instance layer). Distinctive values —
	// several differ from the defaults, so adoption cannot pass vacuously.
	CPPUNIT_ASSERT( model.applyMidiControlSettings(
		/*bNoteOffIgnore=*/false, Midi::Channel( 9 ),
		/*bEnableFeedback=*/true, /*bTransportInputHandling=*/false,
		/*bTransportOutputSend=*/true, Midi::Channel( 12 ),
		Preferences::MidiSendNoteOff::OnCustomLengths ) );
	auto pPref = plugin.getHydrogen()->getPreferences();
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_bMidiNoteOffIgnore == false );
	CPPUNIT_ASSERT( pPref->m_midiActionChannel == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( pPref->m_bEnableMidiFeedback == true );
	CPPUNIT_ASSERT( pPref->getMidiTransportInputHandling() == false );
	CPPUNIT_ASSERT( pPref->getMidiTransportOutputSend() == true );
	CPPUNIT_ASSERT( pPref->getMidiFeedbackChannel() == Midi::Channel( 12 ) );
	CPPUNIT_ASSERT( pPref->getMidiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );

	// MIDI instrument map: a custom-mode map with one per-instrument row.
	auto pMap = std::make_shared<MidiInstrumentMap>();
	pMap->setInput( MidiInstrumentMap::Input::Custom );
	pMap->setUseGlobalInputChannel( true );
	pMap->setGlobalInputChannel( Midi::Channel( 5 ) );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	pMap->insertCustomInputMapping( pInstrument, Midi::Note( 42 ),
									Midi::Channel( 5 ) );
	CPPUNIT_ASSERT( model.applyMidiInstrumentMap( pMap ) );

	auto pEngineMap = pPref->getMidiInstrumentMap();
	CPPUNIT_ASSERT( pEngineMap != nullptr );
	CPPUNIT_ASSERT( pEngineMap == pMap );
	CPPUNIT_ASSERT( pEngineMap->getInput() ==
					MidiInstrumentMap::Input::Custom );
	CPPUNIT_ASSERT( pEngineMap->getUseGlobalInputChannel() == true );
	CPPUNIT_ASSERT( pEngineMap->getGlobalInputChannel() ==
					Midi::Channel( 5 ) );
	CPPUNIT_ASSERT( pEngineMap->getCustomInputMappingsType().size() +
					pEngineMap->getCustomInputMappingsId().size() == 1 );

	// No write-back: the user config file is byte-identical (or still
	// absent) after everything the model applied.
	{
		QFile file( sConfigPath );
		if ( bConfigExists ) {
			CPPUNIT_ASSERT( file.exists() );
			CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly ) );
			CPPUNIT_ASSERT( file.readAll() == sConfigBefore );
		}
		else {
			CPPUNIT_ASSERT( ! file.exists() );
		}
	}

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testLandingMidiConfigExposedReadApply()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// Distinctive values on all seven settings, so neither the reads
	// nor the preservation pin below can pass vacuously.
	CPPUNIT_ASSERT( model.applyMidiControlSettings(
		/*bNoteOffIgnore=*/true, Midi::Channel( 3 ),
		/*bEnableFeedback=*/true, /*bTransportInputHandling=*/false,
		/*bTransportOutputSend=*/true, Midi::Channel( 12 ),
		Preferences::MidiSendNoteOff::Never ) );

	// Read side: the exposed widgets populate from the engine.
	CPPUNIT_ASSERT( model.midiNoteOffIgnore() == true );
	CPPUNIT_ASSERT( model.midiActionChannel() == Midi::Channel( 3 ) );
	CPPUNIT_ASSERT( model.midiSendNoteOff() ==
					Preferences::MidiSendNoteOff::Never );

	// Narrow apply: changes the exposed three, preserves the rest.
	CPPUNIT_ASSERT( model.applyExposedMidiControlSettings(
		/*bNoteOffIgnore=*/false, Midi::Channel( 9 ),
		Preferences::MidiSendNoteOff::OnCustomLengths ) );

	auto pPref = plugin.getHydrogen()->getPreferences();
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_bMidiNoteOffIgnore == false );
	CPPUNIT_ASSERT( pPref->m_midiActionChannel == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( pPref->getMidiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );
	// The four settings the plugin UI does not expose (feedback,
	// transport) keep their engine values — the preservation pin.
	CPPUNIT_ASSERT( pPref->m_bEnableMidiFeedback == true );
	CPPUNIT_ASSERT( pPref->getMidiTransportInputHandling() == false );
	CPPUNIT_ASSERT( pPref->getMidiTransportOutputSend() == true );
	CPPUNIT_ASSERT( pPref->getMidiFeedbackChannel() == Midi::Channel( 12 ) );

	// The reads follow the narrow apply.
	CPPUNIT_ASSERT( model.midiNoteOffIgnore() == false );
	CPPUNIT_ASSERT( model.midiActionChannel() == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( model.midiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelRoundTrip()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMappingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setInputMode(
		MidiInstrumentMap::Input::Custom ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pPreferences = pHydrogen->getPreferences();
	CPPUNIT_ASSERT( pPreferences != nullptr );

	// Map-wide settings — distinctive from-default values, each applied
	// with one command and read back from the engine's live map.
	CPPUNIT_ASSERT( model.setInputMode( MidiInstrumentMap::Input::Custom ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()->getInput() ==
					MidiInstrumentMap::Input::Custom );
	CPPUNIT_ASSERT( model.setOutputMode(
		MidiInstrumentMap::Output::Constant ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()->getOutput() ==
					MidiInstrumentMap::Output::Constant );
	CPPUNIT_ASSERT( model.setUseGlobalInputChannel( true ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getUseGlobalInputChannel() );
	CPPUNIT_ASSERT( model.setGlobalInputChannel( Midi::Channel( 3 ) ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getGlobalInputChannel() == Midi::Channel( 3 ) );
	CPPUNIT_ASSERT( model.setUseGlobalOutputChannel( true ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getUseGlobalOutputChannel() );
	CPPUNIT_ASSERT( model.setGlobalOutputChannel( Midi::Channel( 7 ) ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getGlobalOutputChannel() == Midi::Channel( 7 ) );

	// Per-instrument custom input row — inserted through the model and
	// resolved back through the engine's live map.
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.insertCustomInputMapping(
		pInstrument, Midi::Note( 42 ), Midi::Channel( 3 ) ) );
	auto pLiveMap = pPreferences->getMidiInstrumentMap();
	CPPUNIT_ASSERT( pLiveMap->getCustomInputMappingsType().size() +
					pLiveMap->getCustomInputMappingsId().size() == 1 );
	const auto mapping = pLiveMap->getInputMapping(
		pInstrument, pDrumkit, pHydrogen );
	CPPUNIT_ASSERT( mapping.note == Midi::Note( 42 ) );
	CPPUNIT_ASSERT( mapping.channel == Midi::Channel( 3 ) );

	// Per-instrument output — lives on the instrument itself, keyed by
	// the instrument index within the current kit.
	CPPUNIT_ASSERT( model.setInstrumentMidiOutNote( 0, Midi::Note( 60 ) ) );
	CPPUNIT_ASSERT( model.setInstrumentMidiOutChannel( 0,
													   Midi::Channel( 9 ) ) );
	CPPUNIT_ASSERT( pInstrument->getMidiOutNote() == Midi::Note( 60 ) );
	CPPUNIT_ASSERT( pInstrument->getMidiOutChannel() == Midi::Channel( 9 ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelInstrumentReads()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// The default engine song carries a kit with instruments.
	CPPUNIT_ASSERT( model.instrumentCount() > 0 );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.instrumentName( 0 ) == pInstrument->getName() );
	// Out of range — empty, not garbage.
	CPPUNIT_ASSERT( model.instrumentName( model.instrumentCount() ).isEmpty() );

	// The output note/channel reads follow the commands.
	CPPUNIT_ASSERT( model.setInstrumentMidiOutNote( 0, Midi::Note( 60 ) ) );
	CPPUNIT_ASSERT( model.instrumentMidiOutNote( 0 ) == Midi::Note( 60 ) );
	CPPUNIT_ASSERT( model.setInstrumentMidiOutChannel( 0,
													   Midi::Channel( 9 ) ) );
	CPPUNIT_ASSERT( model.instrumentMidiOutChannel( 0 ) ==
					Midi::Channel( 9 ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelInputMappingRead()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// Custom mode + a custom row for instrument 0, inserted by index —
	// the read resolves the effective incoming note/channel through
	// the engine's live map.
	CPPUNIT_ASSERT( model.setInputMode( MidiInstrumentMap::Input::Custom ) );
	CPPUNIT_ASSERT( model.instrumentCount() > 0 );
	CPPUNIT_ASSERT( model.insertCustomInputMapping(
		0, Midi::Note( 42 ), Midi::Channel( 3 ) ) );

	const auto mapping = model.inputMapping( 0 );
	CPPUNIT_ASSERT( mapping.note == Midi::Note( 42 ) );
	CPPUNIT_ASSERT( mapping.channel == Midi::Channel( 3 ) );

	// Out of range — invalid read, refused insert, no garbage.
	CPPUNIT_ASSERT( model.inputMapping( model.instrumentCount() ).isNull() );
	CPPUNIT_ASSERT( ! model.insertCustomInputMapping(
		model.instrumentCount(), Midi::Note( 42 ), Midi::Channel( 3 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testChannelDisplayName()
{
	___INFOLOG( "" );

	// The channel dropdowns' display convention (the GUI's channel
	// spinboxes show the same): -1 reads "all", 0 reads "Off", 1..16
	// read as themselves. The invalid -2 of unmapped rows reads "Off"
	// too — the GUI disables those widgets instead. Pure static — no
	// engine needed.
	CPPUNIT_ASSERT( PluginUiStrings::channelDisplayName(
						Midi::ChannelAll ) == "all" );
	CPPUNIT_ASSERT( PluginUiStrings::channelDisplayName(
						Midi::ChannelOff ) == "Off" );
	CPPUNIT_ASSERT( PluginUiStrings::channelDisplayName(
						Midi::ChannelInvalid ) == "Off" );
	CPPUNIT_ASSERT( PluginUiStrings::channelDisplayName(
						Midi::Channel( 1 ) ) == "1" );
	CPPUNIT_ASSERT( PluginUiStrings::channelDisplayName(
						Midi::Channel( 16 ) ) == "16" );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelStripsAndMeters()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMixerModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setStripVolume( 0, 0.5f ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	// Strip commands land on the instrument.
	CPPUNIT_ASSERT( model.setStripVolume( 0, 0.42f ) );
	CPPUNIT_ASSERT( pInstrument->getVolume() == 0.42f );
	CPPUNIT_ASSERT( model.setStripPan( 0, -0.25f ) );
	CPPUNIT_ASSERT( pInstrument->getPan() == -0.25f );
	CPPUNIT_ASSERT( model.setStripIsMuted( 0, true ) );
	CPPUNIT_ASSERT( pInstrument->isMuted() );
	CPPUNIT_ASSERT( model.setStripIsSoloed( 0, true ) );
	CPPUNIT_ASSERT( pInstrument->isSoloed() );

	// Master commands land on the song.
	CPPUNIT_ASSERT( model.setMasterVolume( 0.7f ) );
	CPPUNIT_ASSERT( pSong->getVolume() == 0.7f );
	CPPUNIT_ASSERT( model.setMasterIsMuted( true ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() );

	// Meters: no snapshot yet — reads are zeroed, not garbage.
	CPPUNIT_ASSERT( ! model.hasMeterSnapshot() );
	CPPUNIT_ASSERT( model.instrumentPeakL( 0 ) == 0.0f );
	CPPUNIT_ASSERT( model.masterPeakL() == 0.0f );

	// A snapshot accepted through the sink seam is what the reads serve.
	EngineTelemetrySnapshot snapshot;
	snapshot.masterPeakL = 0.7f;
	snapshot.masterPeakR = 0.8f;
	snapshot.instPeakCount = 2;
	snapshot.peakL[ 0 ] = 0.1f;
	snapshot.peakR[ 0 ] = 0.2f;
	snapshot.peakL[ 1 ] = 0.3f;
	snapshot.peakR[ 1 ] = 0.4f;
	model.acceptMeterSnapshot( snapshot );

	CPPUNIT_ASSERT( model.hasMeterSnapshot() );
	CPPUNIT_ASSERT( model.masterPeakL() == 0.7f );
	CPPUNIT_ASSERT( model.masterPeakR() == 0.8f );
	CPPUNIT_ASSERT( model.instrumentPeakL( 0 ) == 0.1f );
	CPPUNIT_ASSERT( model.instrumentPeakR( 0 ) == 0.2f );
	CPPUNIT_ASSERT( model.instrumentPeakL( 1 ) == 0.3f );
	CPPUNIT_ASSERT( model.instrumentPeakR( 1 ) == 0.4f );
	// Beyond the snapshot's instrument count — zeroed, not out-of-bounds.
	CPPUNIT_ASSERT( model.instrumentPeakL( 2 ) == 0.0f );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelStripReads()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// The host context: two buses (ADR 0019).
	CPPUNIT_ASSERT( model.busCount() == 2 );

	// The default engine song carries a kit with instruments.
	CPPUNIT_ASSERT( model.stripCount() > 0 );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.stripName( 0 ) == pInstrument->getName() );
	// Out of range — empty, not garbage.
	CPPUNIT_ASSERT( model.stripName( model.stripCount() ).isEmpty() );

	// The strip reads follow the commands.
	CPPUNIT_ASSERT( model.setStripVolume( 0, 0.42f ) );
	CPPUNIT_ASSERT( model.stripVolume( 0 ) == 0.42f );
	CPPUNIT_ASSERT( model.setStripPan( 0, -0.25f ) );
	CPPUNIT_ASSERT( model.stripPan( 0 ) == -0.25f );
	CPPUNIT_ASSERT( model.setStripIsMuted( 0, true ) );
	CPPUNIT_ASSERT( model.stripIsMuted( 0 ) );
	CPPUNIT_ASSERT( model.setStripIsSoloed( 0, true ) );
	CPPUNIT_ASSERT( model.stripIsSoloed( 0 ) );

	// The master reads follow the commands.
	CPPUNIT_ASSERT( model.setMasterVolume( 0.7f ) );
	CPPUNIT_ASSERT( model.masterVolume() == 0.7f );
	CPPUNIT_ASSERT( model.setMasterIsMuted( true ) );
	CPPUNIT_ASSERT( model.masterIsMuted() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelBusCommands()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMixerModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setInstrumentOutputBus( 0, 2 ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	// Implicit default: no custom mapping stored, the effective bus is
	// the kit position (ADR 0019 1-to-1).
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == -1 );
	CPPUNIT_ASSERT( model.instrumentOutputBus( 0 ) == -1 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 0 );

	// Explicit custom mapping lands on the engine's instrument.
	CPPUNIT_ASSERT( model.setInstrumentOutputBus( 0, 2 ) );
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == 2 );
	CPPUNIT_ASSERT( model.instrumentOutputBus( 0 ) == 2 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 2 );

	// Beyond the host's bus count: no individual bus (master only).
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 2 ) == -1 );

	// Reset to the implicit 1-to-1 default.
	CPPUNIT_ASSERT( model.setInstrumentOutputBus( 0, -1 ) );
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == -1 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 0 );

	___INFOLOG( "passed" );
}
