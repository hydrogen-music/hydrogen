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
#include <core/AudioEngine/TransportPosition.h>
#include <core/AudioEngine/AudioEngine.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Timeline.h>
#include <core/config.h>

#define TRANSPORT_POSITION_DEBUG 0

#define TP_DEBUGLOG(x) if ( __logger->should_log( Logger::Debug ) ) { \
		__logger->log( Logger::Debug, _class_name(), __FUNCTION__, \
					   QString( "%1" ).arg( x ), "\033[33;1m" ); }

namespace H2Core {

TransportPosition::TransportPosition( const QString& sLabel )
	: m_sLabel( sLabel )
{
	m_pPlayingPatterns = std::make_shared<PatternList>();
	m_pPlayingPatterns->setNeedsLock( true );
	m_pNextPatterns = std::make_shared<PatternList>();
	m_pNextPatterns->setNeedsLock( true );
	
	reset();
}

TransportPosition::TransportPosition( std::shared_ptr<TransportPosition> pOther ) {
	m_pPlayingPatterns = std::make_shared<PatternList>();
	m_pPlayingPatterns->setNeedsLock( true );
	m_pNextPatterns = std::make_shared<PatternList>();
	m_pNextPatterns->setNeedsLock( true );

	set( pOther );
}

TransportPosition::~TransportPosition() {
}

void TransportPosition::set( std::shared_ptr<TransportPosition> pOther ) {
	m_nFrame = pOther->m_nFrame;
	m_fTick = pOther->m_fTick;
	m_fTickSize = pOther->m_fTickSize;
	m_fBpm = pOther->m_fBpm;
	m_nPatternStartTick = pOther->m_nPatternStartTick;
	m_nPatternTickPosition = pOther->m_nPatternTickPosition;
	m_nColumn = pOther->m_nColumn;
	m_fTickMismatch = pOther->m_fTickMismatch;
	m_nFrameOffsetTempo = pOther->m_nFrameOffsetTempo;
	m_fTickOffsetQueuing = pOther->m_fTickOffsetQueuing;
	m_fTickOffsetSongSize = pOther->m_fTickOffsetSongSize;

	m_pPlayingPatterns->clear();
	for ( const auto& ppPattern : *pOther->m_pPlayingPatterns ) {
		if ( ppPattern != nullptr ) {
			m_pPlayingPatterns->add( ppPattern );
		}
	}
	m_pNextPatterns->clear();
	for ( const auto& ppPattern : *pOther->m_pNextPatterns ) {
		if ( ppPattern != nullptr ) {
			m_pNextPatterns->add( ppPattern );
		}
	}
	m_nPatternSize = pOther->m_nPatternSize;
	m_nLastLeadLagFactor = pOther->m_nLastLeadLagFactor;
	m_nBar = pOther->m_nBar;
	m_nBeat = pOther->m_nBeat;
}

void TransportPosition::reset() {
	m_nFrame = 0;
	m_fTick = 0;
	m_fTickSize = 400;
	m_fBpm = 120;
	m_nPatternStartTick = 0;
	m_nPatternTickPosition = 0;
	m_nColumn = -1;
	m_fTickMismatch = 0;
	m_nFrameOffsetTempo = 0;
	m_fTickOffsetQueuing = 0;
	m_fTickOffsetSongSize = 0;

	m_pPlayingPatterns->clear();
	m_pNextPatterns->clear();
	m_nPatternSize = 4 * H2Core::nTicksPerQuarter;
	m_nLastLeadLagFactor = 0;
	m_nBar = 1;
	m_nBeat = 1;
}

void TransportPosition::setBpm( float fNewBpm ) {
	if ( fNewBpm > MAX_BPM ) {
		ERRORLOG( QString( "[%1] Provided bpm [%2] is too high. Assigning upper bound %3 instead" )
				  .arg( m_sLabel ).arg( fNewBpm ).arg( MAX_BPM ) );
		fNewBpm = MAX_BPM;
	} else if ( fNewBpm < MIN_BPM ) {
		ERRORLOG( QString( "[%1] Provided bpm [%2] is too low. Assigning lower bound %3 instead" )
				  .arg( m_sLabel ).arg( fNewBpm ).arg( MIN_BPM ) );
		fNewBpm = MIN_BPM;
	}
	
	m_fBpm = fNewBpm;

	if ( Preferences::get_instance()->getRubberBandBatchMode() ) {
		auto pHydrogen = Hydrogen::get_instance();
		auto pSong = pHydrogen->getSong();
		if ( pSong == nullptr ) {
			return;
		}
		auto pDrumkit = pSong->getDrumkit();
		if ( pDrumkit == nullptr ) {
			return;
		}

		pDrumkit->recalculateRubberband( getBpm() );
	}
}
 
void TransportPosition::setFrame( long long nNewFrame ) {
	if ( nNewFrame < 0 ) {
		ERRORLOG( QString( "[%1] Provided frame [%2] is negative. Setting frame 0 instead." )
				  .arg( m_sLabel ).arg( nNewFrame ) );
		nNewFrame = 0;
	}
	
	m_nFrame = nNewFrame;
}

void TransportPosition::setTick( double fNewTick ) {
	if ( fNewTick < 0 ) {
		ERRORLOG( QString( "[%1] Provided tick [%2] is negative. Setting frame 0 instead." )
				  .arg( m_sLabel ).arg( fNewTick ) );
		fNewTick = 0;
	}
	
	m_fTick = fNewTick;
}

void TransportPosition::setTickSize( float fNewTickSize ) {
	if ( fNewTickSize <= 0 ) {
		ERRORLOG( QString( "[%1] Provided tick size [%2] is too small. Using 400 as a fallback instead." )
				  .arg( m_sLabel ).arg( fNewTickSize ) );
		fNewTickSize = 400;
	}

	m_fTickSize = fNewTickSize;
}

void TransportPosition::setPatternStartTick( long nPatternStartTick ) {
	if ( nPatternStartTick < 0 ) {
		ERRORLOG( QString( "[%1] Provided tick [%2] is negative. Setting frame 0 instead." )
				  .arg( m_sLabel ).arg( nPatternStartTick ) );
		nPatternStartTick = 0;
	}
	
	m_nPatternStartTick = nPatternStartTick;
}

void TransportPosition::setPatternTickPosition( long nPatternTickPosition ) {
	if ( nPatternTickPosition < 0 ) {
		ERRORLOG( QString( "[%1] Provided tick [%2] is negative. Setting frame 0 instead." )
				  .arg( m_sLabel ).arg( nPatternTickPosition ) );
		nPatternTickPosition = 0;
	}
	
	m_nPatternTickPosition = nPatternTickPosition;
}

void TransportPosition::setColumn( int nColumn ) {
	if ( nColumn < -1 ) {
		ERRORLOG( QString( "[%1] Provided column [%2] it too small. Using [-1] as a fallback instead." )
				  .arg( m_sLabel ).arg( nColumn ) );
		nColumn = -1;
	}

	m_nColumn = nColumn;
}


void TransportPosition::setPatternSize( int nPatternSize ) {
	if ( nPatternSize < 0 ) {
		ERRORLOG( QString( "[%1] Provided pattern size [%2] it too small. Using [0] as a fallback instead." )
				  .arg( m_sLabel ).arg( nPatternSize ) );
		nPatternSize = 0;
	}

	m_nPatternSize = nPatternSize;
}
void TransportPosition::setBar( int nBar ) {
	if ( nBar < 1 ) {
		ERRORLOG( QString( "[%1] Provided bar [%2] it too small. Using [1] as a fallback instead." )
				  .arg( m_sLabel ).arg( nBar ) );
		nBar = 1;
	}
	m_nBar = nBar;
}

void TransportPosition::setBeat( int nBeat ) {
	if ( nBeat < 1 ) {
		ERRORLOG( QString( "[%1] Provided beat [%2] it too small. Using [1] as a fallback instead." )
				  .arg( m_sLabel ).arg( nBeat ) );
		nBeat = 1;
	}
	m_nBeat = nBeat;
}
				
// This function uses the assumption that sample rate and resolution
// are constant over the whole song.
long long TransportPosition::computeFrameFromTick( const double fTick, double* fTickMismatch, int nSampleRate ) {

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	const auto pAudioEngine = pHydrogen->getAudioEngine();
	const auto pAudioDriver = pHydrogen->getAudioOutput();

	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		*fTickMismatch = 0;
		return 0;
	}

