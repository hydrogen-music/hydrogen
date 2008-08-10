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

#include <hydrogen/action.h>
#include <hydrogen/Song.h>
#include <hydrogen/Object.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/SoundLibrary.h>


// Engine state
#define STATE_UNINITIALIZED	1
#define STATE_INITIALIZED	2
#define STATE_PREPARED		3
#define STATE_READY		4
#define STATE_PLAYING		5

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
	static Hydrogen* get_instance();

	~Hydrogen();

// ***** SEQUENCER ********
	/// Start the internal sequencer
	void sequencer_play();

	/// Stop the internal sequencer
	void sequencer_stop();

	void midi_noteOn( Note *note );
	void midi_noteOff( Note *note );

	void sequencer_setNextPattern( int pos, bool appendPattern, bool deletePattern );
	void togglePlaysSelected( void );
// ***** ~SEQUENCER ********

	/// Set current song
	void setSong( Song *newSong );

	/// Return the current song
	Song* getSong();
	void removeSong();

	void addRealtimeNote ( int instrument, float velocity, float pan_L=1.0, float pan_R=1.0, float pitch=0.0, bool forcePlay=false );

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

	void startExportSong( const QString& filename );
	void stopExportSong();

	AudioOutput* getAudioOutput();
	MidiInput* getMidiInput();

	int getState();

	float getProcessTime();
	float getMaxProcessTime();

	int loadDrumkit( Drumkit *drumkitInfo );

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
	void renameJackPorts();

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
	void setNoteLengh( float notelengh);
	float getNoteLengh();
	int getBcStatus();
	void handleBeatCounter();

	/// jack time master
	unsigned long getHumantimeFrames();
	void setHumantimeFrames(unsigned long hframes);
	void offJackMaster();
	void onJackMaster();
	unsigned long getTimeMasterFrames();
	long getTickForHumanPosition( int humanpos );
	float getNewBpmJTM();
	void ComputeHumantimeFrames(uint32_t nFrames);

private:
	static Hydrogen* instance;

	/// Constructor
	Hydrogen();

	// used for song export
	Song::SongMode m_oldEngineMode;
	bool m_bOldLoopEnabled;

};

};

#endif

