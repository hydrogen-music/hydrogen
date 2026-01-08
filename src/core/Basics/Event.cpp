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

#include <core/Basics/Event.h>
#include <core/EventQueue.h>

namespace H2Core
{

QString Event::TypeToQString( Event::Type type ) {
	switch( type ) {
	case Event::Type::ActionModeChanged:
		return "ActionModeChanged";
	case Event::Type::AudioDriverChanged:
		return "AudioDriverChanged";
	case Event::Type::BbtChanged:
		return "BbtChanged";
	case Event::Type::BeatCounter:
		return "BeatCounter";
	case Event::Type::DrumkitLoaded:
		return "DrumkitLoaded";
	case Event::Type::EffectChanged:
		return "EffectChanged";
	case Event::Type::Error:
		return "Error";
	case Event::Type::GridCellToggled:
		return "GridCellToggled";
	case Event::Type::InstrumentLayerChanged:
		return "InstrumentLayerChanged";
	case Event::Type::InstrumentMuteSoloChanged:
		return "InstrumentMuteSoloChanged";
	case Event::Type::InstrumentParametersChanged:
		return "InstrumentParametersChanged";
	case Event::Type::JackTimebaseStateChanged:
		return "JackTimebaseStateChanged";
	case Event::Type::JackTransportActivation:
		return "JackTransportActivation";
	case Event::Type::LoopModeActivation:
		return "LoopModeActivation";
	case Event::Type::Metronome:
		return "Metronome";
	case Event::Type::MidiClockActivation:
		return "MidiClockActivation";
	case Event::Type::MidiDriverChanged:
		return "MidiDriverChanged";
	case Event::Type::MidiInput:
		return "MidiInput";
	case Event::Type::MidiEventMapChanged:
		return "MidiEventMapChanged";
	case Event::Type::MidiOutput:
		return "MidiOutput";
	case Event::Type::MixerSettingsChanged:
		return "MixerSettingsChanged";
	case Event::Type::NextPatternsChanged:
		return "NextPatternsChanged";
	case Event::Type::NextShot:
		return "NextShot";
	case Event::Type::NoteRender:
		return "NoteRender";
	case Event::Type::PatternEditorLocked:
		return "PatternEditorLocked";
	case Event::Type::PatternModified:
		return "PatternModified";
	case Event::Type::PlaybackTrackChanged:
		return "PlaybackTrackChanged";
	case Event::Type::PlayingPatternsChanged:
		return "PlayingPatternsChanged";
	case Event::Type::PlaylistChanged:
		return "PlaylistChanged";
	case Event::Type::PlaylistLoadSong:
		return "PlaylistLoadSong";
	case Event::Type::Progress:
		return "Progress";
	case Event::Type::Quit:
		return "Quit";
	case Event::Type::RecordModeChanged:
		return "RecordModeChanged";
	case Event::Type::Relocation:
		return "Relocation";
	case Event::Type::SelectedInstrumentChanged:
		return "SelectedInstrumentChanged";
	case Event::Type::SelectedPatternChanged:
		return "SelectedPatternChanged";
	case Event::Type::SongModeActivation:
		return "SongModeActivation";
	case Event::Type::SongModified:
		return "SongModified";
	case Event::Type::SongSizeChanged:
		return "SongSizeChanged";
	case Event::Type::SoundLibraryChanged:
		return "SoundLibraryChanged";
	case Event::Type::StackedModeActivation:
		return "StackedModeActivation";
	case Event::Type::State:
		return "State";
	case Event::Type::TempoChanged:
		return "TempoChanged";
	case Event::Type::TimelineActivation:
		return "TimelineActivation";
	case Event::Type::UndoRedo:
		return "UndoRedo";
	case Event::Type::UpdatePreferences:
		return "UpdatePreferences";
	case Event::Type::UpdateSong:
		return "UpdateSong";
	case Event::Type::UpdateTimeline:
		return "UpdateTimeline";
	case Event::Type::Xrun:
		return "Xrun";
	default:
		break;
	}

	return QString( "Unknown event: [%1]" ).arg( static_cast<int>(type));
}

Event::Event( Event::Type type, int nValue ) : m_type( type )
											 , m_nValue( nValue ) {
	auto pEventQueue = EventQueue::get_instance();
	if ( pEventQueue != nullptr ) {
		// This should always be true
		m_nId = pEventQueue->createEventId();
	}
	else {
		m_nId = Event::nInvalidId;
	}
}

Event::~Event() {
}

QString Event::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Event]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_type: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( Event::TypeToQString( m_type ) ) )
			.append( QString( "%1%2m_nValue: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nValue ) )
			.append( QString( "%1%2m_nId: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nId ) );
	}
	else {
		sOutput = QString( "[Event]" )
			.append( QString( " m_type: %1" ).arg( Event::TypeToQString( m_type ) ) )
			.append( QString( ", m_nValue: %1" ).arg( m_nValue ) )
			.append( QString( ", m_nId: %1" ).arg( m_nId ) );
	}

	return sOutput;
}

};