	if ( nSampleRate == 0 ) {
		nSampleRate = pAudioDriver->getSampleRate();
	}
	const double fSongSizeInTicks = pAudioEngine->getSongSizeInTicks();
	
	if ( nSampleRate == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		*fTickMismatch = 0;
		return 0;
	}

	if ( fTick == 0 ) {
		*fTickMismatch = 0;
		return 0;
	}

	std::vector<std::shared_ptr<const Timeline::TempoMarker>> tempoMarkers;
	bool bSpecialFirstMarker = false;
	if ( pTimeline != nullptr ) {
		tempoMarkers = pTimeline->getAllTempoMarkers();
		bSpecialFirstMarker = pTimeline->isFirstTempoMarkerSpecial();
	}

	int nColumns = 0;
	if ( pSong != nullptr ) {
		nColumns = pSong->getPatternGroupVector()->size();
	}

	// If there are no patterns in the current, we treat song mode
	// like pattern mode.
	long long nNewFrame = 0;
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 && bSpecialFirstMarker ) &&
		 pHydrogen->getMode() == Song::Mode::Song && nColumns > 0 ) {

		double fNewTick = fTick;
		double fRemainingTicks = fTick;
		double fNextTick, fPassedTicks = 0;
		double fNextTickSize;
		double fNewFrame = 0;
		int ii;

		auto handleEnd = [&]() {
			// The next frame is within this segment.
			fNewFrame += fRemainingTicks * fNextTickSize;

			nNewFrame = static_cast<long long>( std::round( fNewFrame ) );

			// Keep track of the rounding error to be able to switch
			// between fTick and its frame counterpart later on.  In
			// case fTick is located close to a tempo marker we will
			// only cover the part up to the tempo marker in here as
			// only this region is governed by fNextTickSize.
			const double fRoundingErrorInTicks =
				( fNewFrame - static_cast<double>( nNewFrame ) ) /
				fNextTickSize;

			// Compares the negative distance between current position
			// (fNewFrame) and the one resulting from rounding -
			// fRoundingErrorInTicks - with the negative distance
			// between current position (fNewFrame) and location of
			// next tempo marker.
			if ( fRoundingErrorInTicks >
				 fPassedTicks + fRemainingTicks - fNextTick ) {
				// Whole mismatch located within the current tempo
				// interval.
				*fTickMismatch = fRoundingErrorInTicks;
						
			}
			else {
				// Mismatch at this side of the tempo marker.
				*fTickMismatch = fPassedTicks + fRemainingTicks - fNextTick;

				const double fFinalFrame = fNewFrame +
					( fNextTick - fPassedTicks - fRemainingTicks ) * fNextTickSize;

				// Mismatch located beyond the tempo marker.
				double fFinalTickSize;
				if ( ii < tempoMarkers.size() ) {
					fFinalTickSize = AudioEngine::computeDoubleTickSize(
						nSampleRate, tempoMarkers[ ii ]->fBpm );
				}
				else {
					fFinalTickSize = AudioEngine::computeDoubleTickSize(
						nSampleRate, tempoMarkers[ 0 ]->fBpm );
				}

#if TRANSPORT_POSITION_DEBUG
				TP_DEBUGLOG( QString( "[::computeFrameFromTick mismatch : 2] fTickMismatch: [%1 + %2], static_cast<double>(nNewFrame): %3, fNewFrame: %4, fFinalFrame: %5, fNextTickSize: %6, fPassedTicks: %7, fRemainingTicks: %8, fFinalTickSize: %9" )
						  .arg( fPassedTicks + fRemainingTicks - fNextTick )
						  .arg( ( fFinalFrame - static_cast<double>(nNewFrame) ) / fNextTickSize )
						  .arg( nNewFrame )
						  .arg( fNewFrame, 0, 'f' )
						  .arg( fFinalFrame, 0, 'f' )
						  .arg( fNextTickSize, 0, 'f' )
						  .arg( fPassedTicks, 0, 'f' )
						  .arg( fRemainingTicks, 0, 'f' )
						  .arg( fFinalTickSize, 0, 'f' ));
#endif

				*fTickMismatch += ( fFinalFrame - static_cast<double>(nNewFrame) ) /
					fFinalTickSize;
			}

#if TRANSPORT_POSITION_DEBUG
			TP_DEBUGLOG( QString( "[::computeFrameFromTick end] fTick: %1, fNewFrame: %2, fNextTick: %3, fRemainingTicks: %4, fPassedTicks: %5, fNextTickSize: %6, tempoMarkers[ ii - 1 ]->nColumn: %7, tempoMarkers[ ii - 1 ]->fBpm: %8, nNewFrame: %9, fTickMismatch: %10, frame increment (fRemainingTicks * fNextTickSize): %11, fRoundingErrorInTicks: %12" )
					  .arg( fTick, 0, 'f' )
					  .arg( fNewFrame, 0, 'g', 30 )
					  .arg( fNextTick, 0, 'f' )
					  .arg( fRemainingTicks, 0, 'f' )
					  .arg( fPassedTicks, 0, 'f' )
					  .arg( fNextTickSize, 0, 'f' )
					  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					  .arg( nNewFrame )
					  .arg( *fTickMismatch, 0, 'g', 30 )
					  .arg( fRemainingTicks * fNextTickSize, 0, 'g', 30 )
					  .arg( fRoundingErrorInTicks, 0, 'f' )
				);
#endif

			fRemainingTicks -= fNewTick - fPassedTicks;
		};

		while ( fRemainingTicks > 0 ) {
		
			for ( ii = 1; ii <= tempoMarkers.size(); ++ii ) {
				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTick = fSongSizeInTicks;
				} else {
					fNextTick =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ) );
				}

				fNextTickSize = AudioEngine::computeDoubleTickSize(
					nSampleRate, tempoMarkers[ ii - 1 ]->fBpm );
				
				if ( fRemainingTicks > ( fNextTick - fPassedTicks ) ) {
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the current transport position.
					fNewFrame += ( fNextTick - fPassedTicks ) * fNextTickSize;

#if TRANSPORT_POSITION_DEBUG
					TP_DEBUGLOG( QString( "[segment] fTick: %1, fNewFrame: %2, fNextTick: %3, fRemainingTicks: %4, fPassedTicks: %5, fNextTickSize: %6, tempoMarkers[ ii - 1 ]->nColumn: %7, tempoMarkers[ ii - 1 ]->fBpm: %8, tick increment (fNextTick - fPassedTicks): %9, frame increment (fRemainingTicks * fNextTickSize): %10" )
							  .arg( fTick, 0, 'f' )
							  .arg( fNewFrame, 0, 'g', 30 )
							  .arg( fNextTick, 0, 'f' )
							  .arg( fRemainingTicks, 0, 'f' )
							  .arg( fPassedTicks, 0, 'f' )
							  .arg( fNextTickSize, 0, 'f' )
							  .arg( tempoMarkers[ ii - 1 ]->nColumn )
							  .arg( tempoMarkers[ ii - 1 ]->fBpm )
							  .arg( fNextTick - fPassedTicks, 0, 'f' )
							  .arg( ( fNextTick - fPassedTicks ) * fNextTickSize, 0, 'g', 30 )
							  );
#endif

					fRemainingTicks -= fNextTick - fPassedTicks;
					
					fPassedTicks = fNextTick;

				}
				else {
					handleEnd();
					break;
				}
			}

			if ( fRemainingTicks > 0 ) {
				// The provided fTick is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames (fNewFrame).
				const int nRepetitions = std::floor(fTick / fSongSizeInTicks);
				const double fSongSizeInFrames = fNewFrame;
				
				fNewFrame *= static_cast<double>(nRepetitions);
				fNewTick = std::fmod( fTick, fSongSizeInTicks );
				fRemainingTicks = fNewTick;
				fPassedTicks = 0;

#if TRANSPORT_POSITION_DEBUG
				TP_DEBUGLOG( QString( "[repeat] fTick: %1, fNewFrames: %2, fNewTick: %3, fRemainingTicks: %4, nRepetitions: %5, fSongSizeInTicks: %6, fSongSizeInFrames: %7" )
						  .arg( fTick, 0, 'g',30 )
						  .arg( fNewFrame, 0, 'g', 30 )
						  .arg( fNewTick, 0, 'g', 30 )
						  .arg( fRemainingTicks, 0, 'g', 30 )
						  .arg( nRepetitions )
						  .arg( fSongSizeInTicks, 0, 'g', 30 )
						  .arg( fSongSizeInFrames, 0, 'g', 30 )
						  );
#endif

				if ( std::isinf( fNewFrame ) ||
					 static_cast<long long>(fNewFrame) >
					 std::numeric_limits<long long>::max() ) {
					ERRORLOG( QString( "Provided ticks [%1] are too large." ).arg( fTick ) );
					return 0;
				}

				// The target tick matches a multiple of the song
				// size. We need to reproduce the context within the
				// last tempo marker in order to get the mismatch
				// right.
				if ( fRemainingTicks == 0 ) {
					ii = tempoMarkers.size();
					fNextTick = static_cast<double>(pHydrogen->getTickForColumn(
														tempoMarkers[ 0 ]->nColumn ) );
					fNextTickSize = AudioEngine::computeDoubleTickSize(
						nSampleRate, tempoMarkers[ ii - 1 ]->fBpm );

					handleEnd();
				}
			}
		}
	}
	else {
		// There may be neither Timeline nor Song.

		// As the timeline is not activate, the column passed is of no
		// importance. But we harness the ability of getBpmAtColumn()
		// to collect and choose between tempo information gathered
		// from various sources.
		const float fBpm = AudioEngine::getBpmAtColumn( 0 );

		const double fTickSize =
			AudioEngine::computeDoubleTickSize( nSampleRate, fBpm );
		
		// Single tempo for the whole song.
		const double fNewFrame = static_cast<double>(fTick) *
			fTickSize;
		nNewFrame = static_cast<long long>( std::round( fNewFrame ) );
		*fTickMismatch = ( fNewFrame - static_cast<double>(nNewFrame ) ) /
			fTickSize;

#if TRANSPORT_POSITION_DEBUG
		TP_DEBUGLOG(QString("[no-timeline] nNewFrame: %1, fTick: %2, fTickSize: %3, fTickMismatch: %4" )
				 .arg( nNewFrame ).arg( fTick, 0, 'f' ).arg( fTickSize, 0, 'f' )
				 .arg( *fTickMismatch, 0, 'g', 30 ));
#endif
		
	}
	
	return nNewFrame;
}

