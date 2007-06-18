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
 * $Id: Song.h,v 1.17 2005/06/14 13:54:06 comix Exp $
 *
 */

#ifndef SONG_H
#define SONG_H


#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include "lib/Globals.h"
#include "Object.h"

class LadspaFX;
class ADSR;
class Sample;
class Note;
class Instrument;
class Sequence;
class Pattern;
class Song;



/// Sequence List
class SequenceList : public Object {
	public:
		SequenceList();
		~SequenceList();

		void add(Sequence* newSequence);
		Sequence* get( int nPos );
		unsigned int getSize();

		SequenceList* copy();

	private:
		vector<Sequence*> list;
};




/// Pattern List
class PatternList : public Object {
	public:
		PatternList();
		~PatternList();

		void add( Pattern* newPattern );
		Pattern* get( int nPos );
		unsigned int getSize();
		void clear();

		void replace( Pattern* newPattern, unsigned nPos );

		void del(Pattern *pattern);
		void del(unsigned index);

	private:
		vector<Pattern*> list;
};




/// Instrument List
class InstrumentList : public Object {
	public:
		InstrumentList();
		~InstrumentList();

		void add(Instrument* newPattern);
		Instrument* get(unsigned int pos);
		int getPos(Instrument* inst);
		unsigned int getSize();

		void replace( Instrument* pNewInstr, unsigned nPos );

	private:
		vector<Instrument*> m_list;
		map<Instrument*, unsigned> m_posmap;
};





/// A note...
class Note : public Object
{
	public:
		unsigned int m_nPosition;	///< Note position inside the pattern
		int m_nLength;
		float m_fSamplePosition;
		float m_fPan_L;			///< Pan of the note (left volume) [0..1]
		float m_fPan_R;			///< Pan of the note (right volume) [0..1]
		float m_fVelocity;		///< Velocity (intensity) of the note [0..1]
		float m_fPitch;
		unsigned m_nHumanizeDelay;	///< Used in "humanize" function

		ADSR* m_pADSR;

		// Low pass resonant filter
		float m_fCutoff;		///< Filter cutoff (0..1)
		float m_fResonance;	///< Filter resonant frequency (0..1)
		float m_fBandPassFilterBuffer_L;		///< Band pass filter buffer
		float m_fBandPassFilterBuffer_R;		///< Band pass filter buffer
		float m_fLowPassFilterBuffer_L;		///< Low pass filter buffer
		float m_fLowPassFilterBuffer_R;		///< Low pass filter buffer
		//~ filter

		Note(
			Instrument *pInstrument,
			unsigned nPosition,
			float fVelocity,
			float fPan_L,
			float fPan_R,
			int nLength,
			float fPitch
		);

		~Note();

		Note* copy();

		void setInstrument(Instrument* pInstrument);
		Instrument* getInstrument() {	return m_pInstrument;	}

	private:
		Instrument* m_pInstrument;
};



/// A layer...
class InstrumentLayer : public Object
{
	public:
		float m_fStartVelocity;		///< Start velocity
		float m_fEndVelocity;		///< End velocity
		float m_fPitch;
		float m_fGain;
		Sample *m_pSample;


		InstrumentLayer( Sample *pSample );
		~InstrumentLayer();

};



/// Instrument class
class Instrument : public Object
{
	public:
		ADSR* m_pADSR;

		string m_sId;			///< ID of the instrument
		string m_sName;		///< Instrument name
		bool m_bFilterActive;	///< Is filter active?
		float m_fCutoff;			///< Filter cutoff (0..1)
		float m_fResonance;		///< Filter resonant frequency (0..1)
		float m_fRandomPitchFactor;
		bool m_bActive;		///< is the instrument active?
		float m_fVolume;		///< Volume of the instrument
		bool m_bIsMuted;
		bool m_bIsLocked;
		float m_fPeak_L;		///< current peak value (left)
		float m_fPeak_R;		///< current peak value (right)
		float m_fPan_L;			///< Pan of the instrument (left)
		float m_fPan_R;			///< Pan of the instrument (right)
		string m_sDrumkitName;	///< Drumkit name
		float m_fGain;

		/** exclude these instruments */
		vector<Instrument*> m_excludeVect;
		vector<int> m_excludeVectId;


		Instrument(
				const string& sId = "",
				const string& sName = "",
				float fVolume = 0,
				bool bMuted = false,
				bool bLocked = false,
				float fPan_L = 1.0,
				float fPan_R = 1.0,
				const string& sDrumkitName = ""
		);
		~Instrument();


		// LADSPA FX
		float getFXLevel(unsigned nFX){	return m_fFXLevel[nFX];	}
		void setFXLevel( unsigned nFX, float value ) {	m_fFXLevel[nFX] = value;	}


		InstrumentLayer* getLayer( int nLayer );
		void setLayer( InstrumentLayer* pLayer, unsigned nLayer );

	private:
		float m_fFXLevel[MAX_FX];	///< Ladspa FX level
		InstrumentLayer* m_layers[MAX_LAYERS];
};





