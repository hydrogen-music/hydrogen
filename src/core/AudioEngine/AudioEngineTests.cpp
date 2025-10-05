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

#include <core/AudioEngine/AudioEngineTests.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>

#include <random>
#include <stdexcept>

#include <QTest>

namespace H2Core
{

#ifdef H2CORE_HAVE_JACK
JackAudioDriver::Timebase AudioEngineTests::m_referenceTimebase =
	JackAudioDriver::Timebase::None;
#endif

void AudioEngineTests::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	
	CoreActionController::activateTimeline( true );
	CoreActionController::addTempoMarker( 0, 120 );
	CoreActionController::addTempoMarker( 3, 100 );
	CoreActionController::addTempoMarker( 5, 40 );
	CoreActionController::addTempoMarker( 7, 200 );

	auto checkFrame = []( long long nFrame, double fTolerance ) {
		const double fTick = TransportPosition::computeTickFromFrame( nFrame );

		double fTickMismatch;
		const long long nFrameCheck =
			TransportPosition::computeFrameFromTick( fTick, &fTickMismatch );
		
		if ( nFrameCheck != nFrame || std::abs( fTickMismatch ) > fTolerance ) {
			AudioEngineTests::throwException(
				QString( "[testFrameToTickConversion::checkFrame] nFrame: %1, fTick: %2, nFrameComputed: %3, fTickMismatch: %4, frame diff: %5, fTolerance: %6" )
				.arg( nFrame ).arg( fTick, 0, 'E', -1 ).arg( nFrameCheck )
				.arg( fTickMismatch, 0, 'E', -1 ).arg( nFrameCheck - nFrame )
				.arg( fTolerance, 0, 'E', -1 ) );
		}
	};
	checkFrame( 342732, 1e-10 );
	checkFrame( 1037223, 1e-10 );
	checkFrame( 453610333722, 1e-6 );

	auto checkTick = []( double fTick, double fTolerance ) {
		double fTickMismatch;
		const long long nFrame =
			TransportPosition::computeFrameFromTick( fTick, &fTickMismatch );
		
		const double fTickCheck =
			TransportPosition::computeTickFromFrame( nFrame ) + fTickMismatch;

		if ( abs( fTickCheck - fTick ) > fTolerance ) {
			AudioEngineTests::throwException(
				QString( "[testFrameToTickConversion::checkTick] nFrame: %1, fTick: %2, fTickComputed: %3, fTickMismatch: %4, tick diff: %5, fTolerance: %6" )
				.arg( nFrame ).arg( fTick, 0, 'E', -1 ).arg( fTickCheck, 0, 'E', -1 )
				.arg( fTickMismatch, 0, 'E', -1 ).arg( fTickCheck - fTick, 0, 'E', -1 )
				.arg( fTolerance, 0, 'E', -1 ));
		}
	};
	checkTick( 552, 1e-9 );
	checkTick( 1939, 1e-9 );
	checkTick( 534623409, 1e-6 );
	checkTick( pAE->m_fSongSizeInTicks * 3, 1e-9 );
}

void AudioEngineTests::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	auto pQueuingPos = pAE->m_pQueuingPosition;
	
	CoreActionController::activateTimeline( false );
	CoreActionController::activateLoopMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	pAE->reset( false );

	// Check consistency of updated frames, ticks, and queuing
	// position while using a random buffer size (e.g. like PulseAudio
	// does).
	uint32_t nFrames;
	double fCheckTick, fLastTickIntervalEnd;
	long long nCheckFrame, nLastTransportFrame, nTotalFrames, nLastLookahead;
	long nLastQueuingTick;
	int nn;

	auto resetVariables = [&]() {
		nLastTransportFrame = 0;
		nLastQueuingTick = 0;
		fLastTickIntervalEnd = 0;
		nTotalFrames = 0;
		nLastLookahead = 0;
		nn = 0;
	};
	resetVariables();

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {
		nFrames = frameDist( randomEngine );
		processTransport(
			"testTransportProcessing : song mode : constant tempo", nFrames,
			&nLastLookahead, &nLastTransportFrame, &nTotalFrames,
			&nLastQueuingTick, &fLastTickIntervalEnd, true );

		nn++;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing] [song mode : constant tempo] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, pTransportPos->getTickSize(): %3, pAE->getSongSizeInTicks(): %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->getSongSizeInTicks(), 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	pAE->reset( false );

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	// Check whether all frames are covered when running playback in song mode
	// without looping.
	CoreActionController::activateLoopMode( false );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );
	resetVariables();
	while ( nn <= nMaxCycles ) {
		nFrames = frameDist( randomEngine );
		pAE->incrementTransportPosition( nFrames );

		if ( pAE->isEndOfSongReached( pAE->m_pTransportPosition ) ) {
			// End of song reached
			if ( pTransportPos->getTick() < pAE->getSongSizeInTicks() ) {
				AudioEngineTests::throwException(
					QString( "[testTransportProcessing] [song mode : no looping] final tick was not reached at song end. pTransportPos->getTick: [%1], pAE->getSongSizeInTicks: %2" )
					.arg( pTransportPos->getTick() ).arg( pAE->getSongSizeInTicks() ) );
			}
			break;
		}

		nn++;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing] [song mode : no looping] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, pTransportPos->getTickSize(): %3, pAE->getSongSizeInTicks(): %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->getSongSizeInTicks(), 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	pAE->reset( false );
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	// Check whether all frames are covered when running playback in song mode
	// without looping.
	CoreActionController::activateLoopMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	resetVariables();

	float fBpm;
	float fLastBpm = pTransportPos->getBpm();
	
	const int nCyclesPerTempo = 11;
	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {

		fBpm = tempoDist( randomEngine );
		pAE->setNextBpm( fBpm );
		pAE->updateBpmAndTickSize( pTransportPos );
		pAE->updateBpmAndTickSize( pQueuingPos );
		
		nLastLookahead = 0;
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			nFrames = frameDist( randomEngine );
			processTransport(
				QString( "testTransportProcessing : song mode : variable tempo %1->%2" )
				.arg( fLastBpm, 0, 'f' ).arg( fBpm, 0, 'f' ), nFrames, &nLastLookahead,
				&nLastTransportFrame, &nTotalFrames, &nLastQueuingTick,
				&fLastTickIntervalEnd, true );
		}
		
		fLastBpm = fBpm;

		nn++;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				"[testTransportProcessing] [song mode : variable tempo] end of the song wasn't reached in time." );
		}
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	// Check consistency of playback in PatternMode
	CoreActionController::activateSongMode( false );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	resetVariables();

	fLastBpm = pTransportPos->getBpm();

	const int nDifferentTempos = 10;
	for ( int tt = 0; tt < nDifferentTempos; ++tt ) {

		fBpm = tempoDist( randomEngine );
		
		pAE->setNextBpm( fBpm );
		pAE->updateBpmAndTickSize( pTransportPos );
		pAE->updateBpmAndTickSize( pQueuingPos );
		
		nLastLookahead = 0;

		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			nFrames = frameDist( randomEngine );
			processTransport(
				QString( "testTransportProcessing : pattern mode : variable tempo %1->%2" )
				.arg( fLastBpm ).arg( fBpm ), nFrames, &nLastLookahead,
				&nLastTransportFrame, &nTotalFrames, &nLastQueuingTick,
				&fLastTickIntervalEnd, true );
		}

		fLastBpm = fBpm;
	}
	
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	CoreActionController::activateSongMode( true );
}

