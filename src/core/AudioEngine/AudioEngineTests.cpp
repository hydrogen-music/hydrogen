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
#include <random>
#include <stdexcept>

#include <core/AudioEngine/AudioEngineTests.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>

#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Sampler/Sampler.h>
#include <core/Hydrogen.h>
#include <core/CoreActionController.h>
#include <core/Preferences/Preferences.h>
#include <core/config.h>

namespace H2Core
{

void AudioEngineTests::testFrameToTickConversion() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->addTempoMarker( 0, 120 );
	pCoreActionController->addTempoMarker( 3, 100 );
	pCoreActionController->addTempoMarker( 5, 40 );
	pCoreActionController->addTempoMarker( 7, 200 );

	double fFrameMismatch1, fFrameMismatch2, fFrameMismatch3,
		fFrameMismatch4, fFrameMismatch5, fFrameMismatch6;
	
	long long nFrame1 = 342732;
	long long nFrame2 = 1037223;
	long long nFrame3 = 453610333722;
	double fTick1 = TransportPosition::computeTickFromFrame(
		  nFrame1 );
	long long nFrame1Computed =
		TransportPosition::computeFrameFromTick( fTick1, &fFrameMismatch1 );
	double fTick2 = TransportPosition::computeTickFromFrame( nFrame2 );
	long long nFrame2Computed =
		TransportPosition::computeFrameFromTick( fTick2, &fFrameMismatch2 );
	double fTick3 = TransportPosition::computeTickFromFrame( nFrame3 );
	long long nFrame3Computed =
		TransportPosition::computeFrameFromTick( fTick3, &fFrameMismatch3 );
	
	if ( nFrame1Computed != nFrame1 || std::abs( fFrameMismatch1 ) > 1e-10 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [1] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameMismatch: %4, frame diff: %5" )
			.arg( nFrame1 ).arg( fTick1, 0, 'f' ).arg( nFrame1Computed )
			.arg( fFrameMismatch1, 0, 'E', -1 )
			.arg( nFrame1Computed - nFrame1 ) );
	}
	if ( nFrame2Computed != nFrame2 || std::abs( fFrameMismatch2 ) > 1e-10 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [2] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameMismatch: %4, frame diff: %5" )
			.arg( nFrame2 ).arg( fTick2, 0, 'f' ).arg( nFrame2Computed )
			.arg( fFrameMismatch2, 0, 'E', -1 )
			.arg( nFrame2Computed - nFrame2 ) );
	}
	if ( nFrame3Computed != nFrame3 || std::abs( fFrameMismatch3 ) > 1e-6 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [3] nFrame: %1, fTick: %2, nFrameComputed: %3, fFrameMismatch: %4, frame diff: %5" )
			.arg( nFrame3 ).arg( fTick3, 0, 'f' ).arg( nFrame3Computed )
			.arg( fFrameMismatch3, 0, 'E', -1 )
			.arg( nFrame3Computed - nFrame3 ) );
	}

	double fTick4 = 552;
	double fTick5 = 1939;
	double fTick6 = 534623409;
	long long nFrame4 =
		TransportPosition::computeFrameFromTick( fTick4, &fFrameMismatch4 );
	double fTick4Computed =
		TransportPosition::computeTickFromFrame( nFrame4 ) + fFrameMismatch4;
	long long nFrame5 =
		TransportPosition::computeFrameFromTick( fTick5, &fFrameMismatch5 );
	double fTick5Computed =
		TransportPosition::computeTickFromFrame( nFrame5 ) + fFrameMismatch5;
	long long nFrame6 =
		TransportPosition::computeFrameFromTick( fTick6, &fFrameMismatch6 );
	double fTick6Computed =
		TransportPosition::computeTickFromFrame( nFrame6 ) + fFrameMismatch6;
	
	
	if ( abs( fTick4Computed - fTick4 ) > 1e-9 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [4] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameMismatch: %4, tick diff: %5" )
			.arg( nFrame4 ).arg( fTick4, 0, 'f' ).arg( fTick4Computed, 0, 'f' )
			.arg( fFrameMismatch4, 0, 'E' )
			.arg( fTick4Computed - fTick4 ) );
	}

	if ( abs( fTick5Computed - fTick5 ) > 1e-9 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [5] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameMismatch: %4, tick diff: %5" )
			.arg( nFrame5 ).arg( fTick5, 0, 'f' ).arg( fTick5Computed, 0, 'f' )
			.arg( fFrameMismatch5, 0, 'E' )
			.arg( fTick5Computed - fTick5 ) );
	}

	if ( abs( fTick6Computed - fTick6 ) > 1e-6 ) {
		AudioEngineTests::throwException(
			QString( "[testFrameToTickConversion] [6] nFrame: %1, fTick: %2, fTickComputed: %3, fFrameMismatch: %4, tick diff: %5" )
			.arg( nFrame6 ).arg( fTick6, 0, 'f' ).arg( fTick6Computed, 0, 'f' )
			.arg( fFrameMismatch6, 0, 'E' )
			.arg( fTick6Computed - fTick6 ) );
	}
}

