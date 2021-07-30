/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/config.h>
#include <core/Basics/Song.h>
#include <core/Basics/Sample.h>
#include <core/Object.h>
#include <core/Timeline.h>
#include <core/IO/AudioOutput.h>
#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>
#include <core/IO/JackAudioDriver.h>
#include <core/Basics/Drumkit.h>
#include <core/CoreActionController.h>
#include <core/Timehelper.h>
#include <core/AudioEngine/AudioEngine.h>

#include <stdint.h> // for uint32_t et al
#include <cassert>
#include <memory>

inline int randomValue( int max );

namespace H2Core
{
	class CoreActionController;
///
/// Hydrogen Audio Engine.
///
class Hydrogen : public H2Core::Object
{
	H2_OBJECT
public:
	
	/**
	 * Creates all the instances used within Hydrogen in the right
	 * order. 
	 *
	 * -# H2Core::Logger::create_instance()
	 * -# MidiMap::create_instance()
	 * -# Preferences::create_instance()
	 * -# EventQueue::create_instance()
	 * -# MidiActionManager::create_instance()
	 *
	 * If #H2CORE_HAVE_OSC was set during compilation, the
	 * following instances will be created as well.
	 *
	 * -# NsmClient::create_instance()
	 * -# OscServer::create_instance() using
	 *    Preferences::get_instance() as input
	 *
	 * If all instances are created and the actual Hydrogen
	 * instance #__instance is still 0, it will be properly
	 * constructed via Hydrogen().
	 *
	 * The AudioEngine::create_instance(),
	 * Effects::create_instance(), and Playlist::create_instance()
	 * functions will be called from within audioEngine_init().
	 */
	static void		create_instance();
	/**
	 * Returns the current Hydrogen instance #__instance.
	 */
	static Hydrogen*	get_instance(){ assert(__instance); return __instance; };

	/*
	 * return central instance of the audio engine
	 */
	AudioEngine*		getAudioEngine() const;

	/**
	 * Destructor taking care of most of the clean up.
	 *
	 * -# Shuts down the NsmClient using NsmClient::shutdown() and
              deletes it.
	 * -# Deletes the OscServer object.
	 * -# Stops the AudioEngine if playing via audioEngine_stop().
	 * -# Calls removeSong(), audioEngine_stopAudioDrivers(),
              audioEngine_destroy(), __kill_instruments()
         * -# Deletes the #m_pCoreActionController and #m_pTimeline
              object
	 * -# Sets #__instance to NULL.
	 */
	~Hydrogen();

// ***** SEQUENCER ********
	/// Start the internal sequencer
	void			sequencer_play();

	/// Stop the internal sequencer
	void			sequencer_stop();

	void			midi_noteOn( Note *note );

	///Last received midi message
	QString			lastMidiEvent;
	int				lastMidiEventParameter;

	// TODO: more descriptive name since it is able to both delete and
	// add a pattern. Possibly without the sequencer_ prefix for
	// consistency.
	/**
	 * Adding and removing a Pattern from #m_pNextPatterns.
	 *
	 * After locking the AudioEngine the function retrieves the
	 * particular pattern @a pos from the Song::m_pPatternList and
	 * either deletes it from #m_pNextPatterns if already present or
	 * add it to the same pattern list if not present yet.
	 *
	 * If the Song is not in Song::PATTERN_MODE or @a pos is not
	 * within the range of Song::m_pPatternList, #m_pNextPatterns will
	 * be cleared instead.
	 *
	 * \param pos Index of a particular pattern in
	 * Song::m_pPatternList, which should be added to
	 * #m_pNextPatterns.
	 */
	void			sequencer_setNextPattern( int pos );
	// TODO: Possibly without the sequencer_ prefix for consistency.
	/**
	 * Clear #m_pNextPatterns and add one Pattern.
	 *
	 * After locking the AudioEngine the function clears
	 * #m_pNextPatterns, fills it with all currently played one in
	 * #m_pPlayingPatterns, and appends the particular pattern @a pos
	 * from the Song::m_pPatternList.
	 *
	 * If the Song is not in Song::PATTERN_MODE or @a pos is not
	 * within the range of Song::m_pPatternList, #m_pNextPatterns will
	 * be just cleared.
	 *
	 * \param pos Index of a particular pattern in
	 * Song::m_pPatternList, which should be added to
	 * #m_pNextPatterns.
	 */
	void			sequencer_setOnlyNextPattern( int pos );
	/**
	 * Switches playback to focused pattern.
	 *
	 * If the current Song is in Song::PATTERN_MODE, the AudioEngine
	 * will be locked and Preferences::m_bPatternModePlaysSelected
	 * negated. If the latter was true before calling this function,
	 * #m_pPlayingPatterns will be cleared and replaced by the
	 * Pattern indexed with #m_nSelectedPatternNumber.
	 *
	 * This function will be called either by MainForm::eventFilter()
	 * when pressing Qt::Key_L or by
	 * SongEditorPanel::modeActionBtnPressed().
	 */
	void			togglePlaysSelected();
	
