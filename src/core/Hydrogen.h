/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#ifndef HYDROGEN_H
#define HYDROGEN_H

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/Helpers/Time.h>
#include <core/IO/JackDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiEvent.h>
#include <core/Object.h>
#include <core/Sampler/Interpolation.h>

#include <stdint.h> // for uint32_t et al
#include <cassert>
#include <chrono>
#include <memory>
#include <vector>

namespace H2Core
{
	class AudioEngine;
	class AudioDriver;
	class CoreActionController;
	class Drumkit;
	class EventQueue;
	class MidiBaseDriver;
	class MidiActionManager;
	class NsmClient;
	class OscServer;
	class Playlist;
	class Preferences;
	class SoundLibraryDatabase;
	class TimeHelper;

///
/// Hydrogen Audio Engine.
///
/** \ingroup docCore*/
class Hydrogen : public H2Core::Object<Hydrogen>
{
	H2_OBJECT(Hydrogen)
public:

	/** If the new tap tempo interval results in a tempo deviating more than
	 * this value from the current average, the average will be reset to the
	 * newest tempo. */
	static constexpr int nTapTempoMaxDiff = 20;
	
	/** Specifies where the #AudioEngine does get its current tempo
		updates from.*/
	enum class Tempo {
		/** BeatCounter, TapTempo, OSC and MIDI commands as well as
			the BPM widget in the MainToolBar are used to change the
			tempo.*/
		Song = 0,
		/** Only tempo markers on the Timeline are considered.*/
		Timeline = 1,
		/** Hydrogen is synchronizing tempo and transport handling - like start,
		 * stop, and relocation - to incoming MIDI (clock) events. This will
		 * only work, if there is _no_ external JACK Timebase controller
		 * present. */
		Midi = 2,
		/** Hydrogen will disregard all internal tempo settings and uses the
			ones provided by the JACK server instead. This mode is only used in
			case the JACK audio driver is used, JACK Timebase support is
			activated in the Preferences, and an external Timebase controller is
			registered to the JACK server.*/
		Jack = 3,
		/** Hydrogen runs as a plugin and follows the tempo broadcast by the
			host transport (ADR 0013). Takes precedence over the Timeline. */
		Plugin = 4
	};

	enum ErrorMessages {
		/**
		 * Unable to connect the audio driver stored. The StubAudioDriver will be
		 * used as a fallback instead.
		 */
		ERROR_STARTING_DRIVER,
		JACK_SERVER_SHUTDOWN,
		JACK_CANNOT_ACTIVATE_CLIENT,
		/** Can't connect the main audio output ports to the default. */
		JACK_CANNOT_CONNECT_OUTPUT_PORT,
		/** Our JACK client can not be disconnected from the JACK server. */
		JACK_CANNOT_CLOSE_CLIENT,
		/** Can't register either audio or MIDI ports for our JACK client. */
		JACK_ERROR_IN_PORT_REGISTER,
		/**
		 * Unable to start the OSC server with the given
		 * port number.
		 */
		OSC_CANNOT_CONNECT_TO_PORT,
		PLAYBACK_TRACK_INVALID
	};

	/** Specifies the state of the Qt GUI*/
	enum class GUIState {
		/** Hydrogen is still starting up. Core maybe already somewhat ready but
		 * GUI is most probably not.*/
		startup = -1,
		/** Hydrogen is up and running but there is no GUI available. */
		headless = 0,
		/** Hydrogen is up and running and there is a working GUI. */
		ready = 1,
		/** Teardown of Hydrogen was initialized and the Event handling system
		 * might not work anymore. */
		shutdown
	};
		static QString GUIStateToQString( const GUIState& state );

	/**
	 * Standalone helper: loads the process-current Preferences and constructs
	 * the single Hydrogen instance used by the standalone application.
	 *
	 * A thin wrapper around the instance constructor (ADR 0015); plugin hosts
	 * construct instances directly via #Hydrogen().
	 */
	static Hydrogen*	create_instance( int nOscPort,
										 std::shared_ptr<Preferences> pPreferences );

	/**
	 * Constructs a Hydrogen instance owning the given @a pPref Preferences and
	 * its own EventQueue (ADR 0015). Multiple instances may coexist in one
	 * process; the caller owns the instance.
	 *
	 * @param pPref Preferences owned by this instance.
	 * @param nOscPort OSC port for this instance, or -1 to disable OSC.
	 */
	Hydrogen( std::shared_ptr<Preferences> pPref, int nOscPort );

