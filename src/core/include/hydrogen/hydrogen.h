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
 * constructors have been called. Numerical value: 1.
 */
#define STATE_UNINITIALIZED	1
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Not ready,
 * but most pointers are now valid or NULL. Numerical value: 2.
 */
#define STATE_INITIALIZED	2
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Drivers are
 * set up, but not ready to process audio. Numerical value: 3.
 */
#define STATE_PREPARED		3
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Ready to
 * process audio. Numerical value: 4.
 */
#define STATE_READY		4
/**
 * State of the H2Core::AudioEngine H2Core::m_audioEngineState. Currently
 * playing a sequence. Numerical value: 5.
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
	 * If H2CORE_HAVE_OSC was set during compilation, the
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
	int			lastMidiEventParameter;

	void			sequencer_setNextPattern( int pos );
	void			togglePlaysSelected( void );
// ***** ~SEQUENCER ********

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

	unsigned long		getTickPosition();
	unsigned long		getRealtimeTickPosition();
	unsigned long		getTotalFrames();

	void			setRealtimeFrames( unsigned long frames );
	unsigned long		getRealtimeFrames();

	PatternList *		getCurrentPatternList();
	void			setCurrentPatternList( PatternList * pPatternList );

	PatternList *		getNextPatterns();

	int			getPatternPos();
	void			setPatternPos( int pos );
	int			getPosForTick( unsigned long TickPos );

	void			triggerRelocateDuringPlay();

	long			getTickForPosition( int );

	void			restartDrivers();

	AudioOutput*		getAudioOutput();
	MidiInput*		getMidiInput();
	MidiOutput*		getMidiOutput();

	int			getState();

	float			getProcessTime();
	float			getMaxProcessTime();

	int			loadDrumkit( Drumkit *pDrumkitInfo );
	int			loadDrumkit( Drumkit *pDrumkitInfo, bool conditional );

	//  Test if an instrument has notes in the pattern (used to test before deleting an insturment)
	bool 			instrumentHasNotes( Instrument *pInst );

	/// delete an instrument. If `conditional` is true, and there are patterns that
	/// use this instrument, it's not deleted anyway
	void			removeInstrument( int instrumentnumber, bool conditional );

	//return the name of the current drumkit
	QString			m_currentDrumkit;

	const QString&		getCurrentDrumkitname();
	void			setCurrentDrumkitname( const QString& currentdrumkitname );

	void			raiseError( unsigned nErrorCode );


	void			previewSample( Sample *pSample );
	void			previewInstrument( Instrument *pInstr );

	enum ErrorMessages {
		UNKNOWN_DRIVER,
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
		 * Unable to register the "out_L" and/or "out_R"
		 * output ports for the JACK client using
		 * _jack_port_register()_ (jack/jack.h) in
		 * JackAudioDriver::init().
		 */
		JACK_ERROR_IN_PORT_REGISTER
	};

	void			onTapTempoAccelEvent();
	void			setTapTempo( float fInterval );
	void			setBPM( float fBPM );

	void			restartLadspaFX();
	void			setSelectedPatternNumberWithoutGuiEvent( int nPat );
	int			getSelectedPatternNumber();
	void			setSelectedPatternNumber( int nPat );

	int			getSelectedInstrumentNumber();
	void			setSelectedInstrumentNumber( int nInstrument );


	void			refreshInstrumentParameters( int nInstrument );

#if defined(H2CORE_HAVE_JACK) || _DOXYGEN_
	void			renameJackPorts(Song* pSong);
#endif

#if defined(H2CORE_HAVE_OSC) || _DOXYGEN_
	void			startOscServer();
	void			startNsmClient();
#endif

	///beatconter
	void			setbeatsToCount( int beatstocount);
	int			getbeatsToCount();
	void			setNoteLength( float notelength);
	float			getNoteLength();
	int			getBcStatus();
	void			handleBeatCounter();
	void			setBcOffsetAdjust();

	/// jack time master
	unsigned long		getHumantimeFrames();
	void			setHumantimeFrames(unsigned long hframes);
	void			offJackMaster();
	void			onJackMaster();
	unsigned long		getTimeMasterFrames();
	long			getTickForHumanPosition( int humanpos );
	float			getNewBpmJTM();
	void			setNewBpmJTM( float bpmJTM);
	void			ComputeHumantimeFrames(uint32_t nFrames);

	void			__panic();
	int			__get_selected_PatterNumber();
	unsigned int		__getMidiRealtimeNoteTickPosition();

	void			setTimelineBpm();
	float			getTimelineBpm( int Beat );
	Timeline*		getTimeline() const;
	
	//export management
	bool			getIsExportSessionActive() const;
	void			startExportSession( int rate, int depth );
	void			stopExportSession();
	void			startExportSong( const QString& filename );
	void			stopExportSong();
	
	CoreActionController* 	getCoreActionController() const;

	///playback track
	bool			setPlaybackTrackState(bool);
	bool			getPlaybackTrackState();
	void			loadPlaybackTrack(QString filename);


	///midi lookuptable
	int 			m_nInstrumentLookupTable[MAX_INSTRUMENTS];

private:
	/**
	 * Static reference to the Hydrogen singleton. It is created
	 * using the Hydrogen::Hydrogen() constructor.
	 *
	 * It is initialized with NULL and assigned a new Hydrogen
	 * instance if still 0 in create_instance().
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
	 * - Sets InstrumentComponent::maxLayers to
	 *   Preferences::maxLayers via
	 *   InstrumentComponent::setMaxLayers() and
	 *   Preferences::getMaxLayers() 
	 * - Starts the OscServer using OscServer::start() if
	 *   H2CORE_HAVE_OSC was set during compilation.
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


};

#endif

