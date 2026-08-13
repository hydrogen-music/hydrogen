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

#include <core/IPC/IpcMessage.h>

namespace H2Core {

QByteArray IpcMessage::encode() const {
	// Body: opcode + args + payload.
	QByteArray body;
	{
		QDataStream ds( &body, QIODevice::WriteOnly );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds << static_cast<quint16>( m_opcode );
		ds << m_requestId;
		ds << m_args;
		ds << m_payload;
	}

	// Frame: [u32 bodyLength][body].
	QByteArray frame;
	{
		QDataStream ds( &frame, QIODevice::WriteOnly );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds << static_cast<quint32>( body.size() );
	}
	frame.append( body );
	return frame;
}

bool IpcMessage::decodeBody( const QByteArray& body, IpcMessage& out ) {
	QDataStream ds( body );
	ds.setVersion( IPC_DATASTREAM_VERSION );

	quint16 nOpcode = 0;
	quint32 nRequestId = 0;
	QVector<QVariant> args;
	QByteArray payload;
	ds >> nOpcode >> nRequestId >> args >> payload;
	if ( ds.status() != QDataStream::Ok ) {
		return false;
	}
	if ( nOpcode >= static_cast<quint16>( IpcOpcode::OpcodeCount ) ) {
		return false;
	}

	out.m_opcode = static_cast<IpcOpcode>( nOpcode );
	out.m_requestId = nRequestId;
	out.m_args = args;
	out.m_payload = payload;
	return true;
}

IpcMessage IpcMessage::fromEvent( Event::Type type, int nValue, long nId ) {
	IpcMessage msg( IpcOpcode::Event );
	msg.arg( static_cast<int>( type ) )
		.arg( nValue )
		.arg( static_cast<qlonglong>( nId ) );
	return msg;
}

bool IpcMessage::toEventFields( Event::Type& type, int& nValue, long& nId ) const {
	if ( m_opcode != IpcOpcode::Event || m_args.size() < 3 ) {
		return false;
	}
	type = static_cast<Event::Type>( m_args[0].toInt() );
	nValue = m_args[1].toInt();
	nId = static_cast<long>( m_args[2].toLongLong() );
	return true;
}

IpcMessage IpcMessage::hello( quint16 nProtocolVersion ) {
	IpcMessage msg( IpcOpcode::Hello );
	msg.arg( static_cast<int>( nProtocolVersion ) );
	return msg;
}

quint16 IpcMessage::helloProtocolVersion() const {
	if ( m_opcode != IpcOpcode::Hello || m_args.isEmpty() ) {
		return 0;
	}
	return static_cast<quint16>( m_args[0].toInt() );
}

bool IpcFrameReader::next( IpcMessage& out ) {
	// Need the 4-byte length prefix first.
	if ( m_buffer.size() < static_cast<int>( sizeof( quint32 ) ) ) {
		return false;
	}

	quint32 nBodyLength = 0;
	{
		QDataStream ds( m_buffer );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds >> nBodyLength;
	}

	const int nFrameSize =
		static_cast<int>( sizeof( quint32 ) ) + static_cast<int>( nBodyLength );
	if ( m_buffer.size() < nFrameSize ) {
		return false; // frame not fully received yet
	}

	const QByteArray body =
		m_buffer.mid( sizeof( quint32 ), static_cast<int>( nBodyLength ) );
	m_buffer.remove( 0, nFrameSize );

	return IpcMessage::decodeBody( body, out );
}

bool isEngineOriginEvent( Event::Type type ) {
	// Editor-internal events originate and are consumed inside the editor
	// process and must not cross IPC (ADR 0016/0018). OnlineImportProgress is
	// the worked example: online library import is editor-side (OnlineImporter),
	// so its progress events stay local. Everything else is engine-origin.
	switch ( type ) {
	case Event::Type::OnlineImportProgress:
		return false;
	default:
		return true;
	}
}

QString IpcOpcodeToQString( quint16 nOpcode ) {
	if ( nOpcode >= static_cast<quint16>( IpcOpcode::OpcodeCount ) ) {
		return QString( "Unknown IPC opcode" );
	}

	QString sOpcode;
	switch ( static_cast<IpcOpcode>( nOpcode ) ) {
	case IpcOpcode::Hello:
		sOpcode = "HELLO";
		break;
	case IpcOpcode::Event:
		sOpcode = "EVENT";
		break;
	case IpcOpcode::Reply:
		sOpcode = "REPLY";
		break;
	case IpcOpcode::RescanSoundLibrary:
		sOpcode = "RESCAN_SOUND_LIBRARY";
		break;
	case IpcOpcode::Play:
		sOpcode = "PLAY";
		break;
	case IpcOpcode::Stop:
		sOpcode = "STOP";
		break;
	case IpcOpcode::Quit:
		sOpcode = "QUIT";
		break;
	case IpcOpcode::SetBpm:
		sOpcode = "SET_BPM";
		break;
	case IpcOpcode::SetMasterVolume:
		sOpcode = "SET_MASTER_VOLUME";
		break;
	case IpcOpcode::SetMasterIsMuted:
		sOpcode = "SET_MASTER_IS_MUTED";
		break;
	case IpcOpcode::SetMetronomeIsActive:
		sOpcode = "SET_METRONOME_IS_ACTIVE";
		break;
	case IpcOpcode::LocateToColumn:
		sOpcode = "LOCATE_TO_COLUMN";
		break;
	case IpcOpcode::LocateToTick:
		sOpcode = "LOCATE_TO_TICK";
		break;
	case IpcOpcode::SelectPattern:
		sOpcode = "SELECT_PATTERN";
		break;
	case IpcOpcode::SetStripVolume:
		sOpcode = "SET_STRIP_VOLUME";
		break;
	case IpcOpcode::SetStripPan:
		sOpcode = "SET_STRIP_PAN";
		break;
	case IpcOpcode::ActivateLoopMode:
		sOpcode = "ACTIVATE_LOOP_MODE";
		break;
	case IpcOpcode::ActivateSongMode:
		sOpcode = "ACTIVATE_SONG_MODE";
		break;
	case IpcOpcode::ActivateRecordMode:
		sOpcode = "ACTIVATE_RECORD_MODE";
		break;
	case IpcOpcode::AddTempoMarker:
		sOpcode = "ADD_TEMPO_MARKER";
		break;
	case IpcOpcode::AddTag:
		sOpcode = "ADD_TAG";
		break;
	case IpcOpcode::AddAutomationPoint:
		sOpcode = "ADD_AUTOMATION_POINT";
		break;
	case IpcOpcode::RemoveAutomationPoint:
		sOpcode = "REMOVE_AUTOMATION_POINT";
		break;
	case IpcOpcode::SetSong:
		sOpcode = "SET_SONG";
		break;
	case IpcOpcode::SetDrumkit:
		sOpcode = "SET_DRUMKIT";
		break;
	case IpcOpcode::LoadState:
		sOpcode = "LOAD_STATE";
		break;
	case IpcOpcode::SetPreferences:
		sOpcode = "SET_PREFERENCES";
		break;
	case IpcOpcode::SetInstrumentPitch:
		sOpcode = "SET_INSTRUMENT_PITCH";
		break;
	case IpcOpcode::SetInstrumentGain:
		sOpcode = "SET_INSTRUMENT_GAIN";
		break;
	case IpcOpcode::SetInstrumentRandomPitch:
		sOpcode = "SET_INSTRUMENT_RANDOM_PITCH";
		break;
	case IpcOpcode::SetInstrumentFilterCutoff:
		sOpcode = "SET_INSTRUMENT_FILTER_CUTOFF";
		break;
	case IpcOpcode::SetInstrumentFilterResonance:
		sOpcode = "SET_INSTRUMENT_FILTER_RESONANCE";
		break;
	case IpcOpcode::SetInstrumentAttack:
		sOpcode = "SET_INSTRUMENT_ATTACK";
		break;
	case IpcOpcode::SetInstrumentDecay:
		sOpcode = "SET_INSTRUMENT_DECAY";
		break;
	case IpcOpcode::SetInstrumentSustain:
		sOpcode = "SET_INSTRUMENT_SUSTAIN";
		break;
	case IpcOpcode::SetInstrumentRelease:
		sOpcode = "SET_INSTRUMENT_RELEASE";
		break;
	case IpcOpcode::SetInstrumentFilterActive:
		sOpcode = "SET_INSTRUMENT_FILTER_ACTIVE";
		break;
	case IpcOpcode::SetInstrumentMuteGroup:
		sOpcode = "SET_INSTRUMENT_MUTE_GROUP";
		break;
	case IpcOpcode::SetInstrumentStopNotes:
		sOpcode = "SET_INSTRUMENT_STOP_NOTES";
		break;
	case IpcOpcode::SetInstrumentApplyVelocity:
		sOpcode = "SET_INSTRUMENT_APPLY_VELOCITY";
		break;
	case IpcOpcode::SetInstrumentHihatGroup:
		sOpcode = "SET_INSTRUMENT_HIHAT_GROUP";
		break;
	case IpcOpcode::SetInstrumentLowerCc:
		sOpcode = "SET_INSTRUMENT_LOWER_CC";
		break;
	case IpcOpcode::SetInstrumentHigherCc:
		sOpcode = "SET_INSTRUMENT_HIGHER_CC";
		break;
	case IpcOpcode::SetInstrumentMidiOutNote:
		sOpcode = "SET_INSTRUMENT_MIDI_OUT_NOTE";
		break;
	case IpcOpcode::SetInstrumentMidiOutChannel:
		sOpcode = "SET_INSTRUMENT_MIDI_OUT_CHANNEL";
		break;
	case IpcOpcode::SetComponentIsMuted:
		sOpcode = "SET_COMPONENT_IS_MUTED";
		break;
	case IpcOpcode::SetComponentIsSoloed:
		sOpcode = "SET_COMPONENT_IS_SOLOED";
		break;
	case IpcOpcode::SetComponentGain:
		sOpcode = "SET_COMPONENT_GAIN";
		break;
	case IpcOpcode::SetComponentSelection:
		sOpcode = "SET_COMPONENT_SELECTION";
		break;
	case IpcOpcode::SetLayerIsMuted:
		sOpcode = "SET_LAYER_IS_MUTED";
		break;
	case IpcOpcode::SetLayerIsSoloed:
		sOpcode = "SET_LAYER_IS_SOLOED";
		break;
	case IpcOpcode::SetLayerGain:
		sOpcode = "SET_LAYER_GAIN";
		break;
	case IpcOpcode::SetLayerPitchOffset:
		sOpcode = "SET_LAYER_PITCH_OFFSET";
		break;
	case IpcOpcode::SetLayerStartVelocity:
		sOpcode = "SET_LAYER_START_VELOCITY";
		break;
	case IpcOpcode::SetLayerEndVelocity:
		sOpcode = "SET_LAYER_END_VELOCITY";
		break;
	case IpcOpcode::SetStripIsMuted:
		sOpcode = "SET_STRIP_IS_MUTED";
		break;
	case IpcOpcode::SetStripIsSoloed:
		sOpcode = "SET_STRIP_IS_SOLOED";
		break;
	case IpcOpcode::SetStripPanSym:
		sOpcode = "SET_STRIP_PAN_SYM";
		break;
	case IpcOpcode::SetHumanizeTime:
		sOpcode = "SET_HUMANIZE_TIME";
		break;
	case IpcOpcode::SetHumanizeVelocity:
		sOpcode = "SET_HUMANIZE_VELOCITY";
		break;
	case IpcOpcode::SetSwing:
		sOpcode = "SET_SWING";
		break;
	case IpcOpcode::SetPanLaw:
		sOpcode = "SET_PAN_LAW";
		break;
	case IpcOpcode::SetPlaybackTrackMuted:
		sOpcode = "SET_PLAYBACK_TRACK_MUTED";
		break;
	case IpcOpcode::SetPlaybackTrackVolume:
		sOpcode = "SET_PLAYBACK_TRACK_VOLUME";
		break;
	case IpcOpcode::PreviewInstrument:
		sOpcode = "PREVIEW_INSTRUMENT";
		break;
	case IpcOpcode::ActivateTimeline:
		sOpcode = "ACTIVATE_TIMELINE";
		break;
	case IpcOpcode::ToggleTimeline:
		sOpcode = "TOGGLE_TIMELINE";
		break;
	case IpcOpcode::DeleteTempoMarker:
		sOpcode = "DELETE_TEMPO_MARKER";
		break;
	case IpcOpcode::DeleteTag:
		sOpcode = "DELETE_TAG";
		break;
	case IpcOpcode::ActivateJackTransport:
		sOpcode = "ACTIVATE_JACK_TRANSPORT";
		break;
	case IpcOpcode::ToggleJackTransport:
		sOpcode = "TOGGLE_JACK_TRANSPORT";
		break;
	case IpcOpcode::ActivateJackTimebaseControl:
		sOpcode = "ACTIVATE_JACK_TIMEBASE_CONTROL";
		break;
	case IpcOpcode::ToggleJackTimebaseControl:
		sOpcode = "TOGGLE_JACK_TIMEBASE_CONTROL";
		break;
	case IpcOpcode::ToggleSongMode:
		sOpcode = "TOGGLE_SONG_MODE";
		break;
	case IpcOpcode::ToggleLoopMode:
		sOpcode = "TOGGLE_LOOP_MODE";
		break;
	case IpcOpcode::MoveInstrument:
		sOpcode = "MOVE_INSTRUMENT";
		break;
	case IpcOpcode::RenameComponent:
		sOpcode = "RENAME_COMPONENT";
		break;
	case IpcOpcode::ToggleNextPattern:
		sOpcode = "TOGGLE_NEXT_PATTERN";
		break;
	case IpcOpcode::MovePattern:
		sOpcode = "MOVE_PATTERN";
		break;
	case IpcOpcode::RemovePattern:
		sOpcode = "REMOVE_PATTERN";
		break;
	case IpcOpcode::SetPatternSize:
		sOpcode = "SET_PATTERN_SIZE";
		break;
	case IpcOpcode::StartCountIn:
		sOpcode = "START_COUNT_IN";
		break;
	case IpcOpcode::ActivatePlaylistSong:
		sOpcode = "ACTIVATE_PLAYLIST_SONG";
		break;
	case IpcOpcode::SetMidiClockInputHandling:
		sOpcode = "SET_MIDI_CLOCK_INPUT_HANDLING";
		break;
	case IpcOpcode::SetMidiClockOutputSend:
		sOpcode = "SET_MIDI_CLOCK_OUTPUT_SEND";
		break;
	case IpcOpcode::ClearMidiInputLog:
		sOpcode = "CLEAR_MIDI_INPUT_LOG";
		break;
	case IpcOpcode::ClearMidiOutputLog:
		sOpcode = "CLEAR_MIDI_OUTPUT_LOG";
		break;
	case IpcOpcode::EditNoteProperty:
		sOpcode = "EDIT_NOTE_PROPERTY";
		break;
	case IpcOpcode::RemoveNote:
		sOpcode = "REMOVE_NOTE";
		break;
	case IpcOpcode::ToggleGridCell:
		sOpcode = "TOGGLE_GRID_CELL";
		break;
	case IpcOpcode::AddOrRemoveNote:
		sOpcode = "ADD_OR_REMOVE_NOTE";
		break;
	case IpcOpcode::HandleNote:
		sOpcode = "HANDLE_NOTE";
		break;
	case IpcOpcode::SetSongProperties:
		sOpcode = "SET_SONG_PROPERTIES";
		break;
	case IpcOpcode::SetPatternProperties:
		sOpcode = "SET_PATTERN_PROPERTIES";
		break;
	case IpcOpcode::SetPattern:
		sOpcode = "SET_PATTERN";
		break;
	case IpcOpcode::ReplaceInstrument:
		sOpcode = "REPLACE_INSTRUMENT";
		break;
	case IpcOpcode::AddInstrument:
		sOpcode = "ADD_INSTRUMENT";
		break;
	case IpcOpcode::RemoveInstrument:
		sOpcode = "REMOVE_INSTRUMENT";
		break;
	case IpcOpcode::SaveSong:
		sOpcode = "SAVE_SONG";
		break;
	case IpcOpcode::SaveSongAs:
		sOpcode = "SAVE_SONG_AS";
		break;
	case IpcOpcode::SavePlaylist:
		sOpcode = "SAVE_PLAYLIST";
		break;
	case IpcOpcode::SavePlaylistAs:
		sOpcode = "SAVE_PLAYLIST_AS";
		break;
	case IpcOpcode::SetPlaylist:
		sOpcode = "SET_PLAYLIST";
		break;
	case IpcOpcode::AddToPlaylist:
		sOpcode = "ADD_TO_PLAYLIST";
		break;
	case IpcOpcode::RemoveFromPlaylist:
		sOpcode = "REMOVE_FROM_PLAYLIST";
		break;
	case IpcOpcode::GetSong:
		sOpcode = "GET_SONG";
		break;
	case IpcOpcode::GetPlaylist:
		sOpcode = "GET_PLAYLIST";
		break;
	case IpcOpcode::GetSelectedPattern:
		sOpcode = "GET_SELECTED_PATTERN";
		break;
	case IpcOpcode::GetSelectedInstrument:
		sOpcode = "GET_SELECTED_INSTRUMENT";
		break;
	case IpcOpcode::GetRecordEnabled:
		sOpcode = "GET_RECORD_ENABLED";
		break;
	case IpcOpcode::GetCorePreferences:
		sOpcode = "GET_CORE_PREFERENCES";
		break;
	case IpcOpcode::GetSoundLibraryInfo:
		sOpcode = "GET_SOUND_LIBRARY_INFO";
		break;
	case IpcOpcode::GetAudioDriverInfo:
		sOpcode = "GET_AUDIO_DRIVER_INFO";
		break;
	case IpcOpcode::GetMidiDriverInfo:
		sOpcode = "GET_MIDI_DRIVER_INFO";
		break;
	case IpcOpcode::GetIsUnderSessionManagement:
		sOpcode = "GET_IS_UNDER_SESSION_MANAGEMENT";
		break;
	case IpcOpcode::GetIsUnderPluginHost:
		sOpcode = "GET_IS_UNDER_PLUGIN_HOST";
		break;
	case IpcOpcode::Panic:
		sOpcode = "PANIC";
		break;
	case IpcOpcode::NoteOn:
		sOpcode = "NOTE_ON";
		break;
	case IpcOpcode::ReleasePlayingNotes:
		sOpcode = "RELEASE_PLAYING_NOTES";
		break;
	case IpcOpcode::OpcodeCount:
	default:
		sOpcode = "Unknown IPC opcode";
	}

	return std::move( sOpcode );
}

QString IpcMessage::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
	sOutput = QString( "%1[IpcMessage]\n" )
				  .arg( sPrefix )
				  .append( QString( "%1%2m_opcode: %3\n" )
							   .arg( sPrefix )
							   .arg( s )
							   .arg( IpcOpcodeToQString(
								   static_cast<quint16>( m_opcode )
							   ) ) )
				  .append( QString( "%1%2m_requestId: %3\n" )
							   .arg( sPrefix )
							   .arg( s )
							   .arg( m_requestId ) )
				  .append( QString( "%1%2m_payload size: %3\n" )
							   .arg( sPrefix )
							   .arg( s )
							   .arg( m_payload.size() ) );
	}
	else {
	sOutput =
		QString( "[IpcMessage]" )
			.append(
				QString( " m_opcode: %1" )
					.arg( IpcOpcodeToQString( static_cast<quint16>( m_opcode ) )
					)
			)
			.append( QString( ", m_requestId: %1" ).arg( m_requestId ) )
			.append( QString( ", m_payload size: %1" ).arg( m_payload.size() )
			);
	}

	return sOutput;
}

};	// namespace H2Core