void AudioEngineTests::testTransportProcessingTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pTimeline = pHydrogen->getTimeline();
	auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	auto pQueuingPos = pAE->m_pQueuingPosition;
	
	CoreActionController::activateLoopMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	// Activating the Timeline without requiring the AudioEngine to be locked.
	auto activateTimeline = [&]( bool bEnabled ) {
		pSong->setIsTimelineActivated( bEnabled );

		if ( bEnabled ) {
			pTimeline->activate();
		} else {
			pTimeline->deactivate();
		}

		pAE->handleTimelineChange();
	};
	activateTimeline( true );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	pAE->reset( false );

	// Check consistency of updated frames, ticks, and queuing
	// position while using a random buffer size (e.g. like PulseAudio
	// does).
	uint32_t nFrames;
	double fCheckTick, fLastTickIntervalEnd;
	long long nCheckFrame, nLastTransportFrame, nTotalFrames, nLastLookahead;
	long nLastQueuingTick;
	int nn;

	auto resetVariables = [&]() {
		nLastTransportFrame = 0;
		nLastQueuingTick = 0;
		fLastTickIntervalEnd = 0;
		nTotalFrames = 0;
		nLastLookahead = 0;
		nn = 0;
	};
	resetVariables();

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {
		nFrames = frameDist( randomEngine );
		processTransport(
			QString( "[testTransportProcessingTimeline : song mode : all timeline]" ),
			nFrames, &nLastLookahead, &nLastTransportFrame, &nTotalFrames,
			&nLastQueuingTick, &fLastTickIntervalEnd, false );

		nn++;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingTimeline] [all timeline] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, pTransportPos->getTickSize(): %3, pAE->getSongSizeInTicks(): %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->getSongSizeInTicks(), 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	// Alternate Timeline usage and timeline deactivation with
	// "classical" bpm change".

	pAE->reset( false );
	resetVariables();

	float fBpm;
	float fLastBpm = pTransportPos->getBpm();
	
	const int nCyclesPerTempo = 11;
	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {

		QString sContext;
		if ( nn % 2 == 0 ){
			activateTimeline( false );
			fBpm = tempoDist( randomEngine );
			pAE->setNextBpm( fBpm );
			pAE->updateBpmAndTickSize( pTransportPos );
			pAE->updateBpmAndTickSize( pQueuingPos );

			sContext = "no timeline";
		}
		else {
			activateTimeline( true );
			fBpm = AudioEngine::getBpmAtColumn( pTransportPos->getColumn() );
			
			sContext = "timeline";
		}
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			nFrames = frameDist( randomEngine );
			processTransport(
				QString( "testTransportProcessing : alternating timeline : bpm %1->%2 : %3" )
				.arg( fLastBpm ).arg( fBpm ).arg( sContext ),
				nFrames, &nLastLookahead, &nLastTransportFrame, &nTotalFrames,
				&nLastQueuingTick, &fLastTickIntervalEnd, false );
		}
		
		fLastBpm = fBpm;

		nn++;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				"[testTransportProcessingTimeline] [alternating timeline] end of the song wasn't reached in time." );
		}
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	
	CoreActionController::activateLoopMode( true );
	CoreActionController::activateSongMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	pAE->reset( false );

	// Check consistency of updated frames, ticks, and queuing
	// position while using a random buffer size (e.g. like PulseAudio
	// does).
	double fLastTickIntervalEnd;
	long long nLastTransportFrame, nTotalFrames, nLastLookahead;
	long nLastQueuingTick;
	int nn;

	auto resetVariables = [&]() {
		nLastTransportFrame = 0;
		nLastQueuingTick = 0;
		fLastTickIntervalEnd = 0;
		nTotalFrames = 0;
		nLastLookahead = 0;
		nn = 0;
	};
	resetVariables();

	const int nLoops = 3;
	const double fSongSizeInTicks = pAE->m_fSongSizeInTicks;

	const int nMaxCycles =
		std::max( std::ceil( fSongSizeInTicks /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  fSongSizeInTicks ) *
		nLoops;

	// Run nLoops cycles in total. nLoops - 1 with loop mode enabled
	// and disabling it in the nLoops'th run. This should cause the
	// transport stop when reaching the end of the song again. The
	// condition of the while loop will be true for some longer just
	// to be sure.
	bool bLoopEnabled = true;
	int nRet = 0;
	while ( pTransportPos->getDoubleTick() <
			fSongSizeInTicks * ( nLoops + 2 ) ) {
		nRet = processTransport(
			QString( "[testTransportProcessingTimeline : song mode : all timeline]" ),
			pPref->m_nBufferSize, &nLastLookahead, &nLastTransportFrame,
			&nTotalFrames, &nLastQueuingTick, &fLastTickIntervalEnd, false );

		if ( nRet == -1 ) {
			break;
		}
			

		// Transport did run for nLoops - 1 rounds, let's deactivate
		// loop mode.
		if ( bLoopEnabled && pTransportPos->getDoubleTick() >
			 fSongSizeInTicks * ( nLoops - 1 ) ) {
			pAE->setState( AudioEngine::State::Ready );
			pAE->unlock();
			CoreActionController::activateLoopMode( false );
			pAE->lock( RIGHT_HERE );
			pAE->setState( AudioEngine::State::Testing );
		}
		
		nn++;
		if ( nn > nMaxCycles ||
			 pTransportPos->getDoubleTick() > fSongSizeInTicks * nLoops ) {
			AudioEngineTests::throwException(
				QString( "[testLoopMode] transport is rolling for too long. pTransportPos: %1,\n\tfSongSizeInTicks(): %2, nLoops: %3, pPref->m_nBufferSize: %4, nMaxCycles: %5" )
				.arg( pTransportPos->toQString() )
				.arg( fSongSizeInTicks, 0, 'f' ).arg( nLoops )
				.arg( pPref->m_nBufferSize ).arg( nMaxCycles ) );
		}
	}

	// Ensure transport did run the requested number of loops.
	if ( pAE->m_pQueuingPosition->getDoubleTick() < fSongSizeInTicks * nLoops ) {
		AudioEngineTests::throwException(
			QString( "[testLoopMode] transport ended prematurely. pAE->m_pQueuingPosition: %1,\n\tfSongSizeInTicks(): %2, nLoops: %3, pPref->m_nBufferSize: %4" )
			.arg( pAE->m_pQueuingPosition->toQString() )
			.arg( fSongSizeInTicks, 0, 'f' ).arg( nLoops )
			.arg( pPref->m_nBufferSize ) );
	}
		

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

int AudioEngineTests::processTransport( const QString& sContext,
										 int nFrames,
										 long long* nLastLookahead,
										 long long* nLastTransportFrame,
										 long long* nTotalFrames,
										 long* nLastQueuingTick,
										 double* fLastTickIntervalEnd,
										 bool bCheckLookahead ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	auto pAE = Hydrogen::get_instance()->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	auto pQueuingPos = pAE->m_pQueuingPosition;

	double fTickStart, fTickEnd;
	const long long nLeadLag =
		pAE->computeTickInterval( &fTickStart, &fTickEnd, nFrames );
	fTickStart = pAE->coarseGrainTick( fTickStart );
	fTickEnd = pAE->coarseGrainTick( fTickEnd );

	if ( bCheckLookahead ) {
		// If this is the first call after a tempo change, the last
		// lookahead will be set to 0
		if ( *nLastLookahead != 0 &&
			 *nLastLookahead != nLeadLag + AudioEngine::nMaxTimeHumanize + 1 ) {
			AudioEngineTests::throwException(
				QString( "[processTransport : lookahead] [%1] with one and the same BPM/tick size the lookahead must be consistent! [ %2 -> %3 ]" )
				.arg( sContext ).arg( *nLastLookahead )
				.arg( nLeadLag + AudioEngine::nMaxTimeHumanize + 1 ) );
		}
		*nLastLookahead = nLeadLag + AudioEngine::nMaxTimeHumanize + 1;
	}

	pAE->updateNoteQueue( nFrames );
	pAE->incrementTransportPosition( nFrames );

	if ( pAE->isEndOfSongReached( pAE->m_pTransportPosition ) ) {
		// Don't check consistency at the end of the song as just the
		// remaining frames are covered.
		return -1;
	}

	AudioEngineTests::checkTransportPosition(
		pTransportPos, "[processTransport] " + sContext );

	AudioEngineTests::checkTransportPosition(
		pQueuingPos, "[processTransport] " + sContext );

	if ( pTransportPos->getFrame() - nFrames -
		 pTransportPos->getFrameOffsetTempo() != *nLastTransportFrame ) {
		AudioEngineTests::throwException(
			QString( "[processTransport : transport] [%1] inconsistent frame update. pTransportPos->getFrame(): %2, nFrames: %3, nLastTransportFrame: %4, pTransportPos->getFrameOffsetTempo(): %5, pAE->m_fSongSizeInTicks: %6, pAE->m_nLoopsDone: %7" )
			.arg( sContext ).arg( pTransportPos->getFrame() ).arg( nFrames )
			.arg( *nLastTransportFrame ).arg( pTransportPos->getFrameOffsetTempo() )
			.arg( pAE->m_fSongSizeInTicks ).arg( pAE->m_nLoopsDone ) );
	}
	*nLastTransportFrame = pTransportPos->getFrame() -
			pTransportPos->getFrameOffsetTempo();

	const int nNoteQueueUpdate =
		static_cast<int>( fTickEnd ) - static_cast<int>( fTickStart );
	// We will only compare the queuing position in case interval
	// in updateNoteQueue covers at least one tick and, thus,
	// an update has actually taken place.
	if ( *nLastQueuingTick > 0 && nNoteQueueUpdate > 0 ) {
		if ( pQueuingPos->getTick() - nNoteQueueUpdate !=
			 *nLastQueuingTick &&
			 ! pAE->isEndOfSongReached( pQueuingPos ) ) {
			AudioEngineTests::throwException(
				QString( "[processTransport : queuing pos] [%1] inconsistent tick update. pQueuingPos->getTick(): %2, nNoteQueueUpdate: %3, nLastQueuingTick: %4, fTickStart: %5, fTickEnd: %6, nFrames = %7, pTransportPos: %8, pQueuingPos: %9, pAE->m_fSongSizeInTicks: %10, pAE->m_nLoopsDone: %11" )
				.arg( sContext ).arg( pQueuingPos->getTick() )
				.arg( nNoteQueueUpdate ).arg( *nLastQueuingTick )
				.arg( fTickStart, 0, 'f' ).arg( fTickEnd, 0, 'f' )
				.arg( nFrames ).arg( pTransportPos->toQString() )
				.arg( pQueuingPos->toQString() )
				.arg( pAE->m_fSongSizeInTicks ).arg( pAE->m_nLoopsDone ));
		}
	}
	*nLastQueuingTick = pQueuingPos->getTick();

	// Check whether the tick interval covered in updateNoteQueue
	// is consistent and does not include holes or overlaps.
	// In combination with testNoteEnqueuing this should
	// guarantuee that all note will be queued properly.
	if ( std::abs( fTickStart - *fLastTickIntervalEnd ) > 1E-4 ||
		 fTickStart > fTickEnd ) {
		AudioEngineTests::throwException(
			QString( "[processTransport : tick interval] [%1] inconsistent update. old: [ ... : %2 ], new: [ %3, %4 ], pTransportPos->getTickOffsetQueuing(): %5, diff: %6" )
			.arg( sContext ).arg( *fLastTickIntervalEnd ).arg( fTickStart )
			.arg( fTickEnd ).arg( pTransportPos->getTickOffsetQueuing() )
			.arg( std::abs( fTickStart - *fLastTickIntervalEnd ), 0, 'E', -1 ) );
	}
	*fLastTickIntervalEnd = fTickEnd;

	// Using the offset Hydrogen can keep track of the actual
	// number of frames passed since the playback was started
	// even in case a tempo change was issued by the user.
	*nTotalFrames += nFrames;
	if ( pTransportPos->getFrame() - pTransportPos->getFrameOffsetTempo() !=
		 *nTotalFrames ) {
		AudioEngineTests::throwException(
			QString( "[processTransport : total] [%1] total frames incorrect. pTransportPos->getFrame(): %2, pTransportPos->getFrameOffsetTempo(): %3, nTotalFrames: %4" )
			.arg( sContext ).arg( pTransportPos->getFrame() )
			.arg( pTransportPos->getFrameOffsetTempo() ).arg( *nTotalFrames ) );
	}

	return 0;
}

void AudioEngineTests::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, pAE->m_fSongSizeInTicks );
	std::uniform_int_distribution<long long> frameDist( 0, pPref->m_nBufferSize );

	pAE->reset( false );

	// Check consistency of updated frames and ticks while relocating
	// transport.
	double fNewTick;
	long long nNewFrame;

	const int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {

		if ( nn < nProcessCycles - 2 ) {
			fNewTick = tickDist( randomEngine );
		}
		else if ( nn < nProcessCycles - 1 ) {
			// Resulted in an unfortunate rounding error due to the
			// song end.
			fNewTick = pSong->lengthInTicks() - 1 + 0.928009209;
		}
		else {
			// There was a rounding error at this particular tick.
			fNewTick = std::fmin( 960, pSong->lengthInTicks() );
		}

		pAE->locate( fNewTick, false );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocation] mismatch tick-based" );

		// Frame-based relocation
		nNewFrame = frameDist( randomEngine );
		pAE->locateToFrame( nNewFrame );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocation] mismatch frame-based" );

	}

	pAE->reset( false );
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testSongSizeChange() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();

	const int nTestColumn = 4;
	
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );
	pAE->reset( false );
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	
	CoreActionController::activateLoopMode( true );
	CoreActionController::locateToColumn( nTestColumn );
	
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	AudioEngineTests::toggleAndCheckConsistency( 1, 1, "[testSongSizeChange] prior" );
		
	// Toggle a grid cell after to the current transport position
	AudioEngineTests::toggleAndCheckConsistency( 6, 6, "[testSongSizeChange] after" );

	// Now we head to the "same" position inside the song but with the
	// transport being looped once.
	long nNextTick = pHydrogen->getTickForColumn( nTestColumn );
	if ( nNextTick == -1 ) {
		AudioEngineTests::throwException(
			QString( "[testSongSizeChange] Bad test design: there is no column [%1]" )
			.arg( nTestColumn ) );
	}

	nNextTick += pSong->lengthInTicks();
	
	pAE->locate( nNextTick );
	
	AudioEngineTests::toggleAndCheckConsistency( 1, 1, "[testSongSizeChange] looped:prior" );
		
	// Toggle a grid cell after to the current transport position
	AudioEngineTests::toggleAndCheckConsistency( 13, 6, "[testSongSizeChange] looped:after" );

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	CoreActionController::activateLoopMode( false );
}

