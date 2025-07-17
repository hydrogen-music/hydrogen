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

#include "SMF.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>

#include <math.h>

#include <QFile>
#include <QTextStream>

namespace H2Core
{

QString SMFHeader::FormatToQString( Format format ) {
	switch( format ) {
	case Format::SingleMultiChannelTrack:
		return "SingleMultiChannelTrack";
	case Format::SimultaneousTracks:
		return "SimultaneousTracks";
	case Format::SequentialIndependentTracks:
		return "SequentialIndependentTracks";
	default:
		return QString( "Unknown format value [%1]" )
			.arg( static_cast<int>(format) );
	}
}

SMFHeader::SMFHeader( Format format )
		: m_format( format )
		, m_nTracks( 0 ) {
}


SMFHeader::~SMFHeader() {
}

void SMFHeader::addTrack() {
	m_nTracks++;	
}

QByteArray SMFHeader::getBuffer() const
{
	SMFBuffer buffer;

	buffer.writeDWord( 1297377380 );		// MThd
	buffer.writeDWord( 6 );				// Header length = 6
	buffer.writeWord( static_cast<int>(m_format) );
	buffer.writeWord( m_nTracks );
	buffer.writeWord( SMF::nTicksPerQuarter );

	return buffer.m_buffer;
}

QString SMFHeader::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFHeader]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_format: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( FormatToQString( m_format ) ) )
			.append( QString( "%1%2m_nTracks: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nTracks ) );
	}
	else {
		sOutput = QString( "[SMFHeader] " )
			.append( QString( "m_format: %1" )
					 .arg( FormatToQString( m_format ) ) )
			.append( QString( ", m_nTracks: %1" ).arg( m_nTracks ) );
	}

	return sOutput;
}

// :::::::::::::::

SMFTrack::SMFTrack( std::shared_ptr<EventList> pOtherEventList ) {
	m_pEventList = std::make_shared<EventList>();

	if ( pOtherEventList != nullptr ) {
		for ( const auto& ppEvent : *pOtherEventList ) {
			m_pEventList->push_back( ppEvent );
		}
	}
}

SMFTrack::~SMFTrack() {
}

QByteArray SMFTrack::getBuffer() const {
	QByteArray trackData;

	for ( const auto& ppEvent : *m_pEventList ) {
		if ( ppEvent == nullptr ) {
			continue;
		}

		auto buf = ppEvent->getBuffer();

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
	trackBuf.push_back( static_cast<char>(SMFEvent::Type::EndOfTrack) );
	trackBuf.push_back( static_cast<char>(0x00) );

	return trackBuf;
}

void SMFTrack::addEvent( std::shared_ptr<SMFEvent> pEvent ) {
	if ( m_pEventList != nullptr ) {
		m_pEventList->push_back( pEvent );
	}
}

void SMFTrack::sortEvents( std::shared_ptr<EventList> pEvents ) {
	if ( pEvents == nullptr ) {
		return;
	}
	// awful bubble sort..
	for ( unsigned i = 0; i < pEvents->size(); i++ ) {
		for ( auto it = pEvents->begin() ;
			  it != ( pEvents->end() - 1 ) ;
			  it++ ) {
			auto pEvent = *it;
			auto pNextEvent = *( it + 1 );
			if ( pEvent == nullptr || pNextEvent == nullptr ) {
				ERRORLOG( "Abort. Invalid event" );
				return;
			}

			// If at the same tick, meta events will be put first.
			if ( pNextEvent->m_fTicks < pEvent->m_fTicks ||
				 pNextEvent->m_fTicks == pEvent->m_fTicks && (
					! SMFEvent::IsMetaEvent( pEvent->m_type ) &&
					SMFEvent::IsMetaEvent( pNextEvent->m_type ) &&
					pNextEvent->m_type != SMFEvent::Type::EndOfTrack ) ) {
				// swap
				*it = pNextEvent;
				*( it +1 ) = pEvent;
			}
		}
	}
}

void SMFTrack::sortAndTimeEvents() {
	if ( m_pEventList == nullptr ) {
		ERRORLOG( "Invalid event list" );
	}
	// Sort all events according to their tick.
	SMFTrack::sortEvents( m_pEventList );

	// Create delta-time stamps based on the events tick values.
	float fLastTick = 0;
	for ( auto& pEvent : *m_pEventList ) {
		pEvent->m_nDeltaTime = static_cast<int>(
			std::round( ( pEvent->m_fTicks - fLastTick ) * SMF::nTickFactor) );
		fLastTick = pEvent->m_fTicks;
	}
}

QString SMFTrack::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMFTrack] m_pEventList: \n" ).arg( sPrefix );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "%1%2\n" ).arg( sPrefix + s )
							.arg( ppEvent->toQString( "", true ) ) );
		}
	}
	else {
		sOutput = QString( "[SMFTrack] m_pEventList: [" );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "[%1] " )
							.arg( ppEvent->toQString( "", true ) ) );
		}
		sOutput.append( "]" );
	}

	return sOutput;
}