void AudioEngineTests::testTransportProcessing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	auto pPlayheadPos = pAE->getPlayheadPosition();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true );

	pAE->lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame;
	long long nLastTransportFrame = 0;
	long nLastPlayheadTick = 0;
	long long nTotalFrames = 0;

	// Consistency of the playhead update.
	double fLastTickIntervalEnd = 0;
	long long nLastLookahead = 0;

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );
	int nn = 0;

	const auto testTransport = [&]( const QString& sContext,
							 bool bRelaxLastFrames = true ) {
		nFrames = frameDist( randomEngine );

		double fTickStart, fTickEnd;
		bool bOldLookaheadApplied = pAE->m_bLookaheadApplied;
		const long long nLeadLag =
			pAE->computeTickInterval( &fTickStart, &fTickEnd, nFrames );
		pAE->m_bLookaheadApplied = bOldLookaheadApplied;
		
		// If this is the first call after a tempo change, the last
		// lookahead will be set to 0
		if ( nLastLookahead != 0 &&
			 nLastLookahead != nLeadLag + AudioEngine::nMaxTimeHumanize + 1 ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing : lookahead] [%1] with one and the same BPM/tick size the lookahead must be consistent! [ %2 -> %3 ]" )
				.arg( sContext )
				.arg( nLastLookahead )
				.arg( nLeadLag + AudioEngine::nMaxTimeHumanize + 1 ) );
		}
		nLastLookahead = nLeadLag + AudioEngine::nMaxTimeHumanize + 1;

		pAE->updateNoteQueue( nFrames );
		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportProcessing] " + sContext );

		AudioEngineTests::checkTransportPosition(
			pPlayheadPos, "[testTransportProcessing] " + sContext );

		if ( ( ! bRelaxLastFrames &&
			   ( pTransportPos->getFrame() - nFrames != nLastTransportFrame ) ) ||
			 // errors in the rescaling of nLastTransportFrame are omitted.
			 ( bRelaxLastFrames &&
			   abs( ( pTransportPos->getFrame() - nFrames - nLastTransportFrame ) /
					pTransportPos->getFrame() ) > 1e-8 ) ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing : transport] [%1] inconsistent frame update. pTransportPos->getFrame(): %2, nFrames: %3, nLastTransportFrame: %4, bRelaxLastFrame: %5" )
				.arg( sContext )
				.arg( pTransportPos->getFrame() ).arg( nFrames )
				.arg( nLastTransportFrame ).arg( bRelaxLastFrames ) );
		}
		nLastTransportFrame = pTransportPos->getFrame();

		const int nNoteQueueUpdate =
			static_cast<int>(std::floor( fTickEnd ) - std::floor( fTickStart ));
		// We will only compare the playhead position in case interval
		// in updateNoteQueue covers at least one tick and, thus,
		// an update has actually taken place.
		if ( nLastPlayheadTick > 0 && nNoteQueueUpdate > 0 ) {
			if ( pPlayheadPos->getTick() - nNoteQueueUpdate !=
				 nLastPlayheadTick ) {
				AudioEngineTests::throwException(
					QString( "[testTransportProcessing : playhead] [%1] inconsistent tick update. pPlayheadPos->getTick(): %2, nNoteQueueUpdate: %3, nLastPlayheadTick: %4" )
					.arg( sContext )
					.arg( pPlayheadPos->getTick() )
					.arg( nNoteQueueUpdate )
					.arg( nLastPlayheadTick ) );
			}
		}
		nLastPlayheadTick = pPlayheadPos->getTick();

		// Check whether the tick interval covered in updateNoteQueue
		// is consistent and does not include holes or overlaps.
		// In combination with testNoteEnqueuing this should
		// guarantuee that all note will be queued properly.
		if ( std::abs( fTickStart - fLastTickIntervalEnd ) > 1E-4 ||
			 fTickStart >= fTickEnd ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing : tick interval] [%1] inconsistent update. old: [ ... : %2 ], new: [ %3, %4 ], pTransportPos->getTickOffsetTempo(): %5, diff: %6" )
				.arg( sContext )
				.arg( fLastTickIntervalEnd )
				.arg( fTickStart )
				.arg( fTickEnd )
				.arg( pTransportPos->getTickOffsetTempo() )
				.arg( std::abs( fTickStart - fLastTickIntervalEnd ), 0, 'E', -1 ) );
		}
		fLastTickIntervalEnd = fTickEnd;

		// Using the offset Hydrogen can keep track of the actual
		// number of frames passed since the playback was started
		// even in case a tempo change was issued by the user.
		nTotalFrames += nFrames;
		if ( pTransportPos->getFrame() - pTransportPos->getFrameOffsetTempo() !=
			 nTotalFrames ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessing : transport] [%1] frame offset incorrect. pTransportPos->getFrame(): %2, pTransportPos->getFrameOffsetTempo(): %3, nTotalFrames: %4" )
				.arg( sContext )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getFrameOffsetTempo() )
				.arg( nTotalFrames ) );
		}
	};

	// Check that the playhead position is monotonously increasing
	// (and there are no glitches).
	int nPlayheadColumn = 0;
	long nPlayheadPatternTickPosition = 0;

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {
		testTransport( QString( "[song mode : constant tempo]" ), false );

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
	nLastTransportFrame = 0;
	nLastPlayheadTick = 0;
	fLastTickIntervalEnd = 0;

	float fBpm;
	float fLastBpm = pTransportPos->getBpm();
	int nCyclesPerTempo = 11;

	nTotalFrames = 0;

	nn = 0;

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {

		fBpm = tempoDist( randomEngine );
		pAE->setNextBpm( fBpm );
		pAE->updateBpmAndTickSize( pTransportPos );
		pAE->updateBpmAndTickSize( pPlayheadPos );

		nLastTransportFrame = pTransportPos->getFrame();
		nLastPlayheadTick = pPlayheadPos->getTick();
		nLastLookahead = 0;
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			testTransport( QString( "[song mode : variable tempo %1->%2]" )
						   .arg( fLastBpm ).arg( fBpm ),
						   cc == 0 );
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
	pCoreActionController->activateSongMode( false );

	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	nLastTransportFrame = 0;
	nLastPlayheadTick = 0;
	fLastBpm = 0;
	nTotalFrames = 0;
	fLastTickIntervalEnd = 0;

	int nDifferentTempos = 10;

	for ( int tt = 0; tt < nDifferentTempos; ++tt ) {

		fBpm = tempoDist( randomEngine );
		
		pAE->setNextBpm( fBpm );
		pAE->updateBpmAndTickSize( pTransportPos );
		pAE->updateBpmAndTickSize( pPlayheadPos );

		nLastTransportFrame = pTransportPos->getFrame();
		nLastPlayheadTick = pPlayheadPos->getTick();
		nLastLookahead = 0;

		fLastBpm = fBpm;

		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			testTransport( QString( "[pattern mode : variable tempo %1->%2]" )
						   .arg( fLastBpm ).arg( fBpm ),
						   cc == 0 );
		}
	}

	pAE->reset( false );

	pAE->setState( AudioEngine::State::Ready );

	pAE->unlock();
	pCoreActionController->activateSongMode( true );
}

