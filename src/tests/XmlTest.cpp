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

#include "XmlTest.h"

#include "TestHelper.h"
#include "assertions/File.h"

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Sample.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Helpers/Xml.h>
#include <core/Preferences/Preferences.h>
#include <core/License.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QDir>
#include <QTemporaryDir>
#include <QTime>
#include <QTextStream>

void XmlTest::setUp() {
	// Test for possible side effects by comparing serializations
	m_sPrefPre =
		H2Core::Preferences::get_instance()->toQString( "", true );
	m_sHydrogenPre = H2Core::Hydrogen::get_instance()->toQString( "", true );

}

void XmlTest::tearDown() {

	QDirIterator it( TestHelper::get_instance()->getTestDataDir(),
					 QDirIterator::Subdirectories);
	QStringList filters;
	filters << "*.bak*";

	while ( it.hasNext() ) {
		it.next();
		const QDir testFolder( it.next() );
		const QStringList backupFiles = testFolder.entryList( filters, QDir::NoFilter, QDir::NoSort );

		for ( auto& bbackupFile : backupFiles ) {

			H2Core::Filesystem::rm( testFolder.absolutePath()
									.append( "/" )
									.append( bbackupFile ), false );
		}
	}

	CPPUNIT_ASSERT( m_sPrefPre ==
					H2Core::Preferences::get_instance()->toQString( "", true ) );
	// CPPUNIT_ASSERT( m_sHydrogenPre ==
	// 				H2Core::Hydrogen::get_instance()->toQString( "", true ) );
}

////////////////////////////////////////////////////////////////////////////////

void XmlTest::testDrumkitFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFolder = H2TEST_FILE( "/drumkits/format-integrity/");
	const auto pDrumkit = H2Core::Drumkit::load(
		sTestFolder, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkit != nullptr );

	const QString sTmpDrumkitXml =
		H2Core::Filesystem::tmp_file_path( "drumkit-format-integrity.xml" );

	// We just store the definition. Saving the whole kit is tested in another
	// function.
	H2Core::XMLDoc doc;
	H2Core::XMLNode root = doc.set_root( "drumkit_info", "drumkit" );
	pDrumkit->saveTo( root, false, false );

	CPPUNIT_ASSERT( doc.write( sTmpDrumkitXml ) );

	H2TEST_ASSERT_DRUMKIT_FILES_EQUAL(
		H2Core::Filesystem::drumkit_file( sTestFolder ), sTmpDrumkitXml );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpDrumkitXml ) );
	___INFOLOG( "passed" );
}

void XmlTest::testDrumkit()
{
	___INFOLOG( "" );
	QString sDrumkitPath = H2Core::Filesystem::tmp_dir()+"dk0";

	std::shared_ptr<H2Core::Drumkit> pDrumkitLoaded = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitReloaded = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitCopied = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkitNew = nullptr;
	H2Core::XMLDoc doc;

	QFileInfo info( H2Core::Filesystem::drumkit_file(
						H2TEST_FILE( "/drumkits/baseKit") ) );
	const auto timestampStart = info.lastModified();

	// load without samples
	pDrumkitLoaded = H2Core::Drumkit::load(
		H2TEST_FILE( "/drumkits/baseKit"), false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->areSamplesLoaded()==false );
	CPPUNIT_ASSERT( checkSampleData( pDrumkitLoaded, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, pDrumkitLoaded->getInstruments()->size() );

	// Check for side effect in the file read.
	info.refresh();
	const auto timestampLoaded = info.lastModified();
	CPPUNIT_ASSERT( timestampLoaded == timestampStart );

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( TestHelper::get_instance()->findDrumkitBackupFiles(
						H2TEST_FILE( "drumkits/baseKit/" ) ).size() == 0 );
	
	// manually load samples
	pDrumkitLoaded->loadSamples();
	CPPUNIT_ASSERT( pDrumkitLoaded->areSamplesLoaded()==true );
	CPPUNIT_ASSERT( checkSampleData( pDrumkitLoaded, true ) );

	info.refresh();
	const auto timestampSamplesLoaded = info.lastModified();
	CPPUNIT_ASSERT( timestampSamplesLoaded == timestampStart );

	pDrumkitLoaded = nullptr;
	
	// load with samples
	pDrumkitLoaded = H2Core::Drumkit::load(
		H2TEST_FILE( "/drumkits/baseKit" ), false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );

	pDrumkitLoaded->loadSamples();
	CPPUNIT_ASSERT( pDrumkitLoaded->areSamplesLoaded()==true );
	CPPUNIT_ASSERT( checkSampleData( pDrumkitLoaded, true ) );
	
	// unload samples
	pDrumkitLoaded->unloadSamples();
	CPPUNIT_ASSERT( pDrumkitLoaded->areSamplesLoaded()==false );
	CPPUNIT_ASSERT( checkSampleData( pDrumkitLoaded, false ) );

	info.refresh();
	const auto timestampPreSave = info.lastModified();
	CPPUNIT_ASSERT( timestampPreSave == timestampStart );
	
	// save drumkit elsewhere
	pDrumkitLoaded->setName( "pDrumkitLoaded" );
	CPPUNIT_ASSERT( pDrumkitLoaded->save( sDrumkitPath ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/snare.wav" ) );

	info.refresh();
	const auto timestampSave = info.lastModified();
	CPPUNIT_ASSERT( timestampSave == timestampStart );

	// Check whether the generated drumkit is valid.
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ) ) );
	
	// load file
	pDrumkitReloaded = H2Core::Drumkit::load(
		sDrumkitPath, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitReloaded!=nullptr );

	info.refresh();
	const auto timestampReloaded = info.lastModified();
	CPPUNIT_ASSERT( timestampReloaded == timestampStart );
	
	// copy constructor
	pDrumkitCopied = std::make_shared<H2Core::Drumkit>( pDrumkitReloaded );
	CPPUNIT_ASSERT( pDrumkitCopied!=nullptr );
	// save file
	pDrumkitCopied->setName( "COPY" );
	CPPUNIT_ASSERT( pDrumkitCopied->save( sDrumkitPath ) );

	pDrumkitReloaded = nullptr;

	// Check whether blank drumkits are valid.
	pDrumkitNew = std::make_shared<H2Core::Drumkit>();
	CPPUNIT_ASSERT( pDrumkitNew != nullptr );
	CPPUNIT_ASSERT( pDrumkitNew->save( sDrumkitPath ) );
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ) ) );
	pDrumkitReloaded = H2Core::Drumkit::load(
		sDrumkitPath, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitReloaded != nullptr );

	// Cleanup
	H2Core::Filesystem::rm( sDrumkitPath, true );
	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitLegacy()
{
	___INFOLOG( "" );

	QDir legacyDir( H2TEST_FILE( "drumkits/legacyKits" ) );

	// Check whether all contained kits can be loaded.
	for ( const auto& ssDir : legacyDir.entryList( QDir::Dirs |
													QDir::NoDotAndDotDot ) ) {
		___INFOLOG( ssDir );
		const auto pDrumkit = H2Core::Drumkit::load(
			legacyDir.filePath( ssDir ), false, nullptr, false );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
	}

	// Check wether the names stored in the DrumkitComponents in version 0.9.7 -
	// 1.2.X are properly ported to InstrumentComponents.
	const auto pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "drumkits/legacyKits/kit-1.2.3" ), false, nullptr, false );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getInstruments()->get( 0 ) != nullptr );
	const auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument->getComponent( 0 ) != nullptr );
	CPPUNIT_ASSERT( pInstrument->getComponent( 1 ) != nullptr );
	CPPUNIT_ASSERT( pInstrument->getComponent( 0 )->getName() == "Second" );
	CPPUNIT_ASSERT( pInstrument->getComponent( 1 )->getName() == "First" );


	___INFOLOG( "passed" );
}

