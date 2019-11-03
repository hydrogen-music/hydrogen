/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef HYDROGEN_H
#define HYDROGEN_H

#include <stdint.h> // for uint32_t et al
#include "hydrogen/config.h"
#include <hydrogen/midi_action.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/object.h>
#include <hydrogen/timeline.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/IO/MidiOutput.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/core_action_controller.h>
#include <cassert>
#include <hydrogen/timehelper.h>
// Engine states  (It's ok to use ==, <, and > when testing)
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Not even the
 * constructors have been called.
 */
#define STATE_UNINITIALIZED	1
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Not ready,
 * but most pointers are now valid or NULL.
 */
#define STATE_INITIALIZED	2
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Drivers are
 * set up, but not ready to process audio.
 */
#define STATE_PREPARED		3
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Ready to
 * process audio.
 */
#define STATE_READY		4
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Currently
 * playing a sequence.
 */
#define STATE_PLAYING		5
inline int randomValue( int max );

namespace H2Core
{
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
	 * particular pattern @a pos from the Song::__pattern_list and
	 * either deletes it from #m_pNextPatterns if already present or
	 * add it to the same pattern list if not present yet.
	 *
	 * If the Song is not in Song::PATTERN_MODE or @a pos is not
	 * within the range of Song::__pattern_list, #m_pNextPatterns will
	 * be cleared instead.
	 *
	 * \param pos Index of a particular pattern in
	 * Song::__pattern_list, which should be added to
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
	 * from the Song::__pattern_list.
	 *
	 * If the Song is not in Song::PATTERN_MODE or @a pos is not
	 * within the range of Song::__pattern_list, #m_pNextPatterns will
	 * be just cleared.
	 *
	 * \param pos Index of a particular pattern in
	 * Song::__pattern_list, which should be added to
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
		Song*			getSong(){ return __song; }
		/**
		 * Sets the current song #__song to @a newSong.
		 * \param newSong Pointer to the new Song object.
		 */
		void			setSong	( Song *newSong );

		void			removeSong();

		void			addRealtimeNote ( int instrument,
							  float velocity,
							  float pan_L=1.0,
							  float pan_R=1.0,
							  float pitch=0.0,
							  bool noteoff=false,
							  bool forcePlay=false,
							  int msg1=0 );

		float			getMasterPeak_L();
		void			setMasterPeak_L( float value );

		float			getMasterPeak_R();
		void			setMasterPeak_R( float value );

		void			getLadspaFXPeak( int nFX, float *fL, float *fR );
		void			setLadspaFXPeak( int nFX, float fL, float fR );
	/**
	 * \return The global variable H2Core::m_nPatternTickPosition
	 */
	unsigned long		getTickPosition();
	/** Keep track of the tick position in realtime.
	 *
	 * Firstly, it gets the current transport position in frames
	 * #m_nRealtimeFrames and converts it into ticks using
	 * TransportInfo::m_nTickSize. Afterwards, it accesses how
	 * much time passed since the last update of
	 * #m_currentTickTime, converts the time difference +
	 * AudioOutput::getBufferSize()/ AudioOutput::getSampleRate()
	 * in frames, and adds the result to the first value to
	 * support keyboard and MIDI events as well.
	 *
	 * \return Current position in ticks.
	 */
	unsigned long		getRealtimeTickPosition();
	unsigned long		getTotalFrames();

	/** Sets #m_nRealtimeFrames
	 * \param frames Current transport realtime position*/
	void			setRealtimeFrames( unsigned long frames );
	/** Returns the current realtime transport position
	 * TransportInfo::m_nFrames.
	 * \return #m_nRealtimeFrames */
	unsigned long		getRealtimeFrames();
	/** \return #m_pPlayingPatterns*/
	PatternList *		getCurrentPatternList();
	/** 
	 * Sets #m_pPlayingPatterns.
	 *
	 * Before setting the variable it first locks the AudioEngine. In
	 * addition, it also pushes the Event #EVENT_PATTERN_CHANGED with
	 * the value -1 to the EventQueue.
	 *
	 * \param pPatternList Sets #m_pPlayingPatterns.*/
	void			setCurrentPatternList( PatternList * pPatternList );