		/**
		 * Get the current song.
		 * \return #__song
		 */ 	
		std::shared_ptr<Song>			getSong() const{ return __song; }
		/**
		 * Sets the current song #__song to @a newSong.
		 * \param newSong Pointer to the new Song object.
		 */
		void			setSong	( std::shared_ptr<Song> newSong );

		void			removeSong();

		void			addRealtimeNote ( int instrument,
							  float velocity,
							  float fPan = 0.0f,
							  float pitch=0.0,
							  bool noteoff=false,
							  bool forcePlay=false,
							  int msg1=0 );

		void			restartDrivers();

		AudioOutput*		getAudioOutput() const;
		MidiInput*		getMidiInput() const;
		MidiOutput*		getMidiOutput() const;

		/** Returns the current state of the audio engine.
		 * \return #m_audioEngineState*/
		int			getState() const;

		/** Wrapper around loadDrumkit( Drumkit, bool ) with the
			conditional argument set to true.
		 *
		 * \returns 0 In case something unexpected happens, it will be
		 *   indicated with #ERRORLOG messages.
		 */
		int			loadDrumkit( Drumkit *pDrumkitInfo );
		/** Loads the H2Core::Drumkit provided in \a pDrumkitInfo into
		 * the current session.
		 *
		 * When under session management (see
		 * NsmClient::m_bUnderSessionManagement) the function will
		 * create a symlink to the loaded H2Core::Drumkit using the
		 * name "drumkit" in the folder
		 * NsmClient::m_sSessionFolderPath.
		 *
		 * \param pDrumkitInfo Full-fledged H2Core::Drumkit to load.
		 * \param conditional Argument passed on as second input
		 *   argument to removeInstrument().
		 *
		 * \returns 0 In case something unexpected happens, it will be
		 *   indicated with #ERRORLOG messages.
		 */

		int			loadDrumkit( Drumkit *pDrumkitInfo, bool conditional );

		/** Test if an Instrument has some Note in the Pattern (used to
		    test before deleting an Instrument)*/
		bool 			instrumentHasNotes( std::shared_ptr<Instrument> pInst );

		/** Delete an Instrument. If @a conditional is true, and there
		    are some Pattern that are using this Instrument, it's not
		    deleted anyway.*/
		void			removeInstrument( int instrumentnumber, bool conditional );

		/** \return m_sCurrentDrumkitName */
		const QString&	getCurrentDrumkitName();
		/** \param sName sets m_sCurrentDrumkitName */
		void			setCurrentDrumkitName( const QString& sName );
		/** \return m_currentDrumkitLookup */
		Filesystem::Lookup	getCurrentDrumkitLookup();
		/** \param lookup sets m_currentDrumkitLookup */
		void			setCurrentDrumkitLookup( Filesystem::Lookup lookup );

		void			raiseError( unsigned nErrorCode );


void			previewSample( Sample *pSample );
	void			previewInstrument( std::shared_ptr<Instrument> pInstr );

	enum ErrorMessages {
		/**
		 * The provided input string in createDriver() does
		 * not match any of the choices for
		 * Preferences::m_sAudioDriver.
		 */
		UNKNOWN_DRIVER,
		/**
		 * Unable to connect the audio driver stored in
		 * #m_pAudioDriver in
		 * audioEngine_startAudioDrivers(). The NullDriver
		 * will be used as a fallback instead.
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
		OSC_CANNOT_CONNECT_TO_PORT
	};

	void			onTapTempoAccelEvent();
	void			setTapTempo( float fInterval );
	/** 
	 * Updates the speed.
	 *
	 * It calls AudioOutput::setBpm() and setNewBpmJTM() with @a
	 * fBPM as input argument and sets Song::m_fBpm to @a fBPM.
	 *
	 * This function will be called with the AudioEngine in LOCKED
	 * state.
	 * \param fBPM New speed in beats per minute.
	 */
	void			setBPM( float fBPM );

	void			restartLadspaFX();
	/** \return #m_nSelectedPatternNumber*/
	int				getSelectedPatternNumber();
	/**
	 * Sets #m_nSelectedPatternNumber.
	 *
	 * If Preferences::m_pPatternModePlaysSelected is set to true, the
	 * AudioEngine is locked before @a nPat will be assigned. But in
	 * any case the function will push the
	 * #EVENT_SELECTED_PATTERN_CHANGED Event to the EventQueue.
	 *
	 * If @a nPat is equal to #m_nSelectedPatternNumber, the function
	 * will return right away.
	 *
	 *\param nPat Sets #m_nSelectedPatternNumber*/
	void			setSelectedPatternNumber( int nPat );