// ::::::::::::::::::::::

void SMF::PatternToTimeSignature( std::shared_ptr<Pattern> pPattern,
								  int* pNumerator, int* pDenominator,
								  bool* pRounded, bool* pScaled ) {
	if ( pPattern == nullptr) {
		ERRORLOG( "Invalid pattern" );
		return;
	}

	if ( pRounded != nullptr ) {
		*pRounded = false;
	}
	if ( pScaled != nullptr ) {
		*pScaled = false;
	}

	// For a SMF time signature to be valid, the numerator needs to be an
	// integer and the denominator a power of two.
	auto valid = []( double fNumerator, double fDenominator ) {
		double fLog = std::log2( fDenominator );
		return std::abs( std::round( fNumerator ) - fNumerator ) == 0 &&
			std::abs( std::round( fLog ) - fLog ) == 0;
	};

	double fNumerator = static_cast<double>(pPattern->numerator());
	double fDenominator = static_cast<double>(pPattern->getDenominator());

	if ( ! valid( fNumerator, fDenominator ) ) {
		WARNINGLOG( QString( "Time signature [%1/%2] does not comply with the General MIDI standard." )
					.arg( fNumerator ).arg( static_cast<int>(fDenominator) ) );

		// Factor by which the denominator has to be scaled to be a exactly a
		// power of two. The scaled results will be plain wrong, e.g. 4/3 ->
		// 5.3/4. But this is what it takes to make it compatible with the SMF
		// file standard and just assigning 4/4 would be wrong too. Instead, we
		// prompt to user to address this inconsistency. will be wrong. But it
		// will be most probably "less"
		const double fScale = std::exp2(
			std::abs( std::log2( fDenominator ) -
					  std::round( std::log2( fDenominator ) ) ) );
		if ( fScale != 1.0 ) {
			WARNINGLOG( QString( "The denominator [%1] has to be a power of two! The whole time signature will be scaled by a factor of [%2] to allow MIDI export." )
						.arg( static_cast<int>( fDenominator ) ).arg( fScale ) );
			if ( pScaled != nullptr ) {
				*pScaled = true;
			}
		}

		// Since we allow floating point numerators in Hydrogen, we might have
		// to do some rounding to comply with the SMF time signature. But this
		// rounding will only happen in the numerator. Another option is to
		// scale both numerator and denominator by the same factor of two in
		// order to make the former an integer.
		//
		// The routine below tests various powers of two in order to find the
		// one yielding a numerator closest to an integer. But since large
		// rescalings may be not the thing the user expects/wants, we introduce
		// a penalty (similar to an information criterion, like AIC or BIC) to
		// favor or discourage large scalings of the time signature against
		// rounding errors.
		const int nPowerStart =
			static_cast<int>(std::log2( fScale * fDenominator ));
		int nPowerOffset = 0;

		// Initialized with an arbitrary large value.
		double fDiff = 1000;
		for ( int nnPower = 0; nnPower + nPowerStart <= 255; ++nnPower ) {
			const double fNewNumerator = fNumerator *
				fScale * std::exp2( nnPower - nPowerStart );
			if ( fNewNumerator > 255 ) {
				// We have to represent the numerator with a single byte. The
				// current scaling and all following ones is too much.
				break;
			}

			// Ensure the denominator would still be a power of two
			if ( nnPower < nPowerStart ) {
				const double fNewDenominatorLog = std::log2(
					fDenominator * fScale * std::exp2( nnPower - nPowerStart ) );
				if ( fNewDenominatorLog != std::round( fNewDenominatorLog ) ) {
					continue;
				}
			}

			const double fNewDiff =
				std::abs( std::round( fNewNumerator ) - fNewNumerator );
			const double fPenalty =
				std::abs( SMF::fPenaltyTimeSignature * ( nnPower - nPowerStart ) );
			if ( fNewDiff == 0 ) {
				// With this scaling we can represent both numerator and
				// denominator as integers.
				nPowerOffset = nnPower;
				break;
			}
			else if ( fNewDiff + fPenalty < fDiff ) {
				fDiff = fNewDiff + fPenalty;
				nPowerOffset = nnPower;
			}
		}

		// Apply all scaling values
		fDenominator *= fScale * std::exp2( nPowerOffset - nPowerStart );
		fNumerator *= fScale * std::exp2( nPowerOffset - nPowerStart );

		if ( std::round( fNumerator ) != fNumerator ) {
			WARNINGLOG( QString( "(Scaled) Numerator [%1] must be an integer and will be rounded" )
						.arg( fNumerator ) );
			fNumerator = std::round( fNumerator );
			if ( pRounded != nullptr ) {
				*pRounded = true;
			}
		}

		INFOLOG( QString( "New time signature [%1/%2]" )
				 .arg( fNumerator ).arg( fDenominator ) );
	}

	// Sanity checks.
	fNumerator = std::clamp(
		fNumerator, static_cast<double>(1), static_cast<double>(255) );
	fDenominator = std::clamp(
		fDenominator, static_cast<double>(0), std::exp2(255) );

	// Done. Pass the found values to the arguments.
	if ( pNumerator != nullptr ) {
		*pNumerator = static_cast<int>(std::round( fNumerator ));
	}

	if ( pDenominator != nullptr ) {
		// Another sanity check to ensure we only provide an exact power of two.
		*pDenominator = static_cast<int>(
			std::exp2( std::round( std::log2(fDenominator) )) );
	}
};

