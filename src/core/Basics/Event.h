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

#ifndef EVENT_H
#define EVENT_H

#include <core/Object.h>

namespace H2Core
{

/** Basic types of communication between the core part of Hydrogen and
    its GUI.*/
enum EventType {
	/** Fallback event*/
	EVENT_NONE,
	EVENT_STATE,
	/**
	 * The list of currently played patterns
	 * (AudioEngine::getPlayingPatterns()) did change.
	 *
	 * In #Song::Mode::Song this is triggered every time transport
	 * reaches a new column of the SongEditor grid, either by rolling
	 * or relocation. In #Song::PatternMode::Selected it's triggered
	 * by selecting a different pattern and in
	 * #Song::PatternMode::Stacked as soon as transport is looped to
	 * the beginning after a pattern got activated or deactivated.
	 *
	 * It is handled by EventListener::playingPatternsChangedEvent().
	 */
	EVENT_PLAYING_PATTERNS_CHANGED,
	/**
	 * Used in #Song::PatternMode::Stacked to indicate that a either
	 * AudioEngine::getNextPatterns() did change.
	 *
	 * It is handled by EventListener::nextPatternsChangedEvent().
	 */
	EVENT_NEXT_PATTERNS_CHANGED,
	/**
	 * A pattern was added, deleted, or modified.
	 */
	EVENT_PATTERN_MODIFIED,
	/** Another pattern was selected via MIDI or the GUI without
	 * affecting the audio transport. While the selection in the
	 * former case already happens in the GUI, this event will be used
	 * to tell it the selection was successful and had been done.
	 *
	 * Handled by EventListener::selectedPatternChangedEvent().
	 */
	EVENT_SELECTED_PATTERN_CHANGED,
	EVENT_SELECTED_INSTRUMENT_CHANGED,
	/** Some parameters of an instrument have been changed.
	 *
	 * Numbers `>=0` indicate the number of the instrument that has been
	 * changed. `-1` indicates that multiple instruments were altered.
	 */
	EVENT_INSTRUMENT_PARAMETERS_CHANGED,
	/** Mute or solo state of the instrument specified in the event parameter
	 * changed.
	 *
	 * This event must always be accompanied with
	 * #EVENT_INSTRUMENT_PARAMETERS_CHANGED and is meant to for the instrument
	 * rows in the sidebar of the pattern editor to be updated without forcing
	 * an update on every instrument parameter change. */
	EVENT_INSTRUMENT_MUTE_SOLO_CHANGED,
	EVENT_MIDI_ACTIVITY,
	EVENT_XRUN,
	EVENT_NOTEON,
	EVENT_ERROR,
	/**
	 * Triggered when a metronome note is passed to the
	 * #H2Core::Sampler.
	 *
	 * - 0 - First bar requiring a distinct sound
	 * - 1 - All the other bars
	 * - 2 - Metronome was turned on or off.
	 *
	 * Handled by EventListener::metronomeEvent().
	 */
	EVENT_METRONOME,
	/**
	 * Used by the thread of the `DiskWriterDriver` to indicate
	 * progress of the ongoing audio export (from 0 to 100).
	 *
	 * The value `-1` is used to indicate exporting failed.
	 */
	EVENT_PROGRESS,
	EVENT_JACK_SESSION,
	EVENT_PLAYLIST_LOADSONG,
	EVENT_UNDO_REDO,
	EVENT_SONG_MODIFIED,
	EVENT_TEMPO_CHANGED,
	/**
	 * Event triggering the loading or saving of the
	 * H2Core::Preferences whenever they were changed outside of the
	 * GUI, e.g. by session management or an OSC command.
	 *
	 * If the value of the event is
	 * - 0 - tells the GUI to save the current geometry settings in
	 * the H2Core::Preferences file.
	 * - 1 - tells the GUI to load the Preferences file and to  update
	 * a bunch of widgets, checkboxes etc. to reflect the changes in
	 * the configuration.
	 */
	EVENT_UPDATE_PREFERENCES,
	/**
	 * Event triggering HydrogenApp::updateSongEvent() whenever the
	 * Song was changed outside of the GUI, e.g. by session management
	 * or and OSC command.
	 *
	 * If the value of the event is
	 * - 0 - update the GUI to represent the song loaded by the core.
	 * - 1 - triggered whenever the Song was saved via the core part
	 *       (updated the title and status bar).
	 * - 2 - Song is not writable (inform the user via a QMessageBox)
	 */
	EVENT_UPDATE_SONG,
	/**
	 * Triggering HydrogenApp::quitEvent() and enables a shutdown of
	 * the entire application via the command line.
	 */
	EVENT_QUIT,

