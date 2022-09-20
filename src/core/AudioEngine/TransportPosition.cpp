/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Timeline.h>
#include <core/config.h>

namespace H2Core {

TransportPosition::TransportPosition() {
	reset();
}


TransportPosition::~TransportPosition() {
}

void TransportPosition::set( std::shared_ptr<TransportPosition> pOther ) {
	m_nFrames = pOther->m_nFrames;
	m_fTick = pOther->m_fTick;
	m_fTickSize = pOther->m_fTickSize;
	m_fBpm = pOther->m_fBpm;
	m_nPatternStartTick = pOther->m_nPatternStartTick;
	m_nPatternTickPosition = pOther->m_nPatternTickPosition;
	m_nColumn = pOther->m_nColumn;
	m_fTickMismatch = pOther->m_fTickMismatch;
}

void TransportPosition::reset() {
	m_nFrames = 0;
	m_fTick = 0;
	m_fTickSize = 400;
	m_fBpm = 120;
	m_nPatternStartTick = 0;
	m_nPatternTickPosition = 0;
	m_nColumn = -1;
	m_fTickMismatch = 0;
}

void TransportPosition::setBpm( float fNewBpm ) {
	if ( fNewBpm > MAX_BPM ) {
		ERRORLOG( QString( "Provided bpm [%1] is too high. Assigning upper bound %2 instead" )
					.arg( fNewBpm ).arg( MAX_BPM ) );
		fNewBpm = MAX_BPM;
	} else if ( fNewBpm < MIN_BPM ) {
		ERRORLOG( QString( "Provided bpm [%1] is too low. Assigning lower bound %2 instead" )
					.arg( fNewBpm ).arg( MIN_BPM ) );
		fNewBpm = MIN_BPM;
	}
	
	m_fBpm = fNewBpm;

	if ( Preferences::get_instance()->getRubberBandBatchMode() ) {
		Hydrogen::get_instance()->recalculateRubberband( getBpm() );
	}
}
 
void TransportPosition::setFrames( long long nNewFrames ) {
	if ( nNewFrames < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] is negative. Setting frame 0 instead." )
				  .arg( nNewFrames ) );
		nNewFrames = 0;
	}
	
	m_nFrames = nNewFrames;
}

void TransportPosition::setTick( double fNewTick ) {
	if ( fNewTick < 0 ) {
		ERRORLOG( QString( "Provided tick [%1] is negative. Setting frame 0 instead." )
				  .arg( fNewTick ) );
		fNewTick = 0;
	}
	
	m_fTick = fNewTick;
}

void TransportPosition::setTickSize( float fNewTickSize ) {
	if ( fNewTickSize <= 0 ) {
		ERRORLOG( QString( "Provided tick size [%1] is too small. Using 400 as a fallback instead." )
				  .arg( fNewTickSize ) );
		fNewTickSize = 400;
	}

	m_fTickSize = fNewTickSize;
}

void TransportPosition::setPatternStartTick( long nPatternStartTick ) {
	if ( nPatternStartTick < 0 ) {
		ERRORLOG( QString( "Provided tick [%1] is negative. Setting frame 0 instead." )
				  .arg( nPatternStartTick ) );
		nPatternStartTick = 0;
	}
	
	m_nPatternStartTick = nPatternStartTick;
}

void TransportPosition::setPatternTickPosition( long nPatternTickPosition ) {
	if ( nPatternTickPosition < 0 ) {
		ERRORLOG( QString( "Provided tick [%1] is negative. Setting frame 0 instead." )
				  .arg( nPatternTickPosition ) );
		nPatternTickPosition = 0;
	}
	
	m_nPatternTickPosition = nPatternTickPosition;
}

void TransportPosition::setColumn( int nColumn ) {
	if ( nColumn < -1 ) {
		ERRORLOG( QString( "Provided column [%1] it too small. Using [-1] as a fallback instead." )
				  .arg( nColumn ) );
		nColumn = -1;
	}

	m_nColumn = nColumn;
}
				