void AudioEngineTests::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	
	CoreActionController::activateTimeline( false );
	CoreActionController::activateLoopMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	const int nColumns = pSong->getPatternGroupVector()->size();

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 1, pPref->m_nBufferSize );
	std::uniform_int_distribution<int> columnDist( nColumns, nColumns + 100 );

	pAE->reset( false );

	const uint32_t nFrames = 500;
	const double fInitialSongSize = pAE->m_fSongSizeInTicks;
	int nNewColumn;
	double fTick;

	auto checkState = [&]( const QString& sContext, bool bSongSizeShouldChange ){
		AudioEngineTests::checkTransportPosition(
			pTransportPos,
			QString( "[testSongSizeChangeInLoopMode::checkState] [%1] before increment" )
			.arg( sContext ) );

		if ( bSongSizeShouldChange &&
			 fInitialSongSize == pAE->m_fSongSizeInTicks ) {
			AudioEngineTests::throwException(
				QString( "[testSongSizeChangeInLoopMode] [%1] song size stayed the same [%2->%3]")
				.arg( sContext ).arg( fInitialSongSize ).arg( pAE->m_fSongSizeInTicks ) );
		}
		else if ( ! bSongSizeShouldChange &&
			 fInitialSongSize != pAE->m_fSongSizeInTicks ) {
			AudioEngineTests::throwException(
				QString( "[testSongSizeChangeInLoopMode] [%1] unexpected song enlargement [%2->%3]")
				.arg( sContext ).arg( fInitialSongSize ).arg( pAE->m_fSongSizeInTicks ) );
		}

		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos,
			QString( "[testSongSizeChangeInLoopMode::checkState] [%1] after increment" )
			.arg( sContext ) );
	};

	const int nNumberOfTogglings = 5;
	for ( int nn = 0; nn < nNumberOfTogglings; ++nn ) {

		fTick = tickDist( randomEngine );
		pAE->locate( fInitialSongSize + fTick );

		checkState( QString( "relocation to [%1]" ).arg( fTick ), false );

		nNewColumn = columnDist( randomEngine );

		pAE->setState( AudioEngine::State::Ready );
		pAE->unlock();
		CoreActionController::toggleGridCell( GridPoint( nNewColumn, 0 ) );
		pAE->lock( RIGHT_HERE );
		pAE->setState( AudioEngine::State::Testing );

		checkState( QString( "toggling column [%1]" ).arg( nNewColumn ), true );

		pAE->setState( AudioEngine::State::Ready );
		pAE->unlock();
		CoreActionController::toggleGridCell( GridPoint( nNewColumn, 0 ) );
		pAE->lock( RIGHT_HERE );
		pAE->setState( AudioEngine::State::Testing );

		checkState( QString( "again toggling column [%1]" ).arg( nNewColumn ), false );
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testNoteEnqueuing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	const auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	auto pQueuingPos = pAE->m_pQueuingPosition;

	CoreActionController::activateTimeline( false );
	CoreActionController::activateLoopMode( false );
	CoreActionController::activateSongMode( true );
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( pPref->m_nBufferSize / 2,
												  pPref->m_nBufferSize );

	pAE->reset( false );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	int nn = 0;

	int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );


	// Ensure the sampler is clean.
	AudioEngineTests::resetSampler( "testNoteEnqueuing : song mode" );

	auto notesInSong = pSong->getAllNotes();
	std::vector<std::shared_ptr<Note>> notesInSongQueue;
	std::vector<std::shared_ptr<Note>> notesInSamplerQueue;

	auto retrieveNotes = [&]( const QString& sContext ) {
		// Add freshly enqueued notes.
		AudioEngineTests::mergeQueues( &notesInSongQueue,
									   AudioEngineTests::copySongNoteQueue() );
		pAE->processAudio( nFrames );

		AudioEngineTests::mergeQueues( &notesInSamplerQueue,
									   pSampler->getPlayingNotesQueue() );

		pAE->incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testNoteEnqueuing::retrieveNotes] [%1] end of the song wasn't reached in time. pTransportPos->getFrame(): %2, pTransportPos->getDoubleTick(): %3, getTickSize(): %4, pAE->m_fSongSizeInTicks: %5, nMaxCycles: %6" )
				.arg( sContext ).arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' ).arg( nMaxCycles ) );
		}
	};

	nn = 0;
	int nRes;
	while ( pQueuingPos->getDoubleTick() < pAE->m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );
		pAE->updateNoteQueue( nFrames );
		retrieveNotes( "song mode" );
	}

	auto checkQueueConsistency = [&]( const QString& sContext ) {
		if ( notesInSongQueue.size() !=
			 notesInSong.size() ) {
			QString sMsg = QString( "[testNoteEnqueuing::checkQueueConsistency] [%1] Mismatch between notes count in Song [%2] and NoteQueue [%3]. Song:\n" )
				.arg( sContext ).arg( notesInSong.size() )
				.arg( notesInSongQueue.size() );
			for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
				auto pNote = notesInSong[ ii ];
				sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
							 .arg( ii )
							 .arg( pNote->getInstrument() != nullptr ?
								   pNote->getInstrument()->getName() :
								   "nullptr" )
							 .arg( pNote->getPosition() )
							 .arg( pNote->getNoteStart() )
							 .arg( pNote->getVelocity() ) );
			}
			sMsg.append( "NoteQueue:\n" );
			for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
				auto pNote = notesInSongQueue[ ii ];
				sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
							 .arg( ii )
							 .arg( pNote->getInstrument() != nullptr ?
								   pNote->getInstrument()->getName() :
								   "nullptr" )
							 .arg( pNote->getPosition() )
							 .arg( pNote->getNoteStart() )
							 .arg( pNote->getVelocity() ) );
			}

			AudioEngineTests::throwException( sMsg );
		}

		// We have to relax the test for larger buffer sizes. Else, the
		// notes will be already fully processed in and flush from the
		// Sampler before we had the chance to grab and compare them.
		if ( notesInSamplerQueue.size() !=
			 notesInSong.size() &&
			 pPref->m_nBufferSize < 1024 ) {
			QString sMsg = QString( "[testNoteEnqueuing::checkQueueConsistency] [%1] Mismatch between notes count in Song [%2] and Sampler [%3]. Song:\n" )
				.arg( sContext ).arg( notesInSong.size() )
				.arg( notesInSamplerQueue.size() );
			for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
				auto pNote = notesInSong[ ii ];
				sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
							 .arg( ii )
							 .arg( pNote->getInstrument() != nullptr ?
								   pNote->getInstrument()->getName() :
								   "nullptr" )
							 .arg( pNote->getPosition() )
							 .arg( pNote->getNoteStart() )
							 .arg( pNote->getVelocity() ) );
			}
			sMsg.append( "SamplerQueue:\n" );
			for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
				auto pNote = notesInSamplerQueue[ ii ];
				sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
							 .arg( ii )
							 .arg( pNote->getInstrument() != nullptr ?
								   pNote->getInstrument()->getName() :
								   "nullptr" )
							 .arg( pNote->getPosition() )
							 .arg( pNote->getNoteStart() )
							 .arg( pNote->getVelocity() ) );
			}

			AudioEngineTests::throwException( sMsg );
		}
	};
	checkQueueConsistency( "song mode" );

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	//////////////////////////////////////////////////////////////////
	// Perform the test in pattern mode
	//////////////////////////////////////////////////////////////////

	CoreActionController::activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );
	pHydrogen->setSelectedPatternNumber( 4 );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	AudioEngineTests::resetSampler( "testNoteEnqueuing : pattern mode" );

	auto pPattern = 
		pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );
	if ( pPattern == nullptr ) {
		AudioEngineTests::throwException(
			QString( "[testNoteEnqueuing] null pattern selected [%1]" )
			.arg( pHydrogen->getSelectedPatternNumber() ) );
	}

	int nLoops = 5;
	notesInSong.clear();
	for ( int ii = 0; ii < nLoops; ++ii ) {
		FOREACH_NOTE_CST_IT_BEGIN_LENGTH( pPattern->getNotes(), it, pPattern ) {
			if ( it->second != nullptr ) {
				auto note = std::make_shared<Note>( it->second );
				note->setPosition( note->getPosition() +
									ii * pPattern->getLength() );
				notesInSong.push_back( note );
			}
		}
	}

	notesInSongQueue.clear();
	notesInSamplerQueue.clear();

	nMaxCycles = static_cast<int>(
		std::max( static_cast<float>(pPattern->getLength()) *
				  static_cast<float>(nLoops) *
				  pTransportPos->getTickSize() * 4 /
				  static_cast<float>(pPref->m_nBufferSize),
				  static_cast<float>(4 * H2Core::nTicksPerQuarter) *
				  static_cast<float>(nLoops) ));
	nn = 0;
	while ( pQueuingPos->getDoubleTick() < pPattern->getLength() * nLoops ) {

		nFrames = frameDist( randomEngine );
		pAE->updateNoteQueue( nFrames );
		retrieveNotes( "pattern mode" );
	}

	// Transport in pattern mode is always looped. We have to pop the
	// notes added during the second run due to the lookahead.
	auto popSurplusNotes = [&]( std::vector<std::shared_ptr<Note>>* queue ) {
		const int nNoteNumber = queue->size();
		for ( int ii = 0; ii < nNoteNumber; ++ii ) {
			auto pNote = queue->at( nNoteNumber - 1 - ii );
			if ( pNote != nullptr &&
				 pNote->getPosition() >= pPattern->getLength() * nLoops ) {
				queue->pop_back();
			} else {
				break;
			}
		}
	};
	popSurplusNotes( &notesInSongQueue );
	popSurplusNotes( &notesInSamplerQueue );

	checkQueueConsistency( "pattern mode" );
	
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	//////////////////////////////////////////////////////////////////
	// Perform the test in looped song mode
	//////////////////////////////////////////////////////////////////
	// In case the transport is looped the first note was lost the
	// first time transport was wrapped to the beginning again. This
	// occurred just in song mode.
	
	CoreActionController::activateLoopMode( true );
	CoreActionController::activateSongMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );
	pAE->reset( false );

	nLoops = 1;

	nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) ) *
		( nLoops + 1 );
	
	AudioEngineTests::resetSampler( "testNoteEnqueuing : loop mode" );

	notesInSong.clear();
	for ( int ii = 0; ii <= nLoops; ++ii ) {
		auto notesVec = pSong->getAllNotes();
		for ( auto& nnote : notesVec ) {
			nnote->setPosition( nnote->getPosition() +
								 ii * pAE->m_fSongSizeInTicks );
		}
		notesInSong.insert( notesInSong.end(), notesVec.begin(), notesVec.end() );
	}

	notesInSongQueue.clear();
	notesInSamplerQueue.clear();

	nn = 0;
	while ( pQueuingPos->getDoubleTick() < 
			pAE->m_fSongSizeInTicks * ( nLoops + 1 ) ) {

		nFrames = frameDist( randomEngine );

		// Turn off loop mode once we entered the last loop cycle.
		if ( ( pTransportPos->getDoubleTick() >
			   pAE->m_fSongSizeInTicks * nLoops + 100 ) &&
			 pSong->getLoopMode() == Song::LoopMode::Enabled ) {
			pAE->setState( AudioEngine::State::Ready );
			pAE->unlock();
			CoreActionController::activateLoopMode( false );
			pAE->lock( RIGHT_HERE );
			pAE->setState( AudioEngine::State::Testing );
		}

		pAE->updateNoteQueue( nFrames );
		retrieveNotes( "looped song mode" );
	}

	checkQueueConsistency( "looped song mode" );
	
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testNoteEnqueuingTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	const auto pPref = Preferences::get_instance();

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( pPref->m_nBufferSize / 2,
												  pPref->m_nBufferSize );

	pAE->reset( false );
	AudioEngineTests::resetSampler( __PRETTY_FUNCTION__ );

	uint32_t nFrames;

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );

	auto notesInSong = pSong->getAllNotes();
	std::vector<std::shared_ptr<Note>> notesInSongQueue;
	std::vector<std::shared_ptr<Note>> notesInSamplerQueue;

	int nn = 0;
	while ( pTransportPos->getDoubleTick() <
			pAE->m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		pAE->updateNoteQueue( nFrames );

		// Add freshly enqueued notes.
		AudioEngineTests::mergeQueues( &notesInSongQueue,
									   AudioEngineTests::copySongNoteQueue() );

		pAE->processAudio( nFrames );

		AudioEngineTests::mergeQueues( &notesInSamplerQueue,
									   pSampler->getPlayingNotesQueue() );

		pAE->incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testNoteEnqueuingTimeline] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pAE->m_fSongSizeInTicks: %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	if ( notesInSongQueue.size() != notesInSong.size() ) {
		AudioEngineTests::throwException(
			QString( "Mismatching number of notes in song [%1] and note queue [%2]" )
			.arg( notesInSong.size() )
			.arg( notesInSongQueue.size() ) );
	}

	if ( notesInSamplerQueue.size() != notesInSong.size() ) {
		AudioEngineTests::throwException(
			QString( "Mismatching number of notes in song [%1] and sampler queue [%2]" )
			.arg( notesInSong.size() )
			.arg( notesInSamplerQueue.size() ) );
	}

	// Ensure the ordering of the notes is identical
	for ( int ii = 0; ii < notesInSong.size(); ++ii ) {
		if ( ! notesInSong[ ii ]->match(
				 notesInSongQueue[ ii ]->getInstrumentId(),
				 notesInSongQueue[ ii ]->getType(),
				 notesInSongQueue[ ii ]->getKey(),
				 notesInSongQueue[ ii ]->getOctave() ) ) {
		AudioEngineTests::throwException(
			QString( "Mismatch at note [%1] between song [%2]\n and song queue [%3]" )
			.arg( ii )
			.arg( notesInSong[ ii ]->toQString() )
			.arg( notesInSongQueue[ ii ]->toQString() ) );
		}
		if ( ! notesInSong[ ii ]->match(
				 notesInSamplerQueue[ ii ]->getInstrumentId(),
				 notesInSamplerQueue[ ii ]->getType(),
				 notesInSamplerQueue[ ii ]->getKey(),
				 notesInSamplerQueue[ ii ]->getOctave() ) ) {
		AudioEngineTests::throwException(
			QString( "Mismatch at note [%1] between song [%2]\n and sampler queue [%3]" )
			.arg( ii )
			.arg( notesInSong[ ii ]->toQString() )
			.arg( notesInSamplerQueue[ ii ]->toQString() ) );
		}
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testHumanization() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	const auto pPref = Preferences::get_instance();

	CoreActionController::activateLoopMode( false );
	CoreActionController::activateSongMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	pAE->reset( false );

	// Rolls playback from beginning to the end of the song and
	// captures all notes added to the Sampler.
	auto getNotes = [&]( std::vector<std::shared_ptr<Note>> *notes ) {

		AudioEngineTests::resetSampler( "testHumanization::getNotes" );

		// Factor by which the number of frames processed when
		// retrieving notes will be smaller than the buffer size. This
		// vital because when using a large number of frames below the
		// notes might already be processed and flushed from the
		// Sampler before we had the chance to retrieve them.
		const double fStep = 10.0;
		const int nMaxCycles =
			std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
								 static_cast<double>(pPref->m_nBufferSize) * fStep *
								 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
					  static_cast<double>(pAE->m_fSongSizeInTicks) );
		const uint32_t nFrames = static_cast<uint32_t>(
			std::round( static_cast<double>(pPref->m_nBufferSize) / fStep ) );

		pAE->locate( 0 );

		int nn = 0;
		while ( pTransportPos->getDoubleTick() < pAE->m_fSongSizeInTicks ||
				pAE->getEnqueuedNotesNumber() > 0 ) {

			pAE->updateNoteQueue( nFrames );

			pAE->processAudio( nFrames );

			AudioEngineTests::mergeQueues( notes,
										   pSampler->getPlayingNotesQueue() );

			pAE->incrementTransportPosition( nFrames );

			++nn;
			if ( nn > nMaxCycles ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization::getNotes] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pAE->m_fSongSizeInTicks: %4, nMaxCycles: %5" )
					.arg( pTransportPos->getFrame() )
					.arg( pTransportPos->getDoubleTick(), 0, 'f' )
					.arg( pTransportPos->getTickSize(), 0, 'f' )
					.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
					.arg( nMaxCycles ) );
			}
		}
	};

	// Sets the rotaries of velocity and timinig humanization as well
	// as of the pitch randomization of the Kick Instrument (used for
	// the notes in the test song) to a particular value between 0 and
	// 1.
	auto setHumanization = [&]( double fValue ) {
		fValue = std::clamp( fValue, 0.0, 1.0 );

		pSong->setHumanizeTimeValue( fValue );
		pSong->setHumanizeVelocityValue( fValue );

		pSong->getDrumkit()->getInstruments()->get( 0 )->setRandomPitchFactor( fValue );
	};

	auto setSwing = [&]( double fValue ) {
		fValue = std::clamp( fValue, 0.0, 1.0 );

		pSong->setSwingFactor( fValue );
	};

	// Reference notes with no humanization and property
	// customization.
	auto notesInSong = pSong->getAllNotes();

	// First pattern is activated per default.
	setHumanization( 0 );
	setSwing( 0 );

	std::vector<std::shared_ptr<Note>> notesReference;
	getNotes( &notesReference );

	if ( notesReference.size() != notesInSong.size() ) {
		AudioEngineTests::throwException(
			QString( "[testHumanization] [references] Bad test setup. Mismatching number of notes [%1 : %2]" )
			.arg( notesReference.size() )
			.arg( notesInSong.size() ) );
	}

	//////////////////////////////////////////////////////////////////
	// Pattern 2 contains notes of the same instrument at the same
	// positions but velocity, pan, leag&lag, and note key as well as
	// note octave are all customized. Check whether these
	// customizations reach the Sampler.
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	CoreActionController::toggleGridCell( GridPoint( 0, 0 ) );
	CoreActionController::toggleGridCell( GridPoint( 0, 1 ) );
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	std::vector<std::shared_ptr<Note>> notesCustomized;
	getNotes( &notesCustomized );

	if ( notesReference.size() != notesCustomized.size() ) {
		AudioEngineTests::throwException(
			QString( "[testHumanization] [customization] Mismatching number of notes [%1 : %2]" )
			.arg( notesReference.size() )
			.arg( notesCustomized.size() ) );
	}

	for ( int ii = 0; ii < notesReference.size(); ++ii ) {
		auto pNoteReference = notesReference[ ii ];
		auto pNoteCustomized = notesCustomized[ ii ];

		if ( pNoteReference != nullptr && pNoteCustomized != nullptr ) {
			if ( pNoteReference->getVelocity() ==
				 pNoteCustomized->getVelocity() ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [customization] Velocity of note [%1] was not altered" )
					.arg( ii ) );
			} else if ( pNoteReference->getLeadLag() ==
				 pNoteCustomized->getLeadLag() ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [customization] Lead Lag of note [%1] was not altered" )
					.arg( ii ) );
			} else if ( pNoteReference->getNoteStart() ==
				 pNoteCustomized->getNoteStart() ) {
				// The note start incorporates the lead & lag
				// information and is the property used by the
				// Sampler.
				AudioEngineTests::throwException(
					QString( "[testHumanization] [customization] Note start of note [%1] was not altered" )
					.arg( ii ) );
			} else if ( pNoteReference->getPan() ==
				 pNoteCustomized->getPan() ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [customization] Pan of note [%1] was not altered" )
					.arg( ii ) );
			} else if ( pNoteReference->getTotalPitch() ==
				 pNoteCustomized->getTotalPitch() ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [customization] Total Pitch of note [%1] was not altered" )
					.arg( ii ) );
			}
		} else {
			AudioEngineTests::throwException(
				QString( "[testHumanization] [customization] Unable to access note [%1]" )
				.arg( ii ) );
		}

	}

	//////////////////////////////////////////////////////////////////
	// Check whether deviations of the humanized/randomized properties
	// are indeed distributed as a Gaussian with mean zero and an
	// expected standard deviation.
	//
	// Switch back to pattern 1
	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	CoreActionController::toggleGridCell( GridPoint( 0, 1 ) );
	CoreActionController::toggleGridCell( GridPoint( 0, 0 ) );
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	auto checkHumanization = [&]( double fValue, std::vector<std::shared_ptr<Note>>* pNotes ) {

		if ( notesReference.size() != pNotes->size() ) {
			AudioEngineTests::throwException(
				QString( "[testHumanization] [humanization] Mismatching number of notes [%1 : %2]" )
				.arg( notesReference.size() )
				.arg( pNotes->size() ) );
		}

		auto checkDeviation = []( std::vector<float>* pDeviations, float fTargetSD, const QString& sContext ) {

			float fMean = std::accumulate( pDeviations->begin(), pDeviations->end(),
										   0.0, std::plus<float>() ) /
				static_cast<float>( pDeviations->size() );

			// Standard deviation
			auto compVariance = [&]( float fValue, float fElement ) {
				return fValue + ( fElement - fMean ) * ( fElement - fMean );
			};
			float fSD = std::sqrt( std::accumulate( pDeviations->begin(),
													pDeviations->end(),
													0.0, compVariance ) /
								   static_cast<float>( pDeviations->size() ) );

			// As we look at random numbers, the observed mean and
			// standard deviation won't match. But there should be no
			// more than 50% difference or something when wrong.
			if ( std::abs( fMean ) > std::abs( fSD ) * 0.5 ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [%1] Mismatching mean [%2] != [0] with std. deviation [%3]" )
					.arg( sContext ).arg( fMean, 0, 'E', -1 )
					.arg( fSD, 0, 'E', -1 ) );
			}
			if ( std::abs( fSD - fTargetSD ) > fTargetSD * 0.5 ) {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [%1] Mismatching standard deviation [%2] != [%3], diff [%4]" )
					.arg( sContext ).arg( fSD, 0, 'E', -1 )
					.arg( fTargetSD, 0, 'E', -1 )
					.arg( fSD - fTargetSD, 0, 'E', -1 ) );
			}

		};

		std::vector<float> deviationsPitch( notesReference.size() );
		std::vector<float> deviationsVelocity( notesReference.size() );
		std::vector<float> deviationsTiming( notesReference.size() );

		for ( int ii = 0; ii < pNotes->size(); ++ii ) {
			auto pNoteReference = notesReference[ ii ];
			auto pNoteHumanized = pNotes->at( ii );

			if ( pNoteReference != nullptr && pNoteHumanized != nullptr ) {
				deviationsVelocity[ ii ] =
					pNoteReference->getVelocity() - pNoteHumanized->getVelocity();
				deviationsPitch[ ii ] =
					pNoteReference->getPitch() - pNoteHumanized->getPitch();
				deviationsTiming[ ii ] =
					pNoteReference->getNoteStart() - pNoteHumanized->getNoteStart();
			} else {
				AudioEngineTests::throwException(
					QString( "[testHumanization] [swing] Unable to access note [%1]" )
					.arg( ii ) );
			}
		}

		// Within the audio engine every property has its own factor
		// multiplied with the humanization value set via the
		// GUI. With the latter ranging from 0 to 1 the factor
		// represent the maximum standard deviation available.
		checkDeviation( &deviationsVelocity,
						AudioEngine::fHumanizeVelocitySD * fValue, "velocity" );
		checkDeviation( &deviationsTiming,
						AudioEngine::fHumanizeTimingSD *
						AudioEngine::nMaxTimeHumanize * fValue, "timing" );
		checkDeviation( &deviationsPitch,
						AudioEngine::fHumanizePitchSD * fValue, "pitch" );
	};

	setHumanization( 0.2 );
	std::vector<std::shared_ptr<Note>> notesHumanizedWeak;
	getNotes( &notesHumanizedWeak );
	checkHumanization( 0.2, &notesHumanizedWeak );

	setHumanization( 0.8 );
	std::vector<std::shared_ptr<Note>> notesHumanizedStrong;
	getNotes( &notesHumanizedStrong );
	checkHumanization( 0.8, &notesHumanizedStrong );

	//////////////////////////////////////////////////////////////////
	// Check whether swing works.
	//
	// There is still discussion about HOW the swing should work and
	// whether the current implementation is valid. Therefore, this
	// test will only check whether setting this option alters at
	// least one note position

	setHumanization( 0 );
	setSwing( 0.5 );
	std::vector<std::shared_ptr<Note>> notesSwing;
	getNotes( &notesSwing );

	if ( notesReference.size() != notesSwing.size() ) {
		AudioEngineTests::throwException(
			QString( "[testHumanization] [swing] Mismatching number of notes [%1 : %2]" )
			.arg( notesReference.size() )
			.arg( notesSwing.size() ) );
	}

	bool bNoteAltered = false;
	for ( int ii = 0; ii < notesReference.size(); ++ii ) {
		auto pNoteReference = notesReference[ ii ];
		auto pNoteSwing = notesSwing[ ii ];

		if ( pNoteReference != nullptr && pNoteSwing != nullptr ) {
			if ( pNoteReference->getNoteStart() !=
				 pNoteSwing->getNoteStart() ) {
				bNoteAltered = true;
			}
		} else {
			AudioEngineTests::throwException(
				QString( "[testHumanization] [swing] Unable to access note [%1]" )
				.arg( ii ) );
		}
	}
	if ( ! bNoteAltered ) {
		AudioEngineTests::throwException( "[testHumanization] [swing] No notes affected." );
	}

	//////////////////////////////////////////////////////////////////

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testMuteGroups() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	const auto pPref = Preferences::get_instance();

	CoreActionController::activateLoopMode( false );
	CoreActionController::activateSongMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	pAE->reset( false );

	// Rolls playback from beginning to the end of the song and checks all notes
	// rendered by the Sampler. Due to the song's design only notes of a single
	// instrument are allowed to be rendered. There might be others, but their
	// ADR has to be in release phase.

	AudioEngineTests::resetSampler( "testMuteGroups" );

	// Factor by which the number of frames processed when retrieving notes will
	// be smaller than the buffer size. This vital because when using a large
	// number of frames below the notes might already be processed and flushed
	// from the Sampler before we had the chance to retrieve them.
	const double fStep = 10.0;
	const int nMaxCycles = std::max(
		std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
				   static_cast<double>(pPref->m_nBufferSize) * fStep *
				   static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
		static_cast<double>(pAE->m_fSongSizeInTicks) );
	const uint32_t nFrames = static_cast<uint32_t>(
		std::round( static_cast<double>(pPref->m_nBufferSize) / fStep ) );

	pAE->locate( 0 );

	int nn = 0;
	while ( pTransportPos->getDoubleTick() < pAE->m_fSongSizeInTicks ||
			pAE->getEnqueuedNotesNumber() > 0 ) {

		pAE->updateNoteQueue( nFrames );

		pAE->processAudio( nFrames );

		const auto playingNotes = pSampler->getPlayingNotesQueue();

		std::shared_ptr<Instrument> pPlayingInstrument = nullptr;
		for ( const auto& ppNote : playingNotes ) {
			if ( ppNote == nullptr || ppNote->getInstrument() == nullptr ||
				 ppNote->getAdsr() == nullptr ) {
				AudioEngineTests::throwException(
					QString( "[testMuteGroups] invalid note [%1]" )
					.arg( ppNote->toQString() ) );
			}

			if ( pPlayingInstrument == nullptr &&
				 ppNote->getAdsr()->getState() != ADSR::State::Release ) {
				pPlayingInstrument = ppNote->getInstrument();
			}

			if ( pPlayingInstrument != nullptr &&
				 ppNote->getInstrument() != pPlayingInstrument &&
				 ppNote->getAdsr()->getState() != ADSR::State::Release ) {
				AudioEngineTests::throwException(
					QString( "[testMuteGroups] wrong instrument ([%1] is playing): [%2]" )
					.arg( pPlayingInstrument->getName() )
					.arg( ppNote->toQString() ) );
			}

			// In the current design only Crash 1 and Crash 2 should be played
			// back.
			if ( ppNote->getAdsr()->getState() != ADSR::State::Release &&
				 ppNote->getInstrument()->getName() != "Crash 1" &&
				 ppNote->getInstrument()->getName() != "Crash 2" ) {
				AudioEngineTests::throwException(
					QString( "[testMuteGroups] unexpected instrument: [%1]" )
					.arg( ppNote->toQString() ) );
			}
		}

		pAE->incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testMuteGroups] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pAE->m_fSongSizeInTicks: %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testNoteOff() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	const auto pPref = Preferences::get_instance();

	CoreActionController::activateLoopMode( false );
	CoreActionController::activateSongMode( true );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	pAE->reset( false );

	// Rolls playback from beginning to the end of the song and checks all notes
	// rendered by the Sampler. Due to the song's design not a single note
	// should be rendered (noteoff resides either in a chord or in another
	// pattern at the same position).

	AudioEngineTests::resetSampler( "testNoteOff" );

	// Factor by which the number of frames processed when retrieving notes will
	// be smaller than the buffer size. This vital because when using a large
	// number of frames below the notes might already be processed and flushed
	// from the Sampler before we had the chance to retrieve them.
	const double fStep = 10.0;
	const int nMaxCycles = std::max(
		std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
				   static_cast<double>(pPref->m_nBufferSize) * fStep *
				   static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
		static_cast<double>(pAE->m_fSongSizeInTicks) );
	const uint32_t nFrames = static_cast<uint32_t>(
		std::round( static_cast<double>(pPref->m_nBufferSize) / fStep ) );

	pAE->locate( 0 );

	int nn = 0;
	while ( pTransportPos->getDoubleTick() < pAE->m_fSongSizeInTicks ||
			pAE->getEnqueuedNotesNumber() > 0 ) {

		pAE->updateNoteQueue( nFrames );

		pAE->processAudio( nFrames );

		const auto playingNotes = pSampler->getPlayingNotesQueue();

		std::shared_ptr<Instrument> pPlayingInstrument = nullptr;
		for ( const auto& ppNote : playingNotes ) {
			if ( ppNote != nullptr && ppNote->getAdsr() != nullptr &&
				 ppNote->getAdsr()->getState() != ADSR::State::Release ) {
				AudioEngineTests::throwException(
					QString( "[testNoteOff] no note should be rendered [%1]" )
					.arg( ppNote->toQString() ) );
			}
		}

		pAE->incrementTransportPosition( nFrames );

		++nn;
		if ( nn > nMaxCycles ) {
			AudioEngineTests::throwException(
				QString( "[testNoteOff] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pAE->m_fSongSizeInTicks: %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}
	
void AudioEngineTests::mergeQueues( std::vector<std::shared_ptr<Note>>* noteList, std::vector<std::shared_ptr<Note>> newNotes ) {
	bool bNoteFound;
	for ( const auto& newNote : newNotes ) {
		bNoteFound = false;
		// Check whether the notes is already present.
		for ( const auto& presentNote : *noteList ) {
			if ( newNote != nullptr && presentNote != nullptr ) {
				if ( newNote->match( presentNote ) ) {
					bNoteFound = true;
				}
			}
		}

		if ( ! bNoteFound ) {
			noteList->push_back( std::make_shared<Note>(newNote) );
		}
	}
}

void AudioEngineTests::checkTransportPosition( std::shared_ptr<TransportPosition> pPos, const QString& sContext ) {

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();

	double fCheckTickMismatch;
	const long long nCheckFrame =
		TransportPosition::computeFrameFromTick(
			pPos->getDoubleTick(), &fCheckTickMismatch );
	const double fCheckTick =
		TransportPosition::computeTickFromFrame( pPos->getFrame() );
	
	if ( abs( fCheckTick + fCheckTickMismatch - pPos->getDoubleTick() ) > 1e-9 ||
		 abs( fCheckTickMismatch - pPos->m_fTickMismatch ) > 1e-9 ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%8] [tick mismatch]. original position: [%1],\nnCheckFrame: %2, fCheckTick: %3, fCheckTickMismatch: %4, fCheckTick + fCheckTickMismatch - pPos->getDoubleTick(): %5, fCheckTickMismatch - pPos->m_fTickMismatch: %6, nCheckFrame - pPos->getFrame(): %7" )
			.arg( pPos->toQString( "", true ) ).arg( nCheckFrame )
			.arg( fCheckTick, 0 , 'f', 9 ).arg( fCheckTickMismatch, 0 , 'f', 9 )
			.arg( fCheckTick + fCheckTickMismatch - pPos->getDoubleTick(), 0, 'E' )
			.arg( fCheckTickMismatch - pPos->m_fTickMismatch, 0, 'E' )
			.arg( nCheckFrame - pPos->getFrame() ).arg( sContext ) );
	}

	if ( nCheckFrame != pPos->getFrame() ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%8] [frame mismatch]. original position: [%1],\nnCheckFrame: %2, fCheckTick: %3, fCheckTickMismatch: %4, fCheckTick + fCheckTickMismatch - pPos->getDoubleTick(): %5, fCheckTickMismatch - pPos->m_fTickMismatch: %6, nCheckFrame - pPos->getFrame(): %7" )
			.arg( pPos->toQString( "", true ) ).arg( nCheckFrame )
			.arg( fCheckTick, 0 , 'f', 9 ).arg( fCheckTickMismatch, 0 , 'f', 9 )
			.arg( fCheckTick + fCheckTickMismatch - pPos->getDoubleTick(), 0, 'E' )
			.arg( fCheckTickMismatch - pPos->m_fTickMismatch, 0, 'E' )
			.arg( nCheckFrame - pPos->getFrame() ).arg( sContext ) );
	}

	long nCheckPatternStartTick;
	const int nCheckColumn = pHydrogen->getColumnForTick(
		pPos->getTick(), pSong->isLoopEnabled(), &nCheckPatternStartTick );
	const long nTicksSinceSongStart = static_cast<long>(std::floor(
		std::fmod( pPos->getDoubleTick(), pAE->m_fSongSizeInTicks ) ));

	if ( pHydrogen->getMode() == Song::Mode::Song && pPos->getColumn() != -1 &&
		 ( nCheckColumn != pPos->getColumn() ) ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%7] [column mismatch]. current position: [%1],\nnCheckColumn: %2, nCheckPatternStartTick: %3, nCheckPatternTickPosition: %4, nTicksSinceSongStart: %5, pAE->m_fSongSizeInTicks: %6" )
			.arg( pPos->toQString( "", true ) ).arg( nCheckColumn )
			.arg( nCheckPatternStartTick )
			.arg( nTicksSinceSongStart - nCheckPatternStartTick )
			.arg( nTicksSinceSongStart ).arg( pAE->m_fSongSizeInTicks, 0, 'f' )
			.arg( sContext ) );
	}

	if ( pHydrogen->getMode() == Song::Mode::Song && pPos->getColumn() != -1 &&
		 ( ( nCheckPatternStartTick != pPos->getPatternStartTick() ) ||
		   ( nTicksSinceSongStart - nCheckPatternStartTick !=
			 pPos->getPatternTickPosition() ) ) ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%7] [pattern tick mismatch]. current position: [%1],\nnCheckColumn: %2, nCheckPatternStartTick: %3, nCheckPatternTickPosition: %4, nTicksSinceSongStart: %5, pAE->m_fSongSizeInTicks: %6" )
			.arg( pPos->toQString( "", true ) ).arg( nCheckColumn )
			.arg( nCheckPatternStartTick )
			.arg( nTicksSinceSongStart - nCheckPatternStartTick )
			.arg( nTicksSinceSongStart ).arg( pAE->m_fSongSizeInTicks, 0, 'f' )
			.arg( sContext ) );
	}
}