	/** \return #m_pNextPatterns*/
	PatternList *		getNextPatterns();

		/** Get the position of the current Pattern in the Song.
		 * \return #m_nSongPos */
		int			getPatternPos();
		/**
		 * Relocate the position to another Pattern in the Song.
		 *
		 * The position of a Pattern in frames (see
		 * TransportInfo::m_nFrames for details) will be determined by
		 * retrieving the tick number the Pattern is located at using
		 * getTickForPosition() and multiplying it with
		 * TransportInfo::m_nTickSize. The resulting value will be
		 * used by the AudioOutput::locate() function of your audio
		 * driver to relocate the playback position.
		 *
		 * If #m_audioEngineState is not #STATE_PLAYING, the variables
		 * #m_nSongPos and #m_nPatternTickPosition will be set to @a
		 * pos and 0 right away.
		 *
		 * \param pos Position of the Pattern to relocate at. All
		 *   values smaller than -1 will be set to -1, which marks the
		 *   beginning of the Song.
		 */
		void			setPatternPos( int pos );
		/** Returns the pattern number corresponding to the tick
		 * position @a TickPos.
		 * \param TickPos Position in ticks.
		 * \return 
		 * - __0__ : if the Song isn't specified yet.
		 * - the output of the findPatternInTick() function called
		 *   with @a TickPos and Song::is_loop_enabled() as input
		 *   arguments.
		 */
		int			getPosForTick( unsigned long TickPos );
		/** Resetting #m_nPatternStartTick to -1 if the current Song
		    mode is Song::PATTERN_MODE
		*/
		void			triggerRelocateDuringPlay();
	
		/**
		 * Get the total number of ticks passed up to a Pattern at
		 * position @a pos.
		 *
		 * The function will loop over all and sums up their
		 * Pattern::__length. If one of the Pattern is NULL or no
		 * Pattern is present one of the PatternList, #MAX_NOTES will
		 * be added instead.
		 *
		 * The driver should be LOCKED when calling this!
		 *
		 * \param pos Position of the Pattern in the
		 *   Song::__pattern_group_sequence.
		 * \return
		 *  - -1 : if @a pos is bigger than the number of patterns in
		 *   the Song and Song::__is_loop_enabled is set to false or
		 *   no Patterns could be found at all.
		 *  - >= 0 : the total number of ticks passed.
		 */
		long			getTickForPosition( int pos );

		void			restartDrivers();

		AudioOutput*		getAudioOutput();
		MidiInput*		getMidiInput();
		MidiOutput*		getMidiOutput();

		/** Returns the current state of the audio engine.
		 * \return #m_audioEngineState*/
		int			getState();

		float			getProcessTime();
		float			getMaxProcessTime();

		int			loadDrumkit( Drumkit *pDrumkitInfo );
		int			loadDrumkit( Drumkit *pDrumkitInfo, bool conditional );

		/** Test if an Instrument has some Note in the Pattern (used to
		    test before deleting an Instrument)*/
		bool 			instrumentHasNotes( Instrument *pInst );

		/** Delete an Instrument. If @a conditional is true, and there
		    are some Pattern that are using this Instrument, it's not
		    deleted anyway.*/
		void			removeInstrument( int instrumentnumber, bool conditional );

		/** Return the name of the current Drumkit.*/
		QString			m_currentDrumkit;

		const QString&		getCurrentDrumkitname();
		void			setCurrentDrumkitname( const QString& currentdrumkitname );

		void			raiseError( unsigned nErrorCode );


void			previewSample( Sample *pSample );
	void			previewInstrument( Instrument *pInstr );

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
		JACK_ERROR_IN_PORT_REGISTER
	};

	void			onTapTempoAccelEvent();
	void			setTapTempo( float fInterval );
	/** 
	 * Updates the speed.
	 *
	 * It calls AudioOutput::setBpm() and setNewBpmJTM() with @a
	 * fBPM as input argument and sets Song::__bpm to @a fBPM.
	 *
	 * This function will be called with the AudioEngine in LOCKED
	 * state.
	 * \param fBPM New speed in beats per minute.
	 */
	void			setBPM( float fBPM );

	void			restartLadspaFX();
	/** 
	 * Same as getSelectedPatternNumber() without pushing an event.
	 * \param nPat Sets #m_nSelectedPatternNumber*/
	void			setSelectedPatternNumberWithoutGuiEvent( int nPat );
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
	void			renameJackPorts(Song* pSong);
