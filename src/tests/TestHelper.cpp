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

#include "TestHelper.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Song.h>
#include <core/config.h>
#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/LoopBackMidiDriver.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <chrono>
#include <exception>
#include <random>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <thread>

#include <cppunit/extensions/HelperMacros.h>

TestHelper* TestHelper::m_pInstance = nullptr;
QString TestHelper::sRootDir = CMAKE_SOURCE_DIR;

void TestHelper::createInstance( bool bAppveyor )
{
	if ( m_pInstance == nullptr ) {
		m_pInstance = new TestHelper;
		m_pInstance->m_bAppveyor = bAppveyor;
	}
}

TestHelper::TestHelper()
{
	m_sDataDir = sRootDir + "/data/";
	m_sTestDataDir = sRootDir + "/src/tests/data/";
}

QStringList TestHelper::findDrumkitBackupFiles( const QString& sDir ) const {

	QStringList results;

	if ( ! H2Core::Filesystem::dir_readable( sDir, false ) ){
		// Error messages handled in dir_reabable.

		return results;
	}
	QDir dir( sDir );

	QStringList nameFilters;
	nameFilters << H2Core::Filesystem::drumkit_xml() + "*" + ".bak";

	for ( const auto& ssFile : dir.entryList( nameFilters,
											  QDir::Files ) ) {
		results << sDir + "/" + ssFile;
	}

	return results;
}

void TestHelper::varyAudioDriverConfig( int nIndex ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	switch( nIndex ) {
	case 0:
		pPref->m_nBufferSize = 1024;
		pPref->m_nSampleRate = 44100;
		break;

	case 1:
		pPref->m_nBufferSize = 256;
		pPref->m_nSampleRate = 44100;
		break;

	case 2:
		pPref->m_nBufferSize = 512;
		pPref->m_nSampleRate = 44100;
		break;

	case 3:
		pPref->m_nBufferSize = 128;
		pPref->m_nSampleRate = 48000;
		break;

	case 4:
		pPref->m_nBufferSize = 512;
		pPref->m_nSampleRate = 48000;
		break;

	case 5:
		pPref->m_nBufferSize = 1024;
		pPref->m_nSampleRate = 96000;
		break;

	case 6:
		pPref->m_nBufferSize = 2048;
		pPref->m_nSampleRate = 96000;
		break;

	case 7:
		pPref->m_nBufferSize = 500;
		pPref->m_nSampleRate = 44100;
		break;

	case 8:
		pPref->m_nBufferSize = 500;
		pPref->m_nSampleRate = 36000;
		break;
		
	case 9:
		pPref->m_nBufferSize = 5000;
		pPref->m_nSampleRate = 1024;
		break;

	default:
		// Seed with a real random value, if available
		std::random_device randomSeed;

		std::default_random_engine randomEngine( randomSeed() );
		// Too small values make the unit tests run way too slow. Too
		// large ones (10000) cause a segfault. (The latter might be a
		// bug but such buffer sizes work fine with other drivers).
		std::uniform_int_distribution<int> bufferDist( 256, 5000 );
		std::uniform_int_distribution<int> sampleRateDist( 22050, 192000 );

		pPref->m_nBufferSize = bufferDist( randomEngine );
		pPref->m_nSampleRate = sampleRateDist( randomEngine );
	}

	___INFOLOG( QString( "New bufferSize: %1, new sampleRate: %2" )
				.arg( pPref->m_nBufferSize ).arg( pPref->m_nSampleRate ) );

	H2Core::Hydrogen::get_instance()->restartAudioDriver();
}