// This function uses the assumption that sample rate and resolution
// are constant over the whole song.
long long TransportPosition::computeFrameFromTick( const double fTick, double* fTickMismatch, int nSampleRate ) {

	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	const auto pAudioEngine = pHydrogen->getAudioEngine();
	assert( pSong );

	if ( nSampleRate == 0 ) {
		nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	}
	const int nResolution = pSong->getResolution();
	const double fSongSizeInTicks = pAudioEngine->getSongSizeInTicks();
	
	if ( nSampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		*fTickMismatch = 0;
		return 0;
	}

	if ( fTick == 0 ) {
		*fTickMismatch = 0;
		return 0;
	}
		
	const auto tempoMarkers = pTimeline->getAllTempoMarkers();
	
	long long nNewFrames = 0;
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 &&
			 pTimeline->isFirstTempoMarkerSpecial() ) ) {

		double fNewTick = fTick;
		double fRemainingTicks = fTick;
		double fNextTick, fPassedTicks = 0;
		double fNextTickSize;
		double fNewFrames = 0;

		const int nColumns = pSong->getPatternGroupVector()->size();

		while ( fRemainingTicks > 0 ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {
				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTick = fSongSizeInTicks;
				} else {
					fNextTick =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ) );
				}

				fNextTickSize =
					AudioEngine::computeDoubleTickSize( nSampleRate,
														tempoMarkers[ ii - 1 ]->fBpm,
														nResolution );
				
				if ( fRemainingTicks > ( fNextTick - fPassedTicks ) ) {
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the current transport position.
					fNewFrames += ( fNextTick - fPassedTicks ) * fNextTickSize;

					
					// DEBUGLOG( QString( "[segment] fTick: %1, fNewFrames: %2, fNextTick: %3, fRemainingTicks: %4, fPassedTicks: %5, fNextTickSize: %6, tempoMarkers[ ii - 1 ]->nColumn: %7, tempoMarkers[ ii - 1 ]->fBpm: %8, tick increment (fNextTick - fPassedTicks): %9, frame increment (fRemainingTicks * fNextTickSize): %10" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'g', 30 )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  .arg( fNextTick - fPassedTicks, 0, 'f' )
					// 		  .arg( ( fNextTick - fPassedTicks ) * fNextTickSize, 0, 'g', 30 )
					// 		  );
					
					fRemainingTicks -= fNextTick - fPassedTicks;
					
					fPassedTicks = fNextTick;

				}
				else {
					// The next frame is within this segment.
					fNewFrames += fRemainingTicks * fNextTickSize;

					nNewFrames = static_cast<long long>( std::round( fNewFrames ) );

					// Keep track of the rounding error to be able to
					// switch between fTick and its frame counterpart
					// later on.
					// In case fTick is located close to a tempo
					// marker we will only cover the part up to the
					// tempo marker in here as only this region is
					// governed by fNextTickSize.
					const double fRoundingErrorInTicks =
						( fNewFrames - static_cast<double>( nNewFrames ) ) /
						fNextTickSize;

					// Compares the negative distance between current
					// position (fNewFrames) and the one resulting
					// from rounding - fRoundingErrorInTicks - with
					// the negative distance between current position
					// (fNewFrames) and location of next tempo marker.
					if ( fRoundingErrorInTicks >
						 fPassedTicks + fRemainingTicks - fNextTick ) {
						// Whole mismatch located within the current
						// tempo interval.
						*fTickMismatch = fRoundingErrorInTicks;
					}
					else {
						// Mismatch at this side of the tempo marker.
						*fTickMismatch =
							fPassedTicks + fRemainingTicks - fNextTick;

						const double fFinalFrames = fNewFrames +
							( fNextTick - fPassedTicks - fRemainingTicks ) *
							fNextTickSize;

						// Mismatch located beyond the tempo marker.
						double fFinalTickSize;
						if ( ii < tempoMarkers.size() ) {
							fFinalTickSize =
								AudioEngine::computeDoubleTickSize( nSampleRate,
																	tempoMarkers[ ii ]->fBpm,
																	nResolution );
						}
						else {
							fFinalTickSize =
								AudioEngine::computeDoubleTickSize( nSampleRate,
																	tempoMarkers[ 0 ]->fBpm,
																	nResolution );
						}

						// DEBUGLOG( QString( "[mismatch] fTickMismatch: [%1 + %2], static_cast<double>(nNewFrames): %3, fNewFrames: %4, fFinalFrames: %5, fNextTickSize: %6, fPassedTicks: %7, fRemainingTicks: %8, fFinalTickSize: %9" )
						// 			.arg( fPassedTicks + fRemainingTicks - fNextTick )
						// 			.arg( ( fFinalFrames - static_cast<double>(nNewFrames) ) / fNextTickSize )
						// 			.arg( nNewFrames )
						// 			.arg( fNewFrames, 0, 'f' )
						// 			.arg( fFinalFrames, 0, 'f' )
						// 			.arg( fNextTickSize, 0, 'f' )
						// 			.arg( fPassedTicks, 0, 'f' )
						// 			.arg( fRemainingTicks, 0, 'f' )
						// 			.arg( fFinalTickSize, 0, 'f' ));
						
						*fTickMismatch += 
							( fFinalFrames - static_cast<double>(nNewFrames) ) /
							fFinalTickSize;
					}

					// DEBUGLOG( QString( "[end] fTick: %1, fNewFrames: %2, fNextTick: %3, fRemainingTicks: %4, fPassedTicks: %5, fNextTickSize: %6, tempoMarkers[ ii - 1 ]->nColumn: %7, tempoMarkers[ ii - 1 ]->fBpm: %8, nNewFrames: %9, fTickMismatch: %10, frame increment (fRemainingTicks * fNextTickSize): %11, fRoundingErrorInTicks: %12" )
					// 		  .arg( fTick, 0, 'f' )
					// 		  .arg( fNewFrames, 0, 'g', 30 )
					// 		  .arg( fNextTick, 0, 'f' )
					// 		  .arg( fRemainingTicks, 0, 'f' )
					// 		  .arg( fPassedTicks, 0, 'f' )
					// 		  .arg( fNextTickSize, 0, 'f' )
					// 		  .arg( tempoMarkers[ ii - 1 ]->nColumn )
					// 		  .arg( tempoMarkers[ ii - 1 ]->fBpm )
					// 		  .arg( nNewFrames )
					// 		  .arg( *fTickMismatch, 0, 'g', 30 )
					// 		  .arg( fRemainingTicks * fNextTickSize, 0, 'g', 30 )
					// 		  .arg( fRoundingErrorInTicks, 0, 'f' )
					// 		  );

					fRemainingTicks -= fNewTick - fPassedTicks;
					break;
				}
			}

			if ( fRemainingTicks != 0 ) {
				// The provided fTick is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames (fNewFrames).
				const int nRepetitions = std::floor(fTick / fSongSizeInTicks);
				const double fSongSizeInFrames = fNewFrames;
				
				fNewFrames *= static_cast<double>(nRepetitions);
				fNewTick = std::fmod( fTick, fSongSizeInTicks );
				fRemainingTicks = fNewTick;
				fPassedTicks = 0;

				// DEBUGLOG( QString( "[repeat] frames covered: %1, ticks covered: %2, ticks remaining: %3, nRepetitions: %4, fSongSizeInFrames: %5" )
				// 		  .arg( fNewFrames, 0, 'g', 30 )
				// 		  .arg( fTick - fNewTick, 0, 'g', 30 )
				// 		  .arg( fRemainingTicks, 0, 'g', 30 )
				// 		  .arg( nRepetitions )
				// 		  .arg( fSongSizeInFrames, 0, 'g', 30 )
				// 		  );

				if ( std::isinf( fNewFrames ) ||
					 static_cast<long long>(fNewFrames) >
					 std::numeric_limits<long long>::max() ) {
					ERRORLOG( QString( "Provided ticks [%1] are too large." ).arg( fTick ) );
					return 0;
				}
			}
		}
	} else {

		// As the timeline is not activate, the column passed is of no
		// importance. We harness the ability of the function to
		// collect and choose between tempo information gather from
		// various sources.
		const float fBpm = AudioEngine::getBpmAtColumn( 0 );

		const double fTickSize =
			AudioEngine::computeDoubleTickSize( nSampleRate, fBpm,
												nResolution );
		
		// No Timeline but a single tempo for the whole song.
		const double fNewFrames = static_cast<double>(fTick) *
			fTickSize;
		nNewFrames = static_cast<long long>( std::round( fNewFrames ) );
		*fTickMismatch = ( fNewFrames - static_cast<double>(nNewFrames ) ) /
			fTickSize;

		// DEBUGLOG(QString("[no-timeline] nNewFrames: %1, fTick: %2, fTickSize: %3, fTickMismatch: %4" )
		// 		 .arg( nNewFrames ).arg( fTick, 0, 'f' ).arg( fTickSize, 0, 'f' )
		// 		 .arg( *fTickMismatch, 0, 'g', 30 ));
		
	}
	
	return nNewFrames;
}