// Load drumkit which includes instrument with invalid ADSR values.
void XmlTest::testDrumkit_invalidADSRValues()
{
	___INFOLOG( "" );
	auto pTestHelper = TestHelper::get_instance();
	std::shared_ptr<H2Core::Drumkit> pDrumkit = nullptr;

	//1. Check, if the drumkit has been loaded
	pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "drumkits/invAdsrKit"), true, nullptr, true );
	CPPUNIT_ASSERT( pDrumkit != nullptr );

	//2. Make sure that the instruments of the drumkit have been loaded correctly (see GH issue #839)
	auto pInstruments = pDrumkit->getInstruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );

	auto pFirstInstrument = pInstruments->get(0);
	CPPUNIT_ASSERT( pFirstInstrument != nullptr );

	auto pLayer = pFirstInstrument->getComponents()->front()->getLayer(0);
	CPPUNIT_ASSERT( pLayer != nullptr );
	
	auto pSample = pLayer->getSample();
	CPPUNIT_ASSERT( pSample != nullptr );
	
	CPPUNIT_ASSERT( pSample->getFilename() == QString("snare.wav"));

	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitUpgrade() {
	___INFOLOG( "" );

	// `CoreActionController::validateDrumkit()` will be called on invalid kits
	// in this unit test. This will cause the routine to _not_ clean up
	// extracted artifacts. We have to do ourselves. Else they will pile up in
	// the tmp folder.
	QDir tmpDir( H2Core::Filesystem::tmp_dir() );
	const auto tmpDirContentPre = tmpDir.entryList(
		QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files );

	// For all drumkits in the legacy folder, check whether there are
	// invalid. Then, we upgrade them to the most recent version and
	// check whether there are valid and if a second upgrade is yields
	// the same result.
	QDir legacyDir( H2TEST_FILE( "drumkits/legacyKits" ) );
	QStringList nameFilters;
	nameFilters << "*" + H2Core::Filesystem::drumkit_ext;

	QString sDrumkitPath;

	for ( const auto& ssFile : legacyDir.entryList( nameFilters, QDir::Files ) ) {

		sDrumkitPath = H2TEST_FILE( "drumkits/legacyKits" ) + "/" + ssFile;

		CPPUNIT_ASSERT( ! H2Core::CoreActionController::validateDrumkit(
							sDrumkitPath, false ) );

		// The number of files within the drumkit has to be constant.
		QTemporaryDir contentOriginal( H2Core::Filesystem::tmp_dir() +
									   "testDrumkitUpgrade_orig-" +
									   QTime::currentTime().toString( "hh-mm-ss-zzz" ) +
									   "-XXXXXX" );
		contentOriginal.setAutoRemove( false );
		CPPUNIT_ASSERT( H2Core::CoreActionController::extractDrumkit(
							sDrumkitPath, contentOriginal.path() ) );
		QDir contentDirOriginal( contentOriginal.path() );
		int nFilesOriginal = contentDirOriginal.entryList(
			QDir::AllEntries | QDir::NoDotAndDotDot ).size();

		// Upgrade the legacy kit and store the result in a temporary
		// folder (they will be automatically removed by Qt as soon as
		// the variable gets out of scope)
		QTemporaryDir firstUpgrade( H2Core::Filesystem::tmp_dir() +
									"testDrumkitUpgrade_firstUpgrade-" +
									QTime::currentTime().toString( "hh-mm-ss-zzz" ) +
									"-XXXXXX" );
		firstUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( H2Core::CoreActionController::upgradeDrumkit(
							sDrumkitPath, firstUpgrade.path() ) );
		// The upgrade should have yielded a single .h2drumkit file.
		QDir upgradeFolder( firstUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList(
							QDir::AllEntries | QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sUpgradedKit( firstUpgrade.path() + "/" +
							  upgradeFolder.entryList( QDir::AllEntries |
													   QDir::NoDotAndDotDot )[ 0 ] );
		CPPUNIT_ASSERT( H2Core::CoreActionController::validateDrumkit(
							sUpgradedKit, false ) );

		// Check whether the drumkit call be loaded properly.
		bool b, e;
		QString s1, s2;
		auto pDrumkit = H2Core::CoreActionController::retrieveDrumkit(
			firstUpgrade.path() + "/" + ssFile, &b, &s1, &s2, &e );
		CPPUNIT_ASSERT( pDrumkit != nullptr );
		if ( pDrumkit->getName() == "Boss DR-110" ) {
			// For our default kit we put in some prior knowledge to
			// check whether the upgrade process produce the expected
			// results.
			auto pInstrumentList = pDrumkit->getInstruments();
			CPPUNIT_ASSERT( pInstrumentList != nullptr );
			CPPUNIT_ASSERT( pInstrumentList->size() == 6 );

			auto pInstrument = pInstrumentList->get( 0 );
			CPPUNIT_ASSERT( pInstrument != nullptr );

			auto pComponents = pInstrument->getComponents();
			CPPUNIT_ASSERT( pComponents != nullptr );
			CPPUNIT_ASSERT( pComponents->size() == 1 );

			auto pComponent = pComponents->at( 0 );
			CPPUNIT_ASSERT( pComponent != nullptr );
			
			auto pLayers = pComponent->getLayers();
			CPPUNIT_ASSERT( pLayers.size() == 2 );
		}
		
		QTemporaryDir contentUpgraded( H2Core::Filesystem::tmp_dir() +
									"testDrumkitUpgrade_contentUpgraded-" +
									QTime::currentTime().toString( "hh-mm-ss-zzz" ) +
									"-XXXXXX" );
		contentUpgraded.setAutoRemove( false );
		CPPUNIT_ASSERT( H2Core::CoreActionController::extractDrumkit(
							sUpgradedKit, contentUpgraded.path() ) );
		QDir contentDirUpgraded( contentUpgraded.path() );
		int nFilesUpgraded =
			contentDirUpgraded.entryList( QDir::AllEntries |
										  QDir::NoDotAndDotDot ).size();
		___INFOLOG( nFilesUpgraded );
		if ( nFilesOriginal != nFilesUpgraded ) {
			___ERRORLOG( "Mismatching content of original and upgraded drumkit." );
			___ERRORLOG( QString( "original [%1]:" ).arg( contentOriginal.path() ) );
			for ( const auto& ssFile : contentDirOriginal.entryList( QDir::AllEntries |
																	 QDir::NoDotAndDotDot ) ) {
				___ERRORLOG( "   " + ssFile );
			}
			___ERRORLOG( QString( "upgraded [%1]:" ).arg( contentUpgraded.path() ) );
			for ( const auto& ssFile : contentDirUpgraded.entryList( QDir::AllEntries |
																	 QDir::NoDotAndDotDot ) ) {
				___ERRORLOG( "   " + ssFile );
			}
		}
		CPPUNIT_ASSERT( nFilesOriginal == nFilesUpgraded );

		// Now we upgrade the upgraded drumkit again and bit-compare
		// the results.
		QTemporaryDir secondUpgrade( H2Core::Filesystem::tmp_dir() +
									"testDrumkitUpgrade_secondUpgrade-" +
									QTime::currentTime().toString( "hh-mm-ss-zzz" ) +
									 "-XXXXXX" );
		secondUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( H2Core::CoreActionController::upgradeDrumkit(
							sUpgradedKit, secondUpgrade.path() ) );
		upgradeFolder = QDir( secondUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList( QDir::AllEntries |
												 QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sValidationKit( secondUpgrade.path() + "/" +
								upgradeFolder.entryList( QDir::AllEntries |
														 QDir::NoDotAndDotDot )[ 0 ] );

		QTemporaryDir contentValidation( H2Core::Filesystem::tmp_dir() +
										 "testDrumkitUpgrade_contentValidation-" +
										 QTime::currentTime().toString( "hh-mm-ss-zzz" ) +
										 "-XXXXXX" );
		contentValidation.setAutoRemove( false );
		CPPUNIT_ASSERT( H2Core::CoreActionController::extractDrumkit(
							sUpgradedKit, contentValidation.path() ) );

		// Compare the extracted folders. Attention: in the toplevel
		// temporary folder there is a single directory named
		// according to the drumkit. These ones have to be compared.
		H2TEST_ASSERT_DIRS_EQUAL(
			QDir( contentUpgraded.path() )
			.entryList( QDir::Dirs | QDir::NoDotAndDotDot )[ 0 ],
			QDir( contentValidation.path() )
			.entryList( QDir::Dirs | QDir::NoDotAndDotDot )[ 0 ] );

		// Only clean up if all checks passed.
		H2Core::Filesystem::rm( contentOriginal.path(), true, true );
		H2Core::Filesystem::rm( contentUpgraded.path(), true, true );
		H2Core::Filesystem::rm( contentValidation.path(), true, true );
		H2Core::Filesystem::rm( firstUpgrade.path(), true, true );
		H2Core::Filesystem::rm( secondUpgrade.path(), true, true );
	}

	// Check whether there is new content in the tmp dir.
	const auto tmpDirContentPost = tmpDir.entryList(
		QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files );

	for ( const auto& ssEntry : tmpDirContentPost ) {
		if ( ! tmpDirContentPre.contains( ssEntry ) ) {
			H2Core::Filesystem::rm( tmpDir.filePath( ssEntry ), true, true );
		}
	}

	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitInstrumentTypeUniqueness()
{
	___INFOLOG( "" );

	// Test resilience against loading duplicate type and key. They should both
	// be dropped.
	const QString sRefFolder = H2TEST_FILE( "drumkits/instrument-type-ref" );
	const QString sDuplicateFolder =
		H2TEST_FILE( "drumkits/instrument-type-ref-duplicate" );
	const auto pDrumkitRef = H2Core::Drumkit::load(
		sRefFolder, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitRef != nullptr );
	const auto pDrumkitDuplicates = H2Core::Drumkit::load(
		sDuplicateFolder, false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkitDuplicates != nullptr );

	H2TEST_ASSERT_DRUMKIT_FILES_UNEQUAL( sRefFolder + "/drumkit.xml",
								   sDuplicateFolder + "/drumkit.xml" );

	const QString sTmpRef = H2Core::Filesystem::tmp_dir() + "ref-saved";
	const QString sTmpDuplicate =
		H2Core::Filesystem::tmp_dir() + "duplicate-saved";

	CPPUNIT_ASSERT( pDrumkitRef->save( sTmpRef ) );
	CPPUNIT_ASSERT( pDrumkitDuplicates->save( sTmpDuplicate ) );

	H2TEST_ASSERT_DRUMKIT_FILES_EQUAL( sTmpRef + "/drumkit.xml",
								   sTmpDuplicate + "/drumkit.xml" );
	H2TEST_ASSERT_DIRS_EQUAL( sTmpRef, sTmpDuplicate );

	H2Core::Filesystem::rm( sTmpRef, true );
	H2Core::Filesystem::rm( sTmpDuplicate, true );
	___INFOLOG( "passed" );
}

void XmlTest::testShippedDrumkits()
{
	___INFOLOG( "" );

	// Since there are also optional elements in our XML files, we load,
	// save, and compare all shipped kit to ensure they are cutting edge.
	for ( const auto& ssKit : H2Core::Filesystem::sys_drumkit_list() ) {
		___INFOLOG( ssKit );

		// Since kits are upgraded during startup of Hydrogen, all shipped kits
		// will cutting edge. But we can check for the presence of backup files
		// which indicate that an upgrade was required/took place.
		const auto backupFiles =
			TestHelper::get_instance()->findDrumkitBackupFiles(
				QString( "%1/%2" ).arg( H2Core::Filesystem::sys_drumkits_dir() )
				.arg( ssKit ) );
		if ( backupFiles.size() > 0 ) {
			___ERRORLOG( QString( "backup files found: %1" )
						 .arg( backupFiles.join( ',' ) ) );
			CPPUNIT_ASSERT( backupFiles.size() == 0 );
		}

		const auto pDrumkit = H2Core::Drumkit::load(
			QString( "%1/%2" ).arg( H2Core::Filesystem::sys_drumkits_dir() )
			.arg( ssKit ), false, nullptr, true );
		CPPUNIT_ASSERT( pDrumkit != nullptr );

		const QString sTmpDrumkitXml = H2Core::Filesystem::tmp_file_path(
			QString( "newest-%1.xml" ).arg( ssKit ) );

		H2Core::XMLDoc doc;
		H2Core::XMLNode root = doc.set_root( "drumkit_info", "drumkit" );
		root.appendChild( doc.createComment(
							  H2Core::License::getGPLLicenseNotice(
								  pDrumkit->getAuthor() ) ) );
		pDrumkit->saveTo( root, false, false );

		CPPUNIT_ASSERT( doc.write( sTmpDrumkitXml ) );

		H2TEST_ASSERT_DRUMKIT_FILES_EQUAL(
			H2Core::Filesystem::drumkit_file(
				QString( "%1/%2" ).arg( H2Core::Filesystem::sys_drumkits_dir() )
				.arg( ssKit ) ), sTmpDrumkitXml );

		// Cleanup
		CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpDrumkitXml ) );
	}
	___INFOLOG( "passed" );
}

////////////////////////////////////////////////////////////////////////////////

void XmlTest::testDrumkitMapFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFile = H2TEST_FILE( "/drumkit_map/ref.h2map");
	const auto pDrumkitMap = H2Core::DrumkitMap::load( sTestFile );
	CPPUNIT_ASSERT( pDrumkitMap != nullptr );

	const QString sTmpDrumkitMap =
		H2Core::Filesystem::tmp_file_path( "drumkit-map-format-integrity.h2map" );
	CPPUNIT_ASSERT( pDrumkitMap->save( sTmpDrumkitMap ) );

	H2TEST_ASSERT_XML_FILES_EQUAL( sTestFile, sTmpDrumkitMap );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpDrumkitMap ) );
	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitMap()
{
	___INFOLOG( "" );

	// Test resilience against loading duplicate type and key. They should both
	// be dropped.
	const QString sRefFile = H2TEST_FILE( "drumkit_map/ref.h2map" );
	const auto pDrumkitMapRef = H2Core::DrumkitMap::load( sRefFile );
	CPPUNIT_ASSERT( pDrumkitMapRef != nullptr );
	const auto pDrumkitMapDuplicates = H2Core::DrumkitMap::load(
		H2TEST_FILE( "drumkit_map/ref-duplicates.h2map" ) );
	CPPUNIT_ASSERT( pDrumkitMapDuplicates != nullptr );

	const QString sTmpFile = H2Core::Filesystem::tmp_dir() + "ref-saved.h2map";

	CPPUNIT_ASSERT( pDrumkitMapDuplicates->save( sTmpFile, false ) );
	H2TEST_ASSERT_XML_FILES_EQUAL( sRefFile, sTmpFile );

	H2Core::Filesystem::rm( sTmpFile );
	___INFOLOG( "passed" );
}

void XmlTest::testShippedDrumkitMaps()
{
	___INFOLOG( "" );

	QDir mapDir( H2Core::Filesystem::sys_drumkit_maps_dir() );
	H2Core::XMLDoc doc;
	const auto shippedMaps = mapDir.entryList(
		QStringList( QString( "*%1" ).arg( H2Core::Filesystem::drumkit_map_ext ) ),
		QDir::Files | QDir::NoDotAndDotDot );

	CPPUNIT_ASSERT( shippedMaps.size() > 0 );

	for ( const auto& ssMap : shippedMaps ) {
		auto pDrumkitMap = H2Core::DrumkitMap::load(
			mapDir.filePath( ssMap ), false );
		CPPUNIT_ASSERT( pDrumkitMap != nullptr );
	}
	___INFOLOG( "passed" );
}

////////////////////////////////////////////////////////////////////////////////

void XmlTest::testPatternFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFile = H2TEST_FILE( "/pattern/pattern.h2pattern" );
	const auto pPattern = H2Core::Pattern::load( sTestFile );
	CPPUNIT_ASSERT( pPattern != nullptr );

	const QString sTmpPattern =
		H2Core::Filesystem::tmp_file_path( "pattern-format-integrity.h2pattern" );
	CPPUNIT_ASSERT( pPattern->save( sTmpPattern, true ) );

	H2TEST_ASSERT_XML_FILES_EQUAL( sTestFile, sTmpPattern );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpPattern ) );
	___INFOLOG( "passed" );
}