void TestHelper::exportSong( const QString& sSongFile, const QString& sFileName,
							 int nSampleRate, int nSampleDepth,
							 double fCompressionLevel )
{
	___INFOLOG( QString( "sSongFile: %1, sFileName: %2, nSampleRate: %3, nSampleDepth: %4, fCompressionLevel: %5" )
				.arg( sSongFile ).arg( sFileName ).arg( nSampleRate )
				.arg( nSampleDepth ).arg( fCompressionLevel ) );

	auto t0 = std::chrono::high_resolution_clock::now();

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = H2Core::Song::load( sSongFile );
	CPPUNIT_ASSERT( pSong != nullptr );
		
	pHydrogen->setSong( pSong );

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	for (auto i = 0; i < pInstrumentList->size(); i++) {
		pInstrumentList->get(i)->setCurrentlyExported( true );
	}

	pHydrogen->startExportSession( nSampleRate, nSampleDepth, fCompressionLevel );
	pHydrogen->startExportSong( sFileName );

	auto pDriver = std::dynamic_pointer_cast<H2Core::DiskWriterDriver>(
		pHydrogen->getAudioDriver()
	);
	CPPUNIT_ASSERT( pDriver != nullptr );

	// in 0.1 * `nMaxSleeps` ms
	const int nMaxSleeps = 3000;
	int nSleeps = 0;
	while ( ! pDriver->isDoneWriting() ) {
		usleep(100 * 1000); // 0.1 ms

		// Export should not take that long. There is somethings wrong in
		// here.
		CPPUNIT_ASSERT( nSleeps < nMaxSleeps );
		nSleeps++;
	}

	CPPUNIT_ASSERT( ! pDriver->writingFailed() );

	pHydrogen->stopExportSession();

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("Audio export [%1 | %2] took [%3] seconds")
				.arg( nSampleRate ).arg( nSampleDepth ).arg( t ) );
}

void TestHelper::exportSong( const QString& sFileName )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	for (auto i = 0; i < pInstrumentList->size(); i++) {
		pInstrumentList->get(i)->setCurrentlyExported( true );
	}

	pHydrogen->startExportSession( 44100, 16, 1.0 );
	pHydrogen->startExportSong( sFileName );

	auto pDriver = std::dynamic_pointer_cast<H2Core::DiskWriterDriver>(
		pHydrogen->getAudioDriver()
	);
	CPPUNIT_ASSERT( pDriver != nullptr );

	const int nMaxSleeps = 30;
	int nSleeps = 0;
	while ( ! pDriver->isDoneWriting() ) {
		usleep(100 * 1000);

		// Export should not take that long. There is somethings wrong in
		// here.
		CPPUNIT_ASSERT( nSleeps < nMaxSleeps );
		nSleeps++;
	}
	pHydrogen->stopExportSession();

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("Audio export took %1 seconds").arg(t) );
}

void TestHelper::exportMIDI( const QString& sSongFile, const QString& sFileName,
							 std::shared_ptr<H2Core::SMFWriter> pWriter,
							 bool bUseHumanization )
{
	auto t0 = std::chrono::high_resolution_clock::now();

	auto pSong = H2Core::Song::load( sSongFile );
	CPPUNIT_ASSERT( pSong != nullptr );
	H2Core::CoreActionController::setSong( pSong );

	pWriter->save( sFileName, pSong, bUseHumanization );

	auto t1 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double>( t1 - t0 ).count();
	___INFOLOG( QString("MIDI track export took %1 seconds").arg(t) );
}

void TestHelper::waitForAudioDriver() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	pAudioEngine->lock( RIGHT_HERE );
	const auto nOldRealtimeFrame = pAudioEngine->getRealtimeFrame();
	pAudioEngine->unlock();

	int nNewRealtimeFrame;
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( nnTry < nMaxTries ) {
		pAudioEngine->lock( RIGHT_HERE );
		nNewRealtimeFrame = pAudioEngine->getRealtimeFrame();
		pAudioEngine->unlock();

		if ( nNewRealtimeFrame != nOldRealtimeFrame ) {
			break;
		}

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
	CPPUNIT_ASSERT( nnTry < nMaxTries );
}

void TestHelper::waitForMidiDriver() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	CPPUNIT_ASSERT( pAudioEngine->getMidiDriver() != nullptr );

	auto pDriver = std::dynamic_pointer_cast<H2Core::LoopBackMidiDriver>(
		pAudioEngine->getMidiDriver()
	);
	CPPUNIT_ASSERT( pDriver != nullptr );

	// Wait till the LoopBackMidiDriver did send, receive, and handle the
	// message.
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( pDriver->getBacklogMessages().size() == 0 ) {
		CPPUNIT_ASSERT( nnTry < nMaxTries );

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
	}
}

void TestHelper::waitForMidiActionManagerWorkerThread() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();

	// Since incoming MIDI events are handled asynchronously, we pause execution
	// till all are handled.
	const int nMaxTries = 100;
	int nnTry = 0;
	while ( nnTry < nMaxTries ) {
		if ( pMidiActionManager->getActionQueueSize() == 0 ) {
			break;
		}

		++nnTry;
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
	CPPUNIT_ASSERT( nnTry < nMaxTries );
}