double TransportPosition::computeTickFromFrame( const long long nFrame, int nSampleRate ) {
	const auto pHydrogen = Hydrogen::get_instance();

	if ( nFrame < 0 ) {
		ERRORLOG( QString( "Provided frame [%1] must be non-negative" ).arg( nFrame ) );
	}
	
	const auto pSong = pHydrogen->getSong();
	const auto pTimeline = pHydrogen->getTimeline();
	const auto pAudioEngine = pHydrogen->getAudioEngine();
	assert( pSong );

	if ( nSampleRate == 0 ) {
		nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	}
	const int nResolution = pSong->getResolution();
	double fTick = 0;

	const double fSongSizeInTicks = pAudioEngine->getSongSizeInTicks();
	
	if ( nSampleRate == 0 || nResolution == 0 ) {
		ERRORLOG( "Not properly initialized yet" );
		return fTick;
	}

	if ( nFrame == 0 ) {
		return fTick;
	}
		
	const auto tempoMarkers = pTimeline->getAllTempoMarkers();
	
	if ( pHydrogen->isTimelineEnabled() &&
		 ! ( tempoMarkers.size() == 1 &&
			 pTimeline->isFirstTempoMarkerSpecial() ) ) {

		// We are using double precision in here to avoid rounding
		// errors.
		const double fTargetFrames = static_cast<double>(nFrame);
		double fPassedFrames = 0;
		double fNextFrames = 0;
		double fNextTicks, fPassedTicks = 0;
		double fNextTickSize;
		long long nRemainingFrames;

		const int nColumns = pSong->getPatternGroupVector()->size();

		while ( fPassedFrames < fTargetFrames ) {
		
			for ( int ii = 1; ii <= tempoMarkers.size(); ++ii ) {

				fNextTickSize =
					AudioEngine::computeDoubleTickSize( nSampleRate,
														tempoMarkers[ ii - 1 ]->fBpm,
														nResolution );

				if ( ii == tempoMarkers.size() ||
					 tempoMarkers[ ii ]->nColumn >= nColumns ) {
					fNextTicks = fSongSizeInTicks;
				} else {
					fNextTicks =
						static_cast<double>(pHydrogen->getTickForColumn( tempoMarkers[ ii ]->nColumn ));
				}
				fNextFrames = (fNextTicks - fPassedTicks) * fNextTickSize;
		
				if ( fNextFrames < ( fTargetFrames -
									 fPassedFrames ) ) {
				   
					// DEBUGLOG(QString( "[segment] nFrame: %1, fTick: %2, nSampleRate: %3, fNextTickSize: %4, fNextTicks: %5, fNextFrames: %6, tempoMarkers[ ii -1 ]->nColumn: %7, tempoMarkers[ ii -1 ]->fBpm: %8, fPassedTicks: %9, fPassedFrames: %10, fNewTick (tick increment): %11, fNewTick * fNextTickSize (frame increment): %12" )
					// 		 .arg( nFrame )
					// 		 .arg( fTick, 0, 'f' )
					// 		 .arg( nSampleRate )
					// 		 .arg( fNextTickSize, 0, 'f' )
					// 		 .arg( fNextTicks, 0, 'f' )
					// 		 .arg( fNextFrames, 0, 'f' )
					// 		 .arg( tempoMarkers[ ii -1 ]->nColumn )
					// 		 .arg( tempoMarkers[ ii -1 ]->fBpm )
					// 		 .arg( fPassedTicks, 0, 'f' )
					// 		 .arg( fPassedFrames, 0, 'f' )
					// 		 .arg( fNextTicks - fPassedTicks, 0, 'f' )
					// 		 .arg( (fNextTicks - fPassedTicks) * fNextTickSize, 0, 'g', 30 )
					// 		 );
					
					// The whole segment of the timeline covered by tempo
					// marker ii is left of the transport position.
					fTick += fNextTicks - fPassedTicks;

					fPassedFrames += fNextFrames;
					fPassedTicks = fNextTicks;

				} else {
					// The target frame is located within a segment.
					const double fNewTick = (fTargetFrames - fPassedFrames ) /
						fNextTickSize;

					fTick += fNewTick;
					
					// DEBUGLOG(QString( "[end] nFrame: %1, fTick: %2, nSampleRate: %3, fNextTickSize: %4, fNextTicks: %5, fNextFrames: %6, tempoMarkers[ ii -1 ]->nColumn: %7, tempoMarkers[ ii -1 ]->fBpm: %8, fPassedTicks: %9, fPassedFrames: %10, fNewTick (tick increment): %11, fNewTick * fNextTickSize (frame increment): %12" )
					// 		 .arg( nFrame )
					// 		 .arg( fTick, 0, 'f' )
					// 		 .arg( nSampleRate )
					// 		 .arg( fNextTickSize, 0, 'f' )
					// 		 .arg( fNextTicks, 0, 'f' )
					// 		 .arg( fNextFrames, 0, 'f' )
					// 		 .arg( tempoMarkers[ ii -1 ]->nColumn )
					// 		 .arg( tempoMarkers[ ii -1 ]->fBpm )
					// 		 .arg( fPassedTicks, 0, 'f' )
					// 		 .arg( fPassedFrames, 0, 'f' )
					// 		 .arg( fNewTick, 0, 'f' )
					// 		 .arg( fNewTick * fNextTickSize, 0, 'g', 30 )
					// 		 );
											
					fPassedFrames = fTargetFrames;
					
					break;
				}
			}

			if ( fPassedFrames != fTargetFrames ) {
				// The provided nFrame is larger than the song. But,
				// luckily, we just calculated the song length in
				// frames.
				const double fSongSizeInFrames = fPassedFrames;
				const int nRepetitions = std::floor(fTargetFrames / fSongSizeInFrames);
				if ( fSongSizeInTicks * nRepetitions >
					 std::numeric_limits<double>::max() ) {
					ERRORLOG( QString( "Provided frames [%1] are too large." ).arg( nFrame ) );
					return 0;
				}
				fTick = fSongSizeInTicks * nRepetitions;

				fPassedFrames = static_cast<double>(nRepetitions) *
					fSongSizeInFrames;
				fPassedTicks = 0;

				// DEBUGLOG( QString( "[repeat] frames covered: %1, frames remaining: %2, ticks covered: %3,  nRepetitions: %4, fSongSizeInFrames: %5" )
				// 		  .arg( fPassedFrames, 0, 'g', 30 )
				// 		  .arg( fTargetFrames - fPassedFrames, 0, 'g', 30 )
				// 		  .arg( fTick, 0, 'g', 30 )
				// 		  .arg( nRepetitions )
				// 		  .arg( fSongSizeInFrames, 0, 'g', 30 )
				// 		  );
				
			}
		}
	}
	else {
		// As the timeline is not activate, the column passed is of no
		// importance. We harness the ability of the function to
		// collect and choose between tempo information gather from
		// various sources.
		const float fBpm = AudioEngine::getBpmAtColumn( 0 );
		const double fTickSize =
			AudioEngine::computeDoubleTickSize( nSampleRate, fBpm,
												nResolution );


		// No Timeline. Constant tempo/tick size for the whole song.
		fTick = static_cast<double>(nFrame) / fTickSize;

		// DEBUGLOG(QString( "[no timeline] nFrame: %1, sampleRate: %2, tickSize: %3" )
		// 		 .arg( nFrame ).arg( nSampleRate ).arg( fTickSize, 0, 'f' ) );

	}
	
	return fTick;
}

