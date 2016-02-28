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

#include <hydrogen/smf/SMFEvent.h>
#include <hydrogen/timehelper.h>

namespace H2Core
{

const char* SMFBuffer::__class_name = "SMFBuffer";

SMFBuffer::SMFBuffer() : Object( __class_name ) { }

void SMFBuffer::writeByte( short int nByte )
{
//	infoLog( "[writeByte] " + to_string( nByte ) );
	m_buffer.push_back( nByte );
}



void SMFBuffer::writeWord( int nVal )
{
//	infoLog( "writeWord" );
	writeByte( nVal >> 8 );
	writeByte( nVal );
}



void SMFBuffer::writeDWord( long nVal )
{
	writeByte( nVal >> 24 );
	writeByte( nVal >> 16 );
	writeByte( nVal >> 8 );
	writeByte( nVal );
}



void SMFBuffer::writeString( const QString& sMsg )
{
//	infoLog( "writeString" );
	writeVarLen( sMsg.length() );

	for ( int i = 0; i < sMsg.length(); i++ ) {
				writeByte( sMsg.toLocal8Bit().at(i) );
	}
}



void SMFBuffer::writeVarLen( long value )
{
//	infoLog( "[writeVarLen]" );
	long buffer;
	buffer = value & 0x7f;
	while ( ( value >>= 7 ) > 0 ) {
		INFOLOG( "." );
		buffer <<= 8;
		buffer |= 0x80;
		buffer += ( value & 0x7f );
	}

	while ( true ) {
//		putc( buffer, outfile );
		writeByte( ( char )buffer );
		if ( buffer & 0x80 ) {
			buffer >>= 8;
		} else {
			break;
		}
	}
}


// ::::::::::::::::::

const char* SMFTrackNameMetaEvent::__class_name = "SMFTrackNameMetaEvent";

SMFTrackNameMetaEvent::SMFTrackNameMetaEvent( const QString& sTrackName, unsigned nTicks )
		: SMFEvent( __class_name, nTicks )
		, m_sTrackName( sTrackName )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


std::vector<char> SMFTrackNameMetaEvent::getBuffer()
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( TRACK_NAME );
	buf.writeString( m_sTrackName );

	return buf.getBuffer();
}

// ::::::::::::::::::

const char* SMFSetTempoMetaEvent::__class_name = "SMFSetTempoMetaEvent";

SMFSetTempoMetaEvent::SMFSetTempoMetaEvent( float fBPM, unsigned nTicks )
		: SMFEvent( __class_name, nTicks )
		, m_fBPM( fBPM )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


std::vector<char> SMFSetTempoMetaEvent::getBuffer()
{
	SMFBuffer buf;
	long msPerBeat;
	
	msPerBeat = long( 60000000 / m_fBPM ); // 60 seconds * mills \ BPM
	
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( SET_TEMPO );
	buf.writeByte( 0x03 );	// Length 
	
	buf.writeByte( msPerBeat >> 16 );
	buf.writeByte( msPerBeat >> 8 );
	buf.writeByte( msPerBeat );
	
	return buf.getBuffer();
}

// ::::::::::::::::::

const char* SMFCopyRightNoticeMetaEvent::__class_name = "SMFCopyRightNoticeMetaEvent";

SMFCopyRightNoticeMetaEvent::SMFCopyRightNoticeMetaEvent( const QString& sAuthor, unsigned nTicks )
		: SMFEvent( __class_name, nTicks )
		, m_sAuthor( sAuthor )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


std::vector<char> SMFCopyRightNoticeMetaEvent::getBuffer()
{
	SMFBuffer buf;
	QString sCopyRightString;
	
	time_t now = time(0);
	tm *ltm = localtime(&now);						// Extract the local system time.
	
	// Construct the copyright string in the form "(C) [Author] [CurrentYear]"
	sCopyRightString.append("(C) ");				// Start with the copyright symbol and a seperator space.
	sCopyRightString.append( m_sAuthor );			// add the author
	sCopyRightString.append(" ");					// add a seperator space
	sCopyRightString.append( QString::number( 1900 + ltm->tm_year, 10 ) );	// and finish with the year.
	
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( COPYRIGHT_NOTICE );
	buf.writeString( sCopyRightString );

	return buf.getBuffer();
}