	int				getSelectedInstrumentNumber();
	void			setSelectedInstrumentNumber( int nInstrument );


	void			refreshInstrumentParameters( int nInstrument );

#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_
	/**
	 * Calls audioEngine_renameJackPorts() if
	 * Preferences::m_bJackTrackOuts is set to true.
	 * \param pSong Handed to audioEngine_renameJackPorts().
	 */
	void			renameJackPorts(std::shared_ptr<Song> pSong);
#endif

	/** Starts/stops the OSC server
	 * \param bEnable `true` = start, `false` = stop.*/
	void			toggleOscServer( bool bEnable );
	/** Destroys and recreates the OscServer singleton in order to
		adopt a new OSC port.*/
	void			recreateOscServer();
	void			startNsmClient();

	// beatconter
	void			setbeatsToCount( int beatstocount);
	int			getbeatsToCount();
	void			setNoteLength( float notelength);
	float			getNoteLength();
	int			getBcStatus();
	void			handleBeatCounter();
	void			setBcOffsetAdjust();

	/** Calling JackAudioDriver::releaseTimebaseMaster() directly from
	    the GUI*/
	void			offJackMaster();
	/** Calling JackAudioDriver::initTimebaseMaster() directly from
	    the GUI*/
	void			onJackMaster();
	/** Returns the fallback speed.
	 * \return #m_fNewBpmJTM */
	float			getNewBpmJTM() const;
	/** Set the fallback speed #m_nNewBpmJTM.
	 * \param bpmJTM New default tempo. */ 
	void			setNewBpmJTM( float bpmJTM);

	void			__panic();
	/**
	 * Updates Song::m_fBpm, TransportInfo::m_fBPM, and #m_fNewBpmJTM
	 * to the local speed.
	 *
	 * If Preferences::__useTimelineBpm is set to false or Hydrogen
	 * uses JACK transport in the presence of an external timebase
	 * master, the function will return without performing any
	 * actions.
	 */
	void			setTimelineBpm();
	/**
	 * Returns the local speed at a specific @a nBar in the
	 * Timeline.
	 *
	 * If Hydrogen is in Song::PATTERN_MODE or
	 * Preferences::__useTimelineBpm is set to false, the global
	 * speed of the current Song Song::m_fBpm or, if no Song is
	 * present yet, the result of getNewBpmJTM() will be
	 * returned. 
	 *
	 * Its counterpart is setTimelineBpm().
	 *
	 * \param nBar Position (in whole patterns) along the Timeline to
	 *   access the tempo at.
	 *
	 * \return Speed in beats per minute.
	 */
	float			getTimelineBpm( int nBar );
	Timeline*		getTimeline() const;
	
	//export management
	bool			getIsExportSessionActive() const;
	void			startExportSession( int rate, int depth );
	void			stopExportSession();
	void			startExportSong( const QString& filename );
	void			stopExportSong();
	
	CoreActionController* 	getCoreActionController() const;

	/************************************************************/
	/********************** Playback track **********************/
	/**
	 * Wrapper around Song::setPlaybackTrackEnabled().
	 *
	 * \param state Whether the playback track is enabled. It will
	 * be replaced by false, if no Song was selected (getSong()
	 * return nullptr).
	 */
	bool			setPlaybackTrackState( const bool state );
	/**
	 * Wrapper around Song::getPlaybackTrackEnabled().
	 *
	 * \return Whether the playback track is enabled or false, if
	 * no Song was selected (getSong() return nullptr).
	 */
	bool			getPlaybackTrackState() const;
	/**
	 * Wrapper function for loading the playback track.
	 *
	 * Calls Song::setPlaybackTrackFilename() and
	 * Sampler::reinitialize_playback_track(). While the former
	 * one is responsible to store metadata about the playback
	 * track, the latter one does load it to a new
	 * InstrumentLayer. The function is called by
	 * SongEditorPanel::editPlaybackTrackBtnPressed()
	 *
	 * \param filename Name of the file to load as the playback
	 * track
	 */
	void			loadPlaybackTrack( const QString filename );

	/** Specifies the state of the Qt GUI*/
	enum class		GUIState {
		/**There is a GUI but it is not ready yet (during startup).*/
		notReady = -1,
		/**No GUI available.*/
		unavailable = 0,
		/**There is a working GUI.*/
		ready = 1
	};
	
