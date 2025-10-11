/*
 * Hydrogen
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

#include <cppunit/extensions/HelperMacros.h>

#include <QString>
#include <QtGlobal>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/PatternList.h>
#include "TestHelper.h"
#include "AudioBenchmark.h"

#include <memory>
#include <ctime>

using namespace H2Core;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace Qt {
	auto endl = ::endl;
};
#endif

bool AudioBenchmark::bEnabled = false;

static long long exportCurrentSong( const QString &sFileName, int nSampleRate )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	EventQueue *pQueue = EventQueue::get_instance();
	auto pSong = pHydrogen->getSong();

	if( !pSong ) {
		return 0;
	}

	pHydrogen->startExportSession( nSampleRate, 16 );
	pHydrogen->startExportSong( sFileName );

	long long nStartFrame = pHydrogen->getAudioEngine()->getTransportPosition()->getFrame();

	bool done = false;
	while ( ! done ) {
		auto pEvent = pQueue->popEvent();
		if ( pEvent == nullptr ) {
			usleep(10 * 1000);
			continue;
		}

		// Ensure audio export does always work.
		CPPUNIT_ASSERT( ! ( pEvent->getType() == Event::Type::Progress &&
							pEvent->getValue() == -1 ) );
		
		if ( pEvent->getType() == Event::Type::Progress &&
			 pEvent->getValue() == 100 ) {
			done = true;
		}
	}

	pHydrogen->stopExportSession();

	return pHydrogen->getAudioEngine()->getTransportPosition()->getFrame() - nStartFrame;
}

static QString showNumber( double f ) {
	if ( f > 1000000 ) {
		return QString( "%1M" ).arg( f/1000000 );
	} else if ( f > 1000 ) {
		return QString( "%1K" ).arg( f/1000 );
	} else if ( f > 1 ) {
		return QString( "%1" ).arg( f );
	} else if ( f > 0.001 ) {
		return QString( "%1m" ).arg( f*1000 );
	} else {
		return QString( "%1u" ).arg( f*1000000 );
	}
}


static QString showTimes( std::vector< clock_t > &times, int nFrames,
						  double *pfMean = nullptr, double *pfRMS = nullptr) {
	double fTotal = std::accumulate(times.begin(), times.end(), 0) * 1.0 / CLOCKS_PER_SEC;
	double fMean = fTotal / times.size();

	double fTotalError = 0;
	for ( auto t : times ) {
		double fSeconds = 1.0 * t / CLOCKS_PER_SEC;
		double fError = fMean - fSeconds;
		fTotalError += fError * fError;
	}
	double fRMS = sqrt( fTotalError ) / times.size();

	if ( pfMean ) {
		*pfMean = fMean;
	}
	if ( pfRMS ) {
		*pfRMS = fRMS;
	}

	return QString( "%1s (%2 frames/sec) +/- %3%" )
		.arg( showNumber( fMean ) )
		.arg( showNumber( nFrames * 1.0 / fMean) )
		.arg( 100.0 * fRMS / fMean, 0, 'f', 3 );
}

void AudioBenchmark::timeADSR() {
	const int nFrames = 4096;
	float data_L[nFrames], data_R[nFrames];
	int nIterations = 32;
	std::vector< clock_t > times;

	for ( int i = 0; i < 100; i++ ) {
		for (int i = 0; i < nFrames; i++) {
			data_L[i] = data_R[i] = 1.0;
		}

		ADSR adsr( nFrames / 4, nFrames / 4, 0.5, nFrames / 4 );

		std::clock_t start = std::clock();
		adsr.applyADSR( data_L, data_R, nFrames, 3 * nFrames / 4, 1.0 );
		std::clock_t end = std::clock();

		times.push_back( end - start );
	}

	out << "ADSR time: " << showTimes( times, nFrames ) << Qt::endl;
}

double AudioBenchmark::timeExport( int nSampleRate,
								   Interpolation::InterpolateMode interpolateMode,
								   double fReference,
								   double *pfRMS ) {
	auto outFile = Filesystem::tmp_file_path("test.wav");
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nIterations = 32;
	std::vector< clock_t > times;
	long long nFrames = 0, nFramesNew;

	auto oldInterpolateMode = pHydrogen->getAudioEngine()->getSampler()->getInterpolateMode();
	pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( interpolateMode );

	// Run through once to warm caches etc.
	exportCurrentSong( outFile, 44100 );

	for ( int i = 0; i < nIterations; i++ ) {

		nFramesNew = 0;
		std::clock_t start = std::clock();
		for ( int j = 0; j < 5; j++) {
			nFramesNew += exportCurrentSong( outFile, nSampleRate );
		}
		std::clock_t end = std::clock();

		CPPUNIT_ASSERT( nFramesNew == nFrames || nFrames == 0 );
		nFrames = nFramesNew;

		double fSeconds = 1.0 * (end - start) / CLOCKS_PER_SEC;

		times.push_back( end - start );
	}

	double fRMS, fMean;
	QString sTimes = showTimes( times, nFrames * 5, &fMean, &fRMS );
	out << "Sample rate " << nSampleRate;
	if ( nSampleRate != 44100 ) {
		out << " (" + Interpolation::ModeToQString( interpolateMode ) + ")";
	}
	out << " times: " << sTimes;
	if ( fReference != 0.0 ) {
		double fDelta = 100.0 * ( fMean - fReference) / fReference;
		out << " (" << ( ( fDelta >= 0 ) ? "+" : "" ) << fDelta << "%)";
	}
	out << Qt::endl;

	pHydrogen->getAudioEngine()->getSampler()->setInterpolateMode( oldInterpolateMode );

	Filesystem::rm( outFile );

	if ( pfRMS ) {
		*pfRMS = fRMS;
	}
	return fMean;
}

void AudioBenchmark::audioBenchmark(void)
{
	if ( !bEnabled ) {
		return;
	}

	___INFOLOG( "" );
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	out << "Benchmark ADSR method:" << Qt::endl;
	timeADSR();

	auto songFile = H2TEST_FILE("functional/test.h2song");
	auto songADSRFile = H2TEST_FILE("functional/test_adsr.h2song");

	/* Load song and prepare */
	std::shared_ptr<Song> pSong = Song::load( songFile );
	ASSERT_SONG( pSong );

	pHydrogen->setSong( pSong );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get(i)->setCurrentlyExported( true );
	}

	out << "\n=== Audio engine benchmark ===" << Qt::endl;

	double fRef = timeExport( 44100, Interpolation::InterpolateMode::Linear );
	timeExport( 44101, Interpolation::InterpolateMode::Linear, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Cosine, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Third, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Cubic, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Hermite, fRef );

	out << "Now with ADSR" << Qt::endl;
	pSong = Song::load( songADSRFile );
	ASSERT_SONG( pSong );

	pHydrogen->setSong( pSong );
	pInstrumentList = pSong->getDrumkit()->getInstruments();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get(i)->setCurrentlyExported( true );
	}


	timeExport( 44100, Interpolation::InterpolateMode::Linear, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Linear, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Cosine, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Third, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Cubic, fRef );
	timeExport( 44101, Interpolation::InterpolateMode::Hermite, fRef );

	out << "---" << Qt::endl;
	___INFOLOG( "passed" );
}
