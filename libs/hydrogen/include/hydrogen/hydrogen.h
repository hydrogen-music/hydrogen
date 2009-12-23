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
#include <hydrogen/action.h>
#include <hydrogen/Song.h>
#include <hydrogen/Object.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/IO/MidiOutput.h>
#include <hydrogen/SoundLibrary.h>
#include <cassert>

// Engine states  (It's ok to use ==, <, and > when testing)
#define STATE_UNINITIALIZED	1     // Not even the constructors have been called.
#define STATE_INITIALIZED	2     // Not ready, but most pointers are now valid or NULL
#define STATE_PREPARED		3     // Drivers are set up, but not ready to process audio.
#define STATE_READY		4     // Ready to process audio
#define STATE_PLAYING		5     // Currently playing a sequence.

inline int randomValue( int max );

namespace H2Core
{

///
/// Hydrogen Audio Engine.
///
class Hydrogen : public Object
{
public:
	/// Return the Hydrogen instance
	static void create_instance();  // Also creates other instances, like AudioEngine
	static Hydrogen* get_instance() { assert(__instance); return __instance; };

	~Hydrogen();

// ***** SEQUENCER ********
	/// Start the internal sequencer
	void sequencer_play();

	/// Stop the internal sequencer
	void sequencer_stop();

	void midi_noteOn( Note *note );

	///Last received midi message
	QString lastMidiEvent;
	int lastMidiEventParameter;


	void sequencer_setNextPattern( int pos, bool appendPattern, bool deletePattern );
	void togglePlaysSelected( void );
// ***** ~SEQUENCER ********

	/// Set current song
	void setSong( Song *newSong );

	/// Return the current song
	Song* getSong();
	void removeSong();

	void addRealtimeNote ( int instrument, float velocity, float pan_L=1.0, float pan_R=1.0, float pitch=0.0, bool noteoff=false, bool forcePlay=false, int msg1=0 );

	float getMasterPeak_L();
	void setMasterPeak_L( float value );

	float getMasterPeak_R();
	void setMasterPeak_R( float value );

	void getLadspaFXPeak( int nFX, float *fL, float *fR );
	void setLadspaFXPeak( int nFX, float fL, float fR );


	unsigned long getTickPosition();
	unsigned long getRealtimeTickPosition();
	unsigned long getTotalFrames();
	unsigned long getRealtimeFrames();


	PatternList * getCurrentPatternList();
	void setCurrentPatternList( PatternList * pPatternList );

	PatternList * getNextPatterns();

	int getPatternPos();
	void setPatternPos( int pos );
	
	void triggerRelocateDuringPlay();

	long getTickForPosition( int );

	void restartDrivers();

	void startExportSong( const QString& filename, int rate, int depth  );
	void stopExportSong();

	AudioOutput* getAudioOutput();
	MidiInput* getMidiInput();
	MidiOutput* getMidiOutput();

	int getState();

	float getProcessTime();
	float getMaxProcessTime();

	int loadDrumkit( Drumkit *drumkitInfo );
	
	/// delete an instrument. If `conditional` is true, and there are patterns that
	/// use this instrument, it's not deleted anyway
	void removeInstrument( int instrumentnumber, bool conditional );

	//return the name of the current drumkit
	QString m_currentDrumkit;

	const QString& getCurrentDrumkitname() {
		return m_currentDrumkit;
	}

	void setCurrentDrumkitname( const QString& currentdrumkitname ) {
		this->m_currentDrumkit = currentdrumkitname;
	}

	void raiseError( unsigned nErrorCode );


	void previewSample( Sample *pSample );
	void previewInstrument( Instrument *pInstr );

	enum ErrorMessages {
		UNKNOWN_DRIVER,
		ERROR_STARTING_DRIVER,
		JACK_SERVER_SHUTDOWN,
		JACK_CANNOT_ACTIVATE_CLIENT,
		JACK_CANNOT_CONNECT_OUTPUT_PORT,
		JACK_ERROR_IN_PORT_REGISTER
	};