	/**\return #m_GUIState*/
	GUIState		getGUIState() const;
	/**\param state Specifies whether the Qt5 GUI is active. Sets
	   #m_GUIState.*/
	void			setGUIState( const GUIState state );
	/**
	 * \return Whether JackAudioDriver is used as current audio
	 * driver.
	 */
	bool			haveJackAudioDriver() const;
	/**
	 * \return Whether JackAudioDriver is used as current audio driver
	 * and JACK transport was activated via the GUI
	 * (#Preferences::m_bJackTransportMode).
	 */
	bool			haveJackTransport() const;
	/**
	 * \return Whether we haveJackTransport() and there is an external
	 * JACK timebase master broadcasting us tempo information and
	 * making use disregard Hydrogen's Timeline information (see
	 * #JackAudioDriver::m_timebaseState).
	 */
	JackAudioDriver::Timebase		getJackTimebaseState() const;
	/** \return NsmClient::m_bUnderSessionManagement if NSM is
		supported.*/
	bool			isUnderSessionManagement() const;

	///midi lookuptable
	int 			m_nInstrumentLookupTable[MAX_INSTRUMENTS];
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix, bool bShort = true ) const override;

private:
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
	std::shared_ptr<Song>			__song;

	/**
	 * Auxiliary function setting a bunch of global variables.
	 *
	 * - #m_ntaktoMeterCompute = 1;
	 * - #m_nbeatsToCount = 4;
	 * - #m_nEventCount = 1;
	 * - #m_nTempoChangeCounter = 0;
	 * - #m_nBeatCount = 1;
	 * - #m_nCoutOffset = 0;
	 * - #m_nStartOffset = 0;
	 */
	void initBeatcounter();

	// beatcounter
	float			m_ntaktoMeterCompute;	///< beatcounter note length
	int			m_nbeatsToCount;	///< beatcounter beats to count
	int			m_nEventCount;		///< beatcounter event
	int			m_nTempoChangeCounter;	///< count tempochanges for timeArray
	int			m_nBeatCount;		///< beatcounter beat to count
	double			m_nBeatDiffs[16];	///< beat diff
	timeval 		m_CurrentTime;		///< timeval
	int			m_nCoutOffset;		///ms default 0
	int			m_nStartOffset;		///ms default 0
	//~ beatcounter


	// used for song export
	Song::SongMode		m_oldEngineMode;
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
	Timeline*		m_pTimeline;
	/**
	 * Local instance of the CoreActionController object.
	 */ 
	CoreActionController* 	m_pCoreActionController;

	/** Name of the currently used Drumkit.*/
	QString			m_sCurrentDrumkitName;
	/** Whether the current Drumkit is located at user or system
		level.*/
	Filesystem::Lookup	m_currentDrumkitLookup;
	
	/// Deleting instruments too soon leads to potential crashes.
	std::list<std::shared_ptr<Instrument>> 	__instrument_death_row; 
	
	/**
	 * Fallback speed in beats per minute.
	 *
	 * It is set by Hydrogen::setNewBpmJTM() and accessed via
	 * Hydrogen::getNewBpmJTM().
	 */
	float				m_fNewBpmJTM;

	/**
	 * Instrument currently focused/selected in the GUI. 
	 *
	 * Within the core it is relevant for the MIDI input. Using
	 * Preferences::__playselectedinstrument incoming MIDI signals can be
	 * used to play back only the selected instrument or the whole
	 * drumkit.
	 *
	 * Queried using Hydrogen::getSelectedInstrumentNumber() and set by
	 * Hydrogen::setSelectedInstrumentNumber().
	 */
	int				m_nSelectedInstrumentNumber;

	/*
	 * Central instance of the audio engine. 
	 */
	AudioEngine*	m_pAudioEngine;

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

	void __kill_instruments();

};


/*
 * inline methods
 */
inline Timeline* Hydrogen::getTimeline() const
{
	return m_pTimeline;
}

inline CoreActionController* Hydrogen::getCoreActionController() const
{
	return m_pCoreActionController;
}


inline const QString& Hydrogen::getCurrentDrumkitName()
{
	return m_sCurrentDrumkitName;
}
inline void Hydrogen::setCurrentDrumkitName( const QString& sName )
{
	m_sCurrentDrumkitName = sName;
}
inline Filesystem::Lookup Hydrogen::getCurrentDrumkitLookup()
{
	return m_currentDrumkitLookup;
}
inline void Hydrogen::setCurrentDrumkitLookup( Filesystem::Lookup lookup )
{
	m_currentDrumkitLookup = lookup;
}

inline bool Hydrogen::getIsExportSessionActive() const
{
	return m_bExportSessionIsActive;
}

inline AudioEngine* Hydrogen::getAudioEngine() const {
	return m_pAudioEngine;
}

inline bool Hydrogen::getPlaybackTrackState() const
{
	std::shared_ptr<Song> pSong = getSong();
	bool  bState;

	if(!pSong){
		bState = false;
	} else {
		bState = pSong->getPlaybackTrackEnabled();
	}
	return 	bState;
}

inline Hydrogen::GUIState Hydrogen::getGUIState() const {
	return m_GUIState;
}

inline void Hydrogen::setGUIState( const Hydrogen::GUIState state ) {
	m_GUIState = state;
}
};

#endif