SMF::SMF( SMFHeader::Format format ) {
	m_pHeader = std::make_shared<SMFHeader>( format );
}

SMF::~SMF() {
}

void SMF::addTrack( std::shared_ptr<SMFTrack> pTrack ) {
	if ( pTrack == nullptr ) {
		return;
	}

	if ( m_pHeader == nullptr ) {
		ERRORLOG( "Header not properly set up yet." );
		return;
	}

	m_pHeader->addTrack();
	m_trackList.push_back( pTrack );
}

QByteArray SMF::getBuffer() const {
	// header
	auto smfBuffer = m_pHeader->getBuffer();

	// tracks
	for ( const auto& ppTrack : m_trackList ) {
		if ( ppTrack != nullptr ) {
			smfBuffer.append( ppTrack->getBuffer() );
		}
	}

	return smfBuffer;
}

QString SMF::bufferToQString() const {
	return QString( getBuffer().toHex( ' ' ) );
}

QString SMF::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput.append( QString( "%1[SMF]\n%1%2m_pHeader: %3\n" ).arg( sPrefix )
						.arg( s ).arg( m_pHeader->toQString( s, true ) ) )
			.append( QString( "%1%2m_trackList:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& ppTrack : m_trackList ) {
			sOutput.append( QString( "%1" )
							.arg( ppTrack->toQString( s, false ) ) );
		}
	}
	else {
		sOutput.append( QString( "[SMF] m_pHeader: %1" )
						.arg( m_pHeader->toQString( "", true ) ) )
			.append( ", m_trackList: [" );
		for ( const auto& ppTrack : m_trackList ) {
			sOutput.append( QString( "[%1] " )
							.arg( ppTrack->toQString( "", true ) ) );
		}
		sOutput.append( "]" );
	}

	return sOutput;
}