void XmlTest::testPattern()
{
	___INFOLOG( "" );

	QString sPatternPath =
		H2Core::Filesystem::tmp_dir() + "pattern.h2pattern";

	H2Core::XMLDoc doc;

	auto pDrumkit = H2Core::Drumkit::load(
		H2TEST_FILE( "/drumkits/baseKit" ), false, nullptr, true );
	CPPUNIT_ASSERT( pDrumkit!=nullptr );
	auto pInstrumentList = pDrumkit->getInstruments();
	CPPUNIT_ASSERT( pInstrumentList->size()==4 );

	auto pPatternLoaded = H2Core::Pattern::load(
		H2TEST_FILE( "/pattern/pattern.h2pattern" ) );
	CPPUNIT_ASSERT( pPatternLoaded != nullptr );
	CPPUNIT_ASSERT( pPatternLoaded->save( sPatternPath, true ) );

	H2TEST_ASSERT_XML_FILES_EQUAL( H2TEST_FILE( "pattern/pattern.h2pattern" ),
								   sPatternPath );

	// Check for double freeing when destructing both copy and original.
	auto pPatternCopied = std::make_shared<H2Core::Pattern>( pPatternLoaded );

	// Check whether the constructor produces valid patterns.
	QString sEmptyPatternPath =
		H2Core::Filesystem::tmp_dir() + "empty.h2pattern";
	auto pPatternNew = new H2Core::Pattern( "test", "ladida", "", 1, 1 );
	CPPUNIT_ASSERT( pPatternNew->save( sPatternPath, true ) );
	CPPUNIT_ASSERT( doc.read( sPatternPath ) );
	H2TEST_ASSERT_XML_FILES_EQUAL( H2TEST_FILE( "pattern/empty.h2pattern" ),
								   sPatternPath );

	// Cleanup
	H2Core::Filesystem::rm( sPatternPath );
	H2Core::Filesystem::rm( sEmptyPatternPath );
	___INFOLOG( "passed" );
}