long long TransportPosition::computeFrame( double fTick, float fTickSize ) {
	return std::round( fTick * fTickSize );
}

double TransportPosition::computeTick( long long nFrame, float fTickSize ) {
	return nFrame / fTickSize;
}

QString TransportPosition::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[TransportPosition]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nFrames: %3\n" ).arg( sPrefix ).arg( s ).arg( getFrames() ) )
			.append( QString( "%1%2m_fTick: %3\n" ).arg( sPrefix ).arg( s ).arg( getDoubleTick(), 0, 'f' ) )
			.append( QString( "%1%2m_fTick (rounded): %3\n" ).arg( sPrefix ).arg( s ).arg( getTick() ) )
			.append( QString( "%1%2m_fTickSize: %3\n" ).arg( sPrefix ).arg( s ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( getBpm(), 0, 'f' ) )
			.append( QString( "%1%2m_nPatternStartTick: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternStartTick ) )
			.append( QString( "%1%2m_nPatternTickPosition: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPatternTickPosition ) )
			.append( QString( "%1%2m_nColumn: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nColumn ) )
			.append( QString( "%1%2m_fTickMismatch: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fTickMismatch, 0, 'f' ) );
	}
	else {
		sOutput = QString( "%1[TransportPosition]" ).arg( sPrefix )
			.append( QString( ", m_nFrames: %1" ).arg( getFrames() ) )
			.append( QString( ", m_fTick: %1" ).arg( getDoubleTick(), 0, 'f' ) )
			.append( QString( ", m_fTick (rounded): %1" ).arg( getTick() ) )
			.append( QString( ", m_fTickSize: %1" ).arg( getTickSize(), 0, 'f' ) )
			.append( QString( ", m_fBpm: %1" ).arg( getBpm(), 0, 'f' ) )
			.append( QString( ", m_nPatternStartTick: %1" ).arg( m_nPatternStartTick ) )
			.append( QString( ", m_nPatternTickPosition: %1" ).arg( m_nPatternTickPosition ) )
			.append( QString( ", m_nColumn: %1" ).arg( m_nColumn ) )
			.append( QString( ", m_fTickMismatch: %1" ).arg( m_fTickMismatch, 0, 'f' ) );
	}
	
	return sOutput;
}

};
