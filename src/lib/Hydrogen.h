/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Hydrogen.h,v 1.15 2005/05/09 18:12:24 comix Exp $
 *
 */
#ifndef HYDROGEN_H
#define HYDROGEN_H

#include "Song.h"
#include "Object.h"


// Engine state
#define STATE_UNINITIALIZED	1
#define STATE_INITIALIZED	2
#define STATE_PREPARED		3
#define STATE_READY		4
#define STATE_PLAYING		5


// forward
class MidiDriver;
class GenericDriver;

inline int randomValue( int max );


//----------------------------------------------------------------------------
/**
 * Hydrogen Audio Engine
 */
class Hydrogen : public Object
{
	public:
		/// Return the Hydrogen instance
		static Hydrogen* getInstance();

		~Hydrogen();

		/// Start the internal sequencer
		void start();

		/// Stop the internal sequencer
		void stop();

		/// Set current song
		void setSong(Song *newSong);

		/// Return the current song
		Song* getSong();
		void removeSong();

		void noteOn(Note *note);
		void noteOff(Note *note);

		void addRealtimeNote (int instrument, float velocity, float pan_L=1.0, float pan_R=1.0, float pitch=0.0, bool forcePlay=false);

		float getMasterPeak_L();
		void setMasterPeak_L(float value);

		float getMasterPeak_R();
		void setMasterPeak_R(float value);

		void getLadspaFXPeak( int nFX, float *fL, float *fR );
		void setLadspaFXPeak( int nFX, float fL, float fR );


		unsigned long getTickPosition();
		unsigned long getRealtimeTickPosition();

		void setNextPattern(int pos);

		PatternList* getCurrentPatternList();
		void setCurrentPatternList(PatternList *pPatternList);

		int getPatternPos();
		void setPatternPos( int pos );

		unsigned getPlayingNotes();

		void restartDrivers();

		void startExportSong(const std::string& filename);
		void stopExportSong();

		unsigned getSongNotesQueue();

		GenericDriver* getAudioDriver();
		MidiDriver* getMidiDriver();

		int getState();

		void lockEngine(const std::string& sLocker);
		void unlockEngine();

		float getProcessTime();
		float getMaxProcessTime();

		int loadDrumkit( DrumkitInfo *drumkitInfo );

		void raiseError(unsigned nErrorCode);

		unsigned long getTotalFrames();

		void previewSample( Sample *pSample );

		enum ErrorMessages {
			UNKNOWN_DRIVER,
			ERROR_STARTING_DRIVER,
			JACK_SERVER_SHUTDOWN,
			JACK_CANNOT_ACTIVATE_CLIENT,
			JACK_CANNOT_CONNECT_OUTPUT_PORT,
			JACK_ERROR_IN_PORT_REGISTER
		};

		void setTapTempo( float fInterval );
		void setBPM( float fBPM );

		void restartLadspaFX();

		int getSelectedPatternNumber();
		void setSelectedPatternNumber( int nPat );

		int getSelectedInstrumentNumber();
		void setSelectedInstrumentNumber( int nInstrument );

	private:
		static Hydrogen* instance;

		/** Constructor */
		Hydrogen();


		// used for song export
		Song::SongMode m_oldEngineMode;
		bool m_bOldUseTrackOuts;
		bool m_bOldLoopEnabled;

};


#endif