// ::::::::::::::::::
		
const char* SMFTimeSignatureMetaEvent::__class_name = "SMFTimeSignatureMetaEvent";

SMFTimeSignatureMetaEvent::SMFTimeSignatureMetaEvent( unsigned nBeats, unsigned nNote , unsigned nMTPMC , unsigned nTSNP24 , unsigned nTicks )
		: SMFEvent( __class_name, nTicks )
		, m_nBeats( nBeats )
		, m_nNote( nNote )
		, m_nMTPMC( nMTPMC )
		, m_nTSNP24( nTSNP24 )
		, m_nTicks( nTicks )

{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


std::vector<char> SMFTimeSignatureMetaEvent::getBuffer()
{
	SMFBuffer buf;
	
	unsigned nBeatsCopy = m_nNote , Note2Log =  0;	// Copy Nbeats as the process to generate Note2Log alters the value.
	
	while (nBeatsCopy >>= 1) ++Note2Log;			// Generate a log to base 2 of the note value, so 8 (as in 6/8) becomes 3
	
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( TIME_SIGNATURE );
	buf.writeByte( 0x04 );		// Event length in bytes.
	buf.writeByte( m_nBeats );	// Top line of time signature, eg 6 for 6/8 time
	buf.writeByte( Note2Log );	// Bottom line of time signature expressed as Log2 of the Note value. 
	buf.writeByte( m_nMTPMC );	// MIDI Ticks per Metronome click, normally 24 ( i.e. each quarter note ).
	buf.writeByte( m_nTSNP24 );	// Thirty Second Notes ( as in 1/32 ) per 24 MIDI clocks, normally 8.

	return buf.getBuffer();
}

// :::::::::::::


SMFEvent::SMFEvent( const char* sEventName, unsigned nTicks )
		: Object( sEventName )
		, m_nTicks( nTicks )
		, m_nDeltaTime( -1 )
{
	//infoLog( "INIT" );
}



SMFEvent::~SMFEvent()
{
	//infoLog( "DESTROY" );
}


// ::::::::::::::

const char* SMFNoteOnEvent::__class_name = "SMFNoteOnEvent";

SMFNoteOnEvent::SMFNoteOnEvent( unsigned nTicks, int nChannel, int nPitch, int nVelocity )
		: SMFEvent( __class_name, nTicks )
		, m_nChannel( nChannel )
		, m_nPitch( nPitch )
		, m_nVelocity( nVelocity )
{
	if ( nChannel >= 16 ) {
		ERRORLOG( QString( "nChannel >= 16! nChannel=%1" ).arg( nChannel ) );
	}
}



std::vector<char> SMFNoteOnEvent::getBuffer()
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( NOTE_ON + m_nChannel );
	buf.writeByte( m_nPitch );
	buf.writeByte( m_nVelocity );

	return buf.getBuffer();
}


// :::::::::::


const char* SMFNoteOffEvent::__class_name = "SMFNoteOffEvent";

SMFNoteOffEvent::SMFNoteOffEvent( unsigned nTicks, int nChannel, int nPitch, int nVelocity )
		: SMFEvent( __class_name, nTicks )
		, m_nChannel( nChannel )
		, m_nPitch( nPitch )
		, m_nVelocity( nVelocity )
{
	if ( nChannel >= 16 ) {
		ERRORLOG( QString( "nChannel >= 16! nChannel=%1" ).arg( nChannel ) );
	}
}



std::vector<char> SMFNoteOffEvent::getBuffer()
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( NOTE_OFF + m_nChannel );
	buf.writeByte( m_nPitch );
	buf.writeByte( m_nVelocity );

	return buf.getBuffer();
}

};
