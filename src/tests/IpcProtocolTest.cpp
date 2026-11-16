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

#include "IpcProtocolTest.h"

#include <core/Basics/Event.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/EngineTelemetry.h>
#include <core/Object.h>

#include <QtCore/QDataStream>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace H2Core;

// Every Event::Type, so the codec round-trip covers the full vocabulary.
static std::vector<Event::Type> allEventTypes() {
	using T = Event::Type;
	return {
		T::ActionModeChanged, T::AudioDriverChanged, T::AudioExportProgress,
		T::BbtChanged, T::BeatCounter, T::DrumkitIsModified, T::DrumkitLoaded,
		T::Error, T::GridCellToggled, T::InstrumentLayerChanged,
		T::InstrumentMuteSoloChanged, T::InstrumentParametersChanged,
		T::JackTimebaseStateChanged, T::JackTransportActivation,
		T::LoopModeActivation, T::Metronome, T::MidiClockActivation,
		T::MidiDriverChanged, T::MidiEventMapChanged, T::MidiInput, T::MidiOutput,
		T::MixerSettingsChanged, T::NextPatternsChanged, T::NextShot,
		T::NoteRender, T::OnlineImportProgress, T::PatternChanged,
		T::PatternEditorLocked, T::PatternIsModified, T::PlaybackTrackChanged,
		T::PlayingPatternsChanged, T::PlaylistChanged, T::PlaylistLoadSong,
		T::Quit, T::RecordModeChanged, T::Relocation, T::SelectedInstrumentChanged,
		T::SelectedPatternChanged, T::SongIsModified, T::SongModeActivation,
		T::SongSizeChanged, T::SoundLibraryChanged,
		T::SoundLibraryScanProgress, T::StackedModeActivation,
		T::State, T::TempoChanged, T::TimelineActivation, T::UndoRedo,
		T::UpdatePreferences, T::UpdateSong, T::UpdateTimeline, T::Xrun
	};
}

// Encode a message, then decode it back through the streaming frame reader.
static bool roundTrip( const IpcMessage& in, IpcMessage& out ) {
	IpcFrameReader reader;
	reader.append( in.encode() );
	return reader.next( out );
}