void AudioEngineTests::testTransportProcessingTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pTimeline = pHydrogen->getTimeline();
	auto pPref = Preferences::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	auto pPlayheadPos = pAE->getPlayheadPosition();
	
	pCoreActionController->activateTimeline( true );
	pCoreActionController->activateLoopMode( true );

	pAE->lock( RIGHT_HERE );

	// Activating the Timeline without requiring the AudioEngine to be locked.
	auto activateTimeline = [&]( bool bEnabled ) {
		pPref->setUseTimelineBpm( bEnabled );
		pSong->setIsTimelineActivated( bEnabled );

		if ( bEnabled ) {
			pTimeline->activate();
		} else {
			pTimeline->deactivate();
		}

		pAE->handleTimelineChange();
	};

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_real_distribution<float> tempoDist( MIN_BPM, MAX_BPM );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame;
	long long nLastTransportFrame = 0;
	long nLastPlayheadTick = 0;
	long long nTotalFrames = 0;

	// Consistency of the playhead update.
	double fLastTickIntervalEnd = 0;
	long long nLastLookahead = 0;

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );
	int nn = 0;

	const auto testTransport = [&]( const QString& sContext,
							 bool bRelaxLastFrames = true ) {
		nFrames = frameDist( randomEngine );

		double fTickStart, fTickEnd;
		bool bOldLookaheadApplied = pAE->m_bLookaheadApplied;
		const long long nLeadLag =
			pAE->computeTickInterval( &fTickStart, &fTickEnd, nFrames );
		pAE->m_bLookaheadApplied = bOldLookaheadApplied;
		// No lookahead check in here since tempo does change
		// automatically when passing a tempo marker -> lookahead will
		// change as well.
		pAE->updateNoteQueue( nFrames );
		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testTransportProcessingTimeline] " + sContext );

		AudioEngineTests::checkTransportPosition(
			pPlayheadPos, "[testTransportProcessingTimeline] " + sContext );

		if ( ( ! bRelaxLastFrames &&
			   ( pTransportPos->getFrame() - nFrames -
				 pTransportPos->getFrameOffsetTempo() != nLastTransportFrame ) ) ||
			 // errors in the rescaling of nLastTransportFrame are omitted.
			 ( bRelaxLastFrames &&
			   abs( ( pTransportPos->getFrame() - nFrames -
					  pTransportPos->getFrameOffsetTempo() - nLastTransportFrame ) /
					pTransportPos->getFrame() ) > 1e-8 ) ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingTimeline : transport] [%1] inconsistent frame update. pTransportPos->getFrame(): %2, nFrames: %3, nLastTransportFrame: %4, pTransportPos->getFrameOffsetTempo(): %5, bRelaxLastFrame: %6" )
				.arg( sContext )
				.arg( pTransportPos->getFrame() )
				.arg( nFrames )
				.arg( nLastTransportFrame )
				.arg( pTransportPos->getFrameOffsetTempo() )
				.arg( bRelaxLastFrames ) );
		}
		nLastTransportFrame = pTransportPos->getFrame() -
			pTransportPos->getFrameOffsetTempo();

		const int nNoteQueueUpdate =
			static_cast<int>(std::floor( fTickEnd ) - std::floor( fTickStart ));
		// We will only compare the playhead position in case interval
		// in updateNoteQueue covers at least one tick and, thus,
		// an update has actually taken place.
		if ( nLastPlayheadTick > 0 && nNoteQueueUpdate > 0 ) {
			if ( pPlayheadPos->getTick() - nNoteQueueUpdate !=
				 nLastPlayheadTick ) {
				AudioEngineTests::throwException(
					QString( "[testTransportProcessingTimeline : playhead] [%1] inconsistent tick update. pPlayheadPos->getTick(): %2, nNoteQueueUpdate: %3, nLastPlayheadTick: %4" )
					.arg( sContext )
					.arg( pPlayheadPos->getTick() )
					.arg( nNoteQueueUpdate )
					.arg( nLastPlayheadTick ) );
			}
		}
		nLastPlayheadTick = pPlayheadPos->getTick();

		// Check whether the tick interval covered in updateNoteQueue
		// is consistent and does not include holes or overlaps.
		// In combination with testNoteEnqueuing this should
		// guarantuee that all note will be queued properly.
		if ( std::abs( fTickStart - fLastTickIntervalEnd ) > 1E-4 ||
			 fTickStart >= fTickEnd ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingTimeline : tick interval] [%1] inconsistent update. old: [ ... : %2 ], new: [ %3, %4 ], pTransportPos->getTickOffsetTempo(): %5, diff: %6" )
				.arg( sContext )
				.arg( fLastTickIntervalEnd )
				.arg( fTickStart )
				.arg( fTickEnd )
				.arg( pTransportPos->getTickOffsetTempo() )
				.arg( std::abs( fTickStart - fLastTickIntervalEnd ), 0, 'E', -1 ) );
		}
		fLastTickIntervalEnd = fTickEnd;

		// Using the offset Hydrogen can keep track of the actual
		// number of frames passed since the playback was started
		// even in case a tempo change was issued by the user.
		nTotalFrames += nFrames;
		if ( pTransportPos->getFrame() - pTransportPos->getFrameOffsetTempo() !=
			 nTotalFrames ) {
			AudioEngineTests::throwException(
				QString( "[testTransportProcessingTimeline : transport] [%1] frame offset incorrect. pTransportPos->getFrame(): %2, pTransportPos->getFrameOffsetTempo(): %3, nTotalFrames: %4" )
				.arg( sContext )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getFrameOffsetTempo() )
				.arg( nTotalFrames ) );
		}
	};

	// Check that the playhead position is monotonously increasing
	// (and there are no glitches).
	int nPlayheadColumn = 0;
	long nPlayheadPatternTickPosition = 0;

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {
		testTransport( QString( "[song mode : all timeline]" ), false );

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
	nLastTransportFrame = 0;
	nLastPlayheadTick = 0;
	fLastTickIntervalEnd = 0;

	float fBpm;
	float fLastBpm = pTransportPos->getBpm();
	int nCyclesPerTempo = 11;

	nTotalFrames = 0;

	nn = 0;

	while ( pTransportPos->getDoubleTick() <
			pAE->getSongSizeInTicks() ) {

		QString sContext;
		if ( nn % 2 == 0 ){
			activateTimeline( false );
			fBpm = tempoDist( randomEngine );
			pAE->setNextBpm( fBpm );
			pAE->updateBpmAndTickSize( pTransportPos );
			pAE->updateBpmAndTickSize( pPlayheadPos );

			sContext = "no timeline";
		}
		else {
			activateTimeline( true );
			fBpm = AudioEngine::getBpmAtColumn( pTransportPos->getColumn() );
			
			sContext = "timeline";
		}
		
		nLastLookahead = 0;
		
		for ( int cc = 0; cc < nCyclesPerTempo; ++cc ) {
			testTransport( QString( "[alternating timeline : bpm %1->%2 : %3]" )
						   .arg( fLastBpm ).arg( fBpm ).arg( sContext ),
						   cc == 0 );
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

void AudioEngineTests::testTransportRelocation() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();

	pAE->lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> tickDist( 0, pAE->m_fSongSizeInTicks );
	std::uniform_int_distribution<long long> frameDist( 0, pPref->m_nBufferSize );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while relocating
	// transport.
	double fNewTick;
	long long nNewFrame;

	int nProcessCycles = 100;
	for ( int nn = 0; nn < nProcessCycles; ++nn ) {

		if ( nn < nProcessCycles - 2 ) {
			fNewTick = tickDist( randomEngine );
		}
		else if ( nn < nProcessCycles - 1 ) {
			// Results in an unfortunate rounding error due to the
			// song end at 2112. 
			fNewTick = 2111.928009209;
		}
		else {
			// There was a rounding error at this particular tick.
			fNewTick = 960;
			
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
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();

	pAE->lock( RIGHT_HERE );
	pAE->reset( false );
	pAE->setState( AudioEngine::State::Ready );

	pAE->unlock();
	pCoreActionController->locateToColumn( 4 );
	pAE->lock( RIGHT_HERE );
	pAE->setState( AudioEngine::State::Testing );

	AudioEngineTests::toggleAndCheckConsistency( 1, 1, "[testSongSizeChange] prior" );
		
	// Toggle a grid cell after to the current transport position
	AudioEngineTests::toggleAndCheckConsistency( 6, 6, "[testSongSizeChange] after" );

	// Now we head to the "same" position inside the song but with the
	// transport being looped once.
	int nTestColumn = 4;
	long nNextTick = pHydrogen->getTickForColumn( nTestColumn );
	if ( nNextTick == -1 ) {
		AudioEngineTests::throwException(
			QString( "[testSongSizeChange] Bad test design: there is no column [%1]" )
			.arg( nTestColumn ) );
	}

	nNextTick += pSong->lengthInTicks();
	
	pAE->unlock();
	pCoreActionController->activateLoopMode( true );
	pCoreActionController->locateToTick( nNextTick );
	pAE->lock( RIGHT_HERE );
	
	AudioEngineTests::toggleAndCheckConsistency( 1, 1, "[testSongSizeChange] looped:prior" );
		
	// Toggle a grid cell after to the current transport position
	AudioEngineTests::toggleAndCheckConsistency( 13, 6, "[testSongSizeChange] looped:after" );

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	pCoreActionController->activateLoopMode( false );
}

void AudioEngineTests::testSongSizeChangeInLoopMode() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pTransportPos = pAE->getTransportPosition();
	
	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( true );

	pAE->lock( RIGHT_HERE );

	int nColumns = pHydrogen->getSong()->getPatternGroupVector()->size();

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_real_distribution<double> frameDist( 1, pPref->m_nBufferSize );
	std::uniform_int_distribution<int> columnDist( nColumns, nColumns + 100 );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	uint32_t nFrames = 500;
	double fInitialSongSize = pAE->m_fSongSizeInTicks;
	int nNewColumn;

	int nNumberOfTogglings = 1;

	for ( int nn = 0; nn < nNumberOfTogglings; ++nn ) {

		pAE->locate( fInitialSongSize + frameDist( randomEngine ) );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChangeInLoopMode] relocation" );

		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChangeInLoopMode] first increment" );

		nNewColumn = columnDist( randomEngine );

		pAE->unlock();
		pCoreActionController->toggleGridCell( nNewColumn, 0 );
		pAE->lock( RIGHT_HERE );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChangeInLoopMode] first toggling" );

		if ( fInitialSongSize == pAE->m_fSongSizeInTicks ) {
			AudioEngineTests::throwException(
				QString( "[testSongSizeChangeInLoopMode] [first toggling] no song enlargement %1")
				.arg( pAE->m_fSongSizeInTicks ) );
		}

		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChange] second increment" );
														  
		pAE->unlock();
		pCoreActionController->toggleGridCell( nNewColumn, 0 );
		pAE->lock( RIGHT_HERE );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChange] second toggling" );
		
		if ( fInitialSongSize != pAE->m_fSongSizeInTicks ) {
			AudioEngineTests::throwException(
				QString( "[testSongSizeChange] [second toggling] song size mismatch original: %1, new: %2" )
				.arg( fInitialSongSize )
				.arg( pAE->m_fSongSizeInTicks ) );
		}

		pAE->incrementTransportPosition( nFrames );

		AudioEngineTests::checkTransportPosition(
			pTransportPos, "[testSongSizeChange] third increment" );
	}

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
}