	void onTapTempoAccelEvent();
	void setTapTempo( float fInterval );
	void setBPM( float fBPM );

	void restartLadspaFX();

	int getSelectedPatternNumber();
	void setSelectedPatternNumber( int nPat );

	int getSelectedInstrumentNumber();
	void setSelectedInstrumentNumber( int nInstrument );
#ifdef JACK_SUPPORT
	void renameJackPorts();
#endif

	///playlist vector
	struct HPlayListNode
	{
		QString m_hFile;
		QString m_hScript;
		QString m_hScriptEnabled;
	};

	std::vector<HPlayListNode> m_PlayList;
	
	///beatconter
	void setbeatsToCount( int beatstocount);
	int getbeatsToCount();
	void setNoteLength( float notelength);
	float getNoteLength();
	int getBcStatus();
	void handleBeatCounter();
	void setBcOffsetAdjust();

	/// jack time master
	unsigned long getHumantimeFrames();
	void setHumantimeFrames(unsigned long hframes);
	void offJackMaster();
	void onJackMaster();
	unsigned long getTimeMasterFrames();
	long getTickForHumanPosition( int humanpos );
	float getNewBpmJTM();
	void setNewBpmJTM( float bpmJTM);
	void ComputeHumantimeFrames(uint32_t nFrames);

	void __panic();
	int __get_selected_PatterNumber();
	unsigned int __getMidiRealtimeNoteTickPosition();

	///sample editor vectors

	void sortVolVectors();
	void sortPanVectors();
	void sortTimelineVector();
	void sortTimelineTagVector();

	struct HVeloVector
	{
		int m_hxframe;
		int m_hyvalue;
	};

	std::vector<HVeloVector> m_volumen;

	struct VolComparator
	{
		bool operator()( HVeloVector const& lhs, HVeloVector const& rhs)
		{
			return lhs.m_hxframe < rhs.m_hxframe;
		}
	};

	struct HPanVector
	{
		int m_hxframe;
		int m_hyvalue;
	};

	std::vector<HPanVector> m_pan;

	struct PanComparator
	{
		bool operator()( HPanVector const& lhs, HPanVector const& rhs)
		{
			return lhs.m_hxframe < rhs.m_hxframe;
		}
	};

/// timeline vector
	struct HTimelineVector
	{
		int m_htimelinebeat;		//beat position in timeline 
//		int m_htimelinebar;		//bar position from current beat
		float m_htimelinebpm;		//BPM 
//		bool m_htimelineslide;		//true if slide into new tempo
//		int m_htimelineslidebeatbegin;	//position of slide begin (only beats, no bars)
//		int m_htimelineslideend;	//position of slide end (only beats, no bars)
//		int m_htimelineslidetype;	// 0 = slide up, 1 = slide down
	};
	std::vector<HTimelineVector> m_timelinevector;

	struct TimelineComparator
	{
		bool operator()( HTimelineVector const& lhs, HTimelineVector const& rhs)
		{
			return lhs.m_htimelinebeat < rhs.m_htimelinebeat;
		}
	};

	void setTimelineBpm();

/// timeline tag vector
	struct HTimelineTagVector
	{
		int m_htimelinetagbeat;		//beat position in timeline 
//		int m_htimelineintensity;		//intensity
		QString m_htimelinetag;		// tag
	};
	std::vector<HTimelineTagVector> m_timelinetagvector;

	struct TimelineTagComparator
	{
		bool operator()( HTimelineTagVector const& lhs, HTimelineTagVector const& rhs)
		{
			return lhs.m_htimelinetagbeat < rhs.m_htimelinetagbeat;
		}
	};


private:
	static Hydrogen* __instance;

	// used for song export
	Song::SongMode m_oldEngineMode;
	bool m_bOldLoopEnabled;

	std::list<Instrument*> __instrument_death_row; /// Deleting instruments too soon leads to potential crashes.


	/// Private constructor
	Hydrogen();

	void __kill_instruments();

};

};

#endif