void XmlTest::testPatternLegacy() {
	___INFOLOG( "" );

	QStringList legacyPatterns;
	legacyPatterns << H2TEST_FILE( "pattern/legacy/pattern-1.X.X.h2pattern" )
				   << H2TEST_FILE( "pattern/legacy/legacy_pattern.h2pattern" );

	for ( const auto& ssPattern : legacyPatterns ) {
		auto pPattern = H2Core::Pattern::load( ssPattern );
		CPPUNIT_ASSERT( pPattern );
	}

	___INFOLOG( "passed" );
}

void XmlTest::testPatternInstrumentTypes()
{
	___INFOLOG( "" );

	const QString sTmpWithoutTypes =
		H2Core::Filesystem::tmp_dir() + "pattern-without-types.h2pattern";
	const QString sTmpMismatch =
		H2Core::Filesystem::tmp_dir() + "pattern-with-mismatch.h2pattern";
	// Be sure to remove past artifacts or saving the patterns will fail.
	if ( H2Core::Filesystem::file_exists( sTmpWithoutTypes, true ) ) {
		H2Core::Filesystem::rm( sTmpWithoutTypes );
	}
	if ( H2Core::Filesystem::file_exists( sTmpMismatch, true ) ) {
		H2Core::Filesystem::rm( sTmpMismatch );
	}

	// Check whether the reference pattern is valid.
	const auto pPatternRef = H2Core::Pattern::load(
		H2TEST_FILE( "pattern/pattern.h2pattern") );
	CPPUNIT_ASSERT( pPatternRef != nullptr );

	// The version of the reference without any type information should be
	// filled with those obtained from the shipped .h2map file.
	const auto pPatternWithoutTypes = H2Core::Pattern::load(
		H2TEST_FILE( "pattern/pattern-without-types.h2pattern") );
	CPPUNIT_ASSERT( pPatternWithoutTypes != nullptr );
	CPPUNIT_ASSERT( pPatternWithoutTypes->save( sTmpWithoutTypes ) );
	H2TEST_ASSERT_XML_FILES_EQUAL(
		H2TEST_FILE( "pattern/pattern.h2pattern" ), sTmpWithoutTypes );

	// In this file an instrument id is off. But this should heal itself when
	// switching to another kit and back (as only instrument types are used
	// during switching and the ids are reassigned).
	const auto pPatternMismatch = H2Core::Pattern::load(
		H2TEST_FILE( "pattern/pattern-with-mismatch.h2pattern") );
	CPPUNIT_ASSERT( pPatternMismatch != nullptr );
	// TODO switch back and forth
	// CPPUNIT_ASSERT( pPatternMismatch->save( sTmpMismatch ) );
	// H2TEST_ASSERT_XML_FILES_EQUAL(
	// 	H2TEST_FILE( "pattern/pattern.h2pattern" ), sTmpMismatch );

	H2Core::Filesystem::rm( sTmpWithoutTypes );
	H2Core::Filesystem::rm( sTmpMismatch );
	___INFOLOG( "passed" );
}