	/**
	 * Destructor taking care of most of the clean up.
	 */
	~Hydrogen();

	/** \return The Preferences owned by this instance (#m_pPreferences). */
	std::shared_ptr<Preferences> getPreferences() const { return m_pPreferences; }
	/** Adopts @a pPreferences as the Preferences owned by this instance (ADR
	 * 0015). Used when a different configuration is loaded at runtime. */
	void setPreferences( std::shared_ptr<Preferences> pPreferences ) {
		m_pPreferences = pPreferences;
	}

	/** Effective sample-interpolation mode used by the #H2Core::Sampler: the
	 * audio-export override (#m_interpolateModeOverride) when active, else the
	 * persistent #H2Core::Preferences::m_interpolateMode (ADR 0027). */
	Interpolation::InterpolateMode getInterpolateMode() const;
	/** Temporarily overrides the interpolation mode (audio export only, CLI or
	 * ExportSongDialog). Not persisted; cleared with
	 * clearInterpolateModeOverride() once the export is done. */
	void setInterpolateModeOverride( Interpolation::InterpolateMode mode );
	/** Drops the audio-export interpolation override set via
	 * setInterpolateModeOverride(). */
	void clearInterpolateModeOverride();
	/** \return The EventQueue owned by this instance (#m_pEventQueue). */
	EventQueue*		getEventQueue() const { return m_pEventQueue; }
#ifdef H2CORE_HAVE_OSC
	/** \return The OSC server owned by this instance (ADR 0015). */
	OscServer*		getOscServer() const { return m_pOscServer; }
	/** \return The NSM client owned by this instance (ADR 0015). */
	NsmClient*		getNsmClient() const { return m_pNsmClient; }
#endif
	/** \return The per-instance Logger owned by this instance (ADR 0015, T1.6).
	 * Used with Logger::Scope at the instance entry points so logging routes to
	 * this instance's own log file. */
	Logger*			getLogger() const { return m_pLogger.get(); }

	/*
	 * return central instance of the audio engine
	 */
	AudioEngine*		getAudioEngine() const;
		std::shared_ptr<MidiActionManager> getMidiActionManager() const;
	std::shared_ptr<CoreActionController> getCoreActionController() const {
		return m_pCoreActionController;
	}
	std::shared_ptr<SoundLibraryDatabase> getSoundLibraryDatabase() const {
		return m_pSoundLibraryDatabase;
	}
	std::shared_ptr<Playlist> getPlaylist() const;
	void setPlaylist( std::shared_ptr<Playlist> pPlaylist );
		std::shared_ptr<TimeHelper> getTimeHelper() const;

// ***** SEQUENCER ********
	/// Start the internal sequencer
	void			sequencerPlay();

	/// Stop the internal sequencer
	void			sequencerStop();

	/// Last received midi message
	const MidiEvent::Type& getLastMidiEvent() const;
	void setLastMidiEvent( const MidiEvent::Type& type );
	Midi::Parameter getLastMidiEventParameter() const;
	void setLastMidiEventParameter( Midi::Parameter parameter );

	/** Wrapper around AudioEngine::toggleNextPattern().*/
	void			toggleNextPattern( int nPatternNumber );
	/** Wrapper around AudioEngine::flushAndAddNextPattern().*/
	bool			flushAndAddNextPattern( int nPatternNumber );
	
		/**
		 * Get the current song.
		 * \return #m_pSong
		 */ 	
		std::shared_ptr<Song>			getSong() const{ return m_pSong; }
		/**
		 * Sets the current song #m_pSong to @a pNewSong.
		 * \param pNewSong Pointer to the new Song object.
		 */
		void			setSong( std::shared_ptr<Song> pNewSong );