void AudioEngineTests::testNoteEnqueuing() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pPref = Preferences::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();

	pCoreActionController->activateTimeline( false );
	pCoreActionController->activateLoopMode( false );
	pCoreActionController->activateSongMode( true );
	pAE->lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( pPref->m_nBufferSize / 2,
												  pPref->m_nBufferSize );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	// Check consistency of updated frames and ticks while using a
	// random buffer size (e.g. like PulseAudio does).
	
	uint32_t nFrames;
	double fCheckTick;
	long long nCheckFrame, nLastFrame = 0;

	int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );

	int nn = 0;

	// Ensure the sampler is clean.
	AudioEngineTests::resetSampler( "testNoteEnqueuing : song mode" );

	bool bEndOfSongReached = false;

	auto notesInSong = pSong->getAllNotes();

	std::vector<std::shared_ptr<Note>> notesInSongQueue;
	std::vector<std::shared_ptr<Note>> notesInSamplerQueue;

	while ( pTransportPos->getDoubleTick() <
			pAE->m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		if ( ! bEndOfSongReached ) {
			if ( pAE->updateNoteQueue( nFrames ) == -1 ) {
				bEndOfSongReached = true;
			}
		}

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
				QString( "[testNoteEnqueuing] end of the song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pAE->m_fSongSizeInTicks: %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	if ( notesInSongQueue.size() !=
		 notesInSong.size() ) {
		QString sMsg = QString( "[testNoteEnqueuing] [song mode] Mismatch between notes count in Song [%1] and NoteQueue [%2]. Song:\n" )
			.arg( notesInSong.size() ).arg( notesInSongQueue.size() );
		for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
			auto note = notesInSong[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "NoteQueue:\n" );
		for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
			auto note = notesInSongQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}

	// We have to relax the test for larger buffer sizes. Else, the
	// notes will be already fully processed in and flush from the
	// Sampler before we had the chance to grab and compare them.
	if ( notesInSamplerQueue.size() !=
		 notesInSong.size() &&
		 pPref->m_nBufferSize < 1024 ) {
		QString sMsg = QString( "[testNoteEnqueuing] [song mode] Mismatch between notes count in Song [%1] and Sampler [%2]. Song:\n" )
			.arg( notesInSong.size() ).arg( notesInSamplerQueue.size() );
		for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
			auto note = notesInSong[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "SamplerQueue:\n" );
		for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
			auto note = notesInSamplerQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}

	pAE->setState( AudioEngine::State::Ready );

	pAE->unlock();

	//////////////////////////////////////////////////////////////////
	// Perform the test in pattern mode
	//////////////////////////////////////////////////////////////////
	
	pCoreActionController->activateSongMode( false );
	pHydrogen->setPatternMode( Song::PatternMode::Selected );
	pHydrogen->setSelectedPatternNumber( 4 );

	pAE->lock( RIGHT_HERE );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	int nLoops = 5;
	
	nMaxCycles = MAX_NOTES * 2 * nLoops;
	nn = 0;

	AudioEngineTests::resetSampler( "testNoteEnqueuing : pattern mode" );

	auto pPattern = 
		pSong->getPatternList()->get( pHydrogen->getSelectedPatternNumber() );
	if ( pPattern == nullptr ) {
		AudioEngineTests::throwException(
			QString( "[testNoteEnqueuing] null pattern selected [%1]" )
			.arg( pHydrogen->getSelectedPatternNumber() ) );
	}

	std::vector<std::shared_ptr<Note>> notesInPattern;
	for ( int ii = 0; ii < nLoops; ++ii ) {
		FOREACH_NOTE_CST_IT_BEGIN_END( pPattern->get_notes(), it ) {
			if ( it->second != nullptr ) {
				auto note = std::make_shared<Note>( it->second );
				note->set_position( note->get_position() +
									ii * pPattern->get_length() );
				notesInPattern.push_back( note );
			}
		}
	}

	notesInSongQueue.clear();
	notesInSamplerQueue.clear();

	nMaxCycles =
		static_cast<int>(std::max( static_cast<float>(pPattern->get_length()) *
								   static_cast<float>(nLoops) *
								   pTransportPos->getTickSize() * 4 /
								   static_cast<float>(pPref->m_nBufferSize),
								   static_cast<float>(MAX_NOTES) *
								   static_cast<float>(nLoops) ));
	nn = 0;

	while ( pTransportPos->getDoubleTick() <
			pPattern->get_length() * nLoops ) {

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
				QString( "[testNoteEnqueuing] end of pattern wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, pPattern->get_length(): %4, nMaxCycles: %5, nLoops: %6" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pPattern->get_length() )
				.arg( nMaxCycles )
				.arg( nLoops ) );
		}
	}

	// Transport in pattern mode is always looped. We have to pop the
	// notes added during the second run due to the lookahead.
	int nNoteNumber = notesInSongQueue.size();
	for( int ii = 0; ii < nNoteNumber; ++ii ) {
		auto note = notesInSongQueue[ nNoteNumber - 1 - ii ];
		if ( note != nullptr &&
			 note->get_position() >= pPattern->get_length() * nLoops ) {
			notesInSongQueue.pop_back();
		} else {
			break;
		}
	}

	nNoteNumber = notesInSamplerQueue.size();
	for( int ii = 0; ii < nNoteNumber; ++ii ) {
		auto note = notesInSamplerQueue[ nNoteNumber - 1 - ii ];
		if ( note != nullptr &&
			 note->get_position() >= pPattern->get_length() * nLoops ) {
			notesInSamplerQueue.pop_back();
		} else {
			break;
		}
	}

	if ( notesInSongQueue.size() !=
		 notesInPattern.size() ) {
		QString sMsg = QString( "[testNoteEnqueuing] [pattern mode] Mismatch between notes count in Pattern [%1] and NoteQueue [%2]. Pattern:\n" )
			.arg( notesInPattern.size() ).arg( notesInSongQueue.size() );
		for ( int ii = 0; ii < notesInPattern.size(); ++ii  ) {
			auto note = notesInPattern[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "NoteQueue:\n" );
		for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
			auto note = notesInSongQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}

	// We have to relax the test for larger buffer sizes. Else, the
	// notes will be already fully processed in and flush from the
	// Sampler before we had the chance to grab and compare them.
	if ( notesInSamplerQueue.size() !=
		 notesInPattern.size() &&
		 pPref->m_nBufferSize < 1024 ) {
		QString sMsg = QString( "[testNoteEnqueuing] [pattern mode] Mismatch between notes count in Pattern [%1] and Sampler [%2]. Pattern:\n" )
			.arg( notesInPattern.size() ).arg( notesInSamplerQueue.size() );
		for ( int ii = 0; ii < notesInPattern.size(); ++ii  ) {
			auto note = notesInPattern[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "SamplerQueue:\n" );
		for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
			auto note = notesInSamplerQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}

	pAE->setState( AudioEngine::State::Ready );

	pAE->unlock();

	//////////////////////////////////////////////////////////////////
	// Perform the test in looped pattern mode
	//////////////////////////////////////////////////////////////////

	// In case the transport is looped the first note was lost the
	// first time transport was wrapped to the beginning again. This
	// occurred just in song mode.
	
	pCoreActionController->activateLoopMode( true );
	pCoreActionController->activateSongMode( true );

	pAE->lock( RIGHT_HERE );

	// For this call the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );

	pAE->setState( AudioEngine::State::Testing );

	nLoops = 1;
	nCheckFrame = 0;
	nLastFrame = 0;

	nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) ) *
		( nLoops + 1 );
	
	AudioEngineTests::resetSampler( "testNoteEnqueuing : loop mode" );

	nn = 0;

	bEndOfSongReached = false;

	notesInSong.clear();
	for ( int ii = 0; ii <= nLoops; ++ii ) {
		auto notesVec = pSong->getAllNotes();
		for ( auto nnote : notesVec ) {
			nnote->set_position( nnote->get_position() +
								 ii * pAE->m_fSongSizeInTicks );
		}
		notesInSong.insert( notesInSong.end(), notesVec.begin(), notesVec.end() );
	}

	notesInSongQueue.clear();
	notesInSamplerQueue.clear();

	while ( pTransportPos->getDoubleTick() <
			pAE->m_fSongSizeInTicks * ( nLoops + 1 ) ) {

		nFrames = frameDist( randomEngine );

		// Turn off loop mode once we entered the last loop cycle.
		if ( ( pTransportPos->getDoubleTick() >
			   pAE->m_fSongSizeInTicks * nLoops + 100 ) &&
			 pSong->getLoopMode() == Song::LoopMode::Enabled ) {
			pCoreActionController->activateLoopMode( false );
		}

		if ( ! bEndOfSongReached ) {
			if ( pAE->updateNoteQueue( nFrames ) == -1 ) {
				bEndOfSongReached = true;
			}
		}

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
				QString( "[testNoteEnqueuing] [loop mode] end of song wasn't reached in time. pTransportPos->getFrame(): %1, pTransportPos->getDoubleTick(): %2, getTickSize(): %3, m_fSongSizeInTicks(): %4, nMaxCycles: %5" )
				.arg( pTransportPos->getFrame() )
				.arg( pTransportPos->getDoubleTick(), 0, 'f' )
				.arg( pTransportPos->getTickSize(), 0, 'f' )
				.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
				.arg( nMaxCycles ) );
		}
	}

	if ( notesInSongQueue.size() !=
		 notesInSong.size() ) {
		QString sMsg = QString( "[testNoteEnqueuing] [loop mode] Mismatch between notes count in Song [%1] and NoteQueue [%2]. Song:\n" )
			.arg( notesInSong.size() ).arg( notesInSongQueue.size() );
		for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
			auto note = notesInSong[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "NoteQueue:\n" );
		for ( int ii = 0; ii < notesInSongQueue.size(); ++ii  ) {
			auto note = notesInSongQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}

	// We have to relax the test for larger buffer sizes. Else, the
	// notes will be already fully processed in and flush from the
	// Sampler before we had the chance to grab and compare them.
	if ( notesInSamplerQueue.size() !=
		 notesInSong.size() &&
		 pPref->m_nBufferSize < 1024 ) {
		QString sMsg = QString( "[testNoteEnqueuing] [loop mode] Mismatch between notes count in Song [%1] and Sampler [%2]. Song:\n" )
			.arg( notesInSong.size() ).arg( notesInSamplerQueue.size() );
		for ( int ii = 0; ii < notesInSong.size(); ++ii  ) {
			auto note = notesInSong[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}
		sMsg.append( "SamplerQueue:\n" );
		for ( int ii = 0; ii < notesInSamplerQueue.size(); ++ii  ) {
			auto note = notesInSamplerQueue[ ii ];
			sMsg.append( QString( "\t[%1] instr: %2, position: %3, noteStart: %4, velocity: %5\n")
						 .arg( ii )
						 .arg( note->get_instrument()->get_name() )
						 .arg( note->get_position() )
						 .arg( note->getNoteStart() )
						 .arg( note->get_velocity() ) );
		}

		AudioEngineTests::throwException( sMsg );
	}
	
	pAE->setState( AudioEngine::State::Ready );

	pAE->unlock();
}



void AudioEngineTests::testNoteEnqueuingTimeline() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	auto pPref = Preferences::get_instance();

	pAE->lock( RIGHT_HERE );

	// Seed with a real random value, if available
    std::random_device randomSeed;
 
    // Choose a random mean between 1 and 6
    std::default_random_engine randomEngine( randomSeed() );
    std::uniform_int_distribution<int> frameDist( pPref->m_nBufferSize / 2,
												  pPref->m_nBufferSize );

	// For reset() the AudioEngine still needs to be in state
	// Playing or Ready.
	pAE->reset( false );
	pAE->setState( AudioEngine::State::Testing );
	AudioEngineTests::resetSampler( __PRETTY_FUNCTION__ );

	uint32_t nFrames;

	const int nMaxCycles =
		std::max( std::ceil( static_cast<double>(pAE->m_fSongSizeInTicks) /
							 static_cast<double>(pPref->m_nBufferSize) *
							 static_cast<double>(pTransportPos->getTickSize()) * 4.0 ),
				  static_cast<double>(pAE->m_fSongSizeInTicks) );
	int nn = 0;
	bool bEndOfSongReached = false;

	auto notesInSong = pSong->getAllNotes();

	std::vector<std::shared_ptr<Note>> notesInSongQueue;
	std::vector<std::shared_ptr<Note>> notesInSamplerQueue;

	while ( pTransportPos->getDoubleTick() <
			pAE->m_fSongSizeInTicks ) {

		nFrames = frameDist( randomEngine );

		if ( ! bEndOfSongReached ) {
			if ( pAE->updateNoteQueue( nFrames ) == -1 ) {
				bEndOfSongReached = true;
			}
		}

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
		if ( ! notesInSong[ ii ]->match( notesInSongQueue[ ii ] ) ) {
		AudioEngineTests::throwException(
			QString( "Mismatch at note [%1] between song [%2] and song queue [%3]" )
			.arg( ii )
			.arg( notesInSong[ ii ]->toQString() )
			.arg( notesInSongQueue[ ii ]->toQString() ) );
		}
		if ( ! notesInSong[ ii ]->match( notesInSamplerQueue[ ii ] ) ) {
		AudioEngineTests::throwException(
			QString( "Mismatch at note [%1] between song [%2] and sampler queue [%3]" )
			.arg( ii )
			.arg( notesInSong[ ii ]->toQString() )
			.arg( notesInSamplerQueue[ ii ]->toQString() ) );
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
				if ( newNote->match( presentNote.get() ) &&
					 newNote->get_position() ==
					 presentNote->get_position() &&
					 newNote->get_velocity() ==
					 presentNote->get_velocity() ) {
					bNoteFound = true;
				}
			}
		}

		if ( ! bNoteFound ) {
			noteList->push_back( std::make_shared<Note>(newNote.get()) );
		}
	}
}

