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

namespace H2Core
{

QString Event::typeToQString( EventType type ) {
	switch( type ) {
	case EVENT_NONE:
		return "EVENT_NONE";
	case EVENT_STATE:
		return "EVENT_STATE";
	case EVENT_PLAYING_PATTERNS_CHANGED:
		return "EVENT_PLAYING_PATTERNS_CHANGED";
	case EVENT_NEXT_PATTERNS_CHANGED:
		return "EVENT_NEXT_PATTERNS_CHANGED";
	case EVENT_PATTERN_MODIFIED:
		return "EVENT_PATTERN_MODIFIED";
	case EVENT_SELECTED_PATTERN_CHANGED:
		return "EVENT_SELECTED_PATTERN_CHANGED";
	case EVENT_SELECTED_INSTRUMENT_CHANGED:
		return "EVENT_SELECTED_INSTRUMENT_CHANGED";
	case EVENT_INSTRUMENT_PARAMETERS_CHANGED:
		return "EVENT_INSTRUMENT_PARAMETERS_CHANGED";
	case EVENT_INSTRUMENT_MUTE_SOLO_CHANGED:
		return "EVENT_INSTRUMENT_MUTE_SOLO_CHANGED";
	case EVENT_MIDI_ACTIVITY:
		return "EVENT_MIDI_ACTIVITY";
	case EVENT_XRUN:
		return "EVENT_XRUN";
	case EVENT_NOTEON:
		return "EVENT_NOTEON";
	case EVENT_ERROR:
		return "EVENT_ERROR";
	case EVENT_METRONOME:
		return "EVENT_METRONOME";
	case EVENT_PROGRESS:
		return "EVENT_PROGRESS";
	case EVENT_JACK_SESSION:
		return "EVENT_JACK_SESSION";
	case EVENT_PLAYLIST_LOADSONG:
		return "EVENT_PLAYLIST_LOADSONG";
	case EVENT_UNDO_REDO:
		return "EVENT_UNDO_REDO";
	case EVENT_SONG_MODIFIED:
		return "EVENT_SONG_MODIFIED";
	case EVENT_TEMPO_CHANGED:
		return "EVENT_TEMPO_CHANGED";
	case EVENT_UPDATE_PREFERENCES:
		return "EVENT_UPDATE_PREFERENCES";
	case EVENT_UPDATE_SONG:
		return "EVENT_UPDATE_SONG";
	case EVENT_QUIT:
		return "EVENT_QUIT";
	case EVENT_TIMELINE_ACTIVATION:
		return "EVENT_TIMELINE_ACTIVATION";
	case EVENT_TIMELINE_UPDATE:
		return "EVENT_TIMELINE_UPDATE";
	case EVENT_JACK_TRANSPORT_ACTIVATION:
		return "EVENT_JACK_TRANSPORT_ACTIVATION";
	case EVENT_JACK_TIMEBASE_STATE_CHANGED:
		return "EVENT_JACK_TIMEBASE_STATE_CHANGED";
	case EVENT_SONG_MODE_ACTIVATION:
		return "EVENT_SONG_MODE_ACTIVATION";
	case EVENT_STACKED_MODE_ACTIVATION:
		return "EVENT_STACKED_MODE_ACTIVATION";
	case EVENT_LOOP_MODE_ACTIVATION:
		return "EVENT_LOOP_MODE_ACTIVATION";
	case EVENT_ACTION_MODE_CHANGE:
		return "EVENT_ACTION_MODE_CHANGE";
	case EVENT_GRID_CELL_TOGGLED:
		return "EVENT_GRID_CELL_TOGGLED";
	case EVENT_COLUMN_CHANGED:
		return "EVENT_COLUMN_CHANGED";
	case EVENT_DRUMKIT_LOADED:
		return "EVENT_DRUMKIT_LOADED";
	case EVENT_PATTERN_EDITOR_LOCKED:
		return "EVENT_PATTERN_EDITOR_LOCKED";
	case EVENT_RELOCATION:
		return "EVENT_RELOCATION";
	case EVENT_BBT_CHANGED:
		return "EVENT_BBT_CHANGED";
	case EVENT_SONG_SIZE_CHANGED:
		return "EVENT_SONG_SIZE_CHANGED";
	case EVENT_DRIVER_CHANGED:
		return "EVENT_DRIVER_CHANGED";
	case EVENT_PLAYBACK_TRACK_CHANGED:
		return "EVENT_PLAYBACK_TRACK_CHANGED";
	case EVENT_SOUND_LIBRARY_CHANGED:
		return "EVENT_SOUND_LIBRARY_CHANGED";
	case EVENT_NEXT_SHOT:
		return "EVENT_NEXT_SHOT";
	case EVENT_MIDI_MAP_CHANGED:
		return "EVENT_MIDI_MAP_CHANGED";
	case EVENT_PLAYLIST_CHANGED:
		return "EVENT_PLAYLIST_CHANGED";
	case EVENT_BEAT_COUNTER:
		return "EVENT_BEAT_COUNTER";
	default:
		break;
	}

	return QString( "Unknown event: [%1]" ).arg( static_cast<int>(type));
}

QString Event::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Event]\n" ).arg( sPrefix )
			.append( QString( "%1%2type: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( Event::typeToQString( type ) ) )
			.append( QString( "%1%2value: %3\n" ).arg( sPrefix ).arg( s ).arg( value ) );
	}
	else {
		sOutput = QString( "[Event]" )
			.append( QString( " type: %1" ).arg( Event::typeToQString( type ) ) )
			.append( QString( ", value: %1\n" ).arg( value ) );
	}

	return sOutput;
}

};