	/**
	 * Find a PatternList/column corresponding to the supplied tick
	 * position @a nTick.
	 *
	 * Adds up the lengths of all pattern columns until @a nTick lies in
	 * between the bounds of a Pattern.
	 *
	 * \param nTick Position in ticks.
	 * \param bLoopMode Whether looping is enabled in the Song, see
	 *   Song::is_loop_enabled(). If true, @a nTick is allowed to be
	 *   larger than the total length of the Song.
	 * \param pPatternStartTick Pointer to an integer the beginning of the
	 *   found pattern list will be stored in (in ticks).
	 * \return
	 *   - -1 : pattern list couldn't be found.
	 *   - >=0 : PatternList index in Song::__pattern_group_sequence.
	 */
	int			getColumnForTick( long nTick, bool bLoopMode, long* pPatternStartTick ) const;
	/**
	 * Get the total number of ticks passed up to a @a nColumn /
	 * pattern group.
	 *
	 * The AudioEngine should be LOCKED when calling this!
	 *
	 * \param nColumn pattern group.
	 * \return
	 *  - -1 : if @a nColumn is bigger than the number of patterns in
	 *   the Song and Song::isLoopEnabled() is set to false or
	 *   no Patterns could be found at all.
	 *  - >= 0 : the total number of ticks passed.
	 */
	long			getTickForColumn( int nColumn ) const;

	Song::Mode getMode() const;
	/** Wrapper around Song::setMode() which also triggers
	Event::Type::SongModeActivation and should be used by all parts of the
	code except for song reading/setting.*/
	void setMode( const Song::Mode& mode, Event::Trigger trigger );
	
	Song::ActionMode getActionMode() const;
	/** Wrapper around Song::setActionMode() which also triggers
	Event::Type::ActionModeChanged and should be used by all parts of the
	code except for song reading/setting.*/
	void setActionMode( const Song::ActionMode& mode );

	Song::PatternMode getPatternMode() const;
	/** Wrapper around Song::setPatternMode() which also triggers
	Event::Type::StackedModeActivation and should be used by all parts of the
	code except for song reading/setting.*/
	void setPatternMode( const Song::PatternMode& mode );

	/** Wrapper around both Song::setIsTimelineActivated (recent) and
	Preferences::setUseTimelinebpm() (former place to store the
	variable but kept to maintain backward compatibility) which also
	triggers Event::Type::TimelineActivation.*/
	void setIsTimelineActivated( bool bEnabled );

	void updateSongSize();

	bool addRealtimeNote(
		int instrument,
		float velocity,
		bool noteoff = false,
		Midi::Note note = Midi::NoteDefault
	);

	Midi::Parameter getHihatOpenness() const;
	void setHihatOpenness( Midi::Parameter openness );

	void restartAudioDriver();
	void restartMidiDriver();

	std::shared_ptr<AudioDriver> getAudioDriver() const;
	std::shared_ptr<MidiBaseDriver> getMidiDriver() const;

	/** Sets the state of the current drumkit - the one contained in #m_pSong -
	 * to @a bIsModified.
	 *
	 * Use this wrapper function instead of Drumkit::setIsModified() since it
	 * ensures the modification state of the enclosing song is set as well. */
	void setDrumkitModified( bool bIsModified );
	/** Sets the state of a pattern contained in #m_pSong to @a bIsModified.
	 *
	 * Use this wrapper function instead of Pattern::setIsModified() since it
	 * ensures the modification state of the enclosing song is set as well. */
	void setPatternModified( bool bIsModified, int nIndex );
	/** Wrapper around Song::setIsModified() that checks whether a
		song is set.*/
	void setSongModified( bool bIsModified );
	/** Wrapper around Song::getIsModified() that checks whether a
		song is set.*/
	bool getSongModified() const;

	void			onTapTempoAccelEvent( TimePoint start = TimePoint() );

	/** \return #m_nSelectedPatternNumber*/
	int				getSelectedPatternNumber() const;
	/**
	 * Sets #m_nSelectedPatternNumber.
	 *
	 * \param nPat Sets #m_nSelectedPatternNumber
	 * \param bNeedsLock Whether the function was called with the audio engine
	 *   locked already or it should do so itself.
	 * \param trigger How to handle events. Forcing is important when
	 *   rearranging patterns in the song editor while the pattern editor is
	 *   locked.
	 */
	void setSelectedPatternNumber( int nPat,
								   bool bNeedsLock = true,
								   Event::Trigger trigger = Event::Trigger::Default );

	/**
	 * Updates the selected pattern to the one recorded note will be
	 * inserted to.
	 */
	void updateSelectedPattern( bool bNeedsLock = true );

	int				getSelectedInstrumentNumber() const;
	/**
	 * \param nInstrument #Instrument about to be selected
	 * \param trigger How to handle events.
	 */
	void			setSelectedInstrumentNumber(
		int nInstrument, Event::Trigger trigger = Event::Trigger::Default );
	std::shared_ptr<Instrument>		getSelectedInstrument() const;