void XmlTest::checkTestPatterns()
{
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/pattern/empty.h2pattern" ) ) );
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/pattern/pattern.h2pattern" ) ) );
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE(
								  "/pattern/pattern-with-mismatch.h2pattern" ) ) );
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE(
								  "/pattern/pattern-without-types.h2pattern" ) ) );

	___INFOLOG( "passed" );
}

void XmlTest::testPlaylistFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFile = H2TEST_FILE( "/playlist/test.h2playlist" );
	const auto pPlaylist = H2Core::Playlist::load( sTestFile );
	CPPUNIT_ASSERT( pPlaylist != nullptr );

	// As we are using relative paths to the song files, we have to create the
	// test artifact within the same folder as the original playlist.
	const QString sTmpPlaylist =
		H2TEST_FILE( "/playlist/tmp-duplicate-test.h2playlist" );
	CPPUNIT_ASSERT( pPlaylist->saveAs( sTmpPlaylist, false ) );

	H2TEST_ASSERT_XML_FILES_EQUAL( sTestFile, sTmpPlaylist );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpPlaylist ) );
	___INFOLOG( "passed" );
}

////////////////////////////////////////////////////////////////////////////////

void XmlTest::testPlaylist()
{
	___INFOLOG( "" );

	const QString sTmpPath = H2Core::Filesystem::tmp_dir() +
		"playlist.h2playlist";
	const QString sTmpPathEmpty = H2Core::Filesystem::tmp_dir() +
		"empty.h2playlist";

	// Test constructor
	auto pPlaylist = H2Core::Playlist::load(
		H2TEST_FILE( "playlist/test.h2playlist" ) );
	H2Core::XMLDoc doc;

	CPPUNIT_ASSERT( pPlaylist != nullptr );
	CPPUNIT_ASSERT( pPlaylist->saveAs( sTmpPath ) );
	CPPUNIT_ASSERT( doc.read( sTmpPath ) );
	const auto pPlaylistLoaded = H2Core::Playlist::load( sTmpPath );
	CPPUNIT_ASSERT( pPlaylistLoaded != nullptr );

	// TODO Fails since it does not seem to be clear what relative does actually
	// mean? Relative to the playlist user dir? To the playlist itself?
	//
	// H2TEST_ASSERT_XML_FILES_EQUAL(
	// 	sTmpPath, H2TEST_FILE( "playlist/test.h2playlist" ));

	// Test constructor
	auto pPlaylistEmpty = std::make_shared<H2Core::Playlist>();
	H2Core::XMLDoc docEmpty;

	CPPUNIT_ASSERT( pPlaylistEmpty->saveAs( sTmpPathEmpty ) );
	const auto pPlaylistEmptyLoaded = H2Core::Playlist::load( sTmpPathEmpty );
	CPPUNIT_ASSERT( pPlaylistEmptyLoaded != nullptr );

	H2TEST_ASSERT_XML_FILES_EQUAL(
		sTmpPathEmpty, H2TEST_FILE( "playlist/empty.h2playlist" ));

	// Cleanup
	H2Core::Filesystem::rm( sTmpPath );
	H2Core::Filesystem::rm( sTmpPathEmpty );

	___INFOLOG( "passed" );
}

