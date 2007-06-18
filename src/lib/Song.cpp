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
 * $Id: Song.cpp,v 1.21 2005/06/14 13:54:06 comix Exp $
 *
 *  Changed DATA_PATH to a DataPath::getDataPath() call to accommodate Mac OS X
 *  application bundles (2004/12/15 Jonathan Dempsey)
 */
#include <assert.h>

#include "fx/LadspaFX.h"
#include "ADSR.h"
#include "Song.h"
#include "config.h"
#include "Sample.h"
#include "DataPath.h"
#include "LocalFileMng.h"

SequenceList::SequenceList()
 : Object( "SequenceList" )
{
//	infoLog("Init");
}



SequenceList::~SequenceList() {
//	infoLog("destroy");
	for (unsigned int i = 0; i < list.size(); ++i) {
		if( list[i] != NULL ){
			delete list[i];
			list[i] = NULL;
		}
	}
}



void SequenceList::add(Sequence* newSequence) {
	list.push_back(newSequence);
}



Sequence* SequenceList::get(int pos) {
	assert( ( pos >= 0 ) && ( pos < (int)list.size() ) );
/*	if ( pos < 0 ) {
		errorLog( "[get] pos < 0 - pos = " + toString( pos ) );
		return NULL;
	}
	if (pos > list.size() ) {
		errorLog( "[get] pos > list.size - pos = " + toString( pos ) + " list.size=" + toString(list.size()) );
		return NULL;
	}*/

	return list[pos];
}



unsigned int SequenceList::getSize() {
	return list.size();
}



SequenceList* SequenceList::copy() {
	SequenceList *newSequenceList = new SequenceList();

	for (unsigned nSeq = 0; nSeq < this->getSize(); ++nSeq) {
		Sequence *oldSequence = this->get(nSeq);
		Sequence *newSequence = oldSequence->copy();
		newSequenceList->add(newSequence);
	}

	return newSequenceList;
}




//:::::::::::::::::::::::




PatternList::PatternList()
 : Object( "PatternList" )
{
//	infoLog("Init");
}



PatternList::~PatternList() {
//	infoLog("destroy");

	// find single patterns. (skip duplicates)
	vector<Pattern*> temp;
	for (unsigned int i = 0; i < list.size(); ++i) {
		Pattern *pat = list[i];

		// pat exists in temp?
		bool exists = false;
		for(unsigned int j = 0; j < temp.size(); ++j) {
			if (pat == temp[j]) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			temp.push_back(pat);
		}
	}

	// delete patterns
	for (unsigned int i = 0; i < temp.size(); ++i) {
		Pattern *pat = temp[i];
		if (pat != NULL) {
			delete pat;
			pat = NULL;
		}
	}
}



void PatternList::add(Pattern* newPattern) {
	list.push_back(newPattern);
}



Pattern* PatternList::get(int nPos) {
	if (nPos >= (int)list.size()) {
		errorLog( "[get] Pattern index out of bounds. nPos > list.size() - " + toString( nPos ) + " > " + toString( list.size() ) );
		return NULL;
	}
//	assert( nPos < (int)list.size() );
	return list[ nPos ];
}



unsigned int PatternList::getSize() {
	return list.size();
}



void PatternList::clear() {
	list.clear();
}



/// Replace an existent pattern with another one
void PatternList::replace( Pattern* newPattern, unsigned int pos ) {
	if (pos >= (int)list.size()) {
		errorLog( "[get] Pattern index out of bounds in PatternList::replace. pos >= list.size() - " + toString( pos ) + " > " + toString( list.size() ) );
		return;
	}
	list.insert( list.begin() + pos, newPattern );	// insert the new pattern
	// remove the old pattern
	list.erase( list.begin() + pos + 1 );
}



/// Remove a pattern from the list (every instance in the list), the pattern is not deleted!!!
void PatternList::del(Pattern *pattern) {
	vector<Pattern*>::iterator i;
	for (i = list.begin(); i != list.end(); ++i) {
		if (*i == pattern) {
			i = list.erase( i );
			break;
		}
	}
}



/// Remove one pattern from the list, the pattern is not deleted!!!
void PatternList::del(uint pos) {
	if (pos >= (int)list.size()) {
		errorLog( "[get] Pattern index out of bounds in PatternList::del. pos >= list.size() - " + toString( pos ) + " > " + toString( list.size() ) );
		return;
	}
	list.erase( list.begin()+pos );
}


