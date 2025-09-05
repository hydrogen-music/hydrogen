/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "MidiAction.h"

using namespace H2Core;

QString MidiAction::typeToQString( const Type &type ) {
	switch( type ) {
	case Type::BeatCounter:
		return "BEATCOUNTER";
	case Type::BpmCcRelative:
		return "BPM_CC_RELATIVE";
	case Type::BpmDecr:
		return "BPM_DECR";
	case Type::BpmFineCcRelative:
		return "BPM_FINE_CC_RELATIVE";
	case Type::BpmIncr:
		return "BPM_INCR";
	case Type::ClearPattern:
		return "CLEAR_PATTERN";
	case Type::ClearSelectedInstrument:
		return "CLEAR_SELECTED_INSTRUMENT";
	case Type::EffectLevelAbsolute:
		return "EFFECT_LEVEL_ABSOLUTE";
	case Type::EffectLevelRelative:
		return "EFFECT_LEVEL_RELATIVE";
	case Type::FilterCutoffLevelAbsolute:
		return "FILTER_CUTOFF_LEVEL_ABSOLUTE";
	case Type::GainLevelAbsolute:
		return "GAIN_LEVEL_ABSOLUTE";
	case Type::InstrumentPitch:
		return "INSTRUMENT_PITCH";
	case Type::LoadNextDrumkit:
		return "LOAD_NEXT_DRUMKIT";
	case Type::LoadPrevDrumkit:
		return "LOAD_PREV_DRUMKIT";
	case Type::MasterVolumeAbsolute:
		return "MASTER_VOLUME_ABSOLUTE";
	case Type::MasterVolumeRelative:
		return "MASTER_VOLUME_RELATIVE";
	case Type::Mute:
		return "MUTE";
	case Type::MuteToggle:
		return "MUTE_TOGGLE";
	case Type::NextBar:
		return ">>_NEXT_BAR";
	case Type::Null:
		return "NOTHING";
	case Type::PanAbsolute:
		return "PAN_ABSOLUTE";
	case Type::PanAbsoluteSym:
		return "PAN_ABSOLUTE_SYM";
	case Type::PanRelative:
		return "PAN_RELATIVE";
	case Type::Pause:
		return "PAUSE";
	case Type::PitchLevelAbsolute:
		return "PITCH_LEVEL_ABSOLUTE";
	case Type::Play:
		return "PLAY";
	case Type::PlaylistSong:
		return "PLAYLIST_SONG";
	case Type::PlaylistNextSong:
		return "PLAYLIST_NEXT_SONG";
	case Type::PlaylistPrevSong:
		return "PLAYLIST_PREV_SONG";
	case Type::PlayPauseToggle:
		return "PLAY/PAUSE_TOGGLE";
	case Type::PlayStopToggle:
		return "PLAY/STOP_TOGGLE";
	case Type::PreviousBar:
		return "<<_PREVIOUS_BAR";
	case Type::RecordExit:
		return "RECORD_EXIT";
	case Type::RecordReady:
		return "RECORD_READY";
	case Type::RecordStrobe:
		return "RECORD_STROBE";
	case Type::RecordStrobeToggle:
		return "RECORD/STROBE_TOGGLE";
	case Type::RedoAction:
		return "REDO_ACTION";
	case Type::SelectAndPlayPattern:
		return "SELECT_AND_PLAY_PATTERN";
	case Type::SelectInstrument:
		return "SELECT_INSTRUMENT";
	case Type::SelectNextPattern:
		return "SELECT_NEXT_PATTERN";
	case Type::SelectNextPatternCcAbsolute:
		return "SELECT_NEXT_PATTERN_CC_ABSOLUTE";
	case Type::SelectNextPatternRelative:
		return "SELECT_NEXT_PATTERN_RELATIVE";
	case Type::SelectOnlyNextPattern:
		return "SELECT_ONLY_NEXT_PATTERN";
	case Type::SelectOnlyNextPatternCcAbsolute:
		return "SELECT_ONLY_NEXT_PATTERN_CC_ABSOLUTE";
	case Type::Stop:
		return "STOP";
	case Type::StripMuteToggle:
		return "STRIP_MUTE_TOGGLE";
	case Type::StripSoloToggle:
		return "STRIP_SOLO_TOGGLE";
	case Type::StripVolumeAbsolute:
		return "STRIP_VOLUME_ABSOLUTE";
	case Type::StripVolumeRelative:
		return "STRIP_VOLUME_RELATIVE";
	case Type::TapTempo:
		return "TAP_TEMPO";
	case Type::TimingClockTick:
		return "TIMING_CLOCK_TICK";
	case Type::ToggleMetronome:
		return "TOGGLE_METRONOME";
	case Type::UndoAction:
		return "UNDO_ACTION";
	case Type::Unmute:
		return "UNMUTE";
	default:
		return "Unknown type";
	}
}

