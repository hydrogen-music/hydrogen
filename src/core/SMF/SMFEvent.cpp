/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/SMF/SMFEvent.h>
#include <core/Timehelper.h>

namespace H2Core
{

SMFBuffer::SMFBuffer() {}

void SMFBuffer::writeByte( char nChar ) {
	m_buffer.push_back( nChar );
}



void SMFBuffer::writeWord( int nVal ) {
	writeByte( nVal >> 8 );
	writeByte( nVal );
}



void SMFBuffer::writeDWord( long nVal ) {
	writeByte( nVal >> 24 );
	writeByte( nVal >> 16 );
	writeByte( nVal >> 8 );
	writeByte( nVal );
}



void SMFBuffer::writeString( const QString& sMsg ) {
	writeVarLen( sMsg.length() );

	m_buffer.append( sMsg.toUtf8() );
}



void SMFBuffer::writeVarLen( long value ) {
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

SMFTrackNameMetaEvent::SMFTrackNameMetaEvent( const QString& sTrackName,
											  int nTicks )
		: SMFEvent( nTicks )
		, m_sTrackName( sTrackName )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


QByteArray SMFTrackNameMetaEvent::getBuffer() const
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( TRACK_NAME );
	buf.writeString( m_sTrackName );

	return buf.getBuffer();
}

QString SMFTrackNameMetaEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFTrackNameMetaEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_sTrackName: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sTrackName ) );
	}
	else {
		sOutput = QString( "[SMFTrackNameMetaEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_sTrackName: %1" ).arg( m_sTrackName ) );
	}

	return sOutput;
}

// ::::::::::::::::::

SMFSetTempoMetaEvent::SMFSetTempoMetaEvent( int nBPM, int nTicks )
		: SMFEvent( nTicks )
		, m_nBPM( nBPM )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


QByteArray SMFSetTempoMetaEvent::getBuffer() const
{
	SMFBuffer buf;
	long msPerBeat;
	
	msPerBeat = long( 60000000 / m_nBPM ); // 60 seconds * mills \ BPM
	
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( SET_TEMPO );
	buf.writeByte( 0x03 );	// Length
	
	buf.writeByte( msPerBeat >> 16 );
	buf.writeByte( msPerBeat >> 8 );
	buf.writeByte( msPerBeat );
	
	return buf.getBuffer();
}

QString SMFSetTempoMetaEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFSetTempoMetaEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_nBPM: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nBPM ) );
	}
	else {
		sOutput = QString( "[SMFSetTempoMetaEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nBPM: %1" ).arg( m_nBPM ) );
	}

	return sOutput;
}

// ::::::::::::::::::

SMFCopyRightNoticeMetaEvent::SMFCopyRightNoticeMetaEvent( const QString& sAuthor,
														  int nTicks )
		: SMFEvent( nTicks )
		, m_sAuthor( sAuthor )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


QByteArray SMFCopyRightNoticeMetaEvent::getBuffer() const
{
	SMFBuffer buf;
	QString sCopyRightString;
	
	time_t now = time(nullptr);
	tm *ltm = localtime(&now);						// Extract the local system time.
	
	// Construct the copyright string in the form "(C) [Author] [CurrentYear]"
	sCopyRightString.append("(C) ");				// Start with the copyright symbol and a separator space.
	sCopyRightString.append( m_sAuthor );			// add the author
	sCopyRightString.append(" ");					// add a separator space
	sCopyRightString.append( QString::number( 1900 + ltm->tm_year, 10 ) );	// and finish with the year.
	
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( 0xFF );
	buf.writeByte( COPYRIGHT_NOTICE );
	buf.writeString( sCopyRightString );

	return buf.getBuffer();
}

QString SMFCopyRightNoticeMetaEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFCopyRightNoticeMetaEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_sAuthor: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_sAuthor ) );
	}
	else {
		sOutput = QString( "[SMFCopyRightNoticeMetaEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) );
	}

	return sOutput;
}

// ::::::::::::::::::
		
SMFTimeSignatureMetaEvent::SMFTimeSignatureMetaEvent( unsigned nBeats,
													  unsigned nNote,
													  unsigned nMTPMC,
													  unsigned nTSNP24,
													  int nTicks )
		: SMFEvent( nTicks )
		, m_nBeats( nBeats )
		, m_nNote( nNote )
		, m_nMTPMC( nMTPMC )
		, m_nTSNP24( nTSNP24 )
{
	// it's always at the start of the song
	m_nDeltaTime = 0;
}