// This function uses the assumption that sample rate and resolution
// are constant over the whole song.
double TransportPosition::computeTickFromFrame( const long long nFrame, int nSampleRate ) {
	const auto pHydrogen = Hydrogen::get_instance();

	if ( nFrame < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] must be non-negative" ).arg( nFrame ) );
	}
	
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	const auto pAudioEngine = pHydrogen->getAudioEngine();
	const auto pAudioDriver = pHydrogen->getAudioOutput();

	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return 0;
	}

	if ( nSampleRate == 0 ) {
		nSampleRate = pAudioDriver->getSampleRate();
	}

	double fTick = 0;

	const double fSongSizeInTicks = pAudioEngine->getSongSizeInTicks();

	if ( nSampleRate == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		return fTick;
	}

	if ( nFrame == 0 ) {
		return fTick;
	}
		
	std::vector<std::shared_ptr<const Timeline::TempoMarker>> tempoMarkers;
	bool bSpecialFirstMarker = false;
	if ( pTimeline != nullptr ) {
		tempoMarkers = pTimeline->getAllTempoMarkers();
		bSpecialFirstMarker = pTimeline->isFirstTempoMarkerSpecial();
	}

	int nColumns = 0;
	if ( pSong != nullptr ) {
		nColumns = pSong->getPatternGroupVector()->size();
	}

	// If there are no patterns in the current, we treat song mode
	// like pattern mode.
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 && bSpecialFirstMarker ) &&
		 pHydrogen->getMode() == Song::Mode::Song && nColumns > 0 ) {

		// We are using double precision in here to avoid rounding
		// errors.
		const double fTargetFrame = static_cast<double>(nFrame);
		double fPassedFrames = 0;
		double fNextFrame = 0;
		double fNextTicks, fPassedTicks = 0;
		double fNextTickSize;
		long long nRemainingFrames;

		const int nColumns = pSong->getPatternGroupVector()->size();

		while ( fPassedFrames < fTargetFrame ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {

				fNextTickSize = AudioEngine::computeDoubleTickSize(
					nSampleRate, tempoMarkers[ ii - 1 ]->fBpm );

				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTicks = fSongSizeInTicks;
				} else {
					fNextTicks =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ));
				}
				fNextFrame = (fNextTicks - fPassedTicks) * fNextTickSize;
		
				if ( fNextFrame < ( fTargetFrame -
									 fPassedFrames ) ) {

#if TRANSPORT_POSITION_DEBUG
					TP_DEBUGLOG(QString( "[segment] nFrame: %1, fTick: %2, nSampleRate: %3, fNextTickSize: %4, fNextTicks: %5, fNextFrame: %6, tempoMarkers[ ii -1 ]->nColumn: %7, tempoMarkers[ ii -1 ]->fBpm: %8, fPassedTicks: %9, fPassedFrames: %10, fNewTick (tick increment): %11, fNewTick * fNextTickSize (frame increment): %12" )
							 .arg( nFrame )
							 .arg( fTick, 0, 'f' )
							 .arg( nSampleRate )
							 .arg( fNextTickSize, 0, 'f' )
							 .arg( fNextTicks, 0, 'f' )
							 .arg( fNextFrame, 0, 'f' )
							 .arg( tempoMarkers[ ii -1 ]->nColumn )
							 .arg( tempoMarkers[ ii -1 ]->fBpm )
							 .arg( fPassedTicks, 0, 'f' )
							 .arg( fPassedFrames, 0, 'f' )
							 .arg( fNextTicks - fPassedTicks, 0, 'f' )
							 .arg( (fNextTicks - fPassedTicks) * fNextTickSize, 0, 'g', 30 )
							 );
#endif
					
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the transport position.
					fTick += fNextTicks - fPassedTicks;

					fPassedFrames += fNextFrame;
					fPassedTicks = fNextTicks;

				} else {
					// The target frame is located within a segment.
					const double fNewTick = (fTargetFrame - fPassedFrames ) /
						fNextTickSize;

					fTick += fNewTick;

#if TRANSPORT_POSITION_DEBUG
					TP_DEBUGLOG(QString( "[end] nFrame: %1, fTick: %2, nSampleRate: %3, fNextTickSize: %4, fNextTicks: %5, fNextFrame: %6, tempoMarkers[ ii -1 ]->nColumn: %7, tempoMarkers[ ii -1 ]->fBpm: %8, fPassedTicks: %9, fPassedFrames: %10, fNewTick (tick increment): %11, fNewTick * fNextTickSize (frame increment): %12" )
							 .arg( nFrame )
							 .arg( fTick, 0, 'f' )
							 .arg( nSampleRate )
							 .arg( fNextTickSize, 0, 'f' )
							 .arg( fNextTicks, 0, 'f' )
							 .arg( fNextFrame, 0, 'f' )
							 .arg( tempoMarkers[ ii -1 ]->nColumn )
							 .arg( tempoMarkers[ ii -1 ]->fBpm )
							 .arg( fPassedTicks, 0, 'f' )
							 .arg( fPassedFrames, 0, 'f' )
							 .arg( fNewTick, 0, 'f' )
							 .arg( fNewTick * fNextTickSize, 0, 'g', 30 )
							 );
#endif
											
					fPassedFrames = fTargetFrame;
					
					break;
				}
			}

			if ( fPassedFrames != fTargetFrame ) {
				// The provided nFrame is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames.
				const double fSongSizeInFrames = fPassedFrames;
				const int nRepetitions = std::floor(fTargetFrame / fSongSizeInFrames);
				if ( fSongSizeInTicks * nRepetitions >
					 std::numeric_limits<double>::max() ) {
					ERRORLOG( QString( "Provided frames [%1] are too large." ).arg( nFrame ) );
					return 0;
				}
				fTick = fSongSizeInTicks * nRepetitions;

				fPassedFrames = static_cast<double>(nRepetitions) *
					fSongSizeInFrames;
				fPassedTicks = 0;

#if TRANSPORT_POSITION_DEBUG
				TP_DEBUGLOG( QString( "[repeat] frames covered: %1, frames remaining: %2, ticks covered: %3,  nRepetitions: %4, fSongSizeInFrames: %5, fSongSizeInTicks: %6" )
						  .arg( fPassedFrames, 0, 'g', 30 )
						  .arg( fTargetFrame - fPassedFrames, 0, 'g', 30 )
						  .arg( fTick, 0, 'g', 30 )
						  .arg( nRepetitions )
						  .arg( fSongSizeInFrames, 0, 'g', 30 )
						  .arg( fSongSizeInTicks, 0, 'g', 30 )
						  );
#endif
				
			}
		}
	}
	else {
		// There may be neither Timeline nor Song.

		// As the timeline is not activate, the column passed is of no
		// importance. But we harness the ability of getBpmAtColumn()
		// to collect and choose between tempo information gathered
		// from various sources.
		const float fBpm = AudioEngine::getBpmAtColumn( 0 );
		const double fTickSize =
			AudioEngine::computeDoubleTickSize( nSampleRate, fBpm );

		// Single tempo for the whole song.
		fTick = static_cast<double>(nFrame) / fTickSize;

#if TRANSPORT_POSITION_DEBUG
		TP_DEBUGLOG(QString( "[no timeline] nFrame: %1, sampleRate: %2, tickSize: %3" )
				 .arg( nFrame ).arg( nSampleRate ).arg( fTickSize, 0, 'f' ) );
#endif

	}
	
	return fTick;
}