void AudioEngineTests::checkAudioConsistency( const std::vector<std::shared_ptr<Note>> oldNotes,
											  const std::vector<std::shared_ptr<Note>> newNotes, 
											  const QString& sContext,
											  int nPassedFrames, bool bTestAudio,
											  float fPassedTicks ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	double fPassedFrames = static_cast<double>(nPassedFrames);
	const int nSampleRate = pHydrogen->getAudioOutput()->getSampleRate();
	
	int nNotesFound = 0;
	for ( const auto& ppNewNote : newNotes ) {
		for ( const auto& ppOldNote : oldNotes ) {
			if ( ppOldNote != nullptr && ppNewNote != nullptr &&
				 ppOldNote->getInstrumentId() ==
				 ppNewNote->getInstrumentId() &&
				 ppOldNote->getType() == ppNewNote->getType() &&
				 ppOldNote->getKey() == ppNewNote->getKey() &&
				 ppOldNote->getOctave() == ppNewNote->getOctave() &&
				 ppNewNote->getHumanizeDelay() ==
				 ppOldNote->getHumanizeDelay() &&
				 ppOldNote->getInstrument() != nullptr &&
				 ppNewNote->getInstrument() != nullptr &&
				 ppNewNote->getVelocity() == ppOldNote->getVelocity() ) {
				++nNotesFound;

				if ( bTestAudio ) {
					// Check for consistency in the Sample position
					// advanced by the Sampler upon rendering.
					for ( const auto& [ ppComponent, ppOldSelectedLayerInfo ] :
							  ppOldNote->getAllSelectedLayerInfos() ) {
						if ( ppOldSelectedLayerInfo == nullptr ||
							 ppOldSelectedLayerInfo->pLayer == nullptr ||
							 ppOldSelectedLayerInfo->pLayer->getSample() == nullptr ||
							 ppComponent == nullptr ) {
							AudioEngineTests::throwException(
								QString( "[checkAudioConsistency] [%1] Invalid selected layer" )
								.arg( sContext ) );
						}

						const auto pOldLayer = ppOldSelectedLayerInfo->pLayer;
						const auto pOldSample = pOldLayer->getSample();
						
						// The frames passed during the audio
						// processing depends on the sample rate of
						// the driver and sample and has to be
						// adjusted in here. This is equivalent to the
						// question whether Sampler::renderNote() or
						// Sampler::renderNoteResample() was used.
						if ( pOldSample->getSampleRate() != nSampleRate ||
							 ppOldNote->getTotalPitch() != 0.0 ) {
							// In here we assume the layer pitch is zero.
							fPassedFrames = static_cast<double>(nPassedFrames) *
								Note::pitchToFrequency( ppOldNote->getTotalPitch() ) *
								static_cast<float>(pOldSample->getSampleRate()) /
								static_cast<float>(nSampleRate);
						}

						auto pNewSelectedLayerInfo =
							ppNewNote->getSelecterLayerInfo( ppComponent );
						if ( pNewSelectedLayerInfo == nullptr ||
							 pNewSelectedLayerInfo->pLayer == nullptr ||
							 pNewSelectedLayerInfo->pLayer->getSample() == nullptr ) {
							AudioEngineTests::throwException(
								QString( "[checkAudioConsistency] [%1] Mismatching selection between old and new note." )
								.arg( sContext ) );
						}
						const auto pNewLayer = pNewSelectedLayerInfo->pLayer;
						const auto pNewSample = pNewLayer->getSample();

						const int nSampleFrames = pNewSample->getFrames();
						const double fExpectedFrames =
							std::min( static_cast<double>(ppOldSelectedLayerInfo->fSamplePosition) +
									  fPassedFrames,
									  static_cast<double>(nSampleFrames) );
						if ( std::abs( pNewSelectedLayerInfo->fSamplePosition -
									   fExpectedFrames ) > 1 ) {
							AudioEngineTests::throwException(
								QString( "[checkAudioConsistency] [%4] glitch in audio render. Diff: %9\nPre: %1\nPost: %2\nwith passed frames: %3, nSampleFrames: %5, fExpectedFrames: %6, sample sampleRate: %7, driver sampleRate: %8\n" )
								.arg( ppOldNote->toQString( "", true ) )
								.arg( ppNewNote->toQString( "", true ) )
								.arg( fPassedFrames, 0, 'f' ).arg( sContext )
								.arg( nSampleFrames ).arg( fExpectedFrames, 0, 'f' )
								.arg( pOldSample->getSampleRate() )
								.arg( nSampleRate )
								.arg( pNewSelectedLayerInfo->fSamplePosition -
									  fExpectedFrames, 0, 'g', 30 ) );
						}
					}
				}
				else { // if ( bTestAudio )
					// Check whether changes in note start position
					// were properly applied in the note queue of the
					// audio engine.
					if ( ppNewNote->getPosition() - fPassedTicks !=
						 ppOldNote->getPosition() ) {
						AudioEngineTests::throwException(
							QString( "[checkAudioConsistency] [%5] glitch in note queue.\n\tPre: %1\n\tPost: %2\n\tfPassedTicks: %3, diff (new - passed - old): %4" )
							.arg( ppOldNote->toQString( "", true ) )
							.arg( ppNewNote->toQString( "", true ) )
							.arg( fPassedTicks )
							.arg( ppNewNote->getPosition() - fPassedTicks -
								  ppOldNote->getPosition() ).arg( sContext ) );
					}
				}
			}
		}
	}

	// If one of the note vectors is empty - especially the new notes
	// - we can not test anything. But such things might happen as we
	// try various sample sizes and all notes might be already played
	// back and flushed.
	if ( nNotesFound == 0 &&
		 oldNotes.size() > 0 &&
		 newNotes.size() > 0 ) {
		QString sMsg = QString( "[checkAudioConsistency] [%1] bad test design. No notes played back." )
			.arg( sContext );
		sMsg.append( "\nold notes:" );
		for ( const auto& nnote : oldNotes ) {
			sMsg.append( "\n" + nnote->toQString( "    ", true ) );
		}
		sMsg.append( "\nnew notes:" );
		for ( const auto& nnote : newNotes ) {
			sMsg.append( "\n" + nnote->toQString( "    ", true ) );
		}
		sMsg.append( QString( "\n\npTransportPos->getDoubleTick(): %1, pTransportPos->getFrame(): %2, nPassedFrames: %3, fPassedTicks: %4, pTransportPos->getTickSize(): %5" )
					 .arg( pTransportPos->getDoubleTick(), 0, 'f' )
					 .arg( pTransportPos->getFrame() )
					 .arg( nPassedFrames )
					 .arg( fPassedTicks, 0, 'f' )
					 .arg( pTransportPos->getTickSize(), 0, 'f' ) );
		sMsg.append( "\n\n notes in song:" );
		for ( const auto& nnote : pSong->getAllNotes() ) {
			sMsg.append( "\n" + nnote->toQString( "    ", true ) );
		}
		AudioEngineTests::throwException( sMsg );
	}
}