void IpcProtocolTest::testCommandRoundTrip() {
	___INFOLOG( "" );

	// A representative spread of CoreActionController commands covering every
	// argument shape: none, float, int+bool, int+float+bool, int+QString, and a
	// large XML payload.
	std::vector<IpcMessage> commands;
	commands.push_back( IpcMessage( IpcOpcode::Play ) );
	commands.push_back( IpcMessage( IpcOpcode::RescanSoundLibrary ) );
	commands.push_back( IpcMessage( IpcOpcode::Panic ) );
	commands.push_back( IpcMessage( IpcOpcode::GetAudioDriverInfo ) );
	commands.push_back( IpcMessage( IpcOpcode::GetMidiDriverInfo ) );
	commands.push_back( IpcMessage( IpcOpcode::GetIsUnderSessionManagement ) );
	commands.push_back( IpcMessage( IpcOpcode::GetIsUnderPluginHost ) );
	commands.push_back( IpcMessage( IpcOpcode::NoteOn ) );
	commands.push_back( IpcMessage( IpcOpcode::ReleasePlayingNotes )
						.arg( H2Core::Uuid().toQString() ) );
	commands.push_back( IpcMessage( IpcOpcode::SetBpm ).arg( 137.5f ) );
	commands.push_back( IpcMessage( IpcOpcode::LocateToTick )
							.arg( static_cast<qlonglong>( 192000 ) ).arg( true ) );
	commands.push_back( IpcMessage( IpcOpcode::SetStripPan )
							.arg( 3 ).arg( -0.25f ).arg( false ) );
	commands.push_back( IpcMessage( IpcOpcode::AddTag )
							.arg( 7 ).arg( QString( "intro" ) ) );
	{
		IpcMessage setSong( IpcOpcode::SetSong );
		setSong.setPayload( QByteArray( "<song><name>Test</name></song>" ) );
		commands.push_back( setSong );
	}

	for ( const auto& cmd : commands ) {
		IpcMessage decoded;
		CPPUNIT_ASSERT( roundTrip( cmd, decoded ) );
		CPPUNIT_ASSERT( decoded.getOpcode() == cmd.getOpcode() );
		CPPUNIT_ASSERT( decoded.getArgs() == cmd.getArgs() );
		CPPUNIT_ASSERT( decoded.getPayload() == cmd.getPayload() );
	}

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testEventRoundTrip() {
	___INFOLOG( "" );

	int nValueSeed = 0;
	for ( const auto type : allEventTypes() ) {
		const int nValue = nValueSeed++;
		const long nId = 1000 + nValue;

		IpcMessage decoded;
		CPPUNIT_ASSERT( roundTrip( IpcMessage::fromEvent( type, nValue, nId ),
								   decoded ) );

		Event::Type outType;
		int outValue = -1;
		long outId = -1;
		CPPUNIT_ASSERT( decoded.toEventFields( outType, outValue, outId ) );
		CPPUNIT_ASSERT( outType == type );
		CPPUNIT_ASSERT_EQUAL( nValue, outValue );
		CPPUNIT_ASSERT_EQUAL( nId, outId );
	}

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testStreamingFraming() {
	___INFOLOG( "" );

	// Two frames concatenated, fed to the reader one byte at a time, must come
	// back out intact and in order (QLocalSocket delivers arbitrary chunks).
	const QByteArray wire =
		IpcMessage( IpcOpcode::Play ).encode() +
		IpcMessage( IpcOpcode::SetBpm ).arg( 90.0f ).encode();

	IpcFrameReader reader;
	IpcMessage msg;

	int nDecoded = 0;
	for ( int i = 0; i < wire.size(); ++i ) {
		// Until the very last byte of a frame arrives, next() yields nothing.
		reader.append( wire.mid( i, 1 ) );
		while ( reader.next( msg ) ) {
			if ( nDecoded == 0 ) {
				CPPUNIT_ASSERT( msg.getOpcode() == IpcOpcode::Play );
			} else {
				CPPUNIT_ASSERT( msg.getOpcode() == IpcOpcode::SetBpm );
				CPPUNIT_ASSERT( msg.getArgs().size() == 1 );
			}
			++nDecoded;
		}
	}
	CPPUNIT_ASSERT_EQUAL( 2, nDecoded );
	CPPUNIT_ASSERT( ! reader.next( msg ) ); // nothing left buffered

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testHelloHandshake() {
	___INFOLOG( "" );

	// The hello announces the full version identity: wire-protocol tag plus
	// the application's major/minor/patch/suffix. Only the protocol tag
	// gates compatibility (see testHelloVersionCompatibility); the app
	// fields ride along as diagnostics.
	const IpcHelloVersion local = IpcHelloVersion::local();
	CPPUNIT_ASSERT( local.isValid() );

	IpcMessage decoded;
	CPPUNIT_ASSERT( roundTrip( IpcMessage::hello(), decoded ) );
	CPPUNIT_ASSERT( decoded.getOpcode() == IpcOpcode::Hello );

	// The exact arg layout is wire: [int protocol, int major, int minor,
	// int patch, QString suffix]. A peer built from another commit of the
	// same major.minor must be able to parse it, so pin the shape.
	CPPUNIT_ASSERT_EQUAL( 5, static_cast<int>( decoded.getArgs().size() ) );
	CPPUNIT_ASSERT( decoded.getArgs() ==
					IpcMessage::hello( local ).getArgs() );
	CPPUNIT_ASSERT( decoded.helloVersion() == local );

	// A peer announcing a different protocol version is detectable (mismatch
	// → graceful refusal in the transport layer).
	IpcHelloVersion otherProtocol = local;
	otherProtocol.nProtocolVersion = IPC_PROTOCOL_VERSION + 1;
	IpcMessage other;
	CPPUNIT_ASSERT( roundTrip( IpcMessage::hello( otherProtocol ), other ) );
	CPPUNIT_ASSERT( other.helloVersion().nProtocolVersion !=
					IPC_PROTOCOL_VERSION );

	// A legacy protocol-only hello (the pre-2.0 single-arg shape) carries no
	// version identity: it parses as invalid and must never pass the gate.
	IpcMessage legacy( IpcOpcode::Hello );
	legacy.arg( static_cast<int>( IPC_PROTOCOL_VERSION ) );
	IpcMessage decodedLegacy;
	CPPUNIT_ASSERT( roundTrip( legacy, decodedLegacy ) );
	CPPUNIT_ASSERT( ! decodedLegacy.helloVersion().isValid() );
	CPPUNIT_ASSERT( ! ipcHelloVersionsCompatible(
		decodedLegacy.helloVersion(), local ) );

	___INFOLOG( "passed" );
}

// The hello compatibility gate (ADR 0018): only the wire-protocol tag must
// match. The application version — major, minor, patch, suffix — is
// deliberately not compared: it rides the hello as diagnostics only, so any
// two builds that speak the same wire protocol interoperate. A protocol
// difference or a garbled/legacy hello is refused on both ends of the
// connection.
void IpcProtocolTest::testHelloVersionCompatibility() {
	___INFOLOG( "" );

	const IpcHelloVersion local = IpcHelloVersion::local();
	CPPUNIT_ASSERT( local.isValid() );

	// Identical versions are compatible (and the check is symmetric).
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( local, local ) );

	// The application version never blocks: major, minor, patch, and
	// suffix differences are all compatible — alone and combined, in both
	// directions, including a stable release (empty suffix).
	IpcHelloVersion majorBump = local;
	majorBump.nAppMajor += 1;
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( local, majorBump ) );
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( majorBump, local ) );

	IpcHelloVersion minorBump = local;
	minorBump.nAppMinor += 1;
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( local, minorBump ) );
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( minorBump, local ) );

	IpcHelloVersion otherSuffix = local;
	otherSuffix.sAppSuffix = "rc1";
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( local, otherSuffix ) );

	IpcHelloVersion release = local;
	release.nAppMajor += 5;
	release.sAppSuffix = "";
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( local, release ) );
	CPPUNIT_ASSERT( ipcHelloVersionsCompatible( release, local ) );

	// A wire-protocol difference is refused (the framing/vocabulary tag).
	IpcHelloVersion protocolBump = local;
	protocolBump.nProtocolVersion = IPC_PROTOCOL_VERSION + 1;
	CPPUNIT_ASSERT( ! ipcHelloVersionsCompatible( local, protocolBump ) );

	// An invalid version (legacy/garbled hello, or negative components) is
	// never compatible — in either argument position.
	IpcHelloVersion invalid;
	CPPUNIT_ASSERT( ! invalid.isValid() );
	CPPUNIT_ASSERT( ! ipcHelloVersionsCompatible( local, invalid ) );
	CPPUNIT_ASSERT( ! ipcHelloVersionsCompatible( invalid, local ) );

	IpcHelloVersion negative = local;
	negative.nAppMajor = -1;
	CPPUNIT_ASSERT( ! negative.isValid() );
	CPPUNIT_ASSERT( ! ipcHelloVersionsCompatible( local, negative ) );

	___INFOLOG( "passed" );
}

