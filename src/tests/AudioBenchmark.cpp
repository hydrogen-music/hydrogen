/*
 * Hydrogen
 * Copyright(c) 2022 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/PatternList.h>
#include "TestHelper.h"
#include "AudioBenchmark.h"

#include <memory>
#include <ctime>

using namespace H2Core;

bool AudioBenchmark::bEnabled = false;

CPPUNIT_TEST_SUITE_REGISTRATION( AudioBenchmark );


static long long exportCurrentSong( const QString &fileName, int nSampleRate )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	EventQueue *pQueue = EventQueue::get_instance();
	auto pSong = pHydrogen->getSong();

	if( !pSong ) {
		return 0;
	}

	pHydrogen->startExportSession( nSampleRate, 16 );
	pHydrogen->startExportSong( fileName );

	long long nStartFrame = pHydrogen->getAudioEngine()->getTransportPosition()->getFrame();

	bool done = false;
	while ( ! done ) {
		Event event = pQueue->pop_event();
		
		if (event.type == EVENT_PROGRESS && event.value == 100) {
			done = true;
		}
		else if ( event.type == EVENT_NONE ) {
			usleep(10 * 1000);
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

static QString showTimes( std::vector< clock_t > &times, int nFrames ) {
	double fTotal = std::accumulate(times.begin(), times.end(), 0) * 1.0 / CLOCKS_PER_SEC;
	double fMean = fTotal / times.size();

	double fTotalError = 0;
	for ( auto t : times ) {
		double fSeconds = 1.0 * t / CLOCKS_PER_SEC;
		double fError = fMean - fSeconds;
		fTotalError += fError * fError;
	}
	double fRMS = sqrt( fTotalError ) / times.size();

	return QString( "%1s (%2 frames/sec) +/- %3%" )
		.arg( showNumber( fMean ) )
		.arg( showNumber( nFrames * 1.0 / fMean) )
		.arg( 100.0 * fRMS / fMean, 0, 'f', 3 );
}

static void timeADSR() {
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

	qDebug() << "ADSR time: " << showTimes( times, nFrames );
}

static void timeExport( int nSampleRate ) {
	auto outFile = Filesystem::tmp_file_path("test.wav");
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nIterations = 32;
	std::vector< clock_t > times;
	long long nFrames = 0, nFramesNew;

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

	qDebug() << "Sample rate " << nSampleRate << " times: " << showTimes( times, nFrames * 5 );

	Filesystem::rm( outFile );

}

void AudioBenchmark::audioBenchmark(void)
{
	if ( !bEnabled ) {
		return;
	}
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	qDebug() << "Benchmark ADSR method:";
	timeADSR();

	auto songFile = H2TEST_FILE("functional/test.h2song");
	auto songADSRFile = H2TEST_FILE("functional/test_adsr.h2song");

	/* Load song and prepare */
	std::shared_ptr<Song> pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != nullptr );

	if ( !pSong ) {
		return;
	}

	pHydrogen->setSong( pSong );

	auto pInstrumentList = pSong->getInstrumentList();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get(i)->set_currently_exported( true );
	}

	qDebug() << "\n=== Audio engine benchmark ===";

	timeExport( 44100 );
	timeExport( 48000 );


	qDebug() << "Now with ADSR";
	pSong = Song::load( songADSRFile );
	CPPUNIT_ASSERT( pSong != nullptr );

	if( !pSong ) {
		return;
	}

	pHydrogen->setSong( pSong );
	pInstrumentList = pSong->getInstrumentList();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get(i)->set_currently_exported( true );
	}


	timeExport( 44100 );
	timeExport( 48000 );

	qDebug() << "---";
}