long long TransportPosition::computeFrame( double fTick, float fTickSize ) {
	return std::round( fTick * fTickSize );
}

double TransportPosition::computeTick( long long nFrame, float fTickSize ) {
	return nFrame / fTickSize;
}

bool operator==( std::shared_ptr<TransportPosition> pLhs,
				 std::shared_ptr<TransportPosition> pRhs ) {
	if ( ( pLhs->m_pPlayingPatterns != nullptr &&
		   pRhs->m_pPlayingPatterns == nullptr ) ||
		 ( pLhs->m_pPlayingPatterns == nullptr &&
		   pRhs->m_pPlayingPatterns != nullptr ) ) {
		return false;
	}
	else if ( pLhs->m_pPlayingPatterns != nullptr &&
			  pRhs->m_pPlayingPatterns != nullptr &&
			  *pLhs->m_pPlayingPatterns != *pRhs->m_pPlayingPatterns ) {
		return false;
	}

	if ( ( pLhs->m_pNextPatterns != nullptr &&
		   pRhs->m_pNextPatterns == nullptr ) ||
		 ( pLhs->m_pNextPatterns == nullptr &&
		   pRhs->m_pNextPatterns != nullptr ) ) {
		return false;
	}
	else if ( pLhs->m_pNextPatterns != nullptr &&
			  pRhs->m_pNextPatterns != nullptr &&
			  *pLhs->m_pNextPatterns != *pRhs->m_pNextPatterns ) {
		return false;
	}

	return (
		pLhs->m_nFrame == pRhs->m_nFrame &&
		std::abs( pLhs->m_fTick - pRhs->m_fTick ) < 1E-5 &&
		std::abs( pLhs->m_fTickSize - pRhs->m_fTickSize ) < 1E-2 &&
		std::abs( pLhs->m_fBpm - pRhs->m_fBpm ) < 1E-2 &&
		pLhs->m_nPatternStartTick == pRhs->m_nPatternStartTick &&
		pLhs->m_nPatternTickPosition == pRhs->m_nPatternTickPosition &&
		pLhs->m_nColumn == pRhs->m_nColumn &&
		std::abs( pLhs->m_fTickMismatch - pRhs->m_fTickMismatch ) < 1E-5 &&
		pLhs->m_nFrameOffsetTempo == pRhs->m_nFrameOffsetTempo &&
		std::abs( pLhs->m_fTickOffsetQueuing -
				  pRhs->m_fTickOffsetQueuing ) < 1E-5 &&
		std::abs( pLhs->m_fTickOffsetSongSize -
				  pRhs->m_fTickOffsetSongSize ) < 1E-5 &&
		pLhs->m_nPatternSize == pRhs->m_nPatternSize &&
		pLhs->m_nLastLeadLagFactor == pRhs->m_nLastLeadLagFactor &&
		pLhs->m_nBar == pRhs->m_nBar &&
		pLhs->m_nBeat == pRhs->m_nBeat );
}

