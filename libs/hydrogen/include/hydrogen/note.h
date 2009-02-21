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

#ifndef H2_NOTE_H
#define H2_NOTE_H

#include <cassert>
#include <hydrogen/Object.h>
#include <hydrogen/adsr.h>


namespace H2Core
{

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

	NoteKey() {
		m_key = C;
		m_nOctave = 0;
	}

	NoteKey( const NoteKey& key ) {
		m_key = key.m_key;
		m_nOctave = key.m_nOctave;
	}


};


/**

\brief A note...

*/
class Note : public Object
{
public:

	float m_fSamplePosition; ///< Place marker for overlapping process() cycles
	int m_nHumanizeDelay;	///< Used in "humanize" function
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

	bool m_bJustRecorded; //< Used in record+delete

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

	Note* copy();

	void set_instrument( Instrument* instrument );
	Instrument* get_instrument() {
		return __instrument;
	}

	void dumpInfo();
	static NoteKey stringToKey( const QString& sKey );
	static QString keyToString( NoteKey key );


	/// Return the note position inside a pattern
	unsigned get_position() const {
		return __position;
	}

	/// Set the note position inside a pattern
	void set_position( unsigned position ) {
		__position = position;
	}

	/// Return the note velocity
	float get_velocity() const {
		return __velocity;
	}

	/// Set the note velocity
	void set_velocity( float velocity ) {
		if ( velocity > 1.0 ) {
			velocity = 1.0;
		} else if ( velocity < 0 ) {
			velocity = 0;
		}
		__velocity = velocity;
	}


	float get_pan_l() const {
		return __pan_l;
	}
	void set_pan_l( float pan ) {
		if ( pan > 0.5 ) {
//			INFOLOG( "Pan R > 0.5" );
			pan = 0.5;
		}
		__pan_l = pan;
	}

	float get_pan_r() const {
		return __pan_r;
	}
	void set_pan_r( float pan ) {
		if ( pan > 0.5 ) {
//			INFOLOG( "Pan R > 0.5" );
			pan = 0.5;
		}
		__pan_r = pan;
	}


	void set_leadlag( float leadlag ) {
		if(leadlag > 1.0) {
			__leadlag = 1.0;
		} else if (leadlag < -1.0) {
			__leadlag = -1.0;
		} else {
			__leadlag = leadlag;
		}
	}
	float get_leadlag() const {
		assert(__leadlag <=  1.0);
		assert(__leadlag >= -1.0);
		return __leadlag;
	}

	void set_length( int length ) {
		__length = length;
	}
	int get_length() const {
		return __length;
	}

	void set_pitch( float pitch ) {
		__pitch = pitch;
	}
	float get_pitch() const {
		return __pitch;
	}

	void set_noteoff( bool noteOff ) {
		__noteoff = noteOff;
	}
	bool get_noteoff() const {
		return __noteoff;
	}

	void set_midimsg1( int midimsg ) {
		__midimsg1 = midimsg;
	}
	int get_midimsg1() const {
		return __midimsg1;
	}

	


private:
	Instrument* __instrument;
	unsigned __position;		///< Note position inside the pattern
	float __velocity;		///< Velocity (intensity) of the note [0..1]
	float __pan_l;			///< Pan of the note (left volume) [0..1]
	float __pan_r;			///< Pan of the note (right volume) [0..1]
	float __leadlag;		///< Lead or lag offset of the note
	bool __noteoff;			///< note type

	int __length;
	float __pitch;
	int __midimsg1;

};

};

#endif
