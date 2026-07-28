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
#include <core/IPC/PluginTelemetry.h>
#include <core/Object.h>

#include <atomic>
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
		T::SongSizeChanged, T::SoundLibraryChanged, T::StackedModeActivation,
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

	IpcMessage decoded;
	CPPUNIT_ASSERT( roundTrip( IpcMessage::hello(), decoded ) );
	CPPUNIT_ASSERT( decoded.getOpcode() == IpcOpcode::Hello );
	CPPUNIT_ASSERT_EQUAL( IPC_PROTOCOL_VERSION,
						  decoded.helloProtocolVersion() );

	// A peer announcing a different protocol version is detectable (mismatch
	// → graceful refusal in the transport layer).
	IpcMessage other;
	CPPUNIT_ASSERT( roundTrip( IpcMessage::hello( IPC_PROTOCOL_VERSION + 1 ),
							   other ) );
	CPPUNIT_ASSERT( other.helloProtocolVersion() != IPC_PROTOCOL_VERSION );

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

	PluginTelemetry block;
	telemetryInit( block );

	PluginTelemetrySnapshot in;
	in.frame = 123456789;
	in.bar = 5; in.beat = 3; in.tick = 42;
	in.bpm = 128.0f;
	in.playing = 1; in.looping = 1;
	in.masterPeakL = 0.5f; in.masterPeakR = 0.75f;
	in.procTimeCur = 1.2f; in.procTimeMax = 3.4f;
	in.instPeakCount = 3;
	in.peakL[0] = 0.1f; in.peakR[0] = 0.2f;
	in.peakL[2] = 0.9f; in.peakR[2] = 0.8f;

	telemetryStore( block, in );

	PluginTelemetrySnapshot out;
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
	block.formatVersion = PLUGIN_TELEMETRY_VERSION + 1;
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

	PluginTelemetry block;
	telemetryInit( block );

	std::atomic<bool> bDone{ false };
	std::atomic<bool> bTorn{ false };
	const int64_t nIterations = 200000;

	// Writer: stamp one monotonically increasing generation into several fields
	// at once, mimicking the audio thread publishing every buffer.
	std::thread writer( [&]() {
		for ( int64_t g = 1; g <= nIterations; ++g ) {
			PluginTelemetrySnapshot s;
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
		PluginTelemetrySnapshot s;
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
