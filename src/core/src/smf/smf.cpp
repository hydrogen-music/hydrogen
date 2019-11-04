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

#include <hydrogen/smf/SMF.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/automation_path.h>

#include <fstream>

using std::vector;

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



vector<char> SMFHeader::getBuffer()
{
	SMFBuffer buffer;

	buffer.writeDWord( 1297377380 );		// MThd
	buffer.writeDWord( 6 );				// Header length = 6
	buffer.writeWord( m_nFormat );
	buffer.writeWord( m_nTracks + 1 );
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
	vector<char> trackData;

	for ( unsigned i = 0; i < m_eventList.size(); i++ ) {
		SMFEvent *pEv = m_eventList[ i ];
		vector<char> buf = pEv->getBuffer();

		// copy the buffer into the data vector
		for ( unsigned j = 0; j < buf.size(); j++ ) {
			trackData.push_back( buf[ j ] );
		}
	}


	SMFBuffer buf;

	buf.writeDWord( 1297379947 );		// MTrk
	buf.writeDWord( trackData.size() + 4 );	// Track length

	vector<char> trackBuf = buf.getBuffer();

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

SMF::SMF()
		: Object( __class_name )
{
	INFOLOG( "INIT" );

	m_pHeader = new SMFHeader( 1, -1, 192 );
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
	m_pHeader->m_nTracks++;
	m_trackList.push_back( pTrack );
}



vector<char> SMF::getBuffer()
{
	vector<char> smfVect;

	// header
	vector<char> headerVect = m_pHeader->getBuffer();
	for ( unsigned i = 0; i < headerVect.size(); i++ ) {
		smfVect.push_back( headerVect[ i ] );
	}


	// tracks
	for ( unsigned nTrack = 0; nTrack < m_trackList.size(); nTrack++ ) {
		SMFTrack *pTrack = m_trackList[ nTrack ];
		vector<char> trackVect = pTrack->getBuffer();
		for ( unsigned i = 0; i < trackVect.size(); i++ ) {
			smfVect.push_back( trackVect[ i ] );
		}
	}

	return smfVect;
}



// :::::::::::::::::::...


const char* SMFWriter::__class_name = "SMFWriter";

SMFWriter::SMFWriter()
		: Object( __class_name )
		, m_file( nullptr )
{
	INFOLOG( "INIT" );
}



SMFWriter::~SMFWriter()
{
	INFOLOG( "DESTROY" );
}



void SMFWriter::save( const QString& sFilename, Song *pSong )
{
	INFOLOG( "save" );
	const int DRUM_CHANNEL = 9;

	vector<SMFEvent*> eventList;

	SMF smf;


	// Standard MIDI format 1 files should have the first track being the tempo map
	// which is a track that contains global meta events only.
	SMFTrack *pTrack0 = new SMFTrack();
	pTrack0->addEvent( new SMFCopyRightNoticeMetaEvent( pSong->__author , 0 ) );
	pTrack0->addEvent( new SMFTrackNameMetaEvent( pSong->__name , 0 ) );
	pTrack0->addEvent( new SMFSetTempoMetaEvent( pSong->__bpm , 0 ) );
	pTrack0->addEvent( new SMFTimeSignatureMetaEvent( 4 , 4 , 24 , 8 , 0 ) );
	smf.addTrack( pTrack0 );

	
	// Standard MIDI Format 1 files should have note events in tracks =>2
	SMFTrack *pTrack1 = new SMFTrack();
	smf.addTrack( pTrack1 );

	AutomationPath *vp = pSong->get_velocity_automation_path();

	InstrumentList *iList = pSong->get_instrument_list();
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
						float threshold = pSong->get_threshold();
						if ( pNote->get_probability() < threshold ) {
							continue;
						}

						float fPos = nPatternList + (float)nNote/(float)nMaxPatternLength;
						float velocity_adjustment = vp->get_value(fPos);
						int nVelocity =
							(int)( 127.0 * pNote->get_velocity() * velocity_adjustment );
						
						int nInstr = iList->index(pNote->get_instrument());
						Instrument *pInstr = pNote->get_instrument();
						int nPitch = pNote->get_midi_key();
						
						eventList.push_back(
							new SMFNoteOnEvent(
								nStartTicks + nNote,
								DRUM_CHANNEL,
								nPitch,
								nVelocity
								)
							);
						int nLength = 12;
						if ( pNote->get_length() != -1 ) {
							nLength = pNote->get_length();
						}
						eventList.push_back(
							new SMFNoteOffEvent(
								nStartTicks + nNote + nLength,
								DRUM_CHANNEL,
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

	// awful bubble sort..
	for ( unsigned i = 0; i < eventList.size(); i++ ) {
		for ( vector<SMFEvent*>::iterator it = eventList.begin() ;
			  it != ( eventList.end() - 1 ) ;
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

	unsigned nLastTick = 1;
	for ( vector<SMFEvent*>::iterator it = eventList.begin() ;
		  it != eventList.end();
		  it++ ) {
		SMFEvent *pEvent = *it;
		pEvent->m_nDeltaTime = ( pEvent->m_nTicks - nLastTick ) * 4;
		nLastTick = pEvent->m_nTicks;

		// infoLog( " pos: " + toString( (*it)->m_nTicks ) + ", delta: "
		//          + toString( (*it)->m_nDeltaTime ) );

		pTrack1->addEvent( *it );
	}

	// save the midi file
	m_file = fopen( sFilename.toLocal8Bit(), "wb" );

	if( m_file == nullptr )
		return;

	vector<char> smfVect = smf.getBuffer();
	for ( unsigned i = 0; i < smfVect.size(); i++ ) {
		fwrite( &smfVect[ i ], 1, 1, m_file );
	}
	fclose( m_file );
}

};