/// Sequence of notes
class Sequence : public Object{
	public:
		map <int, Note*> m_noteList;

		Sequence();
		~Sequence();

		Sequence* copy();
};





/// Pattern (a container of sequences)
class Pattern : public Object
{
	public:
		string m_sName;
		SequenceList *m_pSequenceList;
		unsigned m_nSize;

		Pattern( const std::string& sName, unsigned nPatternSize = MAX_NOTES );
		~Pattern();

		static Pattern* getEmptyPattern();
		Pattern* copy();
};






/// Song class.
class Song : public Object{
	public:
		enum SongMode {
			PATTERN_MODE,
			SONG_MODE
		};

		bool m_bIsMuted;
		unsigned m_nResolution;	///< Resolution of the song (number of ticks per quarter)
		float m_fBPM;			///< Beats per minute
		bool m_bIsModified;
		std::string m_sName;	///< song name
		string m_sAuthor;		///< author of the song

		bool m_bDelayFXEnabled;
		float m_fDelayFXWetLevel;
		float m_fDelayFXFeedback;
		unsigned m_nDelayFXTime;

		Song(const std::string& sName, const std::string& sAuthor, float bpm, float volume);
		~Song();

		void setVolume(float fVolume) {	m_fVolume = fVolume;	}
		float getVolume() {	return m_fVolume;	}

		void setMetronomeVolume(float fVolume) {	m_fMetronomeVolume = fVolume;	}
		float getMetronomeVolume() {	return m_fMetronomeVolume;	}

		PatternList* getPatternList(){	return m_pPatternList;	}
		void setPatternList( PatternList *pList ){	m_pPatternList = pList;	}

		vector<PatternList*>* getPatternGroupVector(){	return m_pPatternSequence;	}
		void setPatternGroupVector( vector<PatternList*>* pVect ){	m_pPatternSequence = pVect;	}

		static Song* load(string sFilename);
		void save(string sFilename);

		InstrumentList* getInstrumentList(){	return m_pInstrumentList;	}
		void setInstrumentList( InstrumentList *pList ){	m_pInstrumentList = pList;	}

		static Song* getEmptySong();

		void setNotes(string sNotes) {	m_sNotes = sNotes;	}
		string getNotes() {	return m_sNotes;	}

		string getFilename() {	return m_sFilename;	}
		void setFilename(string sFilename) {	m_sFilename = sFilename;	}

		bool isLoopEnabled() {	return m_bIsLoopEnabled;	}
		void setLoopEnabled( bool bIsLoopEnabled ) {	m_bIsLoopEnabled = bIsLoopEnabled;	}

		float getHumanizeTimeValue() {	return m_fHumanizeTimeValue;	}
		void setHumanizeTimeValue(float fValue) {	m_fHumanizeTimeValue = fValue;	}

		float getHumanizeVelocityValue() {	return m_fHumanizeVelocityValue;	 }
		void setHumanizeVelocityValue(float fValue) { m_fHumanizeVelocityValue = fValue;	}

		float getSwingFactor() {	return m_fSwingFactor;	}
		void setSwingFactor(float fFactor);

		SongMode getMode() {	return m_songMode;	}
		void setMode( SongMode newMode ) {	m_songMode = newMode;	}

#ifdef LADSPA_SUPPORT
		LadspaFX* getLadspaFX(unsigned nFX) {	return m_pLadspaFX[nFX];	}
		void setLadspaFX( unsigned nFX, LadspaFX* pLadspaFX ) {	m_pLadspaFX[nFX] = pLadspaFX;	}
#endif

	private:
		float m_fVolume;			///< volume of the song (0.0..1.0)
		float m_fMetronomeVolume;	///< Metronome volume
		string m_sNotes;
		PatternList *m_pPatternList;	///< Pattern list
		vector<PatternList*>* m_pPatternSequence;	///< Sequence of pattern groups
		InstrumentList *m_pInstrumentList;	///< Instrument list
		string m_sFilename;
		bool m_bIsLoopEnabled;
		float m_fHumanizeTimeValue;
		float m_fHumanizeVelocityValue;
		float m_fSwingFactor;

		SongMode m_songMode;

#ifdef LADSPA_SUPPORT
		LadspaFX* m_pLadspaFX[MAX_FX];
#endif
};





/// Drumkit info
class DrumkitInfo : public Object {
	public:
		DrumkitInfo();
		~DrumkitInfo();

		InstrumentList *getInstrumentList() {	return instrumentList;	}
		void setInstrumentList( InstrumentList* instr ) {	this->instrumentList = instr;	}

		void setName( string name ) {	this->name = name;	}
		string getName() {	return name;	}

		void setAuthor( string author ) {	this->author = author;	}
		string getAuthor() {	return author;	}

		void setInfo( string info ) {	this->info = info;	}
		string getInfo() {	return info;	}

		void dump();

	private:
		InstrumentList *instrumentList;
		string name;
		string author;
		string info;
};

#endif