// :::::::::::::::::::...

constexpr unsigned int DRUM_CHANNEL = 9;
constexpr unsigned int NOTE_LENGTH = 12;

SMFWriter::SMFWriter( SMFHeader::Format format, bool bOmitCopyright )
	: m_format( format )
	, m_bOmitCopyright( bOmitCopyright ) {
}

SMFWriter::~SMFWriter() {
}

std::shared_ptr<SMFTrack> SMFWriter::createTrack0( std::shared_ptr<Song> pSong ) {
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return nullptr;
	}

	auto pTrack0 = std::make_shared<SMFTrack>();
	if ( ! m_bOmitCopyright ) {
		pTrack0->addEvent(
			std::make_shared<SMFCopyRightNoticeMetaEvent>( pSong->getAuthor() , 0 ) );
	}
	pTrack0->addEvent(
		std::make_shared<SMFTrackNameMetaEvent>( pSong->getName() , 0 ) );

	return pTrack0;
}

void SMFWriter::save( const QString& sFilename, std::shared_ptr<Song> pSong,
					  bool bUseHumanization ) {
	if ( pSong == nullptr || pSong->getTimeline() == nullptr ||
		 pSong->getDrumkit() == nullptr ) {
		return;
	}

	INFOLOG( QString( "Export MIDI to [%1]" ).arg( sFilename ) );

	// here writers must prepare to receive pattern events
	prepareEvents( pSong );

	// Initial the tempo information.
	const auto pTimeline = pSong->getTimeline();
	const bool bUseTimeline = pSong->getIsTimelineActivated();
	float fBpm;
	if ( bUseTimeline ) {
		fBpm = pTimeline->getTempoAtColumn( 0 );
	}
	else {
		fBpm = pSong->getBpm();
	}
	addEvent( std::make_shared<SMFSetTempoMetaEvent>(
			static_cast<int>(std::round( fBpm )), 0 ), nullptr );

	// Initial the time signature (already added in createTrack0).
	int nNumerator, nDenominator;
	bool bRounded, bScaled;
	int nLastNumerator = 4;
	int nLastDenominator = 4;
	if ( pSong->getPatternGroupVector()->size() > 0 &&
		 pSong->getPatternGroupVector()->at( 0 )->size() > 0 ) {
		// There is at least one pattern in the first column.
		SMF::PatternToTimeSignature(
			pSong->getPatternGroupVector()->at( 0 )->getLongestPattern( true ),
			&nLastNumerator, &nLastDenominator, &bRounded, &bScaled );
	}
	addEvent( std::make_shared<SMFTimeSignatureMetaEvent>(
				  nLastNumerator, nLastDenominator , 0 ), nullptr );

	AutomationPath* pAutomationPath = pSong->getVelocityAutomationPath();

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	int nTick = 0;
	for ( int nnColumn = 0;
		  nnColumn < pSong->getPatternGroupVector()->size() ;
		  nnColumn++ ) {
		auto pColumn = ( *(pSong->getPatternGroupVector()) )[ nnColumn ];

		if ( bUseTimeline ) {
			// In case the timeline is used, we adopt the new tempo (even if
			// there is no pattern in the current column).
			const float fBpmColumn = pTimeline->getTempoAtColumn( nnColumn );
			if ( fBpmColumn != fBpm ) {
				// Tempo change
				addEvent( std::make_shared<SMFSetTempoMetaEvent>(
							   fBpmColumn, nTick ), nullptr );
				fBpm = fBpmColumn;
			}

			if ( pTimeline->hasColumnTag( nnColumn ) ) {
				addEvent( std::make_shared<SMFMarkerMetaEvent>(
							  pTimeline->getTagAtColumn( nnColumn ), nTick ),
						  nullptr );
			}
		}

		// Instead of working on the raw patternList of the column, we need to
		// expand all virtual patterns.
		auto pPatternList = std::make_shared<PatternList>();
		for ( const auto& ppPattern : *pColumn ) {
			pPatternList->add( ppPattern, true );
		}

		int nColumnLength;
		if ( pPatternList->size() > 0 ) {
			nColumnLength = pPatternList->longestPatternLength( false );
		}
		else {
			nColumnLength = 4 * H2Core::nTicksPerQuarter;
		}

		// Add the time signature event for this column in case it did change.
		auto pLongestPattern = pPatternList->getLongestPattern( true );
		if ( pLongestPattern != nullptr ) {
			SMF::PatternToTimeSignature( pLongestPattern, &nNumerator,
										 &nDenominator, &bRounded, &bScaled );
			if ( bRounded || bScaled ) {
				m_timeSignatureFailures.push_back(
					{ nnColumn, pLongestPattern->numerator(), nNumerator,
					  pLongestPattern->getDenominator(), nDenominator,
					  bRounded, bScaled } );
		}
		}
		else {
			nNumerator = 4;
			nDenominator = 4;
		}
		if ( nNumerator != nLastNumerator || nDenominator != nLastDenominator ) {
			addEvent( std::make_shared<SMFTimeSignatureMetaEvent>(
						  nNumerator, nDenominator, nTick ), nullptr );
			nLastNumerator = nNumerator;
			nLastDenominator = nLastDenominator;
		}

		for ( const auto& ppPattern : *pPatternList ) {
			if ( ppPattern == nullptr ) {
				continue;
			}

			for ( const auto& [ nnNote, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr && ppNote->getInstrument() != nullptr &&
					 ppNote->getProbability() >=
					 static_cast<float>(rand()) / static_cast<float>(RAND_MAX) ) {
				}

				auto pCopiedNote = std::make_shared<Note>( ppNote );

				float fNoteTick = nTick + nnNote;

				// Humanization
				if ( bUseHumanization ) {
					const auto nLeadLagFactor = AudioEngine::getLeadLagInFrames(
						static_cast<double>( pCopiedNote->getPosition() ) );
					pCopiedNote->setHumanizeDelay(
						pCopiedNote->getHumanizeDelay() +
						static_cast<int>(
							static_cast<float>(pCopiedNote->getLeadLag()) *
							static_cast<float>(nLeadLagFactor) ));

					pCopiedNote->setPosition( static_cast<int>(fNoteTick) );
					pCopiedNote->humanize();

					// delay the upbeat 16th-notes by a constant (manual)
					// offset. This must done _after_ setting the position of
					// the note.
					if ( ( static_cast<int>(fNoteTick) %
						   ( H2Core::nTicksPerQuarter / 4 ) == 0 ) &&
						 ( static_cast<int>(fNoteTick) %
						   ( H2Core::nTicksPerQuarter / 2 ) != 0 ) ) {
						pCopiedNote->swing();
					}

					// Frames introduced due to the humanization. Note that we
					// have to convert it into ticks first, in order to use it
					// in the MIDI file. This must be done _after_ setting the
					// position, humanization, and swing.
					const int nHumanizeFrames = std::clamp(
						pCopiedNote->getHumanizeDelay(),
						-1 * AudioEngine::nMaxTimeHumanize,
						AudioEngine::nMaxTimeHumanize );

					// Convert into ticks (while minding possible tempo
					// markers).
					double fMismatch;
					const auto nNoteFrame = TransportPosition::computeFrameFromTick(
						static_cast<double>(fNoteTick), &fMismatch );
					fNoteTick = TransportPosition::computeTickFromFrame(
						std::max( static_cast<long long>(0),
								  nNoteFrame + nHumanizeFrames ) );
				}

				const float fColumnPos = static_cast<float>(nnColumn) +
					(fNoteTick - static_cast<float>(nTick)) /
					static_cast<float>(nColumnLength);
				const float fVelocityAdjustment =
					pAutomationPath->get_value( fColumnPos );
				const int nVelocity = static_cast<int>(
					127.0 * pCopiedNote->getVelocity() * fVelocityAdjustment );

				const auto pInstr = pCopiedNote->getInstrument();
				const int nPitch = pCopiedNote->getMidiKey();
						
				int nChannel =  pInstr->getMidiOutChannel();
				if ( nChannel == -1 ) {
					// A channel of -1 is Hydrogen's old way of disabling
					// MIDI output during playback.
					nChannel = DRUM_CHANNEL;
				}

				int nLength = pCopiedNote->getLength();
				if ( nLength == LENGTH_ENTIRE_SAMPLE ) {
					nLength = NOTE_LENGTH;
				}

				// get events for specific instrument
				addEvent( std::make_shared<SMFNoteOnEvent>(
							  fNoteTick, nChannel, nPitch, nVelocity ),
						  pInstr );

				addEvent( std::make_shared<SMFNoteOffEvent>(
							  fNoteTick + nLength, nChannel, nPitch,
							  nVelocity ), pInstr );
			}
		}

		nTick += nColumnLength;
	}

	//tracks creation
	auto pSmf = std::make_shared<SMF>( m_format );
	packEvents( pSong, pSmf );

	saveSMF( sFilename, pSmf );
}