	void renamePerTrackJackAudioPorts(
		std::shared_ptr<Song> pSong,
		std::shared_ptr<Drumkit> pOldDrumkit = nullptr
	);

	/** Starts/stops the OSC server
	 * \param bEnable `true` = start, `false` = stop.*/
	void			toggleOscServer( bool bEnable );
	/** Destroys and recreates the OscServer singleton in order to
		adopt a new OSC port.*/
	void			recreateOscServer();

	// beatconter
	void			setBeatCounterTotalBeats( int nTotalBeats );
	int			getBeatCounterTotalBeats() const;
	void			setBeatCounterBeatLength( float fBeatLength );
	float			getBeatCounterBeatLength() const;
	int			getBeatCounterEventCount() const;
	bool			handleBeatCounter( TimePoint start = TimePoint() );
	void			updateBeatCounterSettings();

	void			initJackTimebaseControl();
	void			releaseJackTimebaseControl();

	void			panic();

	//export management
	bool			getIsExportSessionActive() const;
	/**
	 * @param nSampleRate sample rate using which to export
	 * @param nSampleDepth sample depth using which to export
	 * @param fCompressionLevel Trades off audio quality against compression
	 *   rate defined between 0.0 (maximum quality) and 1.0 (maximum
	 *   compression).
	 *
	 * \return true on success
	 * .*/
	bool			startExportSession( int nSampleRate, int nSampleDepth,
										double fCompressionLevel = 0.0 );
	void			stopExportSession();
	/** Renders the current song to @a sFileName.
	 *
	 * @param excludedInstruments Identities of instruments to leave out of this
	 *   export (per-instrument/track exports). Empty (the default) exports every
	 *   instrument. The per-instrument export flag is set here rather than by the
	 *   caller (ADR 0027). */
	void			startExportSong(
		const QString& sFileName,
		const std::vector<Uuid>& excludedInstruments = {} );
	void			stopExportSong();
	
	/************************************************************/
	/********************** Playback track **********************/
	/**
	 * Wrapper function for loading the playback track.
	 */
	void			loadPlaybackTrack( const QString& sFileName );
	/************************************************************/

	/**\return #m_GUIState*/
	const GUIState&	getGUIState() const;
	/**\param state Specifies whether the Qt5 GUI is active. Sets
	   #m_GUIState.*/
	void			setGUIState( const GUIState& state );
	bool			hasJackDriver() const;
	/**
	 * \return Whether #H2Core::JackDriver is used as current audio driver and
	 * JACK transport was activated via the GUI
	 * (#H2Core::Preferences::m_nJackTransportMode).
	 */
	bool			hasJackTransport() const;

	/**
	 * Convenience function checking whether using the Timeline tempo is set in
	 * the Preferences, Song::SONG_MODE is set, and there is an external JACK
	 * Timebase controller (application) present.
	 *
	 * \return Whether the Timeline is used to determine the current speed.
	 */
	bool isTimelineEnabled() const;

	/**
	 * Convenience function checking whether using the Pattern Editor
	 * is locked in the song settings and the song is in song mode.
	 */
	bool isPatternEditorLocked() const;
	void setIsPatternEditorLocked( bool bValue );

	Tempo getTempoSource() const;

	/** \return Whether this instance is driven by a plugin host (ADR 0013):
	 * audio, MIDI, and transport come from the host rather than from local
	 * real-time drivers. The single predicate gating plugin-mode behaviour
	 * (host-transport following here; feature disablement in ADR 0026). */
	bool isUnderPluginHost() const;

	/**
	 * \return Whether we hasJackTransport() and there is an external JACK
	 *   Timebase controller broadcasting tempo information. If so, we disregard
	 *   Hydrogen's Timeline information (see
	 *   #H2Core::JackDriver::m_timebaseState).
	 */
	JackDriver::Timebase		getJackTimebaseState() const;
	/** \return NsmClient::m_bUnderSessionManagement if NSM is
		supported.*/
	bool			isUnderSessionManagement() const;

		bool getRecordEnabled() const;
		void setRecordEnabled( bool bEnabled );