std::vector<std::shared_ptr<Note>> AudioEngineTests::copySongNoteQueue() {
	auto pAE = Hydrogen::get_instance()->getAudioEngine();
	std::vector<std::shared_ptr<Note>> originalNotes;
	std::vector<std::shared_ptr<Note>> copiedNotes;
	for ( ; ! pAE->m_songNoteQueue.empty(); pAE->m_songNoteQueue.pop() ) {
		originalNotes.push_back( pAE->m_songNoteQueue.top() );
		copiedNotes.push_back(
			std::make_shared<Note>( pAE->m_songNoteQueue.top() ) );
	}

	for ( auto& nnote : originalNotes ) {
		pAE->m_songNoteQueue.push( nnote );
	}

	return copiedNotes;
}

void AudioEngineTests::toggleAndCheckConsistency( int nToggleColumn, int nToggleRow, const QString& sContext ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	
	const unsigned long nBufferSize = pHydrogen->getAudioOutput()->getBufferSize();

	pAE->updateNoteQueue( nBufferSize );
	pAE->processAudio( nBufferSize );
	pAE->incrementTransportPosition( nBufferSize );

	// Cache some stuff in order to compare it later on.
	long nOldSongSize;
	int nOldColumn;
	float fPrevTempo, fPrevTickSize;
	double fPrevTickStart, fPrevTickEnd;
	long long nPrevLeadLag;

	std::vector<std::shared_ptr<Note>> notesSamplerPreToggle,
		notesSamplerPostToggle, notesSamplerPostRolling;

	auto notesSongQueuePreToggle = AudioEngineTests::copySongNoteQueue();

	auto toggleAndCheck = [&]( const QString& sContext ) {
		notesSamplerPreToggle.clear();
		for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
			notesSamplerPreToggle.push_back( std::make_shared<Note>( ppNote ) );
		}

		nPrevLeadLag = pAE->computeTickInterval( &fPrevTickStart, &fPrevTickEnd,
												 nBufferSize );
		nOldSongSize = pSong->lengthInTicks();
		nOldColumn = pTransportPos->getColumn();
		fPrevTempo = pTransportPos->getBpm();
		fPrevTickSize = pTransportPos->getTickSize();

		pAE->setState( AudioEngine::State::Ready );
		pAE->unlock();
		CoreActionController::toggleGridCell(
			GridPoint( nToggleColumn, nToggleRow ) );
		pAE->lock( RIGHT_HERE );
		pAE->setState( AudioEngine::State::Testing );

		const QString sNewContext =
			QString( "toggleAndCheckConsistency::toggleAndCheck : %1 : toggling (%2,%3)" )
			.arg( sContext ).arg( nToggleColumn ).arg( nToggleRow );

		// Check whether there was a change in song size
		const long nNewSongSize = pSong->lengthInTicks();
		if ( nNewSongSize == nOldSongSize ) {
			AudioEngineTests::throwException(
				QString( "[%1] no change in song size" ).arg( sNewContext ) );
		}

		// Check whether current frame and tick information are still
		// consistent.
		AudioEngineTests::checkTransportPosition( pTransportPos, sNewContext );

		// m_songNoteQueue have been updated properly.
		const auto notesSongQueuePostToggle = AudioEngineTests::copySongNoteQueue();
		AudioEngineTests::checkAudioConsistency(
			notesSongQueuePreToggle, notesSongQueuePostToggle,
			sNewContext + " : song queue", 0, false,
			pTransportPos->getTickOffsetSongSize() );

		// The post toggle state will become the pre toggle state during
		// the next toggling.
		notesSongQueuePreToggle = notesSongQueuePostToggle;

		notesSamplerPostToggle.clear();
		for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
			notesSamplerPostToggle.push_back( std::make_shared<Note>( ppNote ) );
		}
		AudioEngineTests::checkAudioConsistency(
			notesSamplerPreToggle, notesSamplerPostToggle,
			sNewContext + " : sampler queue", 0, true,
			pTransportPos->getTickOffsetSongSize() );

		// Column must be consistent. Unless the song length shrunk due to
		// the toggling and the previous column was located beyond the new
		// end (in which case transport will be reset to 0).
		if ( nOldColumn < pSong->getPatternGroupVector()->size() ) {
			// Transport was not reset to 0 - happens in most cases.

			if ( nOldColumn != pTransportPos->getColumn() &&
				 nOldColumn < pSong->getPatternGroupVector()->size() ) {
				AudioEngineTests::throwException(
					QString( "[%3] Column changed old: %1, new: %2" )
					.arg( nOldColumn ).arg( pTransportPos->getColumn() )
					.arg( sNewContext ) );
			}

			double fTickEnd, fTickStart;
			const long long nLeadLag =
				pAE->computeTickInterval( &fTickStart, &fTickEnd, nBufferSize );
			if ( std::abs( nLeadLag - nPrevLeadLag ) > 1 ) {
				AudioEngineTests::throwException(
					QString( "[%3] LeadLag should be constant since there should be change in tick size. old: %1, new: %2" )
					.arg( nPrevLeadLag ).arg( nLeadLag ).arg( sNewContext ) );
			}
			if ( std::abs( fTickStart - pTransportPos->getTickOffsetSongSize() - fPrevTickStart ) > 4e-3 ) {
				AudioEngineTests::throwException(
					QString( "[%5] Mismatch in the start of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
					.arg( fTickStart, 0, 'f' )
					.arg( fPrevTickStart + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
					.arg( fPrevTickStart, 0, 'f' )
					.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
					.arg( sNewContext ) );
			}
			if ( std::abs( fTickEnd - pTransportPos->getTickOffsetSongSize() - fPrevTickEnd ) > 4e-3 ) {
				AudioEngineTests::throwException(
					QString( "[%5] Mismatch in the end of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
					.arg( fTickEnd, 0, 'f' )
					.arg( fPrevTickEnd + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
					.arg( fPrevTickEnd, 0, 'f' )
					.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
					.arg( sNewContext ) );
			}
		}
		else if ( pTransportPos->getColumn() != 0 &&
				  nOldColumn >= pSong->getPatternGroupVector()->size() ) {
			AudioEngineTests::throwException(
				QString( "[%4] Column reset failed nOldColumn: %1, pTransportPos->getColumn() (new): %2, pSong->getPatternGroupVector()->size() (new): %3" )
				.arg( nOldColumn )
				.arg( pTransportPos->getColumn() )
				.arg( pSong->getPatternGroupVector()->size() )
				.arg( sNewContext ) );
		}
	
		// Now we emulate that playback continues without any new notes
		// being added and expect the rendering of the notes currently
		// played back by the Sampler to start off precisely where we
		// stopped before the song size change. New notes might still be
		// added due to the lookahead, so, we just check for the
		// processing of notes we already encountered.
		pAE->incrementTransportPosition( nBufferSize );
		pAE->processAudio( nBufferSize );

		// Update the end of the tick interval usually handled by
		// updateNoteQueue().
		double fTickEndRolling, fTickStartUnused;
		pAE->computeTickInterval( &fTickStartUnused, &fTickEndRolling, nBufferSize );
		
		pAE->incrementTransportPosition( nBufferSize );
		pAE->processAudio( nBufferSize );

		pAE->m_fLastTickEnd = fTickEndRolling;

		// Check whether tempo and tick size have not changed.
		if ( fPrevTempo != pTransportPos->getBpm() ||
			 fPrevTickSize != pTransportPos->getTickSize() ) {
			AudioEngineTests::throwException(
				QString( "[%1] tempo and ticksize are affected" )
				.arg( sNewContext ) );
		}

		notesSamplerPostRolling.clear();
		for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
			notesSamplerPostRolling.push_back( std::make_shared<Note>( ppNote ) );
		}
		AudioEngineTests::checkAudioConsistency(
			notesSamplerPostToggle, notesSamplerPostRolling,
			QString( "toggleAndCheckConsistency::toggleAndCheck : %1 : rolling after toggle (%2,%3)" )
			.arg( sContext ).arg( nToggleColumn ).arg( nToggleRow ),
			nBufferSize * 2, true );
	};

	// Toggle the grid cell.
	toggleAndCheck( sContext + " : 1. toggle" );

	// Toggle the same grid cell again.
	toggleAndCheck( sContext + " : 2. toggle" );
}

void AudioEngineTests::resetSampler( const QString& sContext ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	const auto pPref = Preferences::get_instance();

	// Larger number to account for both small buffer sizes and long
	// samples.
	const int nMaxCleaningCycles = 5000;
	int nn = 0;

	// Ensure the sampler is clean.
	while ( pSampler->isRenderingNotes() ) {
		pAE->processAudio( pPref->m_nBufferSize );
		pAE->incrementTransportPosition( pPref->m_nBufferSize );
		++nn;
		
		// {//DEBUG
		//  QString msg = QString( "[%1] nn: %2, note:" )
		// 	   .arg( sContext ).arg( nn );
		// 	auto pNoteQueue = pSampler->getPlayingNotesQueue();
		// 	if ( pNoteQueue.size() > 0 ) {
		// 		auto pNote = pNoteQueue[0];
		// 		if ( pNote != nullptr ) {
		// 			msg.append( pNote->toQString("", true ) );
		// 		} else {
		// 			msg.append( " nullptr" );
		// 		}
		// 		DEBUGLOG( msg );
		// 	}
		// }
		
		if ( nn > nMaxCleaningCycles ) {
			AudioEngineTests::throwException(
				QString( "[%1] Sampler is in weird state" )
				.arg( sContext ) );
		}
	}
	
	pAE->reset( false );
}

void AudioEngineTests::testUpdateTransportPosition() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();

	pAE->lock( RIGHT_HERE );
	pAE->reset( true );

	// Check whether the transport positions in the audio engine are untouched
	// by updateTransportPosition.
	pAE->locate( 42 );
	auto pTransportOld =
		std::make_shared<TransportPosition>( pAE->getTransportPosition() );
	auto pQueuingOld =
		std::make_shared<TransportPosition>( pAE->m_pQueuingPosition );

	auto pTestPos = std::make_shared<TransportPosition>(
		TransportPosition::Type::Test0 );
	const long long nFrame = 3521;
	const auto fTick = TransportPosition::computeTickFromFrame( nFrame );
	pAE->updateTransportPosition( fTick, nFrame, pTestPos );

	if ( pAE->getTransportPosition() != pTransportOld ) {
		throwException( QString( "[testUpdateTransportPosition] Glitch in pAE->m_pTransportPosition:\nOld: %1\nNew: %2" )
						.arg( pTransportOld->toQString() )
						.arg( pAE->getTransportPosition()->toQString() ) );
	}
	if ( pAE->m_pQueuingPosition != pQueuingOld ) {
		throwException( QString( "[testUpdateTransportPosition] Glitch in pAE->m_pQueuingPosition:\nOld: %1\nNew: %2" )
						.arg( pQueuingOld->toQString() )
						.arg( pAE->m_pQueuingPosition->toQString() ) );
	}

	if ( pTransportOld == pTestPos ) {
		throwException( "[testUpdateTransportPosition] Test position shouldn't coincide with pAE->m_pTransportPosition" );
	}

	pAE->unlock();

	// Verify that Hydrogen won't explode in case updateTransportPosition is
	// called with no song set (as it is used in
	// JackAudioEngine::JackTimebaseCallback which will be called as long as the
	// driver is running).
	pHydrogen->setSong( nullptr );

	pAE->lock( RIGHT_HERE );
	auto pNullPos = std::make_shared<TransportPosition>(
		TransportPosition::Type::Test1 );
	pAE->updateTransportPosition( fTick, nFrame, pNullPos );
	pAE->unlock();

	pHydrogen->setSong( pSong );
}