MidiAction::Type MidiAction::parseType( const QString &sType ) {
	const QString s = QString( sType ).toLower();
	if ( s == "beatcounter" ) {
		return Type::BeatCounter;
	}
	else if ( s == "bpm_cc_relative" ) {
		return Type::BpmCcRelative;
	}
	else if ( s == "bpm_decr" ) {
		return Type::BpmDecr;
	}
	else if ( s == "bpm_fine_cc_relative" ) {
		return Type::BpmFineCcRelative;
	}
	else if ( s == "bpm_incr" ) {
		return Type::BpmIncr;
	}
	else if ( s == "clear_pattern" ) {
		return Type::ClearPattern;
	}
	else if ( s == "clear_selected_instrument" ) {
		return Type::ClearSelectedInstrument;
	}
	else if ( s == "effect_level_absolute" ) {
		return Type::EffectLevelAbsolute;
	}
	else if ( s == "effect_level_relative" ) {
		return Type::EffectLevelRelative;
	}
	else if ( s == "filter_cutoff_level_absolute" ) {
		return Type::FilterCutoffLevelAbsolute;
	}
	else if ( s == "gain_level_absolute" ) {
		return Type::GainLevelAbsolute;
	}
	else if ( s == "instrument_pitch" ) {
		return Type::InstrumentPitch;
	}
	else if ( s == "load_next_drumkit" ) {
		return Type::LoadNextDrumkit;
	}
	else if ( s == "load_prev_drumkit" ) {
		return Type::LoadPrevDrumkit;
	}
	else if ( s == "master_volume_absolute" ) {
		return Type::MasterVolumeAbsolute;
	}
	else if ( s == "master_volume_relative" ) {
		return Type::MasterVolumeRelative;
	}
	else if ( s == "mute" ) {
		return Type::Mute;
	}
	else if ( s == "mute_toggle" ) {
		return Type::MuteToggle;
	}
	else if ( s == ">>_next_bar" ) {
		return Type::NextBar;
	}
	else if ( s == "nothing" ) {
		return Type::Null;
	}
	else if ( s == "pan_absolute" ) {
		return Type::PanAbsolute;
	}
	else if ( s == "pan_absolute_sym" ) {
		return Type::PanAbsoluteSym;
	}
	else if ( s == "pan_relative" ) {
		return Type::PanRelative;
	}
	else if ( s == "pause" ) {
		return Type::Pause;
	}
	else if ( s == "pitch_level_absolute" ) {
		return Type::PitchLevelAbsolute;
	}
	else if ( s == "play" ) {
		return Type::Play;
	}
	else if ( s == "playlist_song" ) {
		return Type::PlaylistSong;
	}
	else if ( s == "playlist_next_song" ) {
		return Type::PlaylistNextSong;
	}
	else if ( s == "playlist_prev_song" ) {
		return Type::PlaylistPrevSong;
	}
	else if ( s == "play/pause_toggle" ) {
		return Type::PlayPauseToggle;
	}
	else if ( s == "play/stop_toggle" ) {
		return Type::PlayStopToggle;
	}
	else if ( s == "<<_previous_bar" ) {
		return Type::PreviousBar;
	}
	else if ( s == "record_exit" ) {
		return Type::RecordExit;
	}
	else if ( s == "record_ready" ) {
		return Type::RecordReady;
	}
	else if ( s == "record_strobe" ) {
		return Type::RecordStrobe;
	}
	else if ( s == "record/strobe_toggle" ) {
		return Type::RecordStrobeToggle;
	}
	else if ( s == "redo_action" ) {
		return Type::RedoAction;
	}
	else if ( s == "select_and_play_pattern" ) {
		return Type::SelectAndPlayPattern;
	}
	else if ( s == "select_instrument" ) {
		return Type::SelectInstrument;
	}
	else if ( s == "select_next_pattern" ) {
		return Type::SelectNextPattern;
	}
	else if ( s == "select_next_pattern_cc_absolute" ) {
		return Type::SelectNextPatternCcAbsolute;
	}
	else if ( s == "select_next_pattern_relative" ) {
		return Type::SelectNextPatternRelative;
	}
	else if ( s == "select_only_next_pattern" ) {
		return Type::SelectOnlyNextPattern;
	}
	else if ( s == "select_only_next_pattern_cc_absolute" ) {
		return Type::SelectOnlyNextPatternCcAbsolute;
	}
	else if ( s == "stop" ) {
		return Type::Stop;
	}
	else if ( s == "strip_mute_toggle" ) {
		return Type::StripMuteToggle;
	}
	else if ( s == "strip_solo_toggle" ) {
		return Type::StripSoloToggle;
	}
	else if ( s == "strip_volume_absolute" ) {
		return Type::StripVolumeAbsolute;
	}
	else if ( s == "strip_volume_relative" ) {
		return Type::StripVolumeRelative;
	}
	else if ( s == "tap_tempo" ) {
		return Type::TapTempo;
	}
	else if ( s == "timing_clock_tick" ) {
		return Type::TimingClockTick;
	}
	else if ( s == "toggle_metronome" ) {
		return Type::ToggleMetronome;
	}
	else if ( s == "undo_action" ) {
		return Type::UndoAction;
	}
	else if ( s == "unmute" ) {
		return Type::Unmute;
	}
	else {
		return Type::Null;
	}
}