void SMFWriter::saveSMF( const QString& sFilename, std::shared_ptr<SMF> pSmf ) {
	if ( pSmf == nullptr ) {
		ERRORLOG( "Invalid SMF" );
		return;
	}

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

SMF1Writer::SMF1Writer( bool bOmitCopyright ) :
	SMFWriter( SMFHeader::Format::SimultaneousTracks, bOmitCopyright ) {
}

SMF1Writer::~SMF1Writer() {
}

SMF1WriterSingle::SMF1WriterSingle( bool bOmitCopyright )
		: SMF1Writer( bOmitCopyright ),
		 m_pEventList( std::make_shared<EventList>() ) {
}

SMF1WriterSingle::~SMF1WriterSingle() {
}

void SMF1WriterSingle::addEvent( std::shared_ptr<SMFEvent> pEvent,
								 std::shared_ptr<Instrument> pInstr ) {
	if ( pEvent == nullptr ) {
		return;
	}

	if ( m_pEventList == nullptr || m_pTrack0 == nullptr ) {
		ERRORLOG( "Not properly set up" );
		return;
	}

	if ( pEvent->m_type == SMFEvent::Type::NoteOn ||
		 pEvent->m_type == SMFEvent::Type::NoteOff ) {
		m_pEventList->push_back( pEvent );
	}
	else {
		// Meta event inserted into track0
		m_pTrack0->addEvent( pEvent );
	}
}

void SMF1WriterSingle::prepareEvents( std::shared_ptr<Song> pSong ) {
	if ( m_pEventList != nullptr ) {
		m_pEventList->clear();
	}
	m_timeSignatureFailures.clear();

	// Standard MIDI format 1 files should have the first track being the tempo
	// map which is a track that contains global meta events only. Note events
	// should reside in tracks =>2.
	m_pTrack0 = createTrack0( pSong );
}

void SMF1WriterSingle::packEvents( std::shared_ptr<Song> pSong,
								   std::shared_ptr<SMF> pSmf ) {
	if ( pSmf == nullptr ) {
		ERRORLOG( "Invalid SMF" );
		return;
	}

	if ( m_pEventList == nullptr || m_pTrack0 == nullptr ) {
		ERRORLOG( "Not properly set up" );
		return;
	}

	// Tempo, time signature, and text event.
	m_pTrack0->sortAndTimeEvents();
	pSmf->addTrack( m_pTrack0 );

	// Note on and off events
	auto pTrack1 = std::make_shared<SMFTrack>( m_pEventList );
	pTrack1->sortAndTimeEvents();
	pSmf->addTrack( pTrack1 );

	m_pEventList->clear();
}

QString SMF1WriterSingle::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMF1WriterSingle] m_pEventList: \n" ).arg( sPrefix );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "%1%2\n" ).arg( sPrefix + s )
							.arg( ppEvent->toQString( s, true ) ) );
		}
		sOutput.append( QString( "%1%2m_bOmitCopyright: %3\n" ).arg( sPrefix )
						.arg( s ).arg( m_bOmitCopyright ) );
	}
	else {
		sOutput = QString( "[SMF1WriterSingle] m_pEventList: [" );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "[%1] " )
							.arg( ppEvent->toQString( "", true ) ) );
		}
		sOutput.append( QString( "], m_bOmitCopyright: %1" )
						.arg( m_bOmitCopyright ) );
	}

	return sOutput;
}