#ifdef H2CORE_HAVE_JACK
void AudioEngineTests::testTransportProcessingJack() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();

	// Check whether all frames are covered when running playback in song mode
	// without looping.
	CoreActionController::activateLoopMode( false );

	pAE->lock( RIGHT_HERE );
	pAE->reset( true );
	pAE->unlock();

	auto pDriver = startJackAudioDriver();
	if ( pDriver == nullptr ) {
		throwException( "[testTransportProcessingJack] Unable to use JACK driver" );
	}

	// In case the reference Hydrogen is JACK Timebase controller, Timeline of
	// this instance is deactivated and we are listening to tempo changes
	// broadcasted by the JACK server. We need to verify we actually receive
	// them.
	bool bTempoChangeEncountered;
	float fBpm;

	pAE->lock( RIGHT_HERE );
	fBpm = pAE->getBpmAtColumn( 0 );

	// The callback function registered to the JACK server will take care of
	// consistency checking. In here we just start playback and wait till it's
	// done.
	pAE->play();
	pAE->unlock();

	// Transport is started via the JACK server. We have to give it some time to
	// communicate transport start back to us.
	QTest::qSleep( 400 );

	const int nMaxMilliSeconds = 11500;
	int nMilliSeconds = 0;
	const int nIncrement = 100;
	while ( pAE->getState() == AudioEngine::State::Playing ||
			pAE->m_nextState == AudioEngine::State::Playing ) {
		if ( ! bTempoChangeEncountered && fBpm != pAE->getBpmAtColumn( 0 ) ) {
			bTempoChangeEncountered = true;
		}

		if ( nMilliSeconds >= nMaxMilliSeconds ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingJack] playback takes too long" ) );
		}

		QTest::qSleep( nIncrement );
		nMilliSeconds += nIncrement;
	}

	pAE->lock( RIGHT_HERE );
	pAE->stop();
	if ( pAE->getState() == AudioEngine::State::Playing ) {
		pAE->stopPlayback();
	}
	pAE->reset( true );
	pAE->unlock();

	if ( pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Listener &&
		 ! bTempoChangeEncountered ) {
		throwException( "[testTransportProcessingJack] no tempo changes received from JACK Timebase controller" );
	}

	stopJackAudioDriver();
}

void AudioEngineTests::testTransportProcessingOffsetsJack() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	// When being JACK Timebase listener Hydrogen will adopt tempo setting
	// provided by an external application and discards internal changes. This
	// test would be identical to testTransportProcessingJack.
	if ( pHydrogen->getJackTimebaseState() ==
		 JackAudioDriver::Timebase::Listener ) {
		return;
	}

	// Check whether all frames are covered when running playback in song mode
	// without looping.
	CoreActionController::activateLoopMode( false );
	CoreActionController::activateTimeline( false );

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	pAE->lock( RIGHT_HERE );
	pAE->reset( true );
	pAE->unlock();

	auto pDriver = startJackAudioDriver();
	if ( pDriver == nullptr ) {
		throwException( "[testTransportProcessingOffsetsJack] Unable to use JACK driver" );
	}

	float fBpm, fLastBpm;
	bool bTempoChanged = false;
	const int nToggleColumn = 4;
	const int nToggleRow = 4;
	const float fOriginalSongSize = pAE->m_fSongSizeInTicks;

	pAE->lock( RIGHT_HERE );
	fLastBpm = pAE->getBpmAtColumn( 0 );
	// The callback function registered to the JACK server will take care of
	// consistency checking. In here we just start playback and wait till it's
	// done.
	pAE->play();
	pAE->unlock();

	// Transport is started via the JACK server. We have to give it some time to
	// communicate transport start back to us.
	QTest::qSleep( 400 );

	const int nMaxMilliSeconds = 11500;
	int nMilliSeconds = 0;
	const int nIncrement = 100;
	while ( pAE->getState() == AudioEngine::State::Playing ||
			pAE->m_nextState == AudioEngine::State::Playing ) {
		if ( ! bTempoChanged && fLastBpm != pAE->getBpmAtColumn( 0 ) ) {
			bTempoChanged = true;
		}

		if ( nMilliSeconds >= nMaxMilliSeconds ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingOffsetsJack] playback takes too long" ) );
		}

		// Alter song size
		//
		// It's alright to not lock the AudioEngine when accessing the current
		// song size. It should never change during regular playback (which is
		// covered by a separate test).
		const auto nOldSongSize = pAE->m_fSongSizeInTicks;
		CoreActionController::toggleGridCell(
			GridPoint( nToggleColumn, nToggleRow ) );
		if ( nOldSongSize == pAE->m_fSongSizeInTicks ) {
			throwException( "[testTransportProcessingOffsetsJack] song size did not change." );
		}
		INFOLOG( QString( "[testTransportProcessingOffsetsJack] update song size [%1] -> [%2]" )
				 .arg( nOldSongSize ).arg( pAE->m_fSongSizeInTicks ) );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportProcessingOffsetsJack] mismatch after song size update" );

		// The sleep helps us to keep song size and tempo-based exceptions
		// apart.
		QTest::qSleep( nIncrement );
		nMilliSeconds += nIncrement;

		fBpm = tempoDist( randomEngine );
		pAE->lock( RIGHT_HERE );
		INFOLOG( QString( "[testTransportProcessingOffsetsJack] changing tempo [%1]->[%2]" )
				.arg( pAE->getBpmAtColumn( 0 ) ).arg( fBpm ) );
		pAE->setNextBpm( fBpm );
		pAE->unlock();

		QTest::qSleep( nIncrement );
		nMilliSeconds += nIncrement;
	}

	pAE->lock( RIGHT_HERE );
	pAE->stop();
	if ( pAE->getState() == AudioEngine::State::Playing ) {
		pAE->stopPlayback();
	}
	pAE->reset( true );
	pAE->unlock();

	if ( ! bTempoChanged ) {
		throwException( "[testTransportProcessingOffsetsJack] tempo was not change. Decrease time increments!" );
	}

	// Ensure the additional grid cell we activate/deactivate is set to its
	// original state.
	if ( pAE->m_fSongSizeInTicks != fOriginalSongSize ) {
		CoreActionController::toggleGridCell(
			GridPoint( nToggleColumn, nToggleRow ) );
	}

	stopJackAudioDriver();
}

void AudioEngineTests::testTransportRelocationJack() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	pAE->lock( RIGHT_HERE );
	pAE->stop();
	if ( pAE->getState() == AudioEngine::State::Playing ) {
		pAE->stopPlayback();
	}

	pAE->reset( true );
	pAE->unlock();

	auto pDriver = startJackAudioDriver();
	if ( pDriver == nullptr ) {
		throwException( "[testTransportRelocationJack] Unable to use JACK driver" );
	}

	pAE->lock( RIGHT_HERE );
#ifdef HAVE_INTEGRATION_TESTS
	JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
	pDriver->m_bIntegrationCheckRelocationLoop = true;