//::::::::::::::::::



InstrumentList::InstrumentList()
 : Object( "InstrumentList" )
{
//	infoLog("INIT");
}



InstrumentList::~InstrumentList() {
//	infoLog("DESTROY");
	for (unsigned int i = 0; i < m_list.size(); ++i) {
		delete m_list[i];
	}
}



void InstrumentList::add(Instrument* newInstrument) {
	m_list.push_back(newInstrument);
	m_posmap[newInstrument] = m_list.size() - 1;
}



Instrument* InstrumentList::get(unsigned int pos)
{
	if ( pos > m_list.size() ) {
		errorLog( "[get] pos > list.size(). pos = " + toString(pos) );
		return NULL;
	}
	else if ( pos < 0 ) {
		errorLog( "[get] pos < 0. pos = " + toString(pos) );
		return NULL;
	}
	return m_list[pos];
}



/// Returns index of instrument in list, if instrument not found, returns -1
int InstrumentList::getPos( Instrument *pInstr )
{
	if ( m_posmap.find( pInstr ) == m_posmap.end() )
		return -1;
	return m_posmap[ pInstr ];
}



unsigned int InstrumentList::getSize()
{
	return m_list.size();
}


void InstrumentList::replace( Instrument* pNewInstr, unsigned nPos )
{
	if ( nPos >= ( int )m_list.size() ) {
		errorLog( "[get] Instrument index out of bounds in InstrumentList::replace. pos >= list.size() - " + toString( nPos ) + " > " + toString( m_list.size() ) );
		return;
	}
	m_list.insert( m_list.begin() + nPos, pNewInstr );	// insert the new Instrument
	// remove the old Instrument
	m_list.erase( m_list.begin() + nPos + 1 );
}


//::::::::::::::::::::::::::



Song::Song( const std::string& sName, const std::string& sAuthor, float fBpm, float fVolume)
 : Object( "Song     " )
 , m_bIsMuted( false )
 , m_fBPM( fBpm )
 , m_nResolution( 48 )
 , m_fVolume( fVolume )
 , m_fMetronomeVolume( 0.5 )
 , m_sName( sName )
 , m_sAuthor( sAuthor )
 , m_sNotes( "Song info" )	///\todo: attenzione..questo non verra' tradotto
 , m_pPatternList( NULL )
 , m_pPatternSequence( NULL )
 , m_pInstrumentList( NULL )
 , m_sFilename( "" )
 , m_bIsModified( false )
 , m_bIsLoopEnabled( false )
 , m_fHumanizeTimeValue( 0.0 )
 , m_fHumanizeVelocityValue( 0.0 )
 , m_fSwingFactor( 0.0 )
 , m_songMode( PATTERN_MODE )
{
	infoLog( "INIT \"" + m_sName + "\"" );

	m_bDelayFXEnabled = false;
	m_fDelayFXWetLevel = 0.8;
	m_fDelayFXFeedback = 0.5;
	m_nDelayFXTime = MAX_NOTES / 8;

#ifdef LADSPA_SUPPORT
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		setLadspaFX( nFX, NULL );
	}
#endif
}



Song::~Song()
{
	// delete all patterns
	delete m_pPatternList;

	if (m_pPatternSequence) {
		for (unsigned i = 0; i < m_pPatternSequence->size(); ++i) {
			PatternList *pPatternList = (*m_pPatternSequence)[i];
			pPatternList->clear();	// pulisco tutto, i pattern non vanno distrutti qua
			delete pPatternList;
		}
		delete m_pPatternSequence;
	}

	delete m_pInstrumentList;

#ifdef LADSPA_SUPPORT
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		delete m_pLadspaFX[nFX];
	}
#endif

	infoLog( "DESTROY \"" + m_sName + "\"" );
}



///Load a song from file
Song* Song::load(string filename) {
	Song *song = NULL;
	LocalFileMng mng;
	song = mng.loadSong(filename);

	return song;
}




/// Save a song to file
void Song::save(string filename) {
	LocalFileMng mng;
	mng.saveSong(this, filename);
}