// SMF1 MIDI MULTI EXPORT

SMF1WriterMulti::SMF1WriterMulti( bool bOmitCopyright )
		: SMF1Writer( bOmitCopyright ),
		 m_eventLists() {
}

SMF1WriterMulti::~SMF1WriterMulti() {
}

void SMF1WriterMulti::prepareEvents( std::shared_ptr<Song> pSong )
{
	m_eventLists.clear();
	m_timeSignatureFailures.clear();

	// Standard MIDI format 1 files should have the first track being the tempo
	// map which is a track that contains global meta events only. Note events
	// should reside in tracks =>2.
	m_pTrack0 = createTrack0( pSong );

	for ( const auto& ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
		if ( ppInstrument != nullptr ) {
			m_eventLists[ ppInstrument->getId() ] = std::make_shared<EventList>();
		}
	}
}

void SMF1WriterMulti::addEvent( std::shared_ptr<SMFEvent> pEvent,
								std::shared_ptr<Instrument> pInstr ) {
	if ( pEvent == nullptr ) {
		return;
	}

	if ( m_pTrack0 == nullptr ) {
		ERRORLOG( "Not properly set up" );
		return;
	}

	if ( pEvent->m_type == SMFEvent::Type::NoteOn ||
		 pEvent->m_type == SMFEvent::Type::NoteOff ) {
		const int nIndex = pInstr->getId();
		if ( m_eventLists.find( nIndex ) == m_eventLists.end() ) {
			ERRORLOG( QString( "EventList of index [%1] not found" ) );
			return;
		}

		auto pEventList = m_eventLists[ nIndex ];
		if ( pEventList != nullptr ) {
			pEventList->push_back( pEvent );
		}
	}
	else {
		// Meta event inserted into track0
		m_pTrack0->addEvent( pEvent );
	}
}