bool operator!=( std::shared_ptr<TransportPosition> pLhs,
				 std::shared_ptr<TransportPosition> pRhs ) {
	if ( ( pLhs->m_pPlayingPatterns != nullptr &&
		   pRhs->m_pPlayingPatterns == nullptr ) ||
		 ( pLhs->m_pPlayingPatterns == nullptr &&
		   pRhs->m_pPlayingPatterns != nullptr ) ) {
		return true;
	}
	else if ( pLhs->m_pPlayingPatterns != nullptr &&
			  pRhs->m_pPlayingPatterns != nullptr &&
			  *pLhs->m_pPlayingPatterns != *pRhs->m_pPlayingPatterns ) {
		return true;
	}

	if ( ( pLhs->m_pNextPatterns != nullptr &&
		   pRhs->m_pNextPatterns == nullptr ) ||
		 ( pLhs->m_pNextPatterns == nullptr &&
		   pRhs->m_pNextPatterns != nullptr ) ) {
		return true;
	}
	else if ( pLhs->m_pNextPatterns != nullptr &&
			  pRhs->m_pNextPatterns != nullptr &&
			  *pLhs->m_pNextPatterns != *pRhs->m_pNextPatterns ) {
		return true;
	}


	return (
		pLhs->m_nFrame != pRhs->m_nFrame ||
		std::abs( pLhs->m_fTick - pRhs->m_fTick ) > 1E-5 ||
		std::abs( pLhs->m_fTickSize - pRhs->m_fTickSize ) > 1E-2 ||
		std::abs( pLhs->m_fBpm - pRhs->m_fBpm ) > 1E-2 ||
		pLhs->m_nPatternStartTick != pRhs->m_nPatternStartTick ||
		pLhs->m_nPatternTickPosition != pRhs->m_nPatternTickPosition ||
		pLhs->m_nColumn != pRhs->m_nColumn ||
		std::abs( pLhs->m_fTickMismatch - pRhs->m_fTickMismatch ) > 1E-5 ||
		pLhs->m_nFrameOffsetTempo != pRhs->m_nFrameOffsetTempo ||
		std::abs( pLhs->m_fTickOffsetQueuing -
				  pRhs->m_fTickOffsetQueuing ) > 1E-5 ||
		std::abs( pLhs->m_fTickOffsetSongSize -
				  pRhs->m_fTickOffsetSongSize ) > 1E-5 ||
		pLhs->m_nPatternSize != pRhs->m_nPatternSize ||
		pLhs->m_nLastLeadLagFactor != pRhs->m_nLastLeadLagFactor ||
		pLhs->m_nBar != pRhs->m_nBar ||
		pLhs->m_nBeat != pRhs->m_nBeat );
}