void XmlTest::testSongFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFile = H2TEST_FILE( "song/current.h2song" );
	const auto pSong = H2Core::Song::load( sTestFile );
	CPPUNIT_ASSERT( pSong != nullptr );

	const QString sTmpSong =
		H2Core::Filesystem::tmp_file_path( "current-format-integrity.h2song" );
	CPPUNIT_ASSERT( pSong->save( sTmpSong ) );

	H2TEST_ASSERT_H2SONG_FILES_EQUAL( sTestFile, sTmpSong );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpSong ) );
	___INFOLOG( "passed" );
}

void XmlTest::testSong()
{
	___INFOLOG( "" );
	const QString sTmpPath = H2Core::Filesystem::tmp_dir() +
		"song.h2song";
	const QString sTmpPathEmpty = H2Core::Filesystem::tmp_dir() +
		"empty.h2song";
	const QString sTmpPathConstructor = H2Core::Filesystem::tmp_dir() +
		"constructor.h2song";

	// Test constructor
	const auto pSongConstructor = std::make_shared<H2Core::Song>();
	CPPUNIT_ASSERT( pSongConstructor->save( sTmpPathConstructor ) );
	CPPUNIT_ASSERT( H2Core::Song::load( sTmpPathConstructor ) != nullptr );

	H2TEST_ASSERT_H2SONG_FILES_EQUAL(
		sTmpPathConstructor, H2TEST_FILE( "song/constructor.h2song" ));

	// Test empty song (which is using the default kit)
	const auto pSongEmpty = H2Core::Song::getEmptySong();
	CPPUNIT_ASSERT( pSongEmpty->save( sTmpPathEmpty ) );
	CPPUNIT_ASSERT( H2Core::Song::load( sTmpPathEmpty ) != nullptr );

	H2TEST_ASSERT_H2SONG_FILES_EQUAL(
		sTmpPathEmpty, H2TEST_FILE( "song/empty.h2song" ));

	// Cleanup
	H2Core::Filesystem::rm( sTmpPath );
	H2Core::Filesystem::rm( sTmpPathEmpty );
	H2Core::Filesystem::rm( sTmpPathConstructor );

	___INFOLOG( "passed" );
}

