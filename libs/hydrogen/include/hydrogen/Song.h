/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef SONG_H
#define SONG_H


#include <string>
#include <vector>
#include <map>

#include <hydrogen/Object.h>

class TiXmlNode;

namespace H2Core {

class ADSR;
class Sample;
class Note;
class Instrument;
class InstrumentList;
class Pattern;
class Song;
class PatternList;

/**
\ingroup H2CORE
\brief	Song class
*/
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
		std::string m_sAuthor;		///< author of the song

		/*
		// internal delay FX
		bool m_bDelayFXEnabled;
		float m_fDelayFXWetLevel;
		float m_fDelayFXFeedback;
		unsigned m_nDelayFXTime;
		//~ internal delay fx
		*/

		Song(const std::string& sName, const std::string& sAuthor, float bpm, float volume);
		~Song();

		void setVolume(float fVolume) {	m_fVolume = fVolume;	}
		float getVolume() {	return m_fVolume;	}

		void setMetronomeVolume(float fVolume) {	m_fMetronomeVolume = fVolume;	}
		float getMetronomeVolume() {	return m_fMetronomeVolume;	}

		PatternList* getPatternList(){	return m_pPatternList;	}
		void setPatternList( PatternList *pList ){	m_pPatternList = pList;	}

		std::vector<PatternList*>* getPatternGroupVector(){	return m_pPatternSequence;	}
		void setPatternGroupVector( std::vector<PatternList*>* pVect ){	m_pPatternSequence = pVect;	}

		static Song* load( const std::string& sFilename );
		void save( const std::string& sFilename );

		InstrumentList* getInstrumentList(){	return m_pInstrumentList;	}
		void setInstrumentList( InstrumentList *pList ){	m_pInstrumentList = pList;	}

		static Song* getEmptySong();

		void setNotes( const std::string& sNotes ) {	m_sNotes = sNotes;	}
		std::string getNotes() {	return m_sNotes;	}

		std::string getFilename() {	return m_sFilename;	}
		void setFilename( const std::string& sFilename ) {	m_sFilename = sFilename;	}

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

	private:
		float m_fVolume;			///< volume of the song (0.0..1.0)
		float m_fMetronomeVolume;	///< Metronome volume
		std::string m_sNotes;
		PatternList *m_pPatternList;	///< Pattern list
		std::vector<PatternList*>* m_pPatternSequence;	///< Sequence of pattern groups
		InstrumentList *m_pInstrumentList;	///< Instrument list
		std::string m_sFilename;
		bool m_bIsLoopEnabled;
		float m_fHumanizeTimeValue;
		float m_fHumanizeVelocityValue;
		float m_fSwingFactor;

		SongMode m_songMode;
};



/**
\ingroup H2CORE
\brief	Read XML file of a song
*/
class SongReader : public Object {
	public:
		SongReader();
		~SongReader();
		Song* readSong( const std::string& filename);

	private:
		std::string m_sSongVersion;

		/// Dato un XmlNode restituisce un oggetto Pattern
		Pattern* getPattern(::TiXmlNode* pattern, InstrumentList* instrList);
};


};



#endif