QString TransportPosition::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[TransportPosition]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_sLabel: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sLabel ) )
			.append( QString( "%1%2m_nFrame: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nFrame ) )
			.append( QString( "%1%2m_fTick: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTick, 0, 'f' ) )
			.append( QString( "%1%2m_fTick (rounded): %3\n" ).arg( sPrefix ).arg( s )
					 .arg( getTick() ) )
			.append( QString( "%1%2m_fTickSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTickSize, 0, 'f' ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fBpm, 0, 'f' ) )
			.append( QString( "%1%2m_nPatternStartTick: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nPatternStartTick ) )
			.append( QString( "%1%2m_nPatternTickPosition: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nPatternTickPosition ) )
			.append( QString( "%1%2m_nColumn: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nColumn ) )
			.append( QString( "%1%2m_fTickMismatch: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTickMismatch, 0, 'f' ) )
			.append( QString( "%1%2m_nFrameOffsetTempo: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nFrameOffsetTempo ) )
			.append( QString( "%1%2m_fTickOffsetQueuing: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTickOffsetQueuing, 0, 'f' ) )
			.append( QString( "%1%2m_fTickOffsetSongSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fTickOffsetSongSize, 0, 'f' ) );
		if ( m_pNextPatterns != nullptr ) {
			sOutput.append( QString( "%1%2m_pNextPatterns: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pNextPatterns->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pNextPatterns: nullptr\n" )
							.arg( sPrefix ).arg( s ) );
		}
		if ( m_pPlayingPatterns != nullptr ) {
			sOutput.append( QString( "%1%2m_pPlayingPatterns: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pPlayingPatterns->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pPlayingPatterns: nullptr\n" )
							.arg( sPrefix ).arg( s ) );
		}
		sOutput.append( QString( "%1%2m_nPatternSize: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nPatternSize ) )
			.append( QString( "%1%2m_nLastLeadLagFactor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nLastLeadLagFactor ) )
			.append( QString( "%1%2m_nBar: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBar ) )
			.append( QString( "%1%2m_nBeat: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nBeat ) );
	}
	else {
		sOutput = QString( "%1[TransportPosition]" ).arg( sPrefix )
			.append( QString( " m_sLabel: %1" ).arg( m_sLabel ) )
			.append( QString( ", m_nFrame: %1" ).arg( m_nFrame ) )
			.append( QString( ", m_fTick: %1" ).arg( m_fTick, 0, 'f' ) )
			.append( QString( ", m_fTick (rounded): %1" ).arg( getTick() ) )
			.append( QString( ", m_fTickSize: %1" ).arg( m_fTickSize, 0, 'f' ) )
			.append( QString( ", m_fBpm: %1" ).arg( m_fBpm, 0, 'f' ) )
			.append( QString( ", m_nPatternStartTick: %1" ).arg( m_nPatternStartTick ) )
			.append( QString( ", m_nPatternTickPosition: %1" ).arg( m_nPatternTickPosition ) )
			.append( QString( ", m_nColumn: %1" ).arg( m_nColumn ) )
			.append( QString( ", m_fTickMismatch: %1" ).arg( m_fTickMismatch, 0, 'f' ) )
			.append( QString( ", m_nFrameOffsetTempo: %1" ).arg( m_nFrameOffsetTempo ) )
			.append( QString( ", m_fTickOffsetQueuing: %1" ).arg( m_fTickOffsetQueuing, 0, 'f' ) )
			.append( QString( ", m_fTickOffsetSongSize: %1" ).arg( m_fTickOffsetSongSize, 0, 'f' ) );
		if ( m_pNextPatterns != nullptr ) {
			sOutput.append( QString( ", m_pNextPatterns: %1" )
					 .arg( m_pNextPatterns->toQString( "", bShort ) ) );
		} else {
			sOutput.append( ", m_pNextPatterns: nullptr" );
		}
		if ( m_pPlayingPatterns != nullptr ) {
			sOutput.append( QString( ", m_pPlayingPatterns: %1" )
					 .arg( m_pPlayingPatterns->toQString( "", bShort ) ) );
		} else {
			sOutput.append( ", m_pPlayingPatterns: nullptr" );
		}
		sOutput.append( QString( ", m_nPatternSize: %1" ).arg( m_nPatternSize ) )
			.append( QString( ", m_nLastLeadLagFactor: %1" ).arg( m_nLastLeadLagFactor ) )
			.append( QString( ", m_nBar: %1" ).arg( m_nBar ) )
			.append( QString( ", m_nBeat: %1" ).arg( m_nBeat ) );

	}
	
	return sOutput;
}

};
