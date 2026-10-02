/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "SoundLibraryTest.h"
#include "TestHelper.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

void SoundLibraryTest::testContextValidity() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();
	for ( const auto& [ _, ppDrumkit ]: pDB->getDrumkitDatabase() ) {
		CPPUNIT_ASSERT( ppDrumkit != nullptr );
		CPPUNIT_ASSERT(
			ppDrumkit->getContext() != H2Core::Filesystem::Context::Song
		);
	}

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testKitRetrievalCopy() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	const float fNewGainValue = 1.23456;

	auto pKit = pDB->getDrumkit( pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit"
	) );
	CPPUNIT_ASSERT( pKit != nullptr );

	auto pKitCopy = std::make_shared<H2Core::Drumkit>( pKit );

	pKitCopy->getInstruments()->get( 0 )->setGain( fNewGainValue );

	auto pKitAgain = pDB->getDrumkit( pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit"
	) );
	CPPUNIT_ASSERT( pKitAgain != nullptr );
	CPPUNIT_ASSERT(
		pKitAgain->getInstruments()->get( 0 )->getGain() != fNewGainValue
	);

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testKitRetrievalDirect() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	const float fNewGainValue = 1.23456;

	auto pKit = pDB->getDrumkit( pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit"
	) );
	CPPUNIT_ASSERT( pKit != nullptr );

	const float fOldValue = pKit->getInstruments()->get( 0 )->getGain();
	pKit->getInstruments()->get( 0 )->setGain( fNewGainValue );

	auto pKitAgain = pDB->getDrumkit( pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit"
	) );
	CPPUNIT_ASSERT( pKitAgain != nullptr );
	CPPUNIT_ASSERT( pKitAgain->getInstruments()->get( 0 )->getGain() ==
					fNewGainValue );

	pKit->getInstruments()->get( 0 )->setGain( fOldValue );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testSnapshotStableAcrossUpdate() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	const auto pSnapshotBefore = pDB->getSnapshot();
	CPPUNIT_ASSERT( pSnapshotBefore != nullptr );
	CPPUNIT_ASSERT( ! pSnapshotBefore->drumkitDatabase.empty() );

	// Kits of the System context are shipped and immutable: they must
	// survive every rescan.
	QStringList systemKitPaths;
	for ( const auto& [ sPath, ppDrumkit ]:
		  pSnapshotBefore->drumkitDatabase ) {
		if ( ppDrumkit != nullptr &&
			 ppDrumkit->getContext() ==
				 H2Core::Filesystem::Context::System ) {
			systemKitPaths << sPath;
		}
	}
	CPPUNIT_ASSERT( ! systemKitPaths.isEmpty() );

	// All kit paths of the held snapshot, captured before the update.
	QStringList allKitPaths;
	for ( const auto& [ sPath, _ ]: pSnapshotBefore->drumkitDatabase ) {
		allKitPaths << sPath;
	}

	pDB->update();

	// The held snapshot must be unaffected by the publication: it
	// still holds exactly the kits it held before. A publication swaps
	// in a new snapshot instead of mutating held ones.
	CPPUNIT_ASSERT( pSnapshotBefore->drumkitDatabase.size() ==
					static_cast<std::size_t>( allKitPaths.size() ) );
	for ( const auto& sPath : allKitPaths ) {
		CPPUNIT_ASSERT( pSnapshotBefore->drumkitDatabase.find( sPath ) !=
						pSnapshotBefore->drumkitDatabase.end() );
	}
	for ( const auto& [ _, ppDrumkit ]: pSnapshotBefore->drumkitDatabase ) {
		CPPUNIT_ASSERT( ppDrumkit != nullptr );
	}

	// The update published a fresh, usable snapshot.
	const auto pSnapshotAfter = pDB->getSnapshot();
	CPPUNIT_ASSERT( pSnapshotAfter != nullptr );
	CPPUNIT_ASSERT( pSnapshotAfter != pSnapshotBefore );
	CPPUNIT_ASSERT( ! pSnapshotAfter->drumkitDatabase.empty() );
	for ( const auto& sPath : systemKitPaths ) {
		CPPUNIT_ASSERT(
			pSnapshotAfter->drumkitDatabase.find( sPath ) !=
			pSnapshotAfter->drumkitDatabase.end()
		);
	}

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testCustomDrumkitPathSurvivesUpdate() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// A registered custom drumkit path must survive a full rescan: the
	// scan rebuilds the database content from disk but must not lose
	// the registration.
	const QString sKitPath = H2TEST_FILE( "drumkits/baseKit/drumkit.xml" );
	pDB->registerCustomDrumkitPath( sKitPath );
	CPPUNIT_ASSERT( pDB->getCustomDrumkitPaths().contains( sKitPath ) );

	pDB->update();

	CPPUNIT_ASSERT( pDB->getCustomDrumkitPaths().contains( sKitPath ) );

	// The scan picked the registered path up and loaded the kit.
	const auto pSnapshot = pDB->getSnapshot();
	const auto foundKit = pSnapshot->drumkitDatabase.find( sKitPath );
	CPPUNIT_ASSERT( foundKit != pSnapshot->drumkitDatabase.end() );
	CPPUNIT_ASSERT( foundKit->second != nullptr );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testGetDrumkitPublishesSnapshot() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// A kit outside the scanned contexts is not part of the database.
	const QString sKitPath = H2TEST_FILE( "drumkits/invAdsrKit/drumkit.xml" );
	const auto pSnapshotBefore = pDB->getSnapshot();
	CPPUNIT_ASSERT( pSnapshotBefore->drumkitDatabase.find( sKitPath ) ==
					pSnapshotBefore->drumkitDatabase.end() );

	// Retrieving it lazily loads the kit and publishes a new snapshot
	// containing it.
	auto pDrumkit = pDB->getDrumkit( sKitPath );
	CPPUNIT_ASSERT( pDrumkit != nullptr );

	const auto pSnapshotAfter = pDB->getSnapshot();
	const auto foundKit = pSnapshotAfter->drumkitDatabase.find( sKitPath );
	CPPUNIT_ASSERT( foundKit != pSnapshotAfter->drumkitDatabase.end() );
	CPPUNIT_ASSERT( foundKit->second == pDrumkit );

	// The previously held snapshot is unaffected by the publication.
	CPPUNIT_ASSERT( pSnapshotBefore->drumkitDatabase.find( sKitPath ) ==
					pSnapshotBefore->drumkitDatabase.end() );
	CPPUNIT_ASSERT( pSnapshotBefore != pSnapshotAfter );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testFindArtifactPatternDoesNotReturnSong() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// 'GM kit demo #1' is a demo song shipped in the System context.
	// There is no pattern of the same name and a Pattern lookup must not
	// fall through into the Song branch and return the song's path
	// instead.
	CPPUNIT_ASSERT( pDB->findArtifact(
		H2Core::Filesystem::Artifact::Pattern,
		H2Core::Filesystem::Context::System, "GM kit demo #1"
	).isEmpty() );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testFindArtifactStackedFindsLaterContext() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// 'Pop 5' is a pattern shipped in the System context. A stacked
	// lookup starting at the User context has to find it through the
	// cached first pass.
	const QString sExpectedPath = pDB->findArtifact(
		H2Core::Filesystem::Artifact::Pattern,
		H2Core::Filesystem::Context::System, "Pop 5"
	);
	CPPUNIT_ASSERT( ! sExpectedPath.isEmpty() );

	CPPUNIT_ASSERT( pDB->findArtifact(
		H2Core::Filesystem::Artifact::Pattern,
		H2Core::Filesystem::Context::User, "Pop 5", true
	) == sExpectedPath );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testFindArtifactStackedSkipsNonMatching() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// No pattern named 'GM kit demo #1' exists in any context. In a
	// stacked lookup non-matching artifacts must not be cached for a
	// later pass and leak into its result.
	CPPUNIT_ASSERT( pDB->findArtifact(
		H2Core::Filesystem::Artifact::Pattern,
		H2Core::Filesystem::Context::User, "GM kit demo #1", true
	).isEmpty() );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testGetDrumkitCanonicalizesPath() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// 'GMRockKit' is shipped in the System context.
	const QString sKitPath = pDB->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit" );
	CPPUNIT_ASSERT( ! sKitPath.isEmpty() );

	auto pKit = pDB->getDrumkit( sKitPath );
	CPPUNIT_ASSERT( pKit != nullptr );

	// A folder spelling and redundant separators address the same
	// cached kit instead of missing it and lazily registering
	// duplicates of it.
	const QString sFolderPath =
		H2Core::Filesystem::drumkitDirFromPath( sKitPath );
	CPPUNIT_ASSERT( pDB->getDrumkit( sFolderPath ) == pKit );
	CPPUNIT_ASSERT(
		pDB->getDrumkit( sFolderPath + "//drumkit.xml" ) == pKit );

	const QStringList customPaths = pDB->getCustomDrumkitPaths();
	CPPUNIT_ASSERT( ! customPaths.contains( sFolderPath ) );
	CPPUNIT_ASSERT( ! customPaths.contains( sFolderPath + "//drumkit.xml" ) );

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testGetDrumkitUpgradeParameter() {
	___INFOLOG( "" );

	auto pDB = pTestHydrogen()->getSoundLibraryDatabase();

	// The legacy kit fixture has no <formatVersion> and thus triggers
	// the legacy handling of Drumkit::load(). An upgrade rewrites the
	// drumkit.xml on disk (after backing it up) and must only happen
	// on explicit request.
	const QString sLegacyKitPath =
		H2TEST_FILE( "drumkits/legacy_GMkit/drumkit.xml" );

	for ( const auto& bUpgrade : { true, false } ) {
		// A writable copy: the fixture itself must not be modified.
		QTemporaryDir tempDir;
		CPPUNIT_ASSERT( tempDir.isValid() );
		const QString sKitPath =
			QDir( tempDir.path() ).filePath( "drumkit.xml" );
		CPPUNIT_ASSERT( H2Core::Filesystem::fileCopy(
			sLegacyKitPath, sKitPath, false, true ) );

		const auto pDrumkit = pDB->getDrumkit( sKitPath, bUpgrade );
		CPPUNIT_ASSERT( pDrumkit != nullptr );

		// An upgrade leaves a backup next to the re-saved
		// drumkit.xml; an opt-out leaves no trace.
		const QStringList backups = QDir( tempDir.path() ).entryList(
			QStringList() << "drumkit.xml.*.bak" );
		CPPUNIT_ASSERT( backups.size() == ( bUpgrade ? 1 : 0 ) );

		___INFOLOG( QString( "bUpgrade=%1 passed" )
					 .arg( bUpgrade ? "true" : "false" ) );
	}

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testInitialScanCompletes()
{
	___INFOLOG( "" );
	// A fresh engine: its initial sound library scan runs in the
	// background (started at the end of Hydrogen's constructor), so the
	// database content is only complete once waitForInitialScan()
	// returns.
	auto pHydrogen = TestHelper::makeEngine();

	pHydrogen->getSoundLibraryDatabase()->waitForInitialScan();

	// The wait is over: no scan is running anymore and the database is
	// populated with the shipped content.
	CPPUNIT_ASSERT( ! pHydrogen->getSoundLibraryDatabase()
					  ->isInitialScanRunning() );
	CPPUNIT_ASSERT( ! pHydrogen->getSoundLibraryDatabase()->findArtifact(
		H2Core::Filesystem::Artifact::DrumkitExtracted,
		H2Core::Filesystem::Context::System, "GMRockKit" ).isEmpty() );
	CPPUNIT_ASSERT( pHydrogen->getSoundLibraryDatabase()
					  ->getPatternInfos().size() > 0 );
	CPPUNIT_ASSERT( pHydrogen->getSoundLibraryDatabase()
					  ->getSongInfos().size() > 0 );

	delete pHydrogen;

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testInitialScanProgressEvents()
{
	___INFOLOG( "" );
	// The scan reports its progress as SoundLibraryScanProgress
	// events (0-100, throttled to 5% steps). The background scan's own
	// reports race the engine leaving its constructor (events pushed
	// before setFullyOperational() are dropped by the queue), so the
	// throttle contract is asserted on a synchronous update() with a
	// local progress reporter instead.
	auto pHydrogen = TestHelper::makeEngine();

	pHydrogen->getSoundLibraryDatabase()->waitForInitialScan();
	while ( pHydrogen->getEventQueue()->popEvent() != nullptr ) {}

	H2Core::SoundLibraryDatabase::ScanProgress progress( pHydrogen );
	pHydrogen->getSoundLibraryDatabase()->update( &progress );

	std::vector<int> values;
	while ( auto pEvent = pHydrogen->getEventQueue()->popEvent() ) {
		if ( pEvent->getType() ==
			 H2Core::Event::Type::SoundLibraryScanProgress ) {
			values.push_back( pEvent->getValue() );
		}
	}

	// The scan ran to completion: the phase reports up to the final
	// 100% were queued.
	CPPUNIT_ASSERT( values.size() >= 2 );
	for ( const auto& nnValue : values ) {
		CPPUNIT_ASSERT( nnValue >= 0 && nnValue <= 100 );
	}
	// Monotonic: the throttle only reports forward.
	for ( std::size_t ii = 1; ii < values.size(); ++ii ) {
		CPPUNIT_ASSERT( values[ ii ] >= values[ ii - 1 ] );
	}
	CPPUNIT_ASSERT( values.back() == 100 );
	// 5% throttle: at most the 21 full percentage steps are reported.
	CPPUNIT_ASSERT( values.size() <= 21 );

	delete pHydrogen;

	___INFOLOG( "passed" );
}

void SoundLibraryTest::testConcurrentSectionScansSerialize()
{
	___INFOLOG( "" );
	// ADR 0034: section scans serialize. Each updateX() publishes a
	// copy of the snapshot it took on entry, so a second scan
	// overlapping a parked first one would drop the first one's
	// section: the second scan's entry copy predates the first scan's
	// publish. Here a drumkit scan is parked mid-way (through its
	// progress hook) while a second kit lands on disk; a second
	// drumkit scan must hold off until the parked one is done, or the
	// parked scan's stale publish drops the new kit.
	auto pHydrogen = TestHelper::makeEngine();
	pHydrogen->getSoundLibraryDatabase()->waitForInitialScan();

	const QString sTestDataDir = TestHelper::get_instance()->getTestDataDir();
	const QString sKitDir = QDir::cleanPath(
		H2Core::Filesystem::userDrumkitsDir() + QString( "/serialize-kit-%1" )
			.arg( QCoreApplication::applicationPid() ) );

	// Parks the scan between its entry copy and its publish: the
	// first report fires after the listing, during the first kit
	// load.
	std::promise<void> parked;
	std::promise<void> release;
	auto fParked = parked.get_future();
	auto fRelease = release.get_future().share();
	class ParkingProgress
		: public H2Core::SoundLibraryDatabase::ScanProgress
	{
	public:
		ParkingProgress( H2Core::Hydrogen* pH2, std::promise<void> parked,
						 std::shared_future<void> fRelease )
			: ScanProgress( pH2 )
			, m_parked( std::move( parked ) )
			, m_fRelease( std::move( fRelease ) ) {}
		void report( int ) override {
			if ( m_bReported.exchange( true ) ) {
				return;
			}
			m_parked.set_value();
			m_fRelease.get();
		}
	private:
		std::promise<void> m_parked;
		std::shared_future<void> m_fRelease;
		std::atomic<bool> m_bReported{ false };
	};
	ParkingProgress parking( pHydrogen, std::move( parked ), fRelease );

	auto scanA = std::thread( [ & ]() {
		pHydrogen->getSoundLibraryDatabase()->updateDrumkits(
			H2Core::Event::Trigger::Suppress, &parking );
	} );
	// The scan thread now sits between its entry copy and its publish.
	fParked.get();

	// The kit lands behind the back of the parked scan's listing.
	CPPUNIT_ASSERT( QDir().mkpath( sKitDir ) );
	for ( const auto& sFile :
		  QDir( sTestDataDir + "/drumkits/baseKit" ).entryList(
			  QDir::Files ) ) {
		CPPUNIT_ASSERT( QFile::copy( sTestDataDir + "/drumkits/baseKit/" +
									 sFile, sKitDir + "/" + sFile ) );
	}

	const auto fKitKnown = [ & ]() {
		return pHydrogen->getSoundLibraryDatabase()->getDrumkitDatabase()
			.count( H2Core::Filesystem::drumkitPathFromDir( sKitDir ) ) > 0;
	};

	auto scanB = std::thread( [ & ]() {
		pHydrogen->getSoundLibraryDatabase()->updateDrumkits(
			H2Core::Event::Trigger::Suppress, nullptr );
	} );

	// Observation only — an assert unwinding past the joinable scan
	// threads would terminate the whole suite.
	bool bPublishedWhileParked = false;
	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::milliseconds( 1000 );
	while ( std::chrono::steady_clock::now() < deadline ) {
		if ( fKitKnown() ) {
			bPublishedWhileParked = true;
			break;
		}
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}

	release.set_value();
	scanA.join();
	scanB.join();

	// The second scan must not publish while the first one is parked.
	CPPUNIT_ASSERT( ! bPublishedWhileParked );
	// Serialized, the second scan published the kit after the first.
	CPPUNIT_ASSERT( fKitKnown() );

	delete pHydrogen;

	___INFOLOG( "passed" );
}
