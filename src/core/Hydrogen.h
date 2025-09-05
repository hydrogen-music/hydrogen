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
#ifndef HYDROGEN_H
#define HYDROGEN_H

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/Helpers/Time.h>
#include <core/IO/JackAudioDriver.h>
#include <core/Midi/MidiMessage.h>
#include <core/Object.h>
#include <core/Timeline.h>

#include <stdint.h> // for uint32_t et al
#include <cassert>
#include <chrono>
#include <memory>

namespace H2Core
{
	class AudioEngine;
	class AudioOutput;
	class Drumkit;
	class MidiBaseDriver;
	class MidiActionManager;
	class Playlist;
	class SoundLibraryDatabase;

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
		Jack = 3
	};

	enum ErrorMessages {
		/**
		 * The provided input in createDriver() does not match any of the
		 * choices for #H2Core::Preferences::AudioDriver.
		 */
		UNKNOWN_DRIVER,
		/**
		 * Unable to connect the audio driver stored. The NullDriver will be
		 * used as a fallback instead.
		 */
		ERROR_STARTING_DRIVER,
		JACK_SERVER_SHUTDOWN,
		JACK_CANNOT_ACTIVATE_CLIENT,
		/**
		 * Unable to connect either the
		 * JackAudioDriver::output_port_1 and the
		 * JackAudioDriver::output_port_name_1 as well as the
		 * JackAudioDriver::output_port_2 and the
		 * JackAudioDriver::output_port_name_2 port using
		 * _jack_connect()_ (jack/jack.h) or the fallback
		 * version using the first two input ports in
		 * JackAudioDriver::connect().
		 */
		JACK_CANNOT_CONNECT_OUTPUT_PORT,
		/**
		 * The client of Hydrogen can not be disconnected from
		 * the JACK server using _jack_client_close()_
		 * (jack/jack.h). Used within JackAudioDriver::disconnect().
		 */
		JACK_CANNOT_CLOSE_CLIENT,
		/**
		 * Unable to register output ports for the JACK client
		 * using _jack_port_register()_ (jack/jack.h) in
		 * JackAudioDriver::init() or
		 * JackAudioDriver::setTrackOutput().
		 */
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
	 * Creates all the instances used within Hydrogen in the right
	 * order.
	 */
	static void		create_instance();
	/**
	 * Returns the current Hydrogen instance #__instance.
	 */
	static Hydrogen*	get_instance(){ return __instance; };

	/**
	 * Destructor taking care of most of the clean up.
	 */
	~Hydrogen();

	/*
	 * return central instance of the audio engine
	 */
	AudioEngine*		getAudioEngine() const;
		std::shared_ptr<MidiActionManager> getMidiActionManager() const;
	std::shared_ptr<SoundLibraryDatabase> getSoundLibraryDatabase() const {
		return m_pSoundLibraryDatabase;
	}
	std::shared_ptr<Playlist> getPlaylist() const;
	void setPlaylist( std::shared_ptr<Playlist> pPlaylist );

// ***** SEQUENCER ********
	/// Start the internal sequencer
	void			sequencerPlay();

	/// Stop the internal sequencer
	void			sequencerStop();

	///Last received midi message
	const MidiMessage::Event& getLastMidiEvent() const;
	void				setLastMidiEvent( const MidiMessage::Event& event );
	int					getLastMidiEventParameter() const;
	void				setLastMidiEventParameter( int nParam );

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

		bool			addRealtimeNote ( int instrument,
							  float velocity,
							  bool noteoff=false,
							  int msg1=0 );

		int getHihatOpenness() const;
		void setHihatOpenness( int nValue );

		void			restartAudioDriver();
		void			restartMidiDriver();

		AudioOutput*	getAudioOutput() const;
		std::shared_ptr<MidiBaseDriver>	getMidiDriver() const;

	/** Wrapper around Song::setIsModified() that checks whether a
		song is set.*/
	void setIsModified( bool bIsModified );
	/** Wrapper around Song::getIsModified() that checks whether a
		song is set.*/
	bool getIsModified() const;

	void			onTapTempoAccelEvent( TimePoint start = TimePoint() );

	void			restartLadspaFX();
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

	void	renameJackPorts( std::shared_ptr<Song> pSong,
							 std::shared_ptr<Drumkit> pOldDrumkit = nullptr );

	/** Starts/stops the OSC server
	 * \param bEnable `true` = start, `false` = stop.*/
	void			toggleOscServer( bool bEnable );
	/** Destroys and recreates the OscServer singleton in order to
		adopt a new OSC port.*/
	void			recreateOscServer();
	void			startNsmClient();

	// beatconter
	void			setBeatCounterTotalBeats( int nTotalBeats );
	int			getBeatCounterTotalBeats() const;
	void			setBeatCounterBeatLength( float fBeatLength );
	float			getBeatCounterBeatLength() const;
	int			getBeatCounterEventCount() const;
	bool			handleBeatCounter( TimePoint start = TimePoint() );
	void			updateBeatCounterSettings();

	/** Calling JackAudioDriver::releaseTimebaseControl() directly from
	    the GUI*/
	void			releaseJackTimebaseControl();
	/** Calling JackAudioDriver::initTimebaseControl() directly from
	    the GUI*/
	void			initJackTimebaseControl();

	void			panic();
	std::shared_ptr<Timeline>	getTimeline() const;
	void			setTimeline( std::shared_ptr<Timeline> );
	
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
	void			startExportSong( const QString& filename );
	void			stopExportSong();
	
	/************************************************************/
	/********************** Playback track **********************/
	/**
	 * Wrapper around Song::setPlaybackTrackEnabled().
	 */
	void			mutePlaybackTrack( const bool bMuted );
	/**
	 * Wrapper around Song::getPlaybackTrackState().
	 */
	Song::PlaybackTrack		getPlaybackTrackState() const;
	/**
	 * Wrapper function for loading the playback track.
	 */
	void			loadPlaybackTrack( const QString& sFilename );
	/************************************************************/

	/**\return #m_GUIState*/
	const GUIState&	getGUIState() const;
	/**\param state Specifies whether the Qt5 GUI is active. Sets
	   #m_GUIState.*/
	void			setGUIState( const GUIState& state );
	/**
	 * \return Whether JackAudioDriver is used as current audio
	 * driver.
	 */
	bool			hasJackAudioDriver() const;
	/**
	 * \return Whether JackAudioDriver is used as current audio driver
	 * and JACK transport was activated via the GUI
	 * (#H2Core::Preferences::m_nJackTransportMode).
	 */
	bool			hasJackTransport() const;
        float			getJackTimebaseControllerBpm() const;

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
	
	/**
	 * \return Whether we hasJackTransport() and there is an external JACK
	 *   Timebase controller broadcasting tempo information. If so, we disregard
	 *   Hydrogen's Timeline information (see
	 *   #H2Core::JackAudioDriver::m_timebaseState).
	 */
	JackAudioDriver::Timebase		getJackTimebaseState() const;
	/** \return NsmClient::m_bUnderSessionManagement if NSM is
		supported.*/
	bool			isUnderSessionManagement() const;

		bool getRecordEnabled() const;
		void setRecordEnabled( bool bEnabled );

	void			setSessionIsExported( bool bIsExported );
	bool			getSessionIsExported() const;

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
	void updateVirtualPatterns();

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

	/**
	 * Constructor, entry point, and initialization of the
	 * Hydrogen application.
	 *
	 * It is called by the main() function after setting up a
	 * bunch of Qt5 stuff and creating an instance of the Logger
	 * and Preferences.
	 *
	 * Only one Hydrogen object is allowed to exist. If the
	 * #__instance object is present, the constructor will throw
	 * an error.
	 */
	Hydrogen();

		void killInstruments();

	void			midiNoteOn( std::shared_ptr<Note> pNote );

	/**
	 * Static reference to the Hydrogen singleton. 
	 *
	 * It is created using the Hydrogen::Hydrogen() constructor,
	 * initialized with NULL and assigned a new Hydrogen instance
	 * if still 0 in create_instance().
	 */
	static Hydrogen* 	__instance;

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
	
	/**
	 * Local instance of the Timeline object.
	 */
	std::shared_ptr<Timeline>	m_pTimeline;

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
	 * Indicates whether NSM session is saved or exported when entering
	 * the CoreActionController::saveSong() function.
	 */
	bool			m_bSessionIsExported;

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

	std::shared_ptr<SoundLibraryDatabase> m_pSoundLibraryDatabase;

	std::shared_ptr<Playlist> m_pPlaylist;

		/** Controls the instrument selection within a hihat group. */
		int m_nHihatOpenness;

	/**
	 * Cache last incoming MIDI event to be used in #MidiSenseWidget.
	 */
	MidiMessage::Event m_lastMidiEvent;
	int					m_nLastMidiEventParameter;

		/** The update of the Director is event-based. But if the widget is not
		 * visible (probably most of the time) we do omit the corresponding
		 * event for better performance. */
		bool m_bSendBbtChangeEvents;

};