#endif
	pAE->unlock();

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, pAE->m_fSongSizeInTicks );

	// Check consistency of updated frames and ticks while relocating
	// transport.
	double fNewTick;
	long long nNewFrame;


	double fTickMismatch;
	const int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {

		//  When being listener we have no way to properly check the resulting
		//  tick as our ground truth is just the frame information provided by
		//  the JACK server.
		if ( pHydrogen->getJackTimebaseState() !=
				 JackAudioDriver::Timebase::Listener ) {
			if ( nn < nProcessCycles - 2 ) {
				fNewTick = tickDist( randomEngine );
			}
			else if ( nn < nProcessCycles - 1 ) {
				// Resulted in an unfortunate rounding error due to the
				// song end.
				fNewTick = pSong->lengthInTicks() - 1 + 0.928009209;
			}
			else {
				// There was a rounding error at this particular tick.
				fNewTick = std::fmin( 960, pSong->lengthInTicks() );
			}

			pAE->lock( RIGHT_HERE );

			// Ensure we relocate to a new position. Else the assertions in this
			// test will fail.
			while ( std::abs( fNewTick - pTransportPos->getDoubleTick() ) < 1 ) {
				fNewTick = tickDist( randomEngine );
			}

			INFOLOG( QString( "relocate to tick [%1]->[%2]" )
					 .arg( pTransportPos->getDoubleTick() ).arg( fNewTick ) );
			pAE->locate( fNewTick, true );
			pAE->unlock();

			AudioEngineTests::waitForRelocation( pDriver, fNewTick, -1 );
			// We send the tick value to and receive it from the JACK server via
			// routines in the libjack2 library. We have to be relaxed about the
			// precision we can expect.
			if ( abs( pTransportPos->getDoubleTick() - fNewTick ) > 1e-1 ) {
				throwException( QString( "[testTransportRelocationJack::tick] failed to relocate to tick. [%1] != [%2]" )
								.arg( pTransportPos->getDoubleTick() ).arg( fNewTick ) );
			}

#ifdef HAVE_INTEGRATION_TESTS
			// In case there is an issue with the BBT <-> transport position
			// conversion or the m_nTimebaseFrameOffset, the driver will detect
			// multiple relocations (maybe one in each cycle).
			if ( pDriver->m_bIntegrationRelocationLoop ) {
				throwException( "[testTransportRelocationJack::frame] relocation loop detected" );
			}
#endif

			AudioEngineTests::checkTransportPosition(
				pTransportPos, "[testTransportRelocationJack::tick] mismatch tick-based" );
		}

		// Frame-based relocation
		// We sample ticks and convert them since we are using tempo markers.
		if ( nn < nProcessCycles - 1 ) {
			nNewFrame = TransportPosition::computeFrameFromTick(
				tickDist( randomEngine ), &fTickMismatch );
		} else {
			// With this one there were rounding mishaps in v1.2.3
			nNewFrame = std::min( static_cast<long long>(2174246),
								  TransportPosition::computeFrameFromTick(
									  pSong->lengthInTicks(), &fTickMismatch ) );
		}

		pAE->lock( RIGHT_HERE );

		while ( nNewFrame == pTransportPos->getFrame() ) {
			nNewFrame = TransportPosition::computeFrameFromTick(
				tickDist( randomEngine ), &fTickMismatch );
		}

#ifdef HAVE_INTEGRATION_TESTS
		if ( pHydrogen->getJackTimebaseState() ==
				 JackAudioDriver::Timebase::Listener ) {
			// We are listener
			//
			// Discard the previous offset or we do not end up at the frame we
			// provided to locateTransport and the comparison fails.
			pDriver->m_nTimebaseFrameOffset = 0;
			JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
		}
#endif

		INFOLOG( QString( "relocate to frame [%1]->[%2]" )
				 .arg( pTransportPos->getFrame() ).arg( nNewFrame ) );
		pDriver->locateTransport( nNewFrame );
		pAE->unlock();

		AudioEngineTests::waitForRelocation( pDriver, -1, nNewFrame );

		long long nCurrentFrame;
		if ( pHydrogen->getJackTimebaseState() ==
			 JackAudioDriver::Timebase::Listener ) {
			nCurrentFrame = pDriver->m_JackTransportPos.frame;
		}
		else {
			nCurrentFrame = pTransportPos->getFrame();
		}

		if ( nNewFrame != nCurrentFrame ) {
			throwException( QString( "[testTransportRelocationJack::frame] failed to relocate to frame. timebase state: [%1], nNewFrame [%2] != nCurrentFrame [%3], pPos->getFrame(): [%4]" )
							.arg( JackAudioDriver::TimebaseToQString(
									  pDriver->getTimebaseState() ) )
							.arg( nNewFrame )
							.arg( nCurrentFrame )
							.arg( pTransportPos->getFrame() ) );
		}

#ifdef HAVE_INTEGRATION_TESTS
		// In case there is an issue with the BBT <-> transport position
		// conversion or the m_nTimebaseFrameOffset, the driver will detect
		// multiple relocations (maybe one in each cycle).
		if ( pDriver->m_bIntegrationRelocationLoop ) {
			throwException( "[testTransportRelocationJack::frame] relocation loop detected" );
		}
#endif

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocationJack::frame] mismatch frame-based" );
	}

	pAE->lock( RIGHT_HERE );
#ifdef HAVE_INTEGRATION_TESTS
	pDriver->m_bIntegrationCheckRelocationLoop = false;
	JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
#endif
	pAE->reset( true );
	pAE->unlock();

	stopJackAudioDriver();
}

void AudioEngineTests::testTransportRelocationOffsetsJack() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	// When being JACK Timebase listener Hydrogen will adopt tempo setting
	// provided by an external application and discards internal changes. This
	// test would be identical to testTransportProcessingJack.
	if ( pHydrogen->getJackTimebaseState() ==
		 JackAudioDriver::Timebase::Listener ) {
		return;
	}

	CoreActionController::activateTimeline( false );

	pAE->lock( RIGHT_HERE );
	pAE->stop();
	if ( pAE->getState() == AudioEngine::State::Playing ) {
		pAE->stopPlayback();
	}

	pAE->reset( true );
	pAE->unlock();

	auto pDriver = startJackAudioDriver();
	if ( pDriver == nullptr ) {
		throwException( "[testTransportRelocationOffsetsJack] Unable to use JACK driver" );
	}
	float fBpm, fLastBpm;
	bool bTempoChanged = false;
	const int nToggleColumn = 4;
	const int nToggleRow = 4;
	const float fOriginalSongSize = pAE->m_fSongSizeInTicks;

	pAE->lock( RIGHT_HERE );
	fLastBpm = pAE->getBpmAtColumn( 0 );
#ifdef HAVE_INTEGRATION_TESTS
	JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
	pDriver->m_bIntegrationCheckRelocationLoop = true;
#endif
	pAE->unlock();

    std::random_device randomSeed;
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, pAE->m_fSongSizeInTicks );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// Check consistency of updated frames and ticks while relocating
	// transport.
	double fNewTick;
	long long nNewFrame;
	double fTickMismatch;
	const int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {
		if ( ! bTempoChanged && fLastBpm != pAE->getBpmAtColumn( 0 ) ) {
			bTempoChanged = true;
		}

		//  When being listener we have no way to properly check the resulting
		//  tick as our ground truth is just the frame information provided by
		//  the JACK server.
		if ( pHydrogen->getJackTimebaseState() !=
				 JackAudioDriver::Timebase::Listener ) {
			if ( nn < nProcessCycles - 2 ) {
				fNewTick = tickDist( randomEngine );
			}
			else if ( nn < nProcessCycles - 1 ) {
				// Resulted in an unfortunate rounding error due to the
				// song end.
				fNewTick = pSong->lengthInTicks() - 1 + 0.928009209;
			}
			else {
				// There was a rounding error at this particular tick.
				fNewTick = std::fmin( 960, pSong->lengthInTicks() );
			}

			pAE->lock( RIGHT_HERE );

			// Ensure we relocate to a new position. Else the assertions in this
			// test will fail.
			while ( std::abs( fNewTick - pTransportPos->getDoubleTick() ) < 1 ) {
				fNewTick = tickDist( randomEngine );
			}

			INFOLOG( QString( "relocate to tick [%1]->[%2]" )
					 .arg( pTransportPos->getDoubleTick() ).arg( fNewTick ) );
			pAE->locate( fNewTick, true );
			pAE->unlock();

			AudioEngineTests::waitForRelocation( pDriver, fNewTick, -1 );
			// We send the tick value to and receive it from the JACK server via
			// routines in the libjack2 library. We have to be relaxed about the
			// precision we can expect.
			if ( abs( pTransportPos->getDoubleTick() - fNewTick ) > 1e-1 ) {
				throwException( QString( "[testTransportRelocationOffsetsJack::tick] failed to relocate to tick. [%1] != [%2]" )
								.arg( pTransportPos->getDoubleTick() ).arg( fNewTick ) );
			}

#ifdef HAVE_INTEGRATION_TESTS
			// In case there is an issue with the BBT <-> transport position
			// conversion or the m_nTimebaseFrameOffset, the driver will detect
			// multiple relocations (maybe one in each cycle).
			if ( pDriver->m_bIntegrationRelocationLoop ) {
				throwException( "[testTransportRelocationOffsetsJack::frame] relocation loop detected" );
			}
#endif

			AudioEngineTests::checkTransportPosition(
				pTransportPos, "[testTransportRelocationOffsetsJack::tick] mismatch tick-based" );
		}

		// Alter song size
		//
		// It's alright to not lock the AudioEngine when accessing the current
		// song size. It should never change during regular playback (which is
		// covered by a separate test).
		const auto nOldSongSize = pAE->m_fSongSizeInTicks;
		CoreActionController::toggleGridCell(
			GridPoint( nToggleColumn, nToggleRow ) );
		if ( nOldSongSize == pAE->m_fSongSizeInTicks ) {
			throwException( "[testTransportRelocationOffsetsJack] song size did not change." );
		}
		INFOLOG( QString( "[testTransportRelocationOffsetsJack] update song size [%1] -> [%2]" )
				 .arg( nOldSongSize ).arg( pAE->m_fSongSizeInTicks ) );

		// Frame-based relocation
		// We sample ticks and convert them since we are using tempo markers.
		if ( nn < nProcessCycles - 1 ) {
			nNewFrame = TransportPosition::computeFrameFromTick(
				tickDist( randomEngine ), &fTickMismatch );
		} else {
			// With this one there were rounding mishaps in v1.2.3
			nNewFrame = std::min( static_cast<long long>(2174246),
								  TransportPosition::computeFrameFromTick(
									  pSong->lengthInTicks(), &fTickMismatch ) );
		}

		pAE->lock( RIGHT_HERE );

		while ( nNewFrame == pTransportPos->getFrame() ) {
			nNewFrame = TransportPosition::computeFrameFromTick(
				tickDist( randomEngine ), &fTickMismatch );
		}

#ifdef HAVE_INTEGRATION_TESTS
		if ( pHydrogen->getJackTimebaseState() ==
				 JackAudioDriver::Timebase::Listener ) {
			// We are listener
			//
			// Discard the previous offset or we do not end up at the frame we
			// provided to locateTransport and the comparison fails.
			pDriver->m_nTimebaseFrameOffset = 0;
			JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
		}
#endif

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocationOffsetsJack] mismatch after song size update" );

		INFOLOG( QString( "relocate to frame [%1]->[%2]" )
				 .arg( pTransportPos->getFrame() ).arg( nNewFrame ) );
		pDriver->locateTransport( nNewFrame );
		pAE->unlock();

		AudioEngineTests::waitForRelocation( pDriver, -1, nNewFrame );

		long long nCurrentFrame;
		if ( pHydrogen->getJackTimebaseState() ==
			 JackAudioDriver::Timebase::Listener ) {
			nCurrentFrame = pDriver->m_JackTransportPos.frame;
		}
		else {
			nCurrentFrame = pTransportPos->getFrame();
		}

		if ( nNewFrame != nCurrentFrame ) {
			throwException( QString( "[testTransportRelocationOffsetsJack::frame] failed to relocate to frame. timebase state: [%1], nNewFrame [%2] != nCurrentFrame [%3], pPos->getFrame(): [%4]" )
							.arg( JackAudioDriver::TimebaseToQString(
									  pDriver->getTimebaseState() ) )
							.arg( nNewFrame )
							.arg( nCurrentFrame )
							.arg( pTransportPos->getFrame() ) );
		}

#ifdef HAVE_INTEGRATION_TESTS
		// In case there is an issue with the BBT <-> transport position
		// conversion or the m_nTimebaseFrameOffset, the driver will detect
		// multiple relocations (maybe one in each cycle).
		if ( pDriver->m_bIntegrationRelocationLoop ) {
			throwException( "[testTransportRelocationOffsetsJack::frame] relocation loop detected" );
		}
#endif

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocationOffsetsJack::frame] mismatch frame-based" );

		// Alter tempo
		fBpm = tempoDist( randomEngine );
		pAE->lock( RIGHT_HERE );
		INFOLOG( QString( "[testTransportRelocationOffsetsJack] changing tempo [%1]->[%2]" )
				.arg( pAE->getBpmAtColumn( 0 ) ).arg( fBpm ) );
		pAE->setNextBpm( fBpm );
		pAE->unlock();

		// Give Hydrogen some time to apply the tempo changes. Else, we might
		// run into race conditions with relocation being triggered before tempo
		// was applied. In such a case we loose the former offsets and won't be
		// able to properly check for matching position.
		QTest::qSleep( 25 );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportRelocationOffsetsJack::tempo] mismatch after tempo change" );
	}

	pAE->lock( RIGHT_HERE );
#ifdef HAVE_INTEGRATION_TESTS
	pDriver->m_bIntegrationCheckRelocationLoop = false;
	JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
#endif
	pAE->reset( true );
	pAE->unlock();

	if ( ! bTempoChanged ) {
		throwException( "[testTransportRelocationOffsetsJack] tempo was not change." );
	}

	// Ensure the additional grid cell we activate/deactivate is set to its
	// original state.
	if ( pAE->m_fSongSizeInTicks != fOriginalSongSize ) {
		CoreActionController::toggleGridCell(
			GridPoint( nToggleColumn, nToggleRow ) );
	}

	stopJackAudioDriver();
}

JackAudioDriver* AudioEngineTests::startJackAudioDriver() {
	INFOLOG( "Starting custom JACK audio driver..." );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const auto pPref = Preferences::get_instance();

	if ( pAudioEngine->getState() == AudioEngine::State::Testing ) {
		throwException( "[startJackAudioDriver] Engine must not be locked and in state testing yet!" );
	}

	pAudioEngine->stopAudioDriver( Event::Trigger::Default );

	// Start a modified version of the JACK audio driver.
	auto pDriver = new JackAudioDriver( jackTestProcessCallback );
	if ( pDriver == nullptr ) {
		throwException( "[startJackAudioDriver] Unable to create JackAudioDriver" );
	}
#ifdef H2CORE_HAVE_JACK

	// Suppress default audio output
	pDriver->setConnectDefaults( false );
#else
	throwException( "[startJackAudioDriver] This function should not be run without JACK support" );
#endif
	pAudioEngine->lock( RIGHT_HERE );

	if ( pDriver->init( pPref->m_nBufferSize ) != 0 ) {
		delete pDriver;
		pAudioEngine->unlock();
		throwException( "[startJackAudioDriver] Unable to initialize driver" );
	}

	// Driver needs to be initialized in order to properly set its timebase
	// state.
	if ( pDriver->m_timebaseState == JackAudioDriver::Timebase::Controller &&
		 m_referenceTimebase != JackAudioDriver::Timebase::Controller ) {
		INFOLOG( "Releasing test binary as Timebase controller" );
		pDriver->releaseTimebaseControl();
	}
	else if ( pDriver->m_timebaseState != JackAudioDriver::Timebase::Controller &&
		 m_referenceTimebase == JackAudioDriver::Timebase::Controller ) {
		INFOLOG( "Register test binary as Timebase controller" );
		pDriver->initTimebaseControl();
	}
	pDriver->m_timebaseState = m_referenceTimebase;
	pDriver->m_timebaseTracking = JackAudioDriver::TimebaseTracking::Valid;
	pAudioEngine->m_MutexOutputPointer.lock();

	pAudioEngine->m_pAudioDriver = pDriver;
	pAudioEngine->setState( AudioEngine::State::Ready );

	pAudioEngine->m_MutexOutputPointer.unlock();
	pAudioEngine->unlock();

	if ( pDriver->connect() != 0 ) {
		pHydrogen->restartAudioDriver();
		throwException( "[startJackAudioDriver] Unable to connect driver" );
	}

	if ( pHydrogen->getSong() != nullptr ) {
		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->handleDriverChange();
		pAudioEngine->unlock();
	}

	EventQueue::get_instance()->pushEvent( Event::Type::AudioDriverChanged, 0 );

	INFOLOG( "DONE Starting custom JACK audio driver." );

	return pDriver;
}