/// Return an empty song
Song* Song::getEmptySong() {
	string dataDir = DataPath::getDataPath();
	string filename = dataDir + "/DefaultSong.h2song";
	Song *song = Song::load( filename );

	return song;
}





void Song::setSwingFactor( float factor ) {
	if (factor < 0.0) {
		factor = 0.0;
	}
	else if (factor > 1.0) {
		factor = 1.0;
	}

	m_fSwingFactor = factor;
}



//:::::::::::::::::::::::::



Sequence::Sequence()
 : Object( "Sequence" )
{
//	logger.info(this, "INIT " + name);
//	setName( name );

// 	for ( map::iterator it = m_noteList.begin(); it != m_noteList.end(); it++ ) {
// 		*it = NULL;
// 	}

//	for (int i = 0; i < MAX_NOTES; ++i) {
//		m_noteList[i] = NULL;
//	}

}



Sequence::~Sequence(){
	// delete all notes
/*	for( unsigned int i = 0; i < MAX_NOTES; ++i ){
		delete m_noteList[i];
		m_noteList[i] = NULL;
	}*/
	for ( map<int, Note*>::iterator it = m_noteList.begin(); it != m_noteList.end(); it++ ) {
		delete it->second;
		//*it = NULL;
	}

//	logger.info(this, "DESTROY " + name);

}




Sequence* Sequence::copy()
{
	Sequence *newSequence = new Sequence();

	for (unsigned nNote = 0; nNote < m_noteList.size(); ++nNote) {
		Note *oldNote = m_noteList[nNote];
		if (oldNote == NULL) {
			newSequence->m_noteList[nNote] = NULL;
		}
		else {
			Note *newNote = oldNote->copy();
			newSequence->m_noteList[nNote] = newNote;
		}
	}

	return newSequence;
}



//:::::::::::::::::::::::::::



Instrument::Instrument(
		const string& sId,
		const string& sName,
		float fVolume,
		bool bMuted,
		bool bLocked,
		float fPan_L,
		float fPan_R,
		const string& sDrumkitName
)
 : Object( "Instrument" )
 , m_bFilterActive( false )
 , m_fCutoff( 1.0 )
 , m_fResonance( 0.0 )
 , m_sId( sId )
 , m_fVolume( fVolume )
 , m_sName( sName )
 , m_bIsMuted( bMuted )
 , m_bIsLocked( bLocked )
 , m_sDrumkitName( sDrumkitName )
 , m_bActive( true )
 , m_fRandomPitchFactor( 0.0 )
 , m_fPan_L( fPan_L )
 , m_fPan_R( fPan_R )
 , m_fPeak_L( 0.0 )
 , m_fPeak_R( 0.0 )
 , m_fGain( 1.0 )
 , m_pADSR( NULL )
{
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		setFXLevel( nFX, 0.0 );
	}

	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
		m_layers[ nLayer ] = NULL;
	}
}



Instrument::~Instrument(){
	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
		delete m_layers[ nLayer ];
	}
	delete m_pADSR;
}



InstrumentLayer* Instrument::getLayer( int nLayer )
{
	if (nLayer < 0 ) {
		errorLog( "[getLayer] nLayer < 0 (nLayer=" + toString(nLayer) + ")" );
		return NULL;
	}
	if (nLayer >= MAX_LAYERS ) {
		errorLog( "[getLayer] nLayer > MAX_LAYERS (nLayer=" + toString(nLayer) + ")" );
		return NULL;
	}

	return m_layers[ nLayer ];
}



void Instrument::setLayer( InstrumentLayer* pLayer, unsigned nLayer )
{
	if (nLayer < MAX_LAYERS) {
		m_layers[ nLayer ] = pLayer;
	}
	else {
		errorLog( "[setLayer] nLayer > MAX_LAYER" );
	}
}




//::::::::::::::::::::::::