void SMF1WriterMulti::packEvents( std::shared_ptr<Song> pSong,
								  std::shared_ptr<SMF> pSmf )
{
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	// Tempo, time signature, and text event.
	m_pTrack0->sortAndTimeEvents();
	pSmf->addTrack( m_pTrack0 );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	for ( const auto& [ nnInstrumentID, ppEventList ] : m_eventLists ) {
		if ( ppEventList == nullptr ) {
			continue;
		}

		auto pTrack = std::make_shared<SMFTrack>( ppEventList );

		// Set track name
		auto pInstrument = pInstrumentList->find( nnInstrumentID );
		if ( pInstrument != nullptr ) {
			pTrack->addEvent( std::make_shared<SMFTrackNameMetaEvent>(
								  pInstrument->getName(), 0 ) );
		}

		pTrack->sortAndTimeEvents();
		pSmf->addTrack( pTrack );
	}

	m_eventLists.clear();
}

QString SMF1WriterMulti::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMF1WriterMulti] m_eventLists: \n" ).arg( sPrefix );
		for ( const auto& [ nnId, ppEventList ] : m_eventLists ) {
			sOutput.append( QString( "%1%2[%3]:\n" ).arg( sPrefix )
							.arg( s ).arg( nnId ) );
			for ( const auto& ppEvent : *ppEventList ) {
				sOutput.append( QString( "%1%2\n" ).arg( sPrefix + s + s )
								.arg( ppEvent->toQString( "", true ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_bOmitCopyright: %3\n" ).arg( sPrefix )
						.arg( s ).arg( m_bOmitCopyright ) );
}
	else {
		sOutput = QString( "[SMF1WriterMulti] m_eventLists: [" );
		for ( const auto& [ nnId, ppEventList ] : m_eventLists ) {
			sOutput.append( QString( "[[%1]: " ).arg( nnId ) );
			for ( const auto& ppEvent : *ppEventList ) {
				sOutput.append( QString( "[%1] " )
								.arg( ppEvent->toQString( s + s, true ) ) );
			}
			sOutput.append( "] " );
		}
		sOutput.append( QString( "], m_bOmitCopyright: %1" )
						.arg( m_bOmitCopyright ) );
}

	return sOutput;
}

// SMF0 MIDI  EXPORT

SMF0Writer::SMF0Writer( bool bOmitCopyright )
	: SMFWriter( SMFHeader::Format::SingleMultiChannelTrack, bOmitCopyright )
	, m_pTrack( nullptr )
	, m_pEventList( std::make_shared<EventList>() ) {
}

SMF0Writer::~SMF0Writer() {
}

void SMF0Writer::addEvent( std::shared_ptr<SMFEvent> pEvent,
						   std::shared_ptr<Instrument> pInstr ) {
	if ( m_pEventList != nullptr && pEvent != nullptr ) {
		m_pEventList->push_back( pEvent );
	}
}

void SMF0Writer::prepareEvents( std::shared_ptr<Song> pSong ) {
	if ( m_pEventList != nullptr ) {
		m_pEventList->clear();
	}

	m_timeSignatureFailures.clear();

	// MIDI files format 0 have all their events in one track
	m_pTrack = createTrack0( pSong );
}

void SMF0Writer::packEvents( std::shared_ptr<Song> pSong,
							 std::shared_ptr<SMF> pSmf ) {
	if ( pSmf == nullptr ) {
		ERRORLOG( "Invalid SMF" );
		return;
	}

	if ( m_pEventList == nullptr || m_pTrack == nullptr ) {
		ERRORLOG( "Not properly set up" );
		return;
	}

	// We fuse the initial events with the note on/off, tempo etc. events
	// obtained while traversing the song.
	for ( const auto& ppEvent : *m_pEventList ) {
		m_pTrack->addEvent( ppEvent );
	}
	m_pTrack->sortAndTimeEvents();
	pSmf->addTrack( m_pTrack );

	m_pEventList->clear();
}

QString SMF0Writer::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[SMF0Writer] m_pEventList: \n" ).arg( sPrefix );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "%1%2\n" ).arg( sPrefix + s )
							.arg( ppEvent->toQString( "", true ) ) );
		}
		sOutput.append( QString( "%1%2m_pTrack: %3\n" ).arg( sPrefix )
						.arg( s ).arg( m_pTrack->toQString( s, false ) ) )
			.append( QString( "%1%2m_bOmitCopyright: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_bOmitCopyright ) );
	}
	else {
		sOutput = QString( "[SMF0Writer] m_pEventList: [" );
		for ( const auto& ppEvent : *m_pEventList ) {
			sOutput.append( QString( "[%1] " )
							.arg( ppEvent->toQString( "", true ) ) );
		}
		sOutput.append( QString( "], m_pTrack: %1" )
						.arg( m_pTrack->toQString( "", true ) ) )
			.append( QString( ", m_bOmitCopyright: %1" )
					 .arg( m_bOmitCopyright ) );
	}

	return sOutput;
}

};