// Opcodes cross the wire as raw u16 values, so the numeric value of every
// opcode is part of the IPC API: inserting or reordering entries in
// IpcOpcode renumbers everything after it and silently breaks editors that
// passed the version handshake. This pins value→name for the whole
// vocabulary. If it fails, the enum changed: new opcodes must be APPENDED
// at the tail (and this list extended in the same commit, deliberately);
// a mid-enum change requires bumping IPC_PROTOCOL_VERSION.
void IpcProtocolTest::testOpcodeWireStability() {
	___INFOLOG( "" );

	static const char* expected[] = {
		"HELLO", "EVENT", "MIDI_NOTE_RECORDED", "REPLY",
		"RESCAN_SOUND_LIBRARY", "UPDATE_SOUND_LIBRARY",
		"PLAY", "STOP", "QUIT",
		"SET_BPM", "SET_MASTER_VOLUME", "SET_MASTER_IS_MUTED",
		"SET_METRONOME_IS_ACTIVE", "SET_METRONOME_VOLUME",
		"LOCATE_TO_COLUMN", "LOCATE_TO_TICK", "SELECT_PATTERN",
		"SET_STRIP_VOLUME", "SET_STRIP_PAN", "ACTIVATE_LOOP_MODE",
		"ACTIVATE_SONG_MODE", "ACTIVATE_RECORD_MODE", "ADD_TEMPO_MARKER",
		"ADD_TAG", "ADD_AUTOMATION_POINT", "REMOVE_AUTOMATION_POINT",
		"SET_SONG", "SET_DRUMKIT", "SET_PREFERENCES",
		"SET_INSTRUMENT_PITCH", "SET_INSTRUMENT_GAIN",
		"SET_INSTRUMENT_RANDOM_PITCH", "SET_INSTRUMENT_FILTER_CUTOFF",
		"SET_INSTRUMENT_FILTER_RESONANCE", "SET_INSTRUMENT_ATTACK",
		"SET_INSTRUMENT_DECAY", "SET_INSTRUMENT_SUSTAIN",
		"SET_INSTRUMENT_RELEASE", "SET_INSTRUMENT_FILTER_ACTIVE",
		"SET_INSTRUMENT_MUTE_GROUP", "SET_INSTRUMENT_STOP_NOTES",
		"SET_INSTRUMENT_APPLY_VELOCITY", "SET_INSTRUMENT_HIHAT_GROUP",
		"SET_INSTRUMENT_LOWER_CC", "SET_INSTRUMENT_HIGHER_CC",
		"SET_INSTRUMENT_MIDI_OUT_NOTE", "SET_INSTRUMENT_MIDI_OUT_CHANNEL",
		"SET_COMPONENT_IS_MUTED", "SET_COMPONENT_IS_SOLOED",
		"SET_COMPONENT_GAIN", "SET_COMPONENT_SELECTION",
		"SET_LAYER_IS_MUTED", "SET_LAYER_IS_SOLOED", "SET_LAYER_GAIN",
		"SET_LAYER_PITCH_OFFSET", "SET_LAYER_START_VELOCITY",
		"SET_LAYER_END_VELOCITY", "SET_STRIP_IS_MUTED",
		"SET_STRIP_IS_SOLOED", "SET_STRIP_PAN_SYM", "SET_HUMANIZE_TIME",
		"SET_HUMANIZE_VELOCITY", "SET_SWING", "SET_PAN_LAW",
		"SET_PLAYBACK_TRACK_MUTED", "SET_PLAYBACK_TRACK_VOLUME",
		"SET_DEFAULT_MIDI_OUT_NOTES", "HANDLE_MIDI_ACTION",
		"RECALCULATE_RUBBERBAND", "SET_RUBBERBAND_BATCH_MODE",
		"SET_PUNCH_AREA", "SET_MIDI_CONTROL_SETTINGS",
		"MOVE_AUTOMATION_POINT", "SET_VIRTUAL_PATTERNS",
		"SET_QUANTIZE_EVENTS", "SET_PATTERN_EDITOR_GRID",
		"PREVIEW_INSTRUMENT", "ACTIVATE_TIMELINE", "TOGGLE_TIMELINE",
		"DELETE_TEMPO_MARKER", "DELETE_TAG", "ACTIVATE_JACK_TRANSPORT",
		"TOGGLE_JACK_TRANSPORT", "ACTIVATE_JACK_TIMEBASE_CONTROL",
		"TOGGLE_JACK_TIMEBASE_CONTROL", "TOGGLE_SONG_MODE",
		"TOGGLE_LOOP_MODE", "MOVE_INSTRUMENT", "RENAME_COMPONENT",
		"TOGGLE_NEXT_PATTERN", "MOVE_PATTERN", "REMOVE_PATTERN",
		"SET_PATTERN_SIZE", "START_COUNT_IN", "ACTIVATE_PLAYLIST_SONG",
		"SET_MIDI_CLOCK_INPUT_HANDLING", "SET_MIDI_CLOCK_OUTPUT_SEND",
		"CLEAR_MIDI_INPUT_LOG", "CLEAR_MIDI_OUTPUT_LOG",
		"EDIT_NOTE_PROPERTY", "REMOVE_NOTE", "TOGGLE_GRID_CELL",
		"ADD_OR_REMOVE_NOTE", "HANDLE_NOTE", "SET_SONG_PROPERTIES",
		"SET_PATTERN_PROPERTIES", "SET_PATTERN", "REPLACE_INSTRUMENT",
		"ADD_INSTRUMENT", "REMOVE_INSTRUMENT", "SAVE_SONG",
		"SAVE_SONG_AS", "SAVE_PLAYLIST", "SAVE_PLAYLIST_AS",
		"SET_PLAYLIST", "ADD_TO_PLAYLIST", "REMOVE_FROM_PLAYLIST",
		"SET_MIDI_EVENT_MAP", "SET_MIDI_INSTRUMENT_MAP", "GET_SONG",
		"GET_PLAYLIST", "GET_SELECTED_PATTERN", "GET_SELECTED_INSTRUMENT",
		"GET_RECORD_ENABLED", "GET_CORE_PREFERENCES",
		"GET_SOUND_LIBRARY_INFO", "GET_AUDIO_DRIVER_INFO",
		"GET_MIDI_DRIVER_INFO", "GET_IS_UNDER_SESSION_MANAGEMENT",
		"GET_IS_UNDER_PLUGIN_HOST", "PANIC", "NOTE_ON",
		"RELEASE_PLAYING_NOTES", "SET_SELECTED_INSTRUMENT",
		"GET_MIDI_PORTS", "GET_HANDLED_MIDI_INPUTS",
		"GET_HANDLED_MIDI_OUTPUTS", "GET_AUDIO_HOST_APIS",
		"GET_AUDIO_DEVICES", "RECREATE_OSC_SERVER",
		"GET_OSC_TEMPORARY_PORT", "SET_SONG_MODIFIED",
		"GET_SESSION_FOLDER_PATH", "PREVIEW_INSTRUMENT_SERIALIZED",
		"HANDLE_BEAT_COUNTER", "TAP_TEMPO_ACCEL_EVENT",
		"UPDATE_BEAT_COUNTER_SETTINGS", "SET_IS_TIMELINE_ACTIVATED",
		"SET_PATTERN_MODE", "LOAD_PLAYBACK_TRACK",
		"SET_DRUMKIT_MODIFIED", "SET_PATTERN_MODIFIED",
		"SET_PLAYLIST_IS_MODIFIED", "SET_IS_PATTERN_EDITOR_LOCKED",
		"SET_LAST_MIDI_EVENT", "GET_LAST_MIDI_EVENT",
		"ADD_CUSTOM_SOUND_LIBRARY_DIR", "REMOVE_CUSTOM_SOUND_LIBRARY_DIR",
		"EXPORT_SONG", "STOP_EXPORT_SESSION", "GET_EXPORT_WRITING_FAILED"
	};
	const int nCount = sizeof( expected ) / sizeof( expected[0] );

	// The list must cover every opcode (and no more): a size mismatch means
	// the enum grew or shrank — extend/trim this list in the same commit.
	CPPUNIT_ASSERT_EQUAL( static_cast<int>( IpcOpcode::OpcodeCount ), nCount );

	for ( int i = 0; i < nCount; ++i ) {
		CPPUNIT_ASSERT_EQUAL( std::string( expected[i] ),
			IpcOpcodeToQString( static_cast<quint16>( i ) ).toStdString() );
	}

	// The QDataStream serialization version is as much part of the wire
	// format as the opcodes: bumping it changes every frame's encoding.
	CPPUNIT_ASSERT( IPC_DATASTREAM_VERSION == QDataStream::Qt_5_15 );

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testEventClassification() {
	___INFOLOG( "" );

	// Editor-internal: must NOT cross IPC.
	CPPUNIT_ASSERT( ! isEngineOriginEvent( Event::Type::OnlineImportProgress ) );

	// Engine-origin: forwarded engine → editor.
	CPPUNIT_ASSERT( isEngineOriginEvent( Event::Type::Metronome ) );
	CPPUNIT_ASSERT( isEngineOriginEvent( Event::Type::TempoChanged ) );
	CPPUNIT_ASSERT( isEngineOriginEvent( Event::Type::Relocation ) );
	CPPUNIT_ASSERT( isEngineOriginEvent( Event::Type::SongIsModified ) );
	CPPUNIT_ASSERT( isEngineOriginEvent( Event::Type::PlayingPatternsChanged ) );

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testTelemetryRoundTrip() {
	___INFOLOG( "" );

	EngineTelemetry block;
	telemetryInit( block );

	EngineTelemetrySnapshot in;
	in.frame = 123456789;
	in.bar = 5; in.beat = 3; in.tick = 42;
	in.bpm = 128.0f;
	in.playing = 1;
	in.masterPeakL = 0.5f; in.masterPeakR = 0.75f;
	in.procTimeCur = 1.2f; in.procTimeMax = 3.4f;
	in.instPeakCount = 3;
	in.peakL[0] = 0.1f; in.peakR[0] = 0.2f;
	in.peakL[2] = 0.9f; in.peakR[2] = 0.8f;

	telemetryStore( block, in );

	EngineTelemetrySnapshot out;
	CPPUNIT_ASSERT( telemetryLoad( block, out ) );
	CPPUNIT_ASSERT_EQUAL( in.frame, out.frame );
	CPPUNIT_ASSERT_EQUAL( in.bar, out.bar );
	CPPUNIT_ASSERT_EQUAL( in.beat, out.beat );
	CPPUNIT_ASSERT_EQUAL( in.tick, out.tick );
	CPPUNIT_ASSERT_EQUAL( in.bpm, out.bpm );
	CPPUNIT_ASSERT_EQUAL( in.instPeakCount, out.instPeakCount );
	CPPUNIT_ASSERT_EQUAL( in.peakL[2], out.peakL[2] );
	CPPUNIT_ASSERT_EQUAL( in.masterPeakR, out.masterPeakR );

	// A layout-version mismatch disables telemetry (load fails) rather than
	// returning garbage.
	block.formatVersion = ENGINE_TELEMETRY_VERSION + 1;
	CPPUNIT_ASSERT( ! telemetryLoad( block, out ) );

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testOpcodeToQString() {
	___INFOLOG( "" );

	// A representative spread across the opcode vocabulary.
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Hello ) )
					== QString( "HELLO" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Event ) )
					== QString( "EVENT" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Reply ) )
					== QString( "REPLY" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::RescanSoundLibrary ) )
					== QString( "RESCAN_SOUND_LIBRARY" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Play ) )
					== QString( "PLAY" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Stop ) )
					== QString( "STOP" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::Panic ) )
					== QString( "PANIC" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::GetAudioDriverInfo ) )
					== QString( "GET_AUDIO_DRIVER_INFO" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::GetMidiDriverInfo ) )
					== QString( "GET_MIDI_DRIVER_INFO" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>(
							IpcOpcode::GetIsUnderSessionManagement ) )
					== QString( "GET_IS_UNDER_SESSION_MANAGEMENT" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>(
							IpcOpcode::GetIsUnderPluginHost ) )
					== QString( "GET_IS_UNDER_PLUGIN_HOST" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::NoteOn ) )
					== QString( "NOTE_ON" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::SetBpm ) )
					== QString( "SET_BPM" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::SetStripPan ) )
					== QString( "SET_STRIP_PAN" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::EditNoteProperty ) )
					== QString( "EDIT_NOTE_PROPERTY" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::RemoveFromPlaylist ) )
					== QString( "REMOVE_FROM_PLAYLIST" ) );

	// The sentinel is not a real opcode.
	CPPUNIT_ASSERT( IpcOpcodeToQString( static_cast<quint16>( IpcOpcode::OpcodeCount ) )
					== QString( "Unknown IPC opcode" ) );

	// Out-of-range numbers are handled gracefully.
	CPPUNIT_ASSERT( IpcOpcodeToQString( 0xFFFF ) == QString( "Unknown IPC opcode" ) );
	CPPUNIT_ASSERT( IpcOpcodeToQString(
						static_cast<quint16>( IpcOpcode::OpcodeCount ) + 1 )
					== QString( "Unknown IPC opcode" ) );

	___INFOLOG( "passed" );
}