	/** Enables/disables the usage of the Timeline.*/
	EVENT_TIMELINE_ACTIVATION,
	/** Tells the GUI some parts of the Timeline (tempo markers or
		tags) were modified.*/
	EVENT_TIMELINE_UPDATE,
	/** Toggles the button indicating the usage JACK transport.*/
	EVENT_JACK_TRANSPORT_ACTIVATION,
	/** Toggles the button indicating the usage JACK Timebase control and
		informs the GUI about a state change.*/
	EVENT_JACK_TIMEBASE_STATE_CHANGED,
	EVENT_SONG_MODE_ACTIVATION,
	/** Song::PatternMode::Stacked (0) or Song::PatternMode::Selected
		(1) was activated */
	EVENT_STACKED_MODE_ACTIVATION,
/** Toggles the button indicating the usage loop mode.*/
	EVENT_LOOP_MODE_ACTIVATION,
	/** Switches between select mode (0) and draw mode (1) in the *SongEditor.*/
	EVENT_ACTION_MODE_CHANGE,
	EVENT_GRID_CELL_TOGGLED,
	/** Triggered when transport is moved into a different column
		(either during playback or when relocated by the user)*/
	EVENT_COLUMN_CHANGED,
	/** A the current drumkit was replaced by a new one. */
	EVENT_DRUMKIT_LOADED,
	/** Locks the PatternEditor on the pattern currently played back.*/
	EVENT_PATTERN_EDITOR_LOCKED,
	/** Triggered in case there is a relocation of the transport
	 * position while trasnsport is not rolling. This can be either
	 * due to an user interaction or an incoming MIDI/OSC/JACK command
	 * or at the very end of the song in song mode.
	 */
	EVENT_RELOCATION,
	/**
	 * The coarse grained transport position in beats and bars did
	 * change. (Tick - the "T" in BBT - resolution is not implemented
	 * yet as no part of the application requires it).
	 */
	EVENT_BBT_CHANGED,
	EVENT_SONG_SIZE_CHANGED,
	EVENT_DRIVER_CHANGED,
	EVENT_PLAYBACK_TRACK_CHANGED,
	EVENT_SOUND_LIBRARY_CHANGED,
	EVENT_NEXT_SHOT,
	EVENT_MIDI_MAP_CHANGED,
	/**
	 * Event triggered whenever part of or the whole current #H2Core::Playlist
	 * changed.
	 *
	 * If the value of the event is
	 * - 0 - update the GUI to represent the Playlist loaded by the core.
	 * - 1 - triggered whenever the Playlist was saved via the core part
	 *       (updated the title and status bar).
	 * - 2 - Playlist is not writable (inform the user via a QMessageBox)
	 */
	EVENT_PLAYLIST_CHANGED,
	EVENT_BEAT_COUNTER,
	/** Miscellaneous things shown in the Mixer apart from the per-instrument
	 * MixerLines did change. */
	EVENT_MIXER_SETTINGS_CHANGED
};

/** Basic building block for the communication between the core of
 * Hydrogen and its GUI.  The individual Events will be enlisted in
 * the EventQueue singleton.*/
/** \ingroup docCore docEvent */
class Event : public H2Core::Object<Event>
{
	H2_OBJECT(Event)
public:

		/** Tells the associated routine in the core part of Hydrogen how and
		 * whether to communicate with the (G)UI */
		enum class Trigger {
			/** No #Event will be queued */
			Suppress,
			/** #Event queued on change */
			Default,
			/** #Event will be queued regardless whether there are changes or
			 * not.*/
			Force
		};

		/** Get string representation of #EventType. */
		static QString typeToQString( EventType type );

		Event( EventType type = EVENT_NONE, int nValue = 0 );
		~Event();

		EventType getType() const;

		int getValue() const;

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:

		/** Specifies the context the event is create in and which function
			should be triggered to handle it.*/
		EventType m_type;
		/** Additional information to describe the actual context of the
			engine.*/
		int m_nValue;

};

inline EventType Event::getType() const {
	return m_type;
}
inline int Event::getValue() const {
	return m_nValue;
}

};
#endif