void XmlTest::testSongLegacy() {
	___INFOLOG( "" );
	QStringList testSongs;
	testSongs << H2TEST_FILE( "song/legacy/test_song_1.2.2.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.2.0-beta1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.1.0-beta1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.2.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.1.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_1.0.0.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_0.9.7.h2song" );

	for ( const auto& ssSong : testSongs ) {
		___INFOLOG(ssSong);
		auto pSong = H2Core::Song::load( ssSong, false );
		CPPUNIT_ASSERT( pSong != nullptr );
		CPPUNIT_ASSERT( ! pSong->hasMissingSamples() );
	}

	// Check that invalid paths and drumkit names could indeed result in missing
	// samples.
	testSongs.clear();
	testSongs << H2TEST_FILE( "song/legacy/test_song_invalid_drumkit_name.h2song" )
			  << H2TEST_FILE( "song/legacy/test_song_invalid_sample_path.h2song" );

	for ( const auto& ssSong : testSongs ) {
		___INFOLOG(ssSong);
		auto pSong = H2Core::Song::load( ssSong, false );
		CPPUNIT_ASSERT( pSong != nullptr );
		CPPUNIT_ASSERT( pSong->hasMissingSamples() );
	}
	___INFOLOG( "passed" );
}

////////////////////////////////////////////////////////////////////////////////

void XmlTest::testPreferencesFormatIntegrity() {
	___INFOLOG( "" );
	const QString sTestFile = H2TEST_FILE( "preferences/current.conf" );
	const auto pPreferences = H2Core::Preferences::load( sTestFile );
	CPPUNIT_ASSERT( pPreferences != nullptr );

	const QString sTmpPreferences =
		H2Core::Filesystem::tmp_file_path( "current-format-integrity.conf" );
	CPPUNIT_ASSERT( pPreferences->saveCopyAs( sTmpPreferences ) );

	H2TEST_ASSERT_PREFERENCES_FILES_EQUAL( sTestFile, sTmpPreferences );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpPreferences ) );
	___INFOLOG( "passed" );
}

void XmlTest::testShippedPreferences() {
	___INFOLOG( "" );
	const QString sDefaultConfigFile = H2Core::Filesystem::sys_config_path();
	const auto pPreferences = H2Core::Preferences::load( sDefaultConfigFile );
	CPPUNIT_ASSERT( pPreferences != nullptr );

	const QString sTmpPreferences =
		H2Core::Filesystem::tmp_file_path( "check-default-hydrogen.conf" );
	CPPUNIT_ASSERT( pPreferences->saveCopyAs( sTmpPreferences ) );

	H2TEST_ASSERT_PREFERENCES_FILES_EQUAL( sDefaultConfigFile, sTmpPreferences );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpPreferences ) );
	___INFOLOG( "passed" );
}

void XmlTest::testShippedThemes() {
	___INFOLOG( "" );
	QDir themesDir( H2Core::Filesystem::sys_theme_dir() );
	H2Core::XMLDoc doc;
	const auto shippedThemes = themesDir.entryList(
		QStringList( QString( "*%1" ).arg( H2Core::Filesystem::themes_ext ) ),
		QDir::Files | QDir::NoDotAndDotDot );

	CPPUNIT_ASSERT( shippedThemes.size() > 0 );

	for ( const auto& ssTheme : shippedThemes ) {
		const QString sTmpFile =
			H2Core::Filesystem::tmp_file_path( "check-shipped-themes-XXXX.conf" );
		const auto pTheme =
			H2Core::Theme::importFrom( themesDir.filePath( ssTheme ) );
		pTheme->exportTo( sTmpFile );

		H2TEST_ASSERT_PREFERENCES_FILES_EQUAL( themesDir.filePath( ssTheme ),
											   sTmpFile );

		// Cleanup
		CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpFile ) );
	}

	// Check whether the default theme still carries all defaults.
	const QString sDefaultTheme = themesDir.filePath( "default.h2theme" );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_exists( sDefaultTheme ) );

	const QString sTmpFile =
		H2Core::Filesystem::tmp_file_path( "check-default-theme-XXXX.conf" );
	const auto pDefaultTheme = std::make_shared<H2Core::Theme>();
	pDefaultTheme->exportTo( sTmpFile );

	H2TEST_ASSERT_THEME_FILES_EQUAL( sDefaultTheme, sTmpFile );

	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sTmpFile ) );

	___INFOLOG( "passed" );
}

