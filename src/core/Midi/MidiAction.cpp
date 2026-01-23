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

#include <cassert>

using namespace H2Core;

QString MidiAction::typeToQString( const Type& type )
{
	switch ( type ) {
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
		case Type::CountIn:
			return "COUNT_IN";
		case Type::CountInPauseToggle:
			return "COUNT_IN_PAUSE_TOGGLE";
		case Type::CountInStopToggle:
			return "COUNT_IN_STOP_TOGGLE";
		case Type::EffectLevelAbsolute:
			return "EFFECT_LEVEL_ABSOLUTE";
		case Type::EffectLevelRelative:
			return "EFFECT_LEVEL_RELATIVE";
		case Type::FilterCutoffLevelAbsolute:
			return "FILTER_CUTOFF_LEVEL_ABSOLUTE";
		case Type::GainLevelAbsolute:
			return "GAIN_LEVEL_ABSOLUTE";
		case Type::HumanizationSwingAbsolute:
			return "HUMANIZATION_SWING_ABSOLUTE";
		case Type::HumanizationSwingRelative:
			return "HUMANIZATION_SWING_RELATIVE";
		case Type::HumanizationTimingAbsolute:
			return "HUMANIZATION_TIMING_ABSOLUTE";
		case Type::HumanizationTimingRelative:
			return "HUMANIZATION_TIMING_RELATIVE";
		case Type::HumanizationVelocityAbsolute:
			return "HUMANIZATION_VELOCITY_ABSOLUTE";
		case Type::HumanizationVelocityRelative:
			return "HUMANIZATION_VELOCITY_RELATIVE";
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

MidiAction::Type MidiAction::parseType( const QString& sType )
{
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
	else if ( s == "count_in" ) {
		return Type::CountIn;
	}
	else if ( s == "count_in_pause_toggle" ) {
		return Type::CountInPauseToggle;
	}
	else if ( s == "count_in_stop_toggle" ) {
		return Type::CountInStopToggle;
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
	else if ( s == "humanization_swing_absolute" ) {
		return Type::HumanizationSwingAbsolute;
	}
	else if ( s == "humanization_swing_relative" ) {
		return Type::HumanizationSwingRelative;
	}
	else if ( s == "humanization_timing_absolute" ) {
		return Type::HumanizationTimingAbsolute;
	}
	else if ( s == "humanization_timing_relative" ) {
		return Type::HumanizationTimingRelative;
	}
	else if ( s == "humanization_velocity_absolute" ) {
		return Type::HumanizationVelocityAbsolute;
	}
	else if ( s == "humanization_velocity_relative" ) {
		return Type::HumanizationVelocityRelative;
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

MidiAction::Requires MidiAction::requiresFromType( const Type& type )
{
	auto
		requires
	= RequiresNone;

	if ( type == Type::GainLevelAbsolute || type == Type::PitchLevelAbsolute ) {
		requires = static_cast<Requires>( requires | RequiresComponent );
	}

	if ( type == Type::BpmDecr || type == Type::BpmIncr ||
		 type == Type::BpmCcRelative || type == Type::BpmFineCcRelative ) {
		requires = static_cast<Requires>( requires | RequiresFactor );
	}

	if ( type == Type::EffectLevelAbsolute ||
		 type == Type::EffectLevelRelative ) {
		requires = static_cast<Requires>( requires | RequiresFx );
	}

	if ( type == Type::EffectLevelAbsolute ||
		 type == Type::EffectLevelRelative ||
		 type == Type::FilterCutoffLevelAbsolute ||
		 type == Type::GainLevelAbsolute || type == Type::InstrumentPitch ||
		 type == Type::PanAbsolute || type == Type::PanAbsoluteSym ||
		 type == Type::PanRelative || type == Type::PitchLevelAbsolute ||
		 type == Type::StripMuteToggle || type == Type::StripSoloToggle ||
		 type == Type::StripVolumeAbsolute ||
		 type == Type::StripVolumeRelative ) {
		requires = static_cast<Requires>( requires | RequiresInstrument );
	}

	if ( type == Type::GainLevelAbsolute || type == Type::PitchLevelAbsolute ) {
		requires = static_cast<Requires>( requires | RequiresLayer );
	}

	if ( type == Type::SelectNextPattern ||
		 type == Type::SelectNextPatternRelative ||
		 type == Type::SelectOnlyNextPattern ||
		 type == Type::SelectAndPlayPattern ) {
		requires = static_cast<Requires>( requires | RequiresPattern );
	}

	if ( type == Type::PlaylistSong ) {
		requires = static_cast<Requires>( requires | RequiresSong );
	}

	return
		requires;
}

MidiAction::MidiAction( Type type, TimePoint timePoint )
	: m_type( type ),
	  m_sParameter1( "0" ),
	  m_sParameter2( "0" ),
	  m_sParameter3( "0" ),
	  m_sValue( "0" ),
	  m_nComponent( MidiAction::nInvalidParameter ),
	  m_fFactor( MidiAction::fInvalidParameter ),
	  m_nFx( MidiAction::nInvalidParameter ),
	  m_nInstrument( MidiAction::nInvalidParameter ),
	  m_nLayer( MidiAction::nInvalidParameter ),
	  m_nPattern( MidiAction::nInvalidParameter ),
	  m_nSong( MidiAction::nInvalidParameter )
{
	m_requires = MidiAction::requiresFromType( type );

	if ( timePoint == TimePoint() ) {
		m_timePoint = Clock::now();
	}
	else {
		m_timePoint = timePoint;
	}
}

MidiAction::MidiAction( const std::shared_ptr<MidiAction> pOther )
{
	if ( pOther != nullptr ) {
		m_type = pOther->m_type;
		m_sParameter1 = pOther->m_sParameter1;
		m_sParameter2 = pOther->m_sParameter2;
		m_sParameter3 = pOther->m_sParameter3;
		m_sValue = pOther->m_sValue;
		m_requires = pOther->m_requires;
		m_nComponent = pOther->m_nComponent;
		m_fFactor = pOther->m_fFactor;
		m_nFx = pOther->m_nFx;
		m_nInstrument = pOther->m_nInstrument;
		m_nLayer = pOther->m_nLayer;
		m_nPattern = pOther->m_nPattern;
		m_nSong = pOther->m_nSong;
		m_timePoint = pOther->m_timePoint;
	}
}

std::shared_ptr<MidiAction> MidiAction::from(
	const std::shared_ptr<MidiAction> pOther,
	TimePoint timePoint
)
{
	if ( pOther == nullptr ) {
		return nullptr;
	}

	auto pNew = std::make_shared<MidiAction>( pOther );
	if ( timePoint == TimePoint() ) {
		pNew->m_timePoint = Clock::now();
	}
	else {
		pNew->m_timePoint = timePoint;
	}

	return pNew;
}

std::shared_ptr<MidiAction> MidiAction::fromQStrings(
	MidiAction::Type type,
	const QString& sParameter1,
	const QString& sParameter2,
	const QString& sParameter3
)
{
	auto pMidiAction = std::make_shared<MidiAction>( type );
	if ( type == Type::EffectLevelAbsolute ||
		 type == Type::EffectLevelRelative ||
		 type == Type::FilterCutoffLevelAbsolute ||
		 type == Type::GainLevelAbsolute || type == Type::InstrumentPitch ||
		 type == Type::PanAbsolute || type == Type::PanAbsoluteSym ||
		 type == Type::PanRelative || type == Type::PitchLevelAbsolute ||
		 type == Type::StripMuteToggle || type == Type::StripSoloToggle ||
		 type == Type::StripVolumeAbsolute ||
		 type == Type::StripVolumeRelative ) {
		pMidiAction->setInstrument( sParameter1.toInt() );
	}

	if ( type == Type::GainLevelAbsolute || type == Type::PitchLevelAbsolute ) {
		pMidiAction->setComponent( sParameter2.toInt() );
	}

	if ( type == Type::BpmDecr || type == Type::BpmIncr ||
		 type == Type::BpmCcRelative || type == Type::BpmFineCcRelative ) {
		pMidiAction->setFactor( sParameter1.toFloat() );
	}

	if ( type == Type::EffectLevelAbsolute ||
		 type == Type::EffectLevelRelative ) {
		pMidiAction->setFx( sParameter2.toInt() );
	}

	if ( type == Type::GainLevelAbsolute || type == Type::PitchLevelAbsolute ) {
		pMidiAction->setLayer( sParameter3.toInt() );
	}

	if ( type == Type::SelectNextPattern ||
		 type == Type::SelectNextPatternRelative ||
		 type == Type::SelectOnlyNextPattern ||
		 type == Type::SelectAndPlayPattern ) {
		pMidiAction->setPattern( sParameter1.toInt() );
	}

	if ( type == Type::PlaylistSong ) {
		pMidiAction->setSong( sParameter1.toInt() );
	}

	return pMidiAction;
}

void MidiAction::toQStrings(
	QString* pParameter1,
	QString* pParameter2,
	QString* pParameter3
) const
{
	if ( pParameter1 == nullptr || pParameter2 == nullptr ||
		 pParameter3 == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	if ( m_type == Type::EffectLevelAbsolute ||
		 m_type == Type::EffectLevelRelative ||
		 m_type == Type::FilterCutoffLevelAbsolute ||
		 m_type == Type::GainLevelAbsolute || m_type == Type::InstrumentPitch ||
		 m_type == Type::PanAbsolute || m_type == Type::PanAbsoluteSym ||
		 m_type == Type::PanRelative || m_type == Type::PitchLevelAbsolute ||
		 m_type == Type::StripMuteToggle || m_type == Type::StripSoloToggle ||
		 m_type == Type::StripVolumeAbsolute ||
		 m_type == Type::StripVolumeRelative ) {
		*pParameter1 = QString::number( getInstrument() );
	}

	if ( m_type == Type::GainLevelAbsolute ||
		 m_type == Type::PitchLevelAbsolute ) {
		*pParameter2 = QString::number( getComponent() );
	}

	if ( m_type == Type::BpmDecr || m_type == Type::BpmIncr ||
		 m_type == Type::BpmCcRelative || m_type == Type::BpmFineCcRelative ) {
		*pParameter1 = QString::number( getFactor() );
	}

	if ( m_type == Type::EffectLevelAbsolute ||
		 m_type == Type::EffectLevelRelative ) {
		*pParameter2 = QString::number( getFx() );
	}

	if ( m_type == Type::GainLevelAbsolute ||
		 m_type == Type::PitchLevelAbsolute ) {
		*pParameter3 = QString::number( getLayer() );
	}

	if ( m_type == Type::SelectNextPattern ||
		 m_type == Type::SelectNextPatternRelative ||
		 m_type == Type::SelectOnlyNextPattern ||
		 m_type == Type::SelectAndPlayPattern ) {
		*pParameter1 = QString::number( getPattern() );
	}

	if ( m_type == Type::PlaylistSong ) {
		*pParameter1 = QString::number( getSong() );
	}

	// Fallback to previous defaults.
	if ( pParameter1->isEmpty() ) {
		*pParameter1 = "0";
	}
	if ( pParameter2->isEmpty() ) {
		*pParameter2 = "0";
	}
	if ( pParameter3->isEmpty() ) {
		*pParameter3 = "0";
	}
}

bool MidiAction::isNull() const
{
	return m_type == Type::Null;
}

int MidiAction::getComponent() const
{
	if ( !( m_requires & MidiAction::RequiresComponent ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Component parameter"
			)
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nComponent;
}

void MidiAction::setComponent( int newComponent )
{
	if ( !( m_requires & MidiAction::RequiresComponent ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Component parameter"
			)
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_nComponent = std::max( 0, newComponent );
}

float MidiAction::getFactor() const
{
	if ( !( m_requires & MidiAction::RequiresFactor ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Factor parameter" )
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_fFactor;
}

void MidiAction::setFactor( float newFactor )
{
	if ( !( m_requires & MidiAction::RequiresFactor ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Factor parameter" )
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_fFactor = newFactor;
}

int MidiAction::getFx() const
{
	if ( !( m_requires & MidiAction::RequiresFx ) ) {
		ERRORLOG( QString( "Midi action [%1] does not support the Fx parameter"
		)
					  .arg( toQString() ) );
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nFx;
}

void MidiAction::setFx( int newFx )
{
	if ( !( m_requires & MidiAction::RequiresFx ) ) {
		ERRORLOG( QString( "Midi action [%1] does not support the Fx parameter"
		)
					  .arg( toQString() ) );
		assert( false );
		return;
	}

	m_nFx = std::max( 0, newFx );
}

int MidiAction::getInstrument() const
{
	if ( !( m_requires & MidiAction::RequiresInstrument ) ) {
		ERRORLOG(
			QString(
				"Midi action [%1] does not support the Instrument parameter"
			)
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nInstrument;
}

void MidiAction::setInstrument( int newInstrument )
{
	if ( !( m_requires & MidiAction::RequiresInstrument ) ) {
		ERRORLOG(
			QString(
				"Midi action [%1] does not support the Instrument parameter"
			)
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_nInstrument = std::max( 0, newInstrument );
}

int MidiAction::getLayer() const
{
	if ( !( m_requires & MidiAction::RequiresLayer ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Layer parameter" )
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nLayer;
}

void MidiAction::setLayer( int newLayer )
{
	if ( !( m_requires & MidiAction::RequiresLayer ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Layer parameter" )
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_nLayer = std::max( 0, newLayer );
}

int MidiAction::getPattern() const
{
	if ( !( m_requires & MidiAction::RequiresPattern ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Pattern parameter" )
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nPattern;
}

void MidiAction::setPattern( int newPattern )
{
	if ( !( m_requires & MidiAction::RequiresPattern ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Pattern parameter" )
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_nPattern = std::max( 0, newPattern );
}

int MidiAction::getSong() const
{
	if ( !( m_requires & MidiAction::RequiresSong ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Song parameter" )
				.arg( toQString() )
		);
		assert( false );
		return MidiAction::nInvalidParameter;
	}

	return m_nSong;
}

void MidiAction::setSong( int newSong )
{
	if ( !( m_requires & MidiAction::RequiresSong ) ) {
		ERRORLOG(
			QString( "Midi action [%1] does not support the Song parameter" )
				.arg( toQString() )
		);
		assert( false );
		return;
	}

	m_nSong = std::max( 0, newSong );
}

bool MidiAction::isEquivalentTo( const std::shared_ptr<MidiAction> pOther
) const
{
	if ( pOther == nullptr ) {
		return false;
	}

	return (
		m_type == pOther->m_type && m_nComponent == pOther->m_nComponent &&
		m_fFactor == pOther->m_fFactor && m_nFx == pOther->m_nFx &&
		m_nLayer == pOther->m_nLayer && m_nPattern == pOther->m_nPattern &&
		m_nSong == pOther->m_nSong && m_sValue == pOther->m_sValue
	);
}

QString MidiAction::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;

	QStringList
		requires;
	if ( m_requires & MidiAction::RequiresComponent ) {
		requires << "Component";
	}
	if ( m_requires & MidiAction::RequiresFactor ) {
		requires << "Factor";
	}
	if ( m_requires & MidiAction::RequiresFx ) {
		requires << "Fx";
	}
	if ( m_requires & MidiAction::RequiresInstrument ) {
		requires << "Instrument";
	}
	if ( m_requires & MidiAction::RequiresLayer ) {
		requires << "Layer";
	}
	if ( m_requires & MidiAction::RequiresPattern ) {
		requires << "Pattern";
	}
	if ( m_requires & MidiAction::RequiresSong ) {
		requires << "Song";
	}
	if ( requires.size() == 0 ) {
		requires << "None";
	}

	if ( !bShort ) {
		sOutput =
			QString( "%1[MidiAction]\n" )
				.arg( sPrefix )
				.append( QString( "%1%2m_type: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( MidiAction::typeToQString( m_type ) ) )
				.append( QString( "%1%2m_sValue: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sValue ) )
				.append( QString( "%1%2m_sParameter1: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sParameter1 ) )
				.append( QString( "%1%2m_sParameter2: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sParameter2 ) )
				.append( QString( "%1%2m_sParameter3: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sParameter3 ) )
				.append( QString( "%1%2m_requires: [%3]\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( requires.join( ", " ) ) )
				.append( QString( "%1%2m_nComponent: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nComponent ) )
				.append( QString( "%1%2m_fFactor: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_fFactor ) )
				.append( QString( "%1%2m_nFx: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nFx ) )
				.append( QString( "%1%2m_nInstrument: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nInstrument ) )
				.append( QString( "%1%2m_nLayer: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nLayer ) )
				.append( QString( "%1%2m_nPattern: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPattern ) )
				.append( QString( "%1%2m_nSong: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nSong ) )
				.append( QString( "%1%2m_timePoint: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( H2Core::timePointToQString( m_timePoint ) )
				);
	}
	else {
		sOutput =
			QString( "[MidiAction]" )
				.append( QString( " m_type: %1" )
							 .arg( MidiAction::typeToQString( m_type ) ) )
				.append( QString( ", m_sValue: %1" ).arg( m_sValue ) )
				.append( QString( ", m_sParameter1: %1" ).arg( m_sParameter1 ) )
				.append( QString( ", m_sParameter2: %1" ).arg( m_sParameter2 ) )
				.append( QString( ", m_sParameter3: %1" ).arg( m_sParameter3 ) )
				.append(
					QString( ", m_requires: [%1]" ).arg( requires.join( ", " ) )
				)
				.append( QString( ", m_nComponent: %1" ).arg( m_nComponent ) )
				.append( QString( ", m_fFactor: %1" ).arg( m_fFactor ) )
				.append( QString( ", m_nFx: %1" ).arg( m_nFx ) )
				.append( QString( ", m_nInstrument: %1" ).arg( m_nInstrument ) )
				.append( QString( ", m_nLayer: %1" ).arg( m_nLayer ) )
				.append( QString( ", m_nPattern: %1" ).arg( m_nPattern ) )
				.append( QString( ", m_nSong: %1" ).arg( m_nSong ) )
				.append( QString( ", m_timePoint: %1" )
							 .arg( H2Core::timePointToQString( m_timePoint ) )
				);
	}

	return sOutput;
}