// Used for the Sampler note queue
void AudioEngineTests::mergeQueues( std::vector<std::shared_ptr<Note>>* noteList, std::vector<Note*> newNotes ) {
	bool bNoteFound;
	for ( const auto& newNote : newNotes ) {
		bNoteFound = false;
		// Check whether the notes is already present.
		for ( const auto& presentNote : *noteList ) {
			if ( newNote != nullptr && presentNote != nullptr ) {
				if ( newNote->match( presentNote.get() ) &&
					 newNote->get_position() ==
					 presentNote->get_position() &&
					 newNote->get_velocity() ==
					 presentNote->get_velocity() ) {
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
	long long nCheckFrame =
		TransportPosition::computeFrameFromTick(
			pPos->getDoubleTick(), &fCheckTickMismatch );
	double fCheckTick =
		TransportPosition::computeTickFromFrame(
			pPos->getFrame() );
	
	if ( abs( fCheckTick + fCheckTickMismatch - pPos->getDoubleTick() ) > 1e-9 ||
		 abs( fCheckTickMismatch - pPos->m_fTickMismatch ) > 1e-9 ||
		 nCheckFrame != pPos->getFrame() ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%8] [tick or frame mismatch]. original position: [%1], nCheckFrame: %2, fCheckTick: %3, fCheckTickMismatch: %4, fCheckTick + fCheckTickMismatch - pPos->getDoubleTick(): %5, fCheckTickMismatch - pPos->m_fTickMismatch: %6, nCheckFrame - pPos->getFrame(): %7" )
			.arg( pPos->toQString( "", true ) )
			.arg( nCheckFrame )
			.arg( fCheckTick, 0 , 'f', 9 )
			.arg( fCheckTickMismatch, 0 , 'f', 9 )
			.arg( fCheckTick + fCheckTickMismatch -
				  pPos->getDoubleTick(), 0, 'E' )
			.arg( fCheckTickMismatch - pPos->m_fTickMismatch, 0, 'E' )
			.arg( nCheckFrame - pPos->getFrame() )
			.arg( sContext ) );
	}

	long nCheckPatternStartTick;
	int nCheckColumn =
		pHydrogen->getColumnForTick( pPos->getTick(),
									 pSong->isLoopEnabled(),
									 &nCheckPatternStartTick );
	long nTicksSinceSongStart =
		static_cast<long>(std::floor( std::fmod(
			pPos->getDoubleTick(),
			pAE->m_fSongSizeInTicks ) ));
	if ( pHydrogen->getMode() == Song::Mode::Song &&
		 pPos->getColumn() != -1 &&
		 ( nCheckColumn != pPos->getColumn() ||
		   ( nCheckPatternStartTick !=
			 pPos->getPatternStartTick() ) ||
		   ( nTicksSinceSongStart - nCheckPatternStartTick !=
			 pPos->getPatternTickPosition() ) ) ) {
		AudioEngineTests::throwException(
			QString( "[checkTransportPosition] [%7] [column or pattern tick mismatch]. current position: [%1], nCheckColumn: %2, nCheckPatternStartTick: %3, nCheckPatternTickPosition: %4, nTicksSinceSongStart: %5, pAE->m_fSongSizeInTicks: %6" )
			.arg( pPos->toQString( "", true ) )
			.arg( nCheckColumn )
			.arg( nCheckPatternStartTick )
			.arg( nTicksSinceSongStart - nCheckPatternStartTick )
			.arg( nTicksSinceSongStart )
			.arg( pAE->m_fSongSizeInTicks, 0, 'f' )
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
			if ( ppNewNote->match( ppOldNote.get() ) &&
				 ppNewNote->get_humanize_delay() ==
				 ppOldNote->get_humanize_delay() &&
				 ppNewNote->get_velocity() ==
				 ppOldNote->get_velocity() ) {
				++nNotesFound;

				if ( bTestAudio ) {
					// Check for consistency in the Sample position
					// advanced by the Sampler upon rendering.
					for ( int nn = 0; nn < ppNewNote->get_instrument()->get_components()->size(); nn++ ) {
						auto pSelectedLayer = ppOldNote->get_layer_selected( nn );
						
						// The frames passed during the audio
						// processing depends on the sample rate of
						// the driver and sample and has to be
						// adjusted in here. This is equivalent to the
						// question whether Sampler::renderNote() or
						// Sampler::renderNoteResample() was used.
						if ( ppOldNote->getSample( nn )->get_sample_rate() !=
							 nSampleRate ||
							 ppOldNote->get_total_pitch() != 0.0 ) {
							// In here we assume the layer pitcyh is zero.
							fPassedFrames = static_cast<double>(nPassedFrames) *
								Note::pitchToFrequency( ppOldNote->get_total_pitch() ) *
								static_cast<float>(ppOldNote->getSample( nn )->get_sample_rate()) /
								static_cast<float>(nSampleRate);
						}
						
						int nSampleFrames = ( ppNewNote->get_instrument()->get_component( nn )
											  ->get_layer( pSelectedLayer->SelectedLayer )->get_sample()->get_frames() );
						double fExpectedFrames =
							std::min( static_cast<double>(pSelectedLayer->SamplePosition) +
									  fPassedFrames,
									  static_cast<double>(nSampleFrames) );
						if ( std::abs( ppNewNote->get_layer_selected( nn )->SamplePosition -
									   fExpectedFrames ) > 1 ) {
							AudioEngineTests::throwException(
								QString( "[checkAudioConsistency] [%4] glitch in audio render. Diff: %9\nPre: %1\nPost: %2\nwith passed frames: %3, nSampleFrames: %5, fExpectedFrames: %6, sample sampleRate: %7, driver sampleRate: %8\n" )
								.arg( ppOldNote->toQString( "", true ) )
								.arg( ppNewNote->toQString( "", true ) )
								.arg( fPassedFrames, 0, 'f' )
								.arg( sContext )
								.arg( nSampleFrames )
								.arg( fExpectedFrames, 0, 'f' )
								.arg( ppOldNote->getSample( nn )->get_sample_rate() )
								.arg( nSampleRate )
								.arg( ppNewNote->get_layer_selected( nn )->SamplePosition -
									  fExpectedFrames, 0, 'g', 30 ) );
						}
					}
				}
				else { // if ( bTestAudio )
					// Check whether changes in note start position
					// were properly applied in the note queue of the
					// audio engine.
					if ( ppNewNote->get_position() - fPassedTicks !=
						 ppOldNote->get_position() ) {
						AudioEngineTests::throwException(
							QString( "[checkAudioConsistency] [%5] glitch in note queue.\n\tPre: %1\n\tPost: %2\n\tfPassedTicks: %3, diff (new - passed - old): %4" )
							.arg( ppOldNote->toQString( "", true ) )
							.arg( ppNewNote->toQString( "", true ) )
							.arg( fPassedTicks )
							.arg( ppNewNote->get_position() - fPassedTicks -
								  ppOldNote->get_position() )
							.arg( sContext ) );
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
		if ( oldNotes.size() != 0 ) {
			sMsg.append( "\nold notes:" );
											 
			for ( auto const& nnote : oldNotes ) {
				sMsg.append( "\n" + nnote->toQString( "    ", true ) );
			}
		}
		if ( newNotes.size() != 0 ) {
			sMsg.append( "\nnew notes:" );
			for ( auto const& nnote : newNotes ) {
				sMsg.append( "\n" + nnote->toQString( "    ", true ) );
			}
		}
		sMsg.append( QString( "\n\npTransportPos->getDoubleTick(): %1, pTransportPos->getFrame(): %2, nPassedFrames: %3, fPassedTicks: %4, pTransportPos->getTickSize(): %5" )
					 .arg( pTransportPos->getDoubleTick(), 0, 'f' )
					 .arg( pTransportPos->getFrame() )
					 .arg( nPassedFrames )
					 .arg( fPassedTicks, 0, 'f' )
					 .arg( pTransportPos->getTickSize(), 0, 'f' ) );
		sMsg.append( "\n\n notes in song:" );
		for ( auto const& nnote : pSong->getAllNotes() ) {
			sMsg.append( "\n" + nnote->toQString( "    ", true ) );
		}
		AudioEngineTests::throwException( sMsg );
	}
}

std::vector<std::shared_ptr<Note>> AudioEngineTests::copySongNoteQueue() {
	auto pAE = Hydrogen::get_instance()->getAudioEngine();
	std::vector<Note*> rawNotes;
	std::vector<std::shared_ptr<Note>> notes;
	for ( ; ! pAE->m_songNoteQueue.empty(); pAE->m_songNoteQueue.pop() ) {
		rawNotes.push_back( pAE->m_songNoteQueue.top() );
		notes.push_back( std::make_shared<Note>( pAE->m_songNoteQueue.top() ) );
	}

	for ( auto nnote : rawNotes ) {
		pAE->m_songNoteQueue.push( nnote );
	}

	return notes;
}

void AudioEngineTests::toggleAndCheckConsistency( int nToggleColumn, int nToggleRow, const QString& sContext ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pSong = pHydrogen->getSong();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pTransportPos = pAE->getTransportPosition();
	
	const unsigned long nBufferSize = pHydrogen->getAudioOutput()->getBufferSize();

	pAE->updateNoteQueue( nBufferSize );
	pAE->processAudio( nBufferSize );
	pAE->incrementTransportPosition( nBufferSize );

	auto prevNotes = AudioEngineTests::copySongNoteQueue();

	// Cache some stuff in order to compare it later on.
	long nOldSongSize = pSong->lengthInTicks();
	int nOldColumn = pTransportPos->getColumn();
	float fPrevTempo = pTransportPos->getBpm();
	float fPrevTickSize = pTransportPos->getTickSize();
	double fPrevTickStart, fPrevTickEnd;
	long long nPrevLeadLag;

	bool bOldLookaheadApplied = pAE->m_bLookaheadApplied;
	nPrevLeadLag =
		pAE->computeTickInterval( &fPrevTickStart, &fPrevTickEnd, nBufferSize );
	pAE->m_bLookaheadApplied = bOldLookaheadApplied;

	std::vector<std::shared_ptr<Note>> notes1, notes2;
	for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
		notes1.push_back( std::make_shared<Note>( ppNote ) );
	}

	//////
	// Toggle a grid cell prior to the current transport position
	//////
	
	pAE->unlock();
	pCoreActionController->toggleGridCell( nToggleColumn, nToggleRow );
	pAE->lock( RIGHT_HERE );

	QString sFirstContext = QString( "[toggleAndCheckConsistency] %1 : 1. toggling" ).arg( sContext );

	// Check whether there is a change in song size
	long nNewSongSize = pSong->lengthInTicks();
	if ( nNewSongSize == nOldSongSize ) {
		AudioEngineTests::throwException(
			QString( "[%1] no change in song size" )
			.arg( sFirstContext ) );
	}

	// Check whether current frame and tick information are still
	// consistent.
	AudioEngineTests::checkTransportPosition( pTransportPos, sFirstContext );

	// m_songNoteQueue have been updated properly.
	auto afterNotes = AudioEngineTests::copySongNoteQueue();

	AudioEngineTests::checkAudioConsistency(
		prevNotes, afterNotes, sFirstContext + " 1. audio check", 0, false,
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
				.arg( nOldColumn )
				.arg( pTransportPos->getColumn() )
				.arg( sFirstContext ) );
		}

		double fTickEnd, fTickStart;
		bool bOldLookaheadApplied = pAE->m_bLookaheadApplied;
		const long long nLeadLag =
			pAE->computeTickInterval( &fTickStart, &fTickEnd, nBufferSize );
		pAE->m_bLookaheadApplied = bOldLookaheadApplied;
		if ( std::abs( nLeadLag - nPrevLeadLag ) > 1 ) {
			AudioEngineTests::throwException(
				QString( "[%3] LeadLag should be constant since there should be change in tick size. old: %1, new: %2" )
				.arg( nPrevLeadLag ).arg( nLeadLag ).arg( sFirstContext ) );
		}
		if ( std::abs( fTickStart - pTransportPos->getTickOffsetSongSize() - fPrevTickStart ) > 4e-3 ) {
			AudioEngineTests::throwException(
				QString( "[%5] Mismatch in the start of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
				.arg( fTickStart, 0, 'f' )
				.arg( fPrevTickStart + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( fPrevTickStart, 0, 'f' )
				.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( sFirstContext ) );
		}
		if ( std::abs( fTickEnd - pTransportPos->getTickOffsetSongSize() - fPrevTickEnd ) > 4e-3 ) {
			AudioEngineTests::throwException(
				QString( "[%5] Mismatch in the end of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
				.arg( fTickEnd, 0, 'f' )
				.arg( fPrevTickEnd + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( fPrevTickEnd, 0, 'f' )
				.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( sFirstContext ) );
		}
	}
	else if ( pTransportPos->getColumn() != 0 &&
			  nOldColumn >= pSong->getPatternGroupVector()->size() ) {
		AudioEngineTests::throwException(
			QString( "[%4] Column reset failed nOldColumn: %1, pTransportPos->getColumn() (new): %2, pSong->getPatternGroupVector()->size() (new): %3" )
			.arg( nOldColumn )
			.arg( pTransportPos->getColumn() )
			.arg( pSong->getPatternGroupVector()->size() )
			.arg( sFirstContext ) );
	}
	
	// Now we emulate that playback continues without any new notes
	// being added and expect the rendering of the notes currently
	// played back by the Sampler to start off precisely where we
	// stopped before the song size change. New notes might still be
	// added due to the lookahead, so, we just check for the
	// processing of notes we already encountered.
	pAE->incrementTransportPosition( nBufferSize );
	pAE->processAudio( nBufferSize );
	pAE->incrementTransportPosition( nBufferSize );
	pAE->processAudio( nBufferSize );

	// Check whether tempo and tick size have not changed.
	if ( fPrevTempo != pTransportPos->getBpm() ||
		 fPrevTickSize != pTransportPos->getTickSize() ) {
		AudioEngineTests::throwException(
			QString( "[%1] tempo and ticksize are affected" )
			.arg( sFirstContext ) );
	}

	for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
		notes2.push_back( std::make_shared<Note>( ppNote ) );
	}

	AudioEngineTests::checkAudioConsistency(
		notes1, notes2, sFirstContext + " 2. audio check", nBufferSize * 2 );

	//////
	// Toggle the same grid cell again
	//////

	QString sSecondContext = QString( "[toggleAndCheckConsistency] %1 : 2. toggling" )
		.arg( sContext );
	
	notes1.clear();
	for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
		notes1.push_back( std::make_shared<Note>( ppNote ) );
	}

	//TODO:
	// We deal with a slightly artificial situation regarding
	// m_fLastTickIntervalEnd in here. Usually, in addition to
	// incrementTransportPosition() and	processAudio()
	// updateNoteQueue() would have been called too. This would update
	// m_fLastTickIntervalEnd which is not done in here. This way we
	// emulate a situation that occurs when encountering a change in
	// ticksize (passing a tempo marker or a user interaction with the
	// BPM widget) just before the song size changed.
	bOldLookaheadApplied = pAE->m_bLookaheadApplied;
	nPrevLeadLag =
		pAE->computeTickInterval( &fPrevTickStart, &fPrevTickEnd, nBufferSize );
	pAE->m_bLookaheadApplied = bOldLookaheadApplied;

	nOldColumn = pTransportPos->getColumn();
	
	pAE->unlock();
	pCoreActionController->toggleGridCell( nToggleColumn, nToggleRow );
	pAE->lock( RIGHT_HERE );

	// Check whether there is a change in song size
	nOldSongSize = nNewSongSize;
	nNewSongSize = pSong->lengthInTicks();
	if ( nNewSongSize == nOldSongSize ) {
		AudioEngineTests::throwException(
			QString( "[%1] no change in song size" ).arg( sSecondContext ) );
	}

	// Check whether current frame and tick information are still
	// consistent.
	AudioEngineTests::checkTransportPosition( pTransportPos, sSecondContext );

	// Check whether the notes already enqueued into the
	// m_songNoteQueue have been updated properly.
	prevNotes.clear();
	prevNotes = AudioEngineTests::copySongNoteQueue();
	AudioEngineTests::checkAudioConsistency(
		afterNotes, prevNotes, sSecondContext + " 1. audio check", 0, false,
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
				.arg( nOldColumn )
				.arg( pTransportPos->getColumn() )
				.arg( sSecondContext ) );
		}

		double fTickEnd, fTickStart;
		bool bOldLookaheadApplied = pAE->m_bLookaheadApplied;
		const long long nLeadLag =
			pAE->computeTickInterval( &fTickStart, &fTickEnd, nBufferSize );
		pAE->m_bLookaheadApplied = bOldLookaheadApplied;
		if ( std::abs( nLeadLag - nPrevLeadLag ) > 1 ) {
			AudioEngineTests::throwException(
				QString( "[%3] LeadLag should be constant since there should be change in tick size. old: %1, new: %2" )
				.arg( nPrevLeadLag ).arg( nLeadLag ).arg( sSecondContext ) );
		}
		if ( std::abs( fTickStart - pTransportPos->getTickOffsetSongSize() - fPrevTickStart ) > 4e-3 ) {
			AudioEngineTests::throwException(
				QString( "[%5] Mismatch in the start of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
				.arg( fTickStart, 0, 'f' )
				.arg( fPrevTickStart + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( fPrevTickStart, 0, 'f' )
				.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( sSecondContext ) );
		}
		if ( std::abs( fTickEnd - pTransportPos->getTickOffsetSongSize() - fPrevTickEnd ) > 4e-3 ) {
			AudioEngineTests::throwException(
				QString( "[%5] Mismatch in the end of the tick interval handled by updateNoteQueue new [%1] != [%2] old+offset, old: %3, offset: %4" )
				.arg( fTickEnd, 0, 'f' )
				.arg( fPrevTickEnd + pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( fPrevTickEnd, 0, 'f' )
				.arg( pTransportPos->getTickOffsetSongSize(), 0, 'f' )
				.arg( sSecondContext ) );
		}
	}
	else if ( pTransportPos->getColumn() != 0 &&
			  nOldColumn >= pSong->getPatternGroupVector()->size() ) {
		AudioEngineTests::throwException(
			QString( "[%4] Column reset failed nOldColumn: %1, pTransportPos->getColumn() (new): %2, pSong->getPatternGroupVector()->size() (new): %3" )
			.arg( nOldColumn )
			.arg( pTransportPos->getColumn() )
			.arg( pSong->getPatternGroupVector()->size() )
			.arg( sSecondContext ) );
	}

	// Now we emulate that playback continues without any new notes
	// being added and expect the rendering of the notes currently
	// played back by the Sampler to start off precisely where we
	// stopped before the song size change. New notes might still be
	// added due to the lookahead, so, we just check for the
	// processing of notes we already encountered.
	pAE->incrementTransportPosition( nBufferSize );
	pAE->processAudio( nBufferSize );
	pAE->incrementTransportPosition( nBufferSize );
	pAE->processAudio( nBufferSize );

	// Check whether tempo and tick size have not changed.
	if ( fPrevTempo != pTransportPos->getBpm() ||
		 fPrevTickSize != pTransportPos->getTickSize() ) {
		AudioEngineTests::throwException(
			QString( "[%1] tempo and ticksize are affected" ).arg( sSecondContext ) );
	}

	notes2.clear();
	for ( const auto& ppNote : pSampler->getPlayingNotesQueue() ) {
		notes2.push_back( std::make_shared<Note>( ppNote ) );
	}

	AudioEngineTests::checkAudioConsistency(
		notes1, notes2, sSecondContext + " 2. audio check", nBufferSize * 2 );
}

void AudioEngineTests::resetSampler( const QString& sContext ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAE = pHydrogen->getAudioEngine();
	auto pSampler = pAE->getSampler();
	auto pPref = Preferences::get_instance();

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

void AudioEngineTests::throwException( const QString& sMsg ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAE = pHydrogen->getAudioEngine();

	pAE->setState( AudioEngine::State::Ready );
	pAE->unlock();
	
	throw std::runtime_error( sMsg.toLocal8Bit().data() );
}

	
}; // namespace H2Core