void XmlTest::testSamplePathsWritten() {
	___INFOLOG( "" );

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const auto sBaseKit = QString( "%1/GMRockKit" )
		.arg( H2Core::Filesystem::sys_drumkits_dir() );

	// We use a two-stage approach to check all the sample paths: 1. we parse
	// the XML file into a DOM and select only the part corresponding to the
	// drumkit (containing the samples) and 2. convert this part of the DOM back
	// into a string and perform a line-by-line search for the elements
	// containing sample paths. This way we ensure no other filenames, like the
	// ones for the LADSPA effects, will leak into our check and, at the same
	// time, be resilient to changes in the drumkit structure.
	const QString sSamplePathElement( "<filename>" );
	const QString sSamplePathClosingElement( "</filename>" );

	// Create a new drumkit and save it to disk.
	const auto pBaseKit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
		sBaseKit );
	CPPUNIT_ASSERT( pBaseKit != nullptr );

	auto pNewKit = std::make_shared<H2Core::Drumkit>( pBaseKit );
	CPPUNIT_ASSERT( pNewKit != nullptr );

	pNewKit->setName( "testSamplePathsWrittenKit" );
	const auto sNewKitPath = QString( "%1/%2" )
		.arg( H2Core::Filesystem::usr_drumkits_dir() ).arg( pNewKit->getName() );
	CPPUNIT_ASSERT( ! H2Core::Filesystem::dir_exists( sNewKitPath ) );
	CPPUNIT_ASSERT( pNewKit->save( sNewKitPath ) );

	// Load the new drumkit.xml file and validate that there are no absolute
	// paths.
	H2Core::XMLDoc docNewKit;
	CPPUNIT_ASSERT( docNewKit.read( H2Core::Filesystem::drumkit_file(
										sNewKitPath ) ) );

	auto rootNodeNewKit = docNewKit.firstChildElement( "drumkit_info" );
	CPPUNIT_ASSERT( ! rootNodeNewKit.isNull() );

	QString sRootNodeNewKit;
	QTextStream streamNewKit( &sRootNodeNewKit );
	rootNodeNewKit.save( streamNewKit, 0 );
	QString ssLineDrumkit;
	while ( streamNewKit.readLineInto( &ssLineDrumkit ) ) {
		if ( ssLineDrumkit.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineDrumkit.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" ).trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sNewKitPath ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "/" ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "\\" ) );
		}
	}

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sNewKitPath, true ) );

	////////////////////////////////////////////////////////////////////////////

	// Create a new song and save it to disk.
	auto pSong = H2Core::Song::getEmptySong();
	CPPUNIT_ASSERT( pSong != nullptr );

	pSong->setDrumkit( pNewKit );
	const auto sSongPath = H2Core::Filesystem::tmp_file_path(
		"testSamplePathsWritten.h2song");
	CPPUNIT_ASSERT( pSong->save( sSongPath ) );

	// Load the new .h2song file and validate that there are no absolute paths.
	H2Core::XMLDoc docSong;
	CPPUNIT_ASSERT( docSong.read( sSongPath ) );

	auto rootNodeSong = docSong.firstChildElement( "song" );
	CPPUNIT_ASSERT( ! rootNodeSong.isNull() );
	auto drumkitNodeSong = rootNodeSong.firstChildElement( "drumkit_info" );
	CPPUNIT_ASSERT( ! drumkitNodeSong.isNull() );

	QString sDrumkitNodeSong;
	QTextStream streamSong( &sDrumkitNodeSong );
	drumkitNodeSong.save( streamSong, 0 );
	QString ssLineSong;
	while ( streamSong.readLineInto( &ssLineSong ) ) {
		if ( ssLineSong.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineSong
				.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" )
				.trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sSongPath ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "/" ) );
			CPPUNIT_ASSERT( ! sSamplePath.contains( "\\" ) );
		}
	}

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sSongPath ) );

	////////////////////////////////////////////////////////////////////////////

	// Create a new song, manually add a sample, and save it to disk.

	// Create a new song and save it to disk.
	auto pSongCustom = H2Core::Song::getEmptySong();
	CPPUNIT_ASSERT( pSongCustom != nullptr );
	pSongCustom->setDrumkit( pNewKit );

	const QString sCustomSamplePath( "/path/to/custom/sample.wav" );
	auto pInstrument = pSongCustom->getDrumkit()->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	auto pComponent = pInstrument->getComponent( 0 );
	CPPUNIT_ASSERT( pComponent != nullptr );
	auto pLayer = pComponent->getLayer( 0 );
	CPPUNIT_ASSERT( pLayer != nullptr );
	auto pSample = pLayer->getSample();
	CPPUNIT_ASSERT( pSample != nullptr );
	pSample->setFilepath( sCustomSamplePath );

	const auto sSongPathCustom = H2Core::Filesystem::tmp_file_path(
		"testCustomSamplePathsWritten.h2song");
	CPPUNIT_ASSERT( pSongCustom->save( sSongPathCustom ) );

	// Load the new .h2song file and validate that there is a single absolute
	// path.
	H2Core::XMLDoc docSongCustom;
	CPPUNIT_ASSERT( docSongCustom.read( sSongPathCustom ) );

	auto rootNodeSongCustom = docSongCustom.firstChildElement( "song" );
	CPPUNIT_ASSERT( ! rootNodeSongCustom.isNull() );
	auto drumkitNodeSongCustom = rootNodeSongCustom.firstChildElement(
		"drumkit_info" );
	CPPUNIT_ASSERT( ! drumkitNodeSongCustom.isNull() );

	const int nExpectedAbsolutePaths = 1;
	int nnAbsolutePaths = 0;

	QString sDrumkitNodeSongCustom;
	QTextStream streamSongCustom( &sDrumkitNodeSongCustom );
	drumkitNodeSongCustom.save( streamSongCustom, 0 );
	QString ssLineSongCustom;
	while ( streamSongCustom.readLineInto( &ssLineSongCustom ) ) {
		if ( ssLineSongCustom.contains( sSamplePathElement ) ) {
			const auto sSamplePath = ssLineSongCustom
				.replace( sSamplePathElement, "" )
				.replace( sSamplePathClosingElement, "" )
				.trimmed();
			___INFOLOG( QString( "[%1] containing sample path [%2]" )
						.arg( sSongPathCustom ).arg( sSamplePath ) );
			CPPUNIT_ASSERT( ! sSamplePath.isEmpty() );
			CPPUNIT_ASSERT( sSamplePath.contains( "." ) );
			if ( sSamplePath.contains( "/" ) || sSamplePath.contains( "\\" ) ) {
				++nnAbsolutePaths;
			}
		}
	}

	CPPUNIT_ASSERT( nnAbsolutePaths == nExpectedAbsolutePaths );

	CPPUNIT_ASSERT( H2Core::Filesystem::rm( sSongPathCustom ) );

	___INFOLOG( "passed" );
}

bool XmlTest::checkSampleData( std::shared_ptr<H2Core::Drumkit> pKit, bool bLoaded )
{
	int count = 0;
	H2Core::InstrumentComponent::setMaxLayers( 16 );
	auto instruments = pKit->getInstruments();
	for( int i=0; i<instruments->size(); i++ ) {
		count++;
		auto pInstr = ( *instruments )[i];
		for ( const auto& pComponent : *pInstr->getComponents() ) {
			for ( int nLayer = 0; nLayer < H2Core::InstrumentComponent::getMaxLayers(); nLayer++ ) {
				auto pLayer = pComponent->getLayer( nLayer );
				if( pLayer ) {
					auto pSample = pLayer->getSample();
					if ( pSample == nullptr ) {
						return false;
					}
					if( bLoaded ) {
						if( pSample->getData_L()==nullptr || pSample->getData_R()==nullptr ) {
							return false;
						}
					} else {
						if( pSample->getData_L() != nullptr || pSample->getData_R() != nullptr ) {
							return false;
						}
					}
				}

			}
		}
	}
	return ( count==4 );
}