#endif

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
	void			startOscServer();
	void			startNsmClient();
#endif

	// beatconter
	void			setbeatsToCount( int beatstocount);
	int			getbeatsToCount();
	void			setNoteLength( float notelength);
	float			getNoteLength();
	int			getBcStatus();
	void			handleBeatCounter();
	void			setBcOffsetAdjust();

	/** Returns the latest transport position of the JACK server.
	 * \return #m_nHumantimeFrames*/
	unsigned long		getHumantimeFrames();
	/** Sets #m_nHumantimeFrames.
	    \param hframes New transport position in frames.*/
	void			setHumantimeFrames(unsigned long hframes);
	/** Calling JackAudioDriver::com_release() directly from
	    the GUI*/
	void			offJackMaster();
	/** Calling JackAudioDriver::initTimeMaster() directly from
	    the GUI*/
	void			onJackMaster();
	/**
	 * Access the length of the first Pattern found in the
	 * PatternList at @a humanpos - 1.
	 *
	 * This function should also work if the loop mode is enabled
	 * in Song::is_loop_enabled().
	 *
	 * \param humanpos Position + 1 of the desired PatternList.
	 * \return 
	 * - __-1__ : if not Song was initialized yet.
	 * - #MAX_NOTES : if @a humanpos was smaller than 1, larger
	 * than the length of the vector of the PatternList in
	 * Song::__pattern_group_sequence or no Pattern could be found
	 * in the PatternList at @a humanpos - 1.
	 * - __else__ : length of first Pattern found at @a humanpos.
	 */
	long			getTickForHumanPosition( int humanpos );
	/** Returns the fallback speed.
	 * \return #m_nNewBpmJTM */
	float			getNewBpmJTM();
	/** Set the fallback speed #m_nNewBpmJTM.
	 * \param bpmJTM New default tempo. */ 
	void			setNewBpmJTM( float bpmJTM);

	void			__panic();
	unsigned int	__getMidiRealtimeNoteTickPosition();

	/**
	 * Updates Song::__bpm and #m_nNewBpmJTM to the local speed.
	 *
	 * To set the of the Song Song::__bpm, the local speed will be
	 * obtained by calling getTimelineBpm() with getPatternPos()
	 * as input argument. For setting the fallback speed
	 * #m_nNewBpmJTM, getRealtimeTickPosition() will be used
	 * instead.
	 *
	 * If Preferences::__useTimelineBpm is set to false, the
	 * function will return without performing any actions.
	 */
	void			setTimelineBpm();
	/**
	 * Returns the local speed at a specific @a Beat in the
	 * Timeline.
	 *
	 * Timeline::HTimelineVector::m_htimelinebpm of the first
	 * Timeline::HTimelineVector::m_htimelinebeat bigger than @a
	 * Beat will be returned.
	 *
	 * If Hydrogen is in Song::PATTERN_MODE or
	 * Preferences::__useTimelineBpm is set to false, the global
	 * speed of the current Song Song::__bpm or, if no Song is
	 * present yet, the result of getNewBpmJTM() will be
	 * returned. 
	 *
	 * Its counterpart is setTimelineBpm().
	 *
	 * \param Beat Position along the Timeline to access the tempo
	 *   at.
	 * \return Speed in beats per minute.
	 */
	float			getTimelineBpm( int Beat );
	/** \return #m_pTimeline*/
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
	 * Wrapper around Song::set_playback_track_enabled().
	 *
	 * \param state Whether the playback track is enabled. It will
	 * be replaced by false, if no Song was selected (getSong()
	 * return nullptr).
	 */
	bool			setPlaybackTrackState( const bool state );
	/**
	 * Wrapper around Song::get_playback_track_enabled().
	 *
	 * \return Whether the playback track is enabled or false, if
	 * no Song was selected (getSong() return nullptr).
	 */
	bool			getPlaybackTrackState();
	/**
	 * Wrapper function for loading the playback track.
	 *
	 * Calls Song::set_playback_track_filename() and
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

	/**\return #m_bActiveGUI*/
	bool			getActiveGUI() const;
	/**\param bActiveGUI Specifies whether the Qt5 GUI is active. Sets
	   #m_bActiveGUI.*/
	void			setActiveGUI( const bool bActiveGUI );
	
	/**\return #m_pNextSong*/
	Song*			getNextSong() const;
	/**\param pNextSong Sets #m_pNextSong. #Song which is about to be
	   loaded by the GUI.*/
	void			setNextSong( Song* pNextSong );

	///midi lookuptable
	int 			m_nInstrumentLookupTable[MAX_INSTRUMENTS];

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
	Song*			__song;

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
	timeval			m_LastTime;		///< timeval
	double			m_nLastBeatTime;	///< timediff
	double			m_nCurrentBeatTime;	///< timediff
	double			m_nBeatDiff;		///< timediff
	float			m_fBeatCountBpm;	///< bpm
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
	 * When a new #Song is set via the core part of Hydrogen, e.g. in
	 * the context of session management, the #Song *must* be set via
	 * the GUI if active. Else the GUI will freeze.
	 *
	 * Set by setActiveGUI() and accessed via getActiveGUI().
	 */
	bool			m_bActiveGUI;
	
	/**
	 * Stores a new #Song which is about of the loaded by the GUI.
	 *
	 * If #m_bActiveGUI is true, the core part of must not load a new
	 * #Song itself. Instead, the new #Song is prepared and stored in
	 * this object to be loaded by HydrogenApp::updateSongEvent() if
	 * H2Core::EVENT_UPDATE_SONG is pushed with a '1'.
	 *
	 * Set by setNextSong() and accessed via getNextSong().
	 */
	Song*			m_pNextSong;

	/**
	 * Local instance of the Timeline object.
	 */
	Timeline*		m_pTimeline;
	/**
	 * Local instance of the CoreActionController object.
	 */ 
	CoreActionController* 	m_pCoreActionController;
	
	/// Deleting instruments too soon leads to potential crashes.
	std::list<Instrument*> 	__instrument_death_row; 

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
	 *
	 * - Sets the current #__song to NULL
	 * - Sets #m_bExportSessionIsActive to false
	 * - Creates a new Timeline #m_pTimeline 
	 * - Creates a new CoreActionController
	 *   #m_pCoreActionController, 
	 * - Calls initBeatcounter(), audioEngine_init(), and
	 *   audioEngine_startAudioDrivers() 
	 * - Sets InstrumentComponent::m_nMaxLayers to
	 *   Preferences::m_nMaxLayers via
	 *   InstrumentComponent::setMaxLayers() and
	 *   Preferences::getMaxLayers() 
	 * - Starts the OscServer using OscServer::start() if
	 *   #H2CORE_HAVE_OSC was set during compilation.
	 * - Fills #m_nInstrumentLookupTable with the corresponding
	 *   index of each element.
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


inline const QString& Hydrogen::getCurrentDrumkitname()
{
	return m_currentDrumkit;
}

inline bool Hydrogen::getIsExportSessionActive() const
{
	return m_bExportSessionIsActive;
}

inline void Hydrogen::setCurrentDrumkitname( const QString& currentdrumkitname )
{
	this->m_currentDrumkit = currentdrumkitname;
}

inline bool Hydrogen::getPlaybackTrackState()
{
	Song* pSong = getSong();
	bool  bState;

	if(!pSong){
		bState = false;
	} else {
		bState = pSong->get_playback_track_enabled();
	}
	return 	bState;
}

inline bool Hydrogen::getActiveGUI() const {
	return m_bActiveGUI;
}
inline void Hydrogen::setActiveGUI( const bool bActiveGUI ) {
	m_bActiveGUI = bActiveGUI;
}
inline Song* Hydrogen::getNextSong() const {
	return m_pNextSong;
}
inline void Hydrogen::setNextSong( Song* pNextSong ) {
	m_pNextSong = pNextSong;
}

};

#endif