Note::Note(
		Instrument *pInstrument,
		unsigned nPosition,
		float fVelocity,
		float fPan_L,
		float fPan_R,
		int nLength,
		float fPitch
)
 : Object( "Note" )
 , m_nPosition( nPosition )
 , m_fVelocity( fVelocity )
 , m_fPan_L( fPan_L )
 , m_fPan_R( fPan_R )
 , m_nLength( nLength )
 , m_fPitch( fPitch )

 , m_fCutoff( 1.0 )
 , m_fResonance( 0.0 )
 , m_fBandPassFilterBuffer_L( 0.0 )
 , m_fBandPassFilterBuffer_R( 0.0 )
 , m_fLowPassFilterBuffer_L( 0.0 )
 , m_fLowPassFilterBuffer_R( 0.0 )
 , m_nHumanizeDelay( 0 )
 , m_pADSR( NULL )
 , m_fSamplePosition( 0.0 )
{
	setInstrument( pInstrument );
}



Note::~Note(){
	//infoLog("DESTROY");
	delete m_pADSR;
}



void Note::setInstrument(Instrument* pInstrument)
{
	m_pInstrument = pInstrument;
	if ( m_pInstrument ) {
		m_pADSR = new ADSR( *(m_pInstrument->m_pADSR) );
	}
	else {
		m_pADSR = NULL;
	}
}




Note* Note::copy()
{
	Note* newNote = new Note(
		m_pInstrument,
		m_nPosition,
		m_fVelocity,
		m_fPan_L,
		m_fPan_R,
		m_nLength,
		m_fPitch
	);

	return newNote;
}



//:::::::::::::::::::::



Pattern::Pattern( const std::string& sName, unsigned nSize )
 : Object( "Pattern" )
 , m_sName( sName )
 , m_nSize( nSize )
{
//	infoLog("init");
}



Pattern::~Pattern() {
//	infoLog("destroy");

	// delete all Sequences
	delete m_pSequenceList;
}




/// Returns an empty Pattern
Pattern* Pattern::getEmptyPattern()
{
	SequenceList *sequenceList = new SequenceList();
	for (unsigned i = 0; i < MAX_INSTRUMENTS; ++i) {
		Sequence *trk0 = new Sequence();
		sequenceList->add(trk0);
	}

	Pattern *pat = new Pattern("Empty pattern");
	pat->m_pSequenceList = sequenceList;

	return pat;
}



Pattern* Pattern::copy()
{
	Pattern *newPat = new Pattern( m_sName );
	newPat->m_nSize = m_nSize;
	newPat->m_pSequenceList = m_pSequenceList->copy();

	return newPat;
}



//::::::::::::::::::::



DrumkitInfo::DrumkitInfo()
 : Object( "DrumkitInfo" )
 , instrumentList( NULL )
{
//	infoLog( "INIT @" + toString( (int)this ) );
}



DrumkitInfo::~DrumkitInfo()
{
//	infoLog( "DESTROY @" + toString( (int)this ) );
	delete instrumentList;
}



void DrumkitInfo::dump()
{
	infoLog( "Drumkit dump" );
	infoLog( "\t|- Name = " + name );
	infoLog( "\t|- Author = " + author );
	infoLog( "\t|- Info = " + info );

	infoLog( "\t|- Instrument list" );
	for ( unsigned nInstrument = 0; nInstrument < instrumentList->getSize(); ++nInstrument) {
		Instrument *pInstr = instrumentList->get( nInstrument );
		infoLog( "\t\t|- (" + toString( nInstrument ) + " of " + toString( instrumentList->getSize() ) + ") Name = " + pInstr->m_sName );
		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
			InstrumentLayer *pLayer = pInstr->getLayer( nLayer );
			if ( pLayer ) {
				Sample *pSample = pLayer->m_pSample;
				if ( pSample ) {
					infoLog( "\t\t   |- " + pSample->m_sFilename );
				}
				else {
					infoLog( "\t\t   |- NULL sample" );
				}
			}
			else {
				infoLog( "\t\t   |- NULL Layer" );
			}

		}
//		cout << "\t\t" << i << " - " << instr->getSample()->getFilename() << endl;
	}
}



//:::::::::::::::::::::



InstrumentLayer::InstrumentLayer( Sample *pSample )
 : Object( "InstrumentLayer" )
 , m_pSample( pSample )
 , m_fStartVelocity( 0.0 )
 , m_fEndVelocity( 1.0 )
 , m_fPitch( 0.0 )
 , m_fGain( 1.0 )
{
	//infoLog( "INIT" );
}



InstrumentLayer::~InstrumentLayer()
{
	delete m_pSample;
	//infoLog( "DESTROY" );
}

