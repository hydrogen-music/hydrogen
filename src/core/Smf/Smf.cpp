/*
 * Hydrogen
 * Copyright(c) 2002-2004 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <core/Smf/SMF.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/AutomationPath.h>
#include <fstream>

namespace H2Core
{

const char* SMFHeader::__class_name = "SMFHeader";

SMFHeader::SMFHeader( int nFormat, int nTracks, int nTPQN )
		: Object( __class_name )
		, m_nFormat( nFormat )
		, m_nTracks( nTracks )
		, m_nTPQN( nTPQN )
{
	INFOLOG( "INIT" );
}


SMFHeader::~SMFHeader()
{
	INFOLOG( "DESTROY" );
}

void SMFHeader::addTrack() {
	m_nTracks++;	
}

std::vector<char> SMFHeader::getBuffer()
{
	SMFBuffer buffer;

	buffer.writeDWord( 1297377380 );		// MThd
	buffer.writeDWord( 6 );				// Header length = 6
	buffer.writeWord( m_nFormat );
	buffer.writeWord( m_nTracks );
	buffer.writeWord( m_nTPQN );

	return buffer.m_buffer;
}



// :::::::::::::::


const char* SMFTrack::__class_name = "SMFTrack";

//SMFTrack::SMFTrack( const QString& sTrackName )
SMFTrack::SMFTrack()
		: Object( __class_name )
{
	INFOLOG( "INIT" );
}



SMFTrack::~SMFTrack()
{
	INFOLOG( "DESTROY" );

	for ( unsigned i = 0; i < m_eventList.size(); i++ ) {
		delete m_eventList[ i ];
	}
}



std::vector<char> SMFTrack::getBuffer()
{
	// fill the data vector
	std::vector<char> trackData;

	for ( unsigned i = 0; i < m_eventList.size(); i++ ) {
		SMFEvent *pEv = m_eventList[ i ];
		std::vector<char> buf = pEv->getBuffer();

		// copy the buffer into the data vector
		for ( unsigned j = 0; j < buf.size(); j++ ) {
			trackData.push_back( buf[ j ] );
		}
	}


	SMFBuffer buf;

	buf.writeDWord( 1297379947 );		// MTrk
	buf.writeDWord( trackData.size() + 4 );	// Track length

	std::vector<char> trackBuf = buf.getBuffer();

	for ( unsigned i = 0; i < trackData.size(); i++ ) {
		trackBuf.push_back( trackData[i] );
	}


	//  track end
	trackBuf.push_back( 0x00 );		// delta
	trackBuf.push_back( 0xFF );
	trackBuf.push_back( 0x2F );
	trackBuf.push_back( 0x00 );



	return trackBuf;
}



void SMFTrack::addEvent( SMFEvent *pEvent )
{
	m_eventList.push_back( pEvent );
}



// ::::::::::::::::::::::

const char* SMF::__class_name = "SMF";

SMF::SMF(int nFormat, int nTPQN )
		: Object( __class_name )
{
	INFOLOG( "INIT" );

	m_pHeader = new SMFHeader( nFormat, 0, nTPQN );
}



SMF::~SMF()
{
	INFOLOG( "DESTROY" );

	delete m_pHeader;

	for ( unsigned i = 0; i < m_trackList.size(); i++ ) {
		delete m_trackList[i];
	}
}



void SMF::addTrack( SMFTrack *pTrack )
{
	m_pHeader->addTrack();
	m_trackList.push_back( pTrack );
}



std::vector<char> SMF::getBuffer()
{
	std::vector<char> smfVect;

	// header
	std::vector<char> headerVect = m_pHeader->getBuffer();
	for ( unsigned i = 0; i < headerVect.size(); i++ ) {
		smfVect.push_back( headerVect[ i ] );
	}


	// tracks
	for ( unsigned nTrack = 0; nTrack < m_trackList.size(); nTrack++ ) {
		SMFTrack *pTrack = m_trackList[ nTrack ];
		std::vector<char> trackVect = pTrack->getBuffer();
		for ( unsigned i = 0; i < trackVect.size(); i++ ) {
			smfVect.push_back( trackVect[ i ] );
		}
	}

	return smfVect;
}



// :::::::::::::::::::...

constexpr unsigned int TPQN = 192;
constexpr unsigned int DRUM_CHANNEL = 9;
constexpr unsigned int NOTE_LENGTH = 12;

const char* SMFWriter::__class_name = "SMFWriter";

SMFWriter::SMFWriter( const char* sWriterName )
		: Object( sWriterName )
{
	INFOLOG( "INIT" );
}


SMFWriter::~SMFWriter()
{
	INFOLOG( "DESTROY" );
}


SMFTrack* SMFWriter::createTrack0( Song* pSong ) {
	SMFTrack *pTrack0 = new SMFTrack();
	pTrack0->addEvent( new SMFCopyRightNoticeMetaEvent( pSong->__author , 0 ) );
	pTrack0->addEvent( new SMFTrackNameMetaEvent( pSong->__name , 0 ) );
	pTrack0->addEvent( new SMFSetTempoMetaEvent( pSong->__bpm , 0 ) );
	pTrack0->addEvent( new SMFTimeSignatureMetaEvent( 4 , 4 , 24 , 8 , 0 ) );
	return pTrack0;
}


void SMFWriter::save( const QString& sFilename, Song *pSong )
{
	INFOLOG( "save" );

	SMF* pSmf = createSMF( pSong );

	AutomationPath* pAutomationPath = pSong->get_velocity_automation_path();

	// here writers must prepare to receive pattern events
	prepareEvents( pSong, pSmf );

	InstrumentList* pInstrumentList = pSong->get_instrument_list();
	// ogni pattern sara' una diversa traccia
	int nTick = 1;
	for ( unsigned nPatternList = 0 ;
		  nPatternList < pSong->get_pattern_group_vector()->size() ;
		  nPatternList++ ) {
		// infoLog( "[save] pattern list pos: " + toString( nPatternList ) );
		PatternList *pPatternList =
			( *(pSong->get_pattern_group_vector()) )[ nPatternList ];

		int nStartTicks = nTick;
		int nMaxPatternLength = 0;
		for ( unsigned nPattern = 0 ;
			  nPattern < pPatternList->size() ;
			  nPattern++ ) {
			Pattern *pPattern = pPatternList->get( nPattern );
			// infoLog( "      |-> pattern: " + pPattern->getName() );
			if ( ( int )pPattern->get_length() > nMaxPatternLength ) {
				nMaxPatternLength = pPattern->get_length();
			}

			for ( unsigned nNote = 0; nNote < pPattern->get_length(); nNote++ ) {
				const Pattern::notes_t* notes = pPattern->get_notes();
				FOREACH_NOTE_CST_IT_BOUND(notes,it,nNote) {
					Note *pNote = it->second;
					if ( pNote ) {
						float rnd = (float)rand()/(float)RAND_MAX;
						if ( pNote->get_probability() < rnd ) {
							continue;
						}

						float fPos = nPatternList + (float)nNote/(float)nMaxPatternLength;
						float fVelocityAdjustment =  pAutomationPath->get_value(fPos);
						int nVelocity =
							(int)( 127.0 * pNote->get_velocity() * fVelocityAdjustment );

						Instrument *pInstr = pNote->get_instrument();
						int nPitch = pNote->get_midi_key();
						
						int nChannel =  pInstr->get_midi_out_channel();
						if ( nChannel == -1 ) {
							nChannel = DRUM_CHANNEL;
						}
						
						int nLength = pNote->get_length();
						if ( nLength == -1 ) {
							nLength = NOTE_LENGTH;
						}
						
						// get events for specific instrument
						EventList* eventList = getEvents(pSong, pInstr);
						eventList->push_back(
							new SMFNoteOnEvent(
								nStartTicks + nNote,
								nChannel,
								nPitch,
								nVelocity
								)
							);
							
						eventList->push_back(
							new SMFNoteOffEvent(
								nStartTicks + nNote + nLength,
								nChannel,
								nPitch,
								nVelocity
								)
							);
					}
				}
			}
		}
		nTick += nMaxPatternLength;
	}

	//tracks creation
	packEvents(pSong, pSmf);

	saveSMF(sFilename, pSmf);
	delete pSmf;
}


void SMFWriter::sortEvents( EventList *pEvents )
{
	// awful bubble sort..
	for ( unsigned i = 0; i < pEvents->size(); i++ ) {
		for ( std::vector<SMFEvent*>::iterator it = pEvents->begin() ;
			  it != ( pEvents->end() - 1 ) ;
			  it++ ) {
			SMFEvent *pEvent = *it;
			SMFEvent *pNextEvent = *( it + 1 );
			if ( pNextEvent->m_nTicks < pEvent->m_nTicks ) {
				// swap
				*it = pNextEvent;
				*( it +1 ) = pEvent;
			}
		}
	}
}


void SMFWriter::saveSMF( const QString& sFilename, SMF*  pSmf )
{
	// save the midi file
	FILE* pFile = fopen( sFilename.toLocal8Bit(), "wb" );

	if( pFile == nullptr ) {
		return;
	}

	std::vector<char> smfVect = pSmf->getBuffer();
	for ( unsigned i = 0; i < smfVect.size(); i++ ) {
		fwrite( &smfVect[ i ], 1, 1, pFile );
	}
	fclose( pFile );
}


// SMF1Writer - base class for two smf1 writers

const char* SMF1Writer::__class_name = "SMF1Writer";

SMF1Writer::SMF1Writer( const char* sWriterName )
		: SMFWriter( sWriterName )
{
}


SMF1Writer::~SMF1Writer()
{
}


SMF* SMF1Writer::createSMF( Song* pSong ){
	SMF* pSmf =  new SMF( 1, TPQN );	
	// Standard MIDI format 1 files should have the first track being the tempo map
	// which is a track that contains global meta events only.

	SMFTrack* pTrack0 = createTrack0( pSong );
	pSmf->addTrack( pTrack0 );
	
	// Standard MIDI Format 1 files should have note events in tracks =>2
	return pSmf;
}


// SMF1 MIDI SINGLE EXPROT


const char* SMF1WriterSingle::__class_name = "SMFWriterSingle";

SMF1WriterSingle::SMF1WriterSingle()
		: SMF1Writer( __class_name ),
		 m_eventList()
{
}



SMF1WriterSingle::~SMF1WriterSingle()
{
}



EventList* SMF1WriterSingle::getEvents( Song* pSong, Instrument* pInstr )
{
	return &m_eventList;
}



void SMF1WriterSingle::prepareEvents( Song *pSong, SMF* pSmf )
{
   m_eventList.clear();
}



void SMF1WriterSingle::packEvents( Song *pSong, SMF* pSmf )
{
	sortEvents( &m_eventList );

	SMFTrack *pTrack1 = new SMFTrack();
	pSmf->addTrack( pTrack1 );

	unsigned nLastTick = 1;
	for( auto& pEvent : m_eventList ) {
		pEvent->m_nDeltaTime = ( pEvent->m_nTicks - nLastTick ) * 4;
		nLastTick = pEvent->m_nTicks;

		// infoLog( " pos: " + toString( (*it)->m_nTicks ) + ", delta: "
		//          + toString( (*it)->m_nDeltaTime ) );

		pTrack1->addEvent( pEvent );
	}

	m_eventList.clear();
}



// SMF1 MIDI MULTI EXPORT

const char* SMF1WriterMulti::__class_name = "SMFWriterMulti";

SMF1WriterMulti::SMF1WriterMulti()
		: SMF1Writer( __class_name ),
		 m_eventLists()
{
}


SMF1WriterMulti::~SMF1WriterMulti()
{
}


void SMF1WriterMulti::prepareEvents( Song *pSong, SMF* pSmf )
{
	InstrumentList* pInstrumentList = pSong->get_instrument_list();
	m_eventLists.clear();
	for( unsigned nInstr=0; nInstr <  pInstrumentList->size(); nInstr++ ){
		m_eventLists.push_back( new EventList() );
	}
}


EventList* SMF1WriterMulti::getEvents( Song* pSong,  Instrument* pInstr )
{
	int nInstr = pSong->get_instrument_list()->index(pInstr);
	EventList* pEventList = m_eventLists.at( nInstr );
	
	return pEventList;
}


void SMF1WriterMulti::packEvents( Song *pSong, SMF* pSmf )
{
	InstrumentList* pInstrumentList = pSong->get_instrument_list();
	for ( unsigned nTrack = 0; nTrack < m_eventLists.size(); nTrack++ ) {
		EventList* pEventList = m_eventLists.at( nTrack );
		Instrument* instrument =  pInstrumentList->get( nTrack );

		sortEvents( pEventList );

		SMFTrack *pTrack = new SMFTrack();
		pSmf->addTrack( pTrack );
		
		//Set instrument name as track name
		pTrack->addEvent( new SMFTrackNameMetaEvent( instrument->get_name() , 0 ) );
		
		unsigned nLastTick = 1;
		for ( std::vector<SMFEvent*>::iterator it = pEventList->begin();
			it != pEventList->end();
			 it++ ) {
			SMFEvent *pEvent = *it;
			pEvent->m_nDeltaTime = ( pEvent->m_nTicks - nLastTick ) * 4;
			nLastTick = pEvent->m_nTicks;

			pTrack->addEvent( *it );
		}

		// we can safely delete vector with events now
		delete pEventList;
	}
	m_eventLists.clear();
}


// SMF0 MIDI  EXPORT

const char* SMF0Writer::__class_name = "SMF0Writer";

SMF0Writer::SMF0Writer()
		: SMFWriter( __class_name ),
		  m_pTrack( nullptr ),
		 m_eventList()
{
}


SMF0Writer::~SMF0Writer()
{
}


SMF* SMF0Writer::createSMF( Song* pSong ){
	// MIDI files format 0 have all their events in one track
	SMF* pSmf =  new SMF( 0, TPQN );	
	m_pTrack = createTrack0( pSong );
	pSmf->addTrack( m_pTrack );
	return pSmf;
}


EventList* SMF0Writer::getEvents( Song* pSong,  Instrument* pInstr )
{
	return &m_eventList;
}


void SMF0Writer::prepareEvents( Song *pSong, SMF* pSmf )
{
   m_eventList.clear();
}


void SMF0Writer::packEvents( Song *pSong, SMF* pSmf )
{
	sortEvents( &m_eventList );

	unsigned nLastTick = 1;
	for ( std::vector<SMFEvent*>::iterator it = m_eventList.begin();
		it != m_eventList.end();
		 it++ ) {
		SMFEvent *pEvent = *it;
		pEvent->m_nDeltaTime = ( pEvent->m_nTicks - nLastTick ) * 4;
		nLastTick = pEvent->m_nTicks;
		
		m_pTrack->addEvent( *it );
	}

	m_eventList.clear();
}


};