void AudioEngineTests::stopJackAudioDriver() {
	INFOLOG( "Stopping custom JACK audio driver..." );

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pAudioEngine->getState() == AudioEngine::State::Testing ) {
		throwException( "[stopJackAudioDriver] Engine must not be locked and in state testing yet!" );
	}

	// We rely on the driver set via the Preferences (most probably FakeAudioDriver).
	pHydrogen->restartAudioDriver();

#ifdef H2CORE_HAVE_JACK
	auto pDriver = dynamic_cast<JackAudioDriver*>(pAudioEngine->m_pAudioDriver);
	if ( pDriver == nullptr ) {
		AudioEngineTests::throwException(
			"[stopJackAudioDriver] No JACK driver after restart!" );
	}

	pDriver->m_timebaseState = m_referenceTimebase;
	pDriver->m_timebaseTracking = JackAudioDriver::TimebaseTracking::Valid;
#endif

	INFOLOG( "DONE Stopping custom JACK audio driver." );
}

void AudioEngineTests::waitForRelocation( JackAudioDriver* pDriver,
										  double fTick, long long nFrame ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	const int nMaxMilliSeconds = 5000;
	const int nSecondTryMilliSeconds = 1000;
	int nMilliSeconds = 0;
	const int nIncrement = 100;

	// We send the tick value to and received it back from the JACK server
	// via routines of the libjack2 library. We have to be relaxed about the
	// precision we can expect from relocating.
	while ( true ) {
		long long nCurrentFrame;
		if ( pHydrogen->getJackTimebaseState() ==
			 JackAudioDriver::Timebase::Listener ) {
			nCurrentFrame = pDriver->m_JackTransportPos.frame;
		} else {
			nCurrentFrame = pTransportPos->getFrame();
		}

		if ( ( nFrame != -1 && nFrame == nCurrentFrame ) ||
			 ( fTick != -1 &&
			   abs( pTransportPos->getDoubleTick() - fTick ) < 1e-1 ) ) {
			return;
		}

		if ( nMilliSeconds >= nMaxMilliSeconds ) {
			QString sTarget;
			if ( nFrame != -1 ) {
				sTarget = QString( "frame [%1]" ).arg( nFrame );
			} else {
				sTarget = QString( "tick [%1]" ).arg( fTick );
			}
			AudioEngineTests::throwException(
				QString( "[AudioEngineTests::waitForRelocation] playback takes too long to reach %1" )
					.arg( sTarget ) );
		} else if ( nMilliSeconds == nSecondTryMilliSeconds ) {
			WARNINGLOG( QString( "[AudioEngineTests::waitForRelocation] Performing seconds attempt after [%1]ms" )
					.arg( nMilliSeconds ) );

			// Occassionally the JACK server seems to drop our relocation
			// attempt silently. This is not good but acceptable since we
			// are firing them in rapid succession. That's not the usual
			// use-case. In order to not make this behavior break our test,
			// we do a second attempt to be sure.
			if ( fTick != -1 ) {
				pAE->lock( RIGHT_HERE );
				pAE->locate( fTick, true );
				pAE->unlock();
			}
			else {
				pAE->lock( RIGHT_HERE );

#ifdef HAVE_INTEGRATION_TESTS
				if ( pHydrogen->getJackTimebaseState() ==
					 JackAudioDriver::Timebase::Listener ) {
					// We are listener
					//
					// Discard the previous offset or we do not end up at
					// the frame we provided to locateTransport and the
					// comparison fails.
					pDriver->m_nTimebaseFrameOffset = 0;
					JackAudioDriver::m_nIntegrationLastRelocationFrame = -1;
				}
#endif

				pDriver->locateTransport( nFrame );
				pAE->unlock();
			}
		}

		QTest::qSleep( nIncrement );
		nMilliSeconds += nIncrement;
	}
}

int AudioEngineTests::jackTestProcessCallback( uint32_t nframes, void* args ) {

	AudioEngine* pAudioEngine = Hydrogen::get_instance()->getAudioEngine();
	auto pDriver = dynamic_cast<JackAudioDriver*>(pAudioEngine->m_pAudioDriver);
	if ( pDriver == nullptr ) {
		AudioEngineTests::throwException(
			"[jackTestProcessCallback] No JACK driver!" );
	}

	// For the JACK driver it is very important (#1867) to not do anything while
	// the JACK client is stopped/closed. Otherwise it will segfault on mutex
	// locking or message logging.
	if ( ! ( pAudioEngine->getState() == AudioEngine::State::Ready ||
			 pAudioEngine->getState() == AudioEngine::State::Playing ) ) {
		return 0;
	}
	const auto sDrivers = pAudioEngine->getDriverNames();

	if ( pAudioEngine->getState() == AudioEngine::State::Testing ) {
		AudioEngineTests::throwException(
			QString( "[jackTestProcessCallback] [%1] engine must not be in state Testing" )
			.arg( sDrivers ) );
	}

	pAudioEngine->clearAudioBuffers( nframes );

	// Calculate maximum time to wait for audio engine lock. Using the
	// last calculated processing time as an estimate of the expected
	// processing time for this frame.
	float sampleRate = static_cast<float>(pDriver->getSampleRate());
	pAudioEngine->m_fMaxProcessTime = 1000.0 / ( sampleRate / nframes );
	float fSlackTime = pAudioEngine->m_fMaxProcessTime - pAudioEngine->m_fProcessTime;

	// If we expect to take longer than the available time to process,
	// require immediate locking or not at all: we're bound to drop a
	// buffer anyway.
	if ( fSlackTime < 0.0 ) {
		fSlackTime = 0.0;
	}

	/*
	 * The "try_lock" was introduced for Bug #164 (Deadlock after during
	 * alsa driver shutdown). The try_lock *should* only fail in rare circumstances
	 * (like shutting down drivers). In such cases, it seems to be ok to interrupt
	 * audio processing. Returning the special return value "2" enables the disk
	 * writer driver to repeat the processing of the current data.
	 */
	if ( !pAudioEngine->tryLockFor( std::chrono::microseconds( (int)(1000.0*fSlackTime) ),
							  RIGHT_HERE ) ) {
		___ERRORLOG( QString( "[%1] Failed to lock audioEngine in allowed %2 ms, missed buffer" )
					 .arg( sDrivers ).arg( fSlackTime ) );
		return 0;
	}

	// Now that the engine is locked we properly check its state.
	if ( ! ( pAudioEngine->getState() == AudioEngine::State::Ready ||
			 pAudioEngine->getState() == AudioEngine::State::Playing ) ) {
		pAudioEngine->unlock();
		return 0;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		AudioEngineTests::throwException(
			QString( "[jackTestProcessCallback] [%1] no song set yet" )
			.arg( sDrivers ) );
	}

	AudioEngineTests::checkTransportPosition(
			pAudioEngine->getTransportPosition(),
			QString( "[jackTestProcessCallback] [%1] : pre updated" )
			.arg( sDrivers ) );

	// Sync transport with server (in case the current audio driver is
	// designed that way)
#ifdef H2CORE_HAVE_JACK
	if ( Hydrogen::get_instance()->hasJackTransport() ) {
		pDriver->updateTransportPosition();

#ifdef HAVE_INTEGRATION_TESTS
		if ( pDriver->m_bIntegrationRelocationLoop ) {
			AudioEngineTests::throwException(
				"[jackTestProcessCallback] Relocation loop detected!" );
		}
#endif

		// Check consistency of current JACK position.
		const auto jackPos = pDriver->getJackPosition();
		if ( ( jackPos.valid & JackPositionBBT ) &&
			 ! JackAudioDriver::isBBTValid( jackPos ) ) {
			AudioEngineTests::throwException(
				"[jackTestProcessCallback] Inconsistent JACK BBT information" );
		}

		// Check consistency of BBT conversion functions
		const auto pTransportPos = pAudioEngine->getTransportPosition();
		jack_position_t testPos;
		if ( pTransportPos->getDoubleTick() >=
			 pAudioEngine->m_fSongSizeInTicks ) {
			testPos.frame = 0;
			testPos.tick = 0;
		} else {
			testPos.frame = pTransportPos->getFrame();
			testPos.tick = pTransportPos->getDoubleTick();
		}
		JackAudioDriver::transportToBBT( *pTransportPos, &testPos );

		if ( ! JackAudioDriver::isBBTValid( testPos ) ) {
			AudioEngineTests::throwException( QString(
				"[jackTestProcessCallback::transportToBBT] Invalid JACK position: %1,\ntransport pos: %2" )
				.arg( JackAudioDriver::JackTransportPosToQString( testPos ) )
				.arg( pTransportPos->toQString() ) );
		}

		const auto fTick = JackAudioDriver::bbtToTick( testPos );
		if ( pTransportPos->getDoubleTick() <
			 pAudioEngine->m_fSongSizeInTicks &&
			 std::abs( fTick - pTransportPos->getTick() ) > 1e-5 ) {
			AudioEngineTests::throwException( QString(
				"[jackTestProcessCallback] Mismatching ticks after BBT conversion: diff: %1, fTick: %2, pAE->m_fSongSizeInTicks: %3\nJACK pos: %4\nTransport pos: %5" )
				.arg( std::abs( fTick - pTransportPos->getTick() ), 0, 'f' )
				.arg( fTick ).arg( pAudioEngine->m_fSongSizeInTicks )
				.arg( JackAudioDriver::JackTransportPosToQString( testPos ) )
				.arg( pTransportPos->toQString() ) );
		}
		else if ( pTransportPos->getDoubleTick() >=
			 pAudioEngine->m_fSongSizeInTicks && fTick != 0 ) {
			AudioEngineTests::throwException( QString(
				"[jackTestProcessCallback] Mismatching ticks after BBT conversion at end of song: fTick: %1, pAE->m_fSongSizeInTicks: %2\nJACK pos: %3\nTransport pos: %4" )
				.arg( fTick ).arg( pAudioEngine->m_fSongSizeInTicks )
				.arg( JackAudioDriver::JackTransportPosToQString( testPos ) )
				.arg( pTransportPos->toQString() ) );
		}
	}
	else {
		AudioEngineTests::throwException(
			QString( "[jackTestProcessCallback] [%1] callback should only be used with JACK driver!" )
			.arg( sDrivers ) );
	}
#endif
	// Check whether the Timebase state is still the same.
	if ( pDriver->getTimebaseState() != AudioEngineTests::m_referenceTimebase ) {
		AudioEngineTests::throwException(
			QString( "[jackTestProcessCallback] Timebase state changed. Current: [%1], reference: [%2]" )
			.arg( JackAudioDriver::TimebaseToQString( pDriver->getTimebaseState() ) )
			.arg( JackAudioDriver::TimebaseToQString(
					  AudioEngineTests::m_referenceTimebase ) ) );
	}

	// Check whether the tempo was changed.
	pAudioEngine->updateBpmAndTickSize( pAudioEngine->m_pTransportPosition );
	pAudioEngine->updateBpmAndTickSize( pAudioEngine->m_pQueuingPosition );

	AudioEngineTests::checkTransportPosition(
			pAudioEngine->getTransportPosition(),
			QString( "[jackTestProcessCallback] [%1] : post JACK" )
			.arg( sDrivers ) );

	// Update the state of the audio engine depending on whether it
	// was started or stopped by the user.
	if ( pAudioEngine->m_nextState == AudioEngine::State::Playing ) {
		if ( pAudioEngine->getState() == AudioEngine::State::Ready ) {
			pAudioEngine->startPlayback();
		}

		pAudioEngine->setRealtimeFrame( pAudioEngine->m_pTransportPosition->getFrame() );
	} else {
		if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {
			pAudioEngine->stopPlayback();
		}

		// go ahead and increment the realtimeframes by nFrames
		// to support our realtime keyboard and midi event timing
		pAudioEngine->setRealtimeFrame( pAudioEngine->getRealtimeFrame() +
										 static_cast<long long>(nframes) );
	}

	// always update note queue.. could come from pattern or realtime input
	// (midi, keyboard)
	pAudioEngine->updateNoteQueue( nframes );

	pAudioEngine->processAudio( nframes );

	if ( pAudioEngine->getState() == AudioEngine::State::Playing ) {

		// Check whether the end of the song has been reached.
		if ( pAudioEngine->isEndOfSongReached(
				 pAudioEngine->m_pTransportPosition ) ) {

			___INFOLOG( QString( "[%1] End of song received" ).arg( sDrivers ) );

			CoreActionController::sendAllNoteOffMessages();

			pAudioEngine->stop();
			pAudioEngine->stopPlayback();
			pAudioEngine->locate( 0 );
		}
		else {
			// We are not at the end of the song, keep rolling.
			pAudioEngine->incrementTransportPosition( nframes );
		}
	}

	AudioEngineTests::checkTransportPosition(
			pAudioEngine->getTransportPosition(),
			QString( "[jackTestProcessCallback] [%1] : post update" )
			.arg( sDrivers ) );

	pAudioEngine->unlock();

	return 0;
}
#endif // H2CORE_HAVE_JACK

void AudioEngineTests::throwException( const QString& sMsg ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAE = pHydrogen->getAudioEngine();

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();

	const auto sMsgLocal8Bit = sMsg.toLocal8Bit();
	throw std::runtime_error( sMsgLocal8Bit.data() );
}

	
}; // namespace H2Core
