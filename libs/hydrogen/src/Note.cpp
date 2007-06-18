/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/Note.h>
#include <hydrogen/Instrument.h>

#include <cassert>

using std::string;

namespace H2Core {

Note::Note(
		Instrument *pInstrument,
		unsigned nPosition,
		float fVelocity,
		float fPan_L,
		float fPan_R,
		int nLength,
		float fPitch,
		NoteKey key
)
 : Object( "Note" )
 , m_nPosition( nPosition )
 , m_nLength( nLength )
 , m_fSamplePosition( 0.0 )
 , m_fPan_L( fPan_L )
 , m_fPan_R( fPan_R )
 , m_fVelocity( fVelocity )
 , m_fPitch( fPitch )
 , m_nHumanizeDelay( 0 )
 , m_noteKey( key )
// , m_pADSR( NULL )
 , m_fCutoff( 1.0 )
 , m_fResonance( 0.0 )
 , m_fBandPassFilterBuffer_L( 0.0 )
 , m_fBandPassFilterBuffer_R( 0.0 )
 , m_fLowPassFilterBuffer_L( 0.0 )
 , m_fLowPassFilterBuffer_R( 0.0 )
{
	
	if ( m_fPan_L > 0.5 ) {
		m_fPan_L = 0.5;
	}
	if ( m_fPan_R > 0.5 ) {
		m_fPan_R = 0.5;
	}

	setInstrument( pInstrument );
}




Note::Note( const Note* pNote )
 : Object( "Note" )
{
	m_nPosition	=	pNote->m_nPosition;
	m_fVelocity	=	pNote->m_fVelocity;
	m_fPan_L	=	pNote->m_fPan_L;
	m_fPan_R	=	pNote->m_fPan_R;
	m_nLength	= 	pNote->m_nLength;
	m_fPitch	= 	pNote->m_fPitch;
	m_noteKey	=	pNote->m_noteKey;
	m_fCutoff	=	pNote->m_fCutoff;
	m_fResonance	=	pNote->m_fResonance;
	m_fBandPassFilterBuffer_L	= 	pNote->m_fBandPassFilterBuffer_L;
	m_fBandPassFilterBuffer_R	= 	pNote->m_fBandPassFilterBuffer_R;
	m_fLowPassFilterBuffer_L	=	pNote->m_fLowPassFilterBuffer_L;
	m_fLowPassFilterBuffer_R	=	pNote->m_fLowPassFilterBuffer_R;
	m_nHumanizeDelay		= 	pNote->m_nHumanizeDelay;
	m_fSamplePosition		=	pNote->m_fSamplePosition;
	setInstrument( pNote->m_pInstrument );
}



Note::~Note()
{
	//infoLog("DESTROY");
	//delete m_pADSR;
}



void Note::setInstrument( Instrument* pInstrument )
{
	if ( pInstrument == NULL ) {
		return;
	}

	m_pInstrument = pInstrument;
	m_adsr = ADSR( *(m_pInstrument->m_pADSR) );

/*
	if ( pInstrument->m_pADSR == NULL ) {
		ERRORLOG( "NULL ADSR? Instrument: " + pInstrument->m_sName );
	}
	else {
		INFOLOG( "copio l'adsr dallo strumento" );
		if ( m_pADSR ) {
			WARNINGLOG( "Replacing an existing ADSR" );
			delete m_pADSR;
			m_pADSR = NULL;
		}
		m_pADSR = new ADSR( *(m_pInstrument->m_pADSR) );
	}
*/
}



void Note::dumpInfo()
{
	INFOLOG( "pos: " + toString( m_nPosition ) + "\t instr: " + m_pInstrument->m_sName + "\t key: " + keyToString( m_noteKey ) + "\t pitch: " + toString( m_fPitch ) );
}



NoteKey Note::stringToKey( const std::string& str )
{
	NoteKey key;

	string sKey = str.substr( 0, str.length() - 1 );
	string sOct = str.substr( str.length() - 1, str.length() );
	int nOctave = atoi( sOct.c_str() );

//	_INFOLOG( "skey: " + sKey );
//	_INFOLOG( "sOct: " + sOct );
//	_INFOLOG( "nOctave: " + toString( nOctave ) );

	if ( sKey == "C" ) {
		key.m_key = NoteKey::C;
	}
	else if ( sKey == "Cs" ) {
		key.m_key = NoteKey::Cs;
	}
	else if ( sKey == "D" ) {
		key.m_key = NoteKey::D;
	}
	else if ( sKey == "Ef" ) {
		key.m_key = NoteKey::Ef;
	}
	else if ( sKey == "E" ) {
		key.m_key = NoteKey::E;
	}
	else if ( sKey == "F" ) {
		key.m_key = NoteKey::F;
	}
	else if ( sKey == "Fs" ) {
		key.m_key = NoteKey::Fs;
	}
	else if ( sKey == "G" ) {
		key.m_key = NoteKey::G;
	}
	else if ( sKey == "Af" ) {
		key.m_key = NoteKey::Af;
	}
	else if ( sKey == "A" ) {
		key.m_key = NoteKey::A;
	}
	else if ( sKey == "Bf" ) {
		key.m_key = NoteKey::Bf;
	}
	else if ( sKey == "B" ) {
		key.m_key = NoteKey::B;
	}
	else {
		_ERRORLOG( "Unhandled key: " + sKey );
	}
	key.m_nOctave = nOctave;

	return key;
}



std::string Note::keyToString( NoteKey key )
{
	string sKey;

	switch ( key.m_key ) {
		case NoteKey::C:
			sKey = "C";
			break;
		case NoteKey::Cs:
			sKey = "Cs";
			break;
		case NoteKey::D:
			sKey = "D";
			break;
		case NoteKey::Ef:
			sKey = "Ef";
			break;
		case NoteKey::E:
			sKey = "E";
			break;
		case NoteKey::F:
			sKey = "F";
			break;
		case NoteKey::Fs:
			sKey = "Fs";
			break;
		case NoteKey::G:
			sKey = "G";
			break;
		case NoteKey::Af:
			sKey = "Af";
			break;
		case NoteKey::A:
			sKey = "A";
			break;
		case NoteKey::Bf:
			sKey = "Bf";
			break;
		case NoteKey::B:
			sKey = "B";
			break;

	}

	sKey += toString( key.m_nOctave );

	return sKey;
}

};