	/**
	 * Add @a pInstr to death row.
	 *
	 * Since there might still be some notes using @a pInstr left in one of the
	 * note queues, the instrument's samples must not be unloaded right away
	 * (the instrumet's destructor might not be called after deleting it since
	 * it might live on in the undo/redo stack of the GUI). Instead, this
	 * function will add it to a list of instruments marked for deletion and it
	 * will be dealt with at a later time (after audio rendering was stopped).
	 */
	void addInstrumentToDeathRow( std::shared_ptr<Instrument> pInstr );

		/** Since we are flushing the samples of the instruments in the death
		 * row at a delayed point in time (like when stopping transport), we
		 * have to take care not to get into trouble when switching
		 * instrument/kits back and forth (like in undo/redo). */
		void removeInstrumentFromDeathRow( std::shared_ptr<Instrument> pInstr );

	/**
	 * Processes the patterns added to any virtual ones in the
	 * #PatternList of the current #Song and ensure both the playing
	 * pattern representation in the #AudioEngine and the GUI are
	 * synced.
	 */
		void updateVirtualPatterns(
			Event::Trigger trigger = Event::Trigger::Default
		);


		bool getSendBbtChangeEvents() const;
		void setSendBbtChangeEvents( bool bSend );

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

		void killInstruments();

	void			midiNoteOn( std::shared_ptr<Note> pNote );

	/** Per-instance Logger owned by this instance (ADR 0015, T1.6) — own queue,
	 * worker thread and log file. */
	std::shared_ptr<Logger> m_pLogger;
	/** Preferences owned by this instance (ADR 0015). */
	std::shared_ptr<Preferences> m_pPreferences;
	/** Audio-export interpolation override (ADR 0027). When
	 * #m_bUseInterpolateModeOverride is set it takes priority over the
	 * persistent Preferences value; not serialised. */
	Interpolation::InterpolateMode m_interpolateModeOverride;
	bool m_bUseInterpolateModeOverride;
	/** EventQueue owned by this instance (ADR 0015). */
	EventQueue*		m_pEventQueue;

#ifdef H2CORE_HAVE_OSC
	/** OSC server owned by this instance (ADR 0015). Always constructed; only
	 * binds a port when OSC is enabled in this instance's Preferences. */
	OscServer*		m_pOscServer;
	/** NSM client owned by this instance (ADR 0015). */
	NsmClient*		m_pNsmClient;
#endif

	/**
	 * Pointer to the current song. It is initialized with NULL in
	 * the Hydrogen() constructor, set via setSong(), and accessed
	 * via getSong().
	 */
	std::shared_ptr<Song>			m_pSong;

	// beatcounter
	float			m_fBeatCounterBeatLength;	///< beatcounter note length
	int			m_nBeatCounterTotalBeats;	///< beatcounter beats to count
	int			m_nBeatCounterEventCount;		///< beatcounter event
	int			m_nBeatCounterBeatCount;		///< beatcounter beat to count
	std::vector<double>			m_beatCounterDiffs;	///< beat diff
		TimePoint m_lastBeatCounterTimePoint;
	int			m_nBeatCounterDriftCompensation;		///ms default 0
	int			m_nBeatCounterStartOffset;		///ms default 0
	// ~ beatcounter

		TimePoint m_lastTapTempoTimePoint;
		float m_fTapTempoAverageBpm;
		/** Number of events constituting #m_fTapTempoAverageBpm. This is
		 * required to calculate the cumulative average. */
		long m_nTapTempoEventsAveraged;

	// used for song export
	Song::Mode		m_oldEngineMode;
	bool			m_bOldLoopEnabled;
	bool			m_bExportSessionIsActive;
	
	/**
	 * Specifies whether the Qt5 GUI is active.
	 *
	 * When a new Song is set via the core part of Hydrogen, e.g. in
	 * the context of session management, the std::shared_ptr<Song> must* be set via
	 * the GUI if active. Else the GUI will freeze.
	 *
	 * Set by setGUIState() and accessed via getGUIState().
	 */
	GUIState		m_GUIState;

	/** Helper class for time-specific methods. */
	std::shared_ptr<TimeHelper> m_pTimeHelper;

	/// Deleting instruments too soon leads to potential crashes.
	std::list<std::shared_ptr<Instrument>> 	m_instrumentDeathRow;
	