/*
 * inline methods
 */
inline std::shared_ptr<Timeline> Hydrogen::getTimeline() const
{
	return m_pTimeline;
}
inline void Hydrogen::setTimeline( std::shared_ptr<Timeline> pTimeline )
{
	m_pTimeline = pTimeline;
}

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

inline void Hydrogen::setSessionIsExported( bool bSessionIsExported ) {
	m_bSessionIsExported = bSessionIsExported;
}
inline bool Hydrogen::getSessionIsExported() const {
	return m_bSessionIsExported;
}
inline const MidiMessage::Event& Hydrogen::getLastMidiEvent() const {
	return m_lastMidiEvent;
}
inline void Hydrogen::setLastMidiEvent( const MidiMessage::Event& event ) {
	m_lastMidiEvent = event;
}
inline int Hydrogen::getLastMidiEventParameter() const {
	return m_nLastMidiEventParameter;
}
inline void	Hydrogen::setLastMidiEventParameter( int nParam ) {
	m_nLastMidiEventParameter = nParam;
}
inline std::shared_ptr<Playlist> Hydrogen::getPlaylist() const {
	return m_pPlaylist;
}
inline void Hydrogen::setPlaylist( std::shared_ptr<Playlist> pPlaylist ){
	m_pPlaylist = pPlaylist;
}
inline int Hydrogen::getHihatOpenness() const {
	return m_nHihatOpenness;
}
inline void Hydrogen::setHihatOpenness( int nValue ) {
	m_nHihatOpenness = std::clamp( nValue, 0, 127 );
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