void IpcProtocolTest::testTelemetryTearFree() {
	___INFOLOG( "" );

	EngineTelemetry block;
	telemetryInit( block );

	std::atomic<bool> bDone{ false };
	std::atomic<bool> bTorn{ false };
	const int64_t nIterations = 200000;

	// Writer: stamp one monotonically increasing generation into several fields
	// at once, mimicking the audio thread publishing every buffer.
	std::thread writer( [&]() {
		for ( int64_t g = 1; g <= nIterations; ++g ) {
			EngineTelemetrySnapshot s;
			s.frame = g;
			s.bar = static_cast<int32_t>( g & 0x7fffffff );
			s.bpm = static_cast<float>( g % 100000 );
			s.peakL[0] = static_cast<float>( g % 100000 );
			telemetryStore( block, s );
		}
		bDone.store( true );
	} );

	// Reader: every load must be internally consistent (all fields from the same
	// generation) - never a torn mix of two writes.
	while ( ! bDone.load() ) {
		EngineTelemetrySnapshot s;
		if ( telemetryLoad( block, s ) ) {
			const int64_t expectedMod = s.frame % 100000;
			if ( s.bar != static_cast<int32_t>( s.frame & 0x7fffffff ) ||
				 s.bpm != static_cast<float>( expectedMod ) ||
				 s.peakL[0] != static_cast<float>( expectedMod ) ) {
				bTorn.store( true );
			}
		}
	}

	writer.join();
	CPPUNIT_ASSERT( ! bTorn.load() );

	___INFOLOG( "passed" );
}
