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
#include <core/Basics/Adsr.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Playlist.h>
#include <core/Smf/SMF.h>
#include "TestHelper.h"
#include "assertions/File.h"
#include "assertions/AudioFile.h"
#include "AudioBenchmark.h"

#include <chrono>
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

	long long nStartFrames = pHydrogen->getAudioEngine()->getFrames();

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

	return pHydrogen->getAudioEngine()->getFrames() - nStartFrames;
}

void AudioBenchmark::audioBenchmark(void)
{
	if ( !bEnabled ) {
		return;
	}
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	EventQueue *pQueue = EventQueue::get_instance();

	auto songFile = H2TEST_FILE("functional/test.h2song");
	auto outFile = Filesystem::tmp_file_path("test.wav");

	/* Load song and prepare */
	std::shared_ptr<Song> pSong = Song::load( songFile );
	CPPUNIT_ASSERT( pSong != nullptr );

	if( !pSong ) {
		return;
	}

	pHydrogen->setSong( pSong );

	InstrumentList *pInstrumentList = pSong->getInstrumentList();
	for (auto i = 0; i < pInstrumentList->size(); i++) {
		pInstrumentList->get(i)->set_currently_exported( true );
	}

	qDebug() << "\n=== Audio engine benchmark ===";
	int nIterations = 10;
	std::vector< clock_t > times;
	long long nFrames = 0, nFramesNew;

	// Run through once to warm caches etc.
	exportCurrentSong( outFile, 44100 );
	
	for ( int i = 0; i < nIterations; i++ ) {

		nFramesNew = 0;
		std::clock_t start = std::clock();
		for ( int j = 0; j < 10; j++) {
			nFramesNew += exportCurrentSong( outFile, 44100 );
		}
		std::clock_t end = std::clock();

		CPPUNIT_ASSERT( nFramesNew == nFrames || nFrames == 0 );
		nFrames = nFramesNew;

		qDebug() << nFramesNew << "frames in" << ((end - start) * 1.0 / CLOCKS_PER_SEC);
		times.push_back( end - start );
	}

	qDebug() << "nFrames = " << nFrames;

	double fTotalTime = 0;
	for ( auto t : times ) {
		double fSeconds = 1.0 * t / CLOCKS_PER_SEC;
		fTotalTime += fSeconds;
	}
	double fMean = fTotalTime / times.size();
 
	double fTotalError = 0;
	for ( auto t : times ) {
		double fSeconds = 1.0 * t / CLOCKS_PER_SEC;
		double fError = fMean - fSeconds;
		fTotalError += fError * fError;
		qDebug() << fTotalError;
	}
	double fRMS = sqrt( fTotalError );

	qDebug() << "Mean time is " << fMean << "+/-" << 100.0 * fRMS / fMean << "%";

	Filesystem::rm( outFile );
}