	/**
	 * Instrument currently focused/selected in the GUI. 
	 *
	 * Within the core it is relevant for the MIDI input. Using
	 * Preferences::m_bPlaySelectedInstrument incoming MIDI signals can be
	 * used to play back only the selected instrument or the whole
	 * drumkit.
	 */
	int				m_nSelectedInstrumentNumber;
	/**
	 * Index of the pattern selected in the GUI or by a MIDI event.
	 */
	int				m_nSelectedPatternNumber;

	/**
	 * Onset of the recorded last in addRealtimeNote(). It is used to
	 * determine the custom length of the note in case the note on
	 * event is followed by a note off event.
	 */
	int				m_nLastRecordedMIDINoteTick;

		bool m_bRecordEnabled;

	/**
	 * Central instance of the audio engine. 
	 */
	AudioEngine*	m_pAudioEngine;
		std::shared_ptr<MidiActionManager> m_pMidiActionManager;
	std::shared_ptr<CoreActionController> m_pCoreActionController;

	std::shared_ptr<SoundLibraryDatabase> m_pSoundLibraryDatabase;

	std::shared_ptr<Playlist> m_pPlaylist;

	/** Controls the instrument selection within a hihat group. */
	Midi::Parameter m_hihatOpenness;

	/**
	 * Cache last incoming MIDI event to be used in #MidiSenseWidget.
	 */
	MidiEvent::Type m_lastMidiEvent;
	Midi::Parameter m_lastMidiEventParameter;

	/** The update of the Director is event-based. But if the widget is not
	 * visible (probably most of the time) we do omit the corresponding
	 * event for better performance. */
	bool m_bSendBbtChangeEvents;

};

inline bool Hydrogen::getIsExportSessionActive() const
{
	return m_bExportSessionIsActive;
}

inline AudioEngine* Hydrogen::getAudioEngine() const {
	return m_pAudioEngine;
}
inline std::shared_ptr<MidiActionManager> Hydrogen::getMidiActionManager() const {
	return m_pMidiActionManager;
}

inline const Hydrogen::GUIState& Hydrogen::getGUIState() const {
	return m_GUIState;
}

inline void Hydrogen::setGUIState( const Hydrogen::GUIState& state ) {
	m_GUIState = state;
}
inline int Hydrogen::getSelectedPatternNumber() const
{
	return m_nSelectedPatternNumber;
}
inline int Hydrogen::getSelectedInstrumentNumber() const
{
	return m_nSelectedInstrumentNumber;
}

inline bool Hydrogen::getRecordEnabled() const {
	return m_bRecordEnabled;
}
inline void Hydrogen::setRecordEnabled( bool bEnabled ) {
	m_bRecordEnabled = bEnabled;
}

inline const MidiEvent::Type& Hydrogen::getLastMidiEvent() const {
	return m_lastMidiEvent;
}
inline void Hydrogen::setLastMidiEvent( const MidiEvent::Type& event ) {
	m_lastMidiEvent = event;
}
inline Midi::Parameter Hydrogen::getLastMidiEventParameter() const {
	return m_lastMidiEventParameter;
}
inline void	Hydrogen::setLastMidiEventParameter( Midi::Parameter parameter ) {
	m_lastMidiEventParameter = parameter;
}
inline std::shared_ptr<Playlist> Hydrogen::getPlaylist() const {
	return m_pPlaylist;
}
inline void Hydrogen::setPlaylist( std::shared_ptr<Playlist> pPlaylist ){
	m_pPlaylist = pPlaylist;
}
inline std::shared_ptr<TimeHelper> Hydrogen::getTimeHelper() const {
	return m_pTimeHelper;
}
inline Midi::Parameter Hydrogen::getHihatOpenness() const {
	return m_hihatOpenness;
}
inline void Hydrogen::setHihatOpenness( Midi::Parameter openness ) {
	m_hihatOpenness = openness;
}
inline int Hydrogen::getBeatCounterTotalBeats() const {
	return m_nBeatCounterTotalBeats;
}
inline float Hydrogen::getBeatCounterBeatLength() const {
	return m_fBeatCounterBeatLength;
}
inline int Hydrogen::getBeatCounterEventCount() const {
	return m_nBeatCounterEventCount;
}
inline bool Hydrogen::getSendBbtChangeEvents() const {
	return m_bSendBbtChangeEvents;
}
inline void Hydrogen::setSendBbtChangeEvents( bool bSend ) {
	m_bSendBbtChangeEvents = bSend;
}
};

#endif

