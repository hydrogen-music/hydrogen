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

#ifndef H2_NOTE_H
#define H2_NOTE_H

#include <hydrogen/Object.h>
#include <hydrogen/adsr.h>


namespace H2Core {

class ADSR;
class Instrument;

class NoteKey
{
	public:
		enum Key {
			C = 0,
			Cs,
			D,
			Ef,
			E,
			F,
			Fs,
			G,
			Af,
			A,
			Bf,
			B,
		};

		Key m_key;
		int m_nOctave;

		NoteKey()
		{
			m_key = C;
			m_nOctave = 0;
		}

		NoteKey( const NoteKey& key )
		{
			m_key = key.m_key;
			m_nOctave = key.m_nOctave;
		}


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
		NoteKey m_noteKey;

		ADSR m_adsr;

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
			float fPitch,
			NoteKey key = NoteKey()
		);

		/// Copy constructor
		Note( const Note* pNote );


		~Note();

		//Note* copy();

		void setInstrument(Instrument* pInstrument);
		Instrument* getInstrument() {	return m_pInstrument;	}

		void dumpInfo();
		static NoteKey stringToKey( const std::string& sKey );
		static std::string keyToString( NoteKey key );

	private:
		Instrument* m_pInstrument;
};

};

#endif