MidiAction::MidiAction( Type type ) : m_type( type )
									, m_sParameter1( "0" )
									, m_sParameter2( "0" )
									, m_sParameter3( "0" )
									, m_sValue( "0" )
									, m_timePoint( Clock::now() )
{
}

MidiAction::MidiAction( const std::shared_ptr<MidiAction> pOther ) {
	if ( pOther != nullptr ) {
       m_type = pOther->m_type;
       m_sParameter1 = pOther->m_sParameter1;
       m_sParameter2 = pOther->m_sParameter2;
       m_sParameter3 = pOther->m_sParameter3;
       m_sValue = pOther->m_sValue;
	   m_timePoint = pOther->m_timePoint;
	}
}

std::shared_ptr<MidiAction> MidiAction::from( const std::shared_ptr<MidiAction> pOther ) {
	if ( pOther == nullptr ) {
		return nullptr;
	}

	auto pNew = std::make_shared<MidiAction>( pOther );
	pNew->m_timePoint = Clock::now();

	return pNew;
}

bool MidiAction::isNull() const {
	return m_type == Type::Null;
}

bool MidiAction::isEquivalentTo( const std::shared_ptr<MidiAction> pOther ) const {
	if ( pOther == nullptr ) {
		return false;
	}
	
	return ( m_type == pOther->m_type &&
			 m_sParameter1 == pOther->m_sParameter1 &&
			 m_sParameter2 == pOther->m_sParameter2 &&
			 m_sParameter3 == pOther->m_sParameter3 );
}

QString MidiAction::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;

	if ( ! bShort ) {
		sOutput = QString( "%1[MidiAction]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_type: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( MidiAction::typeToQString( m_type ) ) )
			.append( QString( "%1%2m_sValue: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sValue ) )
			.append( QString( "%1%2m_sParameter1: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter1 ) )
			.append( QString( "%1%2m_sParameter2: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter2 ) )
			.append( QString( "%1%2m_sParameter3: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sParameter3 ) )
			.append( QString( "%1%2m_timePoint: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( H2Core::timePointToQString( m_timePoint ) ) );
	}
	else {
		sOutput = QString( "[MidiAction]" )
			.append( QString( " m_type: %1" )
					 .arg( MidiAction::typeToQString( m_type ) ) )
			.append( QString( ", m_sValue: %1" ).arg( m_sValue ) )
			.append( QString( ", m_sParameter1: %1" ).arg( m_sParameter1 ) )
			.append( QString( ", m_sParameter2: %1" ).arg( m_sParameter2 ) )
			.append( QString( ", m_sParameter3: %1" ).arg( m_sParameter3 ) )
			.append( QString( ", m_timePoint: %1" )
					 .arg( H2Core::timePointToQString( m_timePoint ) ) );
	}
	
	return sOutput;
}