QByteArray SMFTimeSignatureMetaEvent::getBuffer() const
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

QString SMFTimeSignatureMetaEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFTimeSignatureMetaEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_nBeats: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nBeats ) )
			.append( QString( "%1%2m_nNote: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nNote ) )
			.append( QString( "%1%2m_nMTPMC: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nMTPMC ) )
			.append( QString( "%1%2m_nTSNP24: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTSNP24 ) );
	}
	else {
		sOutput = QString( "[SMFTimeSignatureMetaEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nBeats: %1" ).arg( m_nBeats ) )
			.append( QString( ", m_nNote: %1" ).arg( m_nNote ) )
			.append( QString( ", m_nMTPMC: %1" ).arg( m_nMTPMC ) )
			.append( QString( ", m_nTSNP24: %1" ).arg( m_nTSNP24 ) );
	}

	return sOutput;
}

// :::::::::::::


SMFEvent::SMFEvent( int nTicks ) : m_nTicks( nTicks )
								 , m_nDeltaTime( -1 ) {
}

SMFEvent::~SMFEvent() {
}


// ::::::::::::::

SMFNoteOnEvent::SMFNoteOnEvent( int nTicks, int nChannel, int nPitch,
								int nVelocity )
		: SMFEvent( nTicks )
		, m_nChannel( nChannel )
		, m_nPitch( nPitch )
		, m_nVelocity( nVelocity )
{
	if ( nChannel >= 16 || nChannel < 0 ) {
		ERRORLOG( QString( "Invalid channel [%1]" ).arg( nChannel ) );
		m_nChannel = std::clamp( nChannel, 0, 15 );
	}
}



QByteArray SMFNoteOnEvent::getBuffer() const
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( NOTE_ON + m_nChannel );
	buf.writeByte( m_nPitch );
	buf.writeByte( m_nVelocity );

	return buf.getBuffer();
}

QString SMFNoteOnEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFNoteOnEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_nChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nChannel ) )
			.append( QString( "%1%2m_nPitch: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nPitch ) )
			.append( QString( "%1%2m_nVelocity: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nVelocity ) );
	}
	else {
		sOutput = QString( "[SMFNoteOnEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nChannel: %1" ).arg( m_nChannel ) )
			.append( QString( ", m_nPitch: %1" ).arg( m_nPitch ) )
			.append( QString( ", m_nVelocity: %1" ).arg( m_nVelocity ) );
	}

	return sOutput;
}

// :::::::::::


SMFNoteOffEvent::SMFNoteOffEvent( int nTicks, int nChannel, int nPitch,
								  int nVelocity )
		: SMFEvent( nTicks )
		, m_nChannel( nChannel )
		, m_nPitch( nPitch )
		, m_nVelocity( nVelocity )
{
	if ( nChannel >= 16 || nChannel < 0 ) {
		ERRORLOG( QString( "Invalid channel [%1]" ).arg( nChannel ) );
		m_nChannel = std::clamp( nChannel, 0, 15 );
	}
}



QByteArray SMFNoteOffEvent::getBuffer() const
{
	SMFBuffer buf;
	buf.writeVarLen( m_nDeltaTime );
	buf.writeByte( NOTE_OFF + m_nChannel );
	buf.writeByte( m_nPitch );
	buf.writeByte( m_nVelocity );

	return buf.getBuffer();
}

QString SMFNoteOffEvent::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFNoteOffEvent]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nTicks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTicks ) )
			.append( QString( "%1%2m_nDeltaTime: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nDeltaTime ) )
			.append( QString( "%1%2m_nChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nChannel ) )
			.append( QString( "%1%2m_nPitch: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nPitch ) )
			.append( QString( "%1%2m_nVelocity: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nVelocity ) );
	}
	else {
		sOutput = QString( "[SMFNoteOffEvent] " )
			.append( QString( "m_nTicks: %1" ).arg( m_nTicks ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nDeltaTime: %1" ).arg( m_nDeltaTime ) )
			.append( QString( ", m_nChannel: %1" ).arg( m_nChannel ) )
			.append( QString( ", m_nPitch: %1" ).arg( m_nPitch ) )
			.append( QString( ", m_nVelocity: %1" ).arg( m_nVelocity ) );
	}

	return sOutput;
}

};
