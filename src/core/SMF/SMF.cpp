/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/SMF/SMF.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/AutomationPath.h>

#include <QFile>
#include <QTextCodec>
#include <QTextStream>

namespace H2Core
{

SMFHeader::SMFHeader( int nFormat, int nTracks, int nTPQN )
		: m_nFormat( nFormat )
		, m_nTracks( nTracks )
		, m_nTPQN( nTPQN )
{
	
}


SMFHeader::~SMFHeader()
{
	INFOLOG( "DESTROY" );
}

void SMFHeader::addTrack() {
	m_nTracks++;	
}

QByteArray SMFHeader::getBuffer() const
{
	SMFBuffer buffer;

	buffer.writeDWord( 1297377380 );		// MThd
	buffer.writeDWord( 6 );				// Header length = 6
	buffer.writeWord( m_nFormat );
	buffer.writeWord( m_nTracks );
	buffer.writeWord( m_nTPQN );

	return buffer.m_buffer;
}

QString SMFHeader::toQString() const {
	return QString( getBuffer().toHex( ' ' ) );
}



// :::::::::::::::

SMFTrack::SMFTrack()
		: Object()
{
	
}



SMFTrack::~SMFTrack()
{
	INFOLOG( "DESTROY" );

	for ( unsigned i = 0; i < m_eventList.size(); i++ ) {
		delete m_eventList[ i ];
	}
}



QByteArray SMFTrack::getBuffer() const
{
	QByteArray trackData;

	for ( unsigned i = 0; i < m_eventList.size(); i++ ) {
		SMFEvent *pEv = m_eventList[ i ];
		auto buf = pEv->getBuffer();

		for ( unsigned j = 0; j < buf.size(); j++ ) {
			trackData.push_back( buf[ j ] );
		}
	}


	SMFBuffer buf;

	buf.writeDWord( 1297379947 );		// MTrk
	buf.writeDWord( trackData.size() + 4 );	// Track length

	auto trackBuf = buf.getBuffer();

	for ( unsigned i = 0; i < trackData.size(); i++ ) {
		trackBuf.push_back( trackData[i] );
	}


	//  track end
	trackBuf.push_back( static_cast<char>(0x00) );		// delta
	trackBuf.push_back( static_cast<char>(0xFF) );
	trackBuf.push_back( static_cast<char>(0x2F) );
	trackBuf.push_back( static_cast<char>(0x00) );



	return trackBuf;
}

QString SMFTrack::toQString() const {
	return QString( getBuffer().toHex( ' ' ) );
}

void SMFTrack::addEvent( SMFEvent *pEvent )
{
	m_eventList.push_back( pEvent );
}



// ::::::::::::::::::::::

SMF::SMF(int nFormat, int nTPQN )
{
	

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



QByteArray SMF::getBuffer() const
{
	// header
	auto smfBuffer = m_pHeader->getBuffer();

	// tracks
	for ( unsigned nTrack = 0; nTrack < m_trackList.size(); nTrack++ ) {
		SMFTrack *pTrack = m_trackList[ nTrack ];
		smfBuffer.append( pTrack->getBuffer() );
	}

	return smfBuffer;
}

QString SMF::toQString() const {
	return QString( getBuffer().toHex( ' ' ) );
}



// :::::::::::::::::::...

constexpr unsigned int TPQN = 192;
constexpr unsigned int DRUM_CHANNEL = 9;
constexpr unsigned int NOTE_LENGTH = 12;

SMFWriter::SMFWriter()
{
	
}


SMFWriter::~SMFWriter()
{
	INFOLOG( "DESTROY" );
}


SMFTrack* SMFWriter::createTrack0( std::shared_ptr<Song> pSong ) {
	SMFTrack *pTrack0 = new SMFTrack();
	pTrack0->addEvent( new SMFCopyRightNoticeMetaEvent( pSong->getAuthor() , 0 ) );
	pTrack0->addEvent( new SMFTrackNameMetaEvent( pSong->getName() , 0 ) );
	pTrack0->addEvent( new SMFSetTempoMetaEvent( pSong->getBpm() , 0 ) );
	pTrack0->addEvent( new SMFTimeSignatureMetaEvent( 4 , 4 , 24 , 8 , 0 ) );
	return pTrack0;
}


void SMFWriter::save( const QString& sFilename, std::shared_ptr<Song> pSong )
{
	INFOLOG( QString( "Export MIDI to [%1]" ).arg( sFilename ) );

	SMF* pSmf = createSMF( pSong );

	AutomationPath* pAutomationPath = pSong->getVelocityAutomationPath();

	// here writers must prepare to receive pattern events
	prepareEvents( pSong, pSmf );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	// ogni pattern sara' una diversa traccia
	int nTick = 1;
	for ( unsigned nPatternList = 0 ;
		  nPatternList < pSong->getPatternGroupVector()->size() ;
		  nPatternList++ ) {
		// infoLog( "[save] pattern list pos: " + toString( nPatternList ) );
		PatternList *pPatternList =
			( *(pSong->getPatternGroupVector()) )[ nPatternList ];

		int nStartTicks = nTick;
		int nMaxPatternLength = 0;
		for ( unsigned nPattern = 0 ;
			  nPattern < pPatternList->size() ;
			  nPattern++ ) {
			auto pPattern = pPatternList->get( nPattern );
			// infoLog( "      |-> pattern: " + pPattern->getName() );
			if ( ( int )pPattern->getLength() > nMaxPatternLength ) {
				nMaxPatternLength = pPattern->getLength();
			}

			for ( unsigned nNote = 0; nNote < pPattern->getLength(); nNote++ ) {
				const Pattern::notes_t* notes = pPattern->getNotes();
				FOREACH_NOTE_CST_IT_BOUND_LENGTH( notes, it, nNote, pPattern ) {
					auto pNote = it->second;
					if ( pNote != nullptr &&
						 pNote->getInstrument() != nullptr ) {
						float rnd = (float)rand()/(float)RAND_MAX;
						if ( pNote->getProbability() < rnd ) {
							continue;
						}

						float fPos = nPatternList + (float)nNote/(float)nMaxPatternLength;
						float fVelocityAdjustment =  pAutomationPath->get_value(fPos);
						int nVelocity =
							(int)( 127.0 * pNote->getVelocity() * fVelocityAdjustment );

						auto pInstr = pNote->getInstrument();
						int nPitch = pNote->getMidiKey();
						
						int nChannel =  pInstr->get_midi_out_channel();
						if ( nChannel == -1 ) {
							nChannel = DRUM_CHANNEL;
						}
						
						int nLength = pNote->getLength();
						if ( nLength == LENGTH_ENTIRE_SAMPLE ) {
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
	QFile file( sFilename );
	if ( ! file.open( QIODevice::WriteOnly ) ) {
		ERRORLOG( QString( "Unable to open file [%1] for writing" )
				  .arg( sFilename ) );
		return;
	}

	QDataStream stream( &file );
	const auto buffer = pSmf->getBuffer();
	stream.writeRawData( buffer.constData(), buffer.size() );

	file.close();
}


// SMF1Writer - base class for two smf1 writers

SMF1Writer::SMF1Writer()
		: SMFWriter()
{
}


SMF1Writer::~SMF1Writer()
{
}


SMF* SMF1Writer::createSMF( std::shared_ptr<Song> pSong ){
	SMF* pSmf =  new SMF( 1, TPQN );	
	// Standard MIDI format 1 files should have the first track being the tempo map
	// which is a track that contains global meta events only.

	SMFTrack* pTrack0 = createTrack0( pSong );
	pSmf->addTrack( pTrack0 );
	
	// Standard MIDI Format 1 files should have note events in tracks =>2
	return pSmf;
}


// SMF1 MIDI SINGLE EXPROT


SMF1WriterSingle::SMF1WriterSingle()
		: SMF1Writer(),
		 m_eventList()
{
}



SMF1WriterSingle::~SMF1WriterSingle()
{
}



EventList* SMF1WriterSingle::getEvents( std::shared_ptr<Song> pSong, std::shared_ptr<Instrument> pInstr )
{
	return &m_eventList;
}



void SMF1WriterSingle::prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
{
   m_eventList.clear();
}



void SMF1WriterSingle::packEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
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

SMF1WriterMulti::SMF1WriterMulti()
		: SMF1Writer(),
		 m_eventLists()
{
}


SMF1WriterMulti::~SMF1WriterMulti()
{
}


void SMF1WriterMulti::prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
{
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	m_eventLists.clear();
	for( unsigned nInstr=0; nInstr <  pInstrumentList->size(); nInstr++ ){
		m_eventLists.push_back( new EventList() );
	}
}


EventList* SMF1WriterMulti::getEvents( std::shared_ptr<Song> pSong,  std::shared_ptr<Instrument> pInstr )
{
	int nInstr = pSong->getDrumkit()->getInstruments()->index(pInstr);
	EventList* pEventList = m_eventLists.at( nInstr );
	
	return pEventList;
}


void SMF1WriterMulti::packEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
{
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	for ( unsigned nTrack = 0; nTrack < m_eventLists.size(); nTrack++ ) {
		EventList* pEventList = m_eventLists.at( nTrack );
		auto instrument =  pInstrumentList->get( nTrack );

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

SMF0Writer::SMF0Writer()
		: SMFWriter(),
		  m_pTrack( nullptr ),
		 m_eventList()
{
}


SMF0Writer::~SMF0Writer()
{
}


SMF* SMF0Writer::createSMF( std::shared_ptr<Song> pSong ){
	// MIDI files format 0 have all their events in one track
	SMF* pSmf =  new SMF( 0, TPQN );	
	m_pTrack = createTrack0( pSong );
	pSmf->addTrack( m_pTrack );
	return pSmf;
}


EventList* SMF0Writer::getEvents( std::shared_ptr<Song> pSong,  std::shared_ptr<Instrument> pInstr )
{
	return &m_eventList;
}


void SMF0Writer::prepareEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
{
   m_eventList.clear();
}


void SMF0Writer::packEvents( std::shared_ptr<Song> pSong, SMF* pSmf )
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
