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
#include <cassert>
#include <hydrogen/timehelper.h>

// Engine states  (It's ok to use ==, <, and > when testing)
#define STATE_UNINITIALIZED	1     // Not even the constructors have been called.
#define STATE_INITIALIZED	2     // Not ready, but most pointers are now valid or NULL
#define STATE_PREPARED		3     // Drivers are set up, but not ready to process audio.
#define STATE_READY			4     // Ready to process audio
#define STATE_PLAYING		5     // Currently playing a sequence.

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
	/// Return the Hydrogen instance
	static void			create_instance();  // Also creates other instances, like AudioEngine
	static Hydrogen*	get_instance() { assert(__instance); return __instance; };

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

	void			sequencer_setNextPattern( int pos );
	void			togglePlaysSelected( void );
// ***** ~SEQUENCER ********

	/// Set/Get current song
	Song*			getSong()	{ return __song; }
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

	unsigned long	getTickPosition();
	unsigned long	getRealtimeTickPosition();
	unsigned long	getTotalFrames();

	void			setRealtimeFrames( unsigned long frames );
	unsigned long	getRealtimeFrames();

	PatternList *	getCurrentPatternList();
	void			setCurrentPatternList( PatternList * pPatternList );

	PatternList *	getNextPatterns();

	int				getPatternPos();
	void			setPatternPos( int pos );
	int				getPosForTick( unsigned long TickPos );

	void			triggerRelocateDuringPlay();

	long			getTickForPosition( int );

	void			restartDrivers();

	void			startExportSong( const QString& filename, int rate, int depth  );
	void			stopExportSong( bool reconnectOldDriver );

	AudioOutput*	getAudioOutput();
	MidiInput*		getMidiInput();
	MidiOutput*		getMidiOutput();

	int				getState();

	float			getProcessTime();
	float			getMaxProcessTime();

	int			loadDrumkit( Drumkit *pDrumkitInfo );
	int			loadDrumkit( Drumkit *pDrumkitInfo, bool conditional );

	/// delete an instrument. If `conditional` is true, and there are patterns that
	/// use this instrument, it's not deleted anyway
	void			removeInstrument( int instrumentnumber, bool conditional );

	//return the name of the current drumkit
	QString			m_currentDrumkit;

	const QString&	getCurrentDrumkitname();
	void			setCurrentDrumkitname( const QString& currentdrumkitname );

	void			raiseError( unsigned nErrorCode );


	void			previewSample( Sample *pSample );
	void			previewInstrument( Instrument *pInstr );

	enum ErrorMessages {
		UNKNOWN_DRIVER,
		ERROR_STARTING_DRIVER,
		JACK_SERVER_SHUTDOWN,
		JACK_CANNOT_ACTIVATE_CLIENT,
		JACK_CANNOT_CONNECT_OUTPUT_PORT,
		JACK_ERROR_IN_PORT_REGISTER
	};

	void			onTapTempoAccelEvent();
	void			setTapTempo( float fInterval );
	void			setBPM( float fBPM );

	void			restartLadspaFX();
	void			setSelectedPatternNumberWithoutGuiEvent( int nPat );
	int				getSelectedPatternNumber();
	void			setSelectedPatternNumber( int nPat );

	int				getSelectedInstrumentNumber();
	void			setSelectedInstrumentNumber( int nInstrument );

#ifdef H2CORE_HAVE_JACK
	void			renameJackPorts(Song* pSong);
#endif

#ifdef H2CORE_HAVE_NSMSESSION
	void			startNsmClient();
#endif

	///playlist vector
	struct HPlayListNode
	{
		QString m_hFile;
		bool m_hFileExists;
		QString m_hScript;
		QString m_hScriptEnabled;
	};

	std::vector<HPlayListNode> m_PlayList;

	///beatconter
	void			setbeatsToCount( int beatstocount);
	int				getbeatsToCount();
	void			setNoteLength( float notelength);
	float			getNoteLength();
	int				getBcStatus();
	void			handleBeatCounter();
	void			setBcOffsetAdjust();

	/// jack time master
	unsigned long	getHumantimeFrames();
	void			setHumantimeFrames(unsigned long hframes);
	void			offJackMaster();
	void			onJackMaster();
	unsigned long	getTimeMasterFrames();
	long			getTickForHumanPosition( int humanpos );
	float			getNewBpmJTM();
	void			setNewBpmJTM( float bpmJTM);
	void			ComputeHumantimeFrames(uint32_t nFrames);

	void			__panic();
	int				__get_selected_PatterNumber();
	unsigned int	__getMidiRealtimeNoteTickPosition();

	void			setTimelineBpm();
	float			getTimelineBpm( int Beat );
	Timeline*		getTimeline() const;

	///midi lookuptable
	int m_nInstrumentLookupTable[MAX_INSTRUMENTS];

private:
	static Hydrogen* __instance;

	Song*	__song; /// < Current song

	void initBeatcounter(void);

	// beatcounter
	float	m_ntaktoMeterCompute;	///< beatcounter note length
	int		m_nbeatsToCount;		///< beatcounter beats to count
	int		m_nEventCount;			///< beatcounter event
	int		m_nTempoChangeCounter;	///< count tempochanges for timeArray
	int		m_nBeatCount;			///< beatcounter beat to count
	double	m_nBeatDiffs[16];		///< beat diff
	timeval m_CurrentTime;			///< timeval
	timeval	m_LastTime;				///< timeval
	double	m_nLastBeatTime;		///< timediff
	double	m_nCurrentBeatTime;		///< timediff
	double	m_nBeatDiff;			///< timediff
	float	m_fBeatCountBpm;		///< bpm
	int		m_nCoutOffset;			///ms default 0
	int		m_nStartOffset;			///ms default 0
	//~ beatcounter


	// used for song export
	Song::SongMode	m_oldEngineMode;
	bool			m_bOldLoopEnabled;

	//Timline information
	Timeline*		m_pTimeline;

	std::list<Instrument*> __instrument_death_row; /// Deleting instruments too soon leads to potential crashes.


	/// Private constructor
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

inline const QString& Hydrogen::getCurrentDrumkitname()
{
	return m_currentDrumkit;
}

inline void Hydrogen::setCurrentDrumkitname( const QString& currentdrumkitname )
{
	this->m_currentDrumkit = currentdrumkitname;
}

};

#endif

