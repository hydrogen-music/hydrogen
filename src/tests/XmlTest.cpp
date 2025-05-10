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

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Playlist.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/CoreActionController.h>

#include <QDir>
#include <QTemporaryDir>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "TestHelper.h"
#include "assertions/File.h"

static bool check_samples_data( std::shared_ptr<H2Core::Drumkit> dk, bool loaded )
{
	int count = 0;
	H2Core::InstrumentComponent::setMaxLayers( 16 );
	auto instruments = dk->get_instruments();
	for( int i=0; i<instruments->size(); i++ ) {
		count++;
		auto pInstr = ( *instruments )[i];
		for ( const auto& pComponent : *pInstr->get_components() ) {
			for ( int nLayer = 0; nLayer < H2Core::InstrumentComponent::getMaxLayers(); nLayer++ ) {
				auto pLayer = pComponent->get_layer( nLayer );
				if( pLayer ) {
					auto pSample = pLayer->get_sample();
					if( loaded ) {
						if( pSample->get_data_l()==nullptr || pSample->get_data_r()==nullptr ) {
							return false;
						}
					} else {
						if( pSample->get_data_l() != nullptr || pSample->get_data_r() != nullptr ) {
							return false;
						}
					}
				}

			}
		}
	}
	return ( count==4 );
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

	// load without samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit") );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, pDrumkitLoaded->get_instruments()->size() );

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( TestHelper::get_instance()->findDrumkitBackupFiles( "drumkits/baseKit/" )
					.size() == 0 );
	
	// manually load samples
	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );

	pDrumkitLoaded = nullptr;
	
	// load with samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );

	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );
	
	// unload samples
	pDrumkitLoaded->unload_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	
	// save drumkit elsewhere
	pDrumkitLoaded->set_name( "pDrumkitLoaded" );
	CPPUNIT_ASSERT( pDrumkitLoaded->save( sDrumkitPath, true ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/snare.wav" ) );

	// Check whether the generated drumkit is valid.
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ),
							  H2Core::Filesystem::drumkit_xsd_path() ) );
	
	// load file
	pDrumkitReloaded = H2Core::Drumkit::load( sDrumkitPath );
	CPPUNIT_ASSERT( pDrumkitReloaded!=nullptr );
	
	// copy constructor
	pDrumkitCopied = std::make_shared<H2Core::Drumkit>( pDrumkitReloaded );
	CPPUNIT_ASSERT( pDrumkitCopied!=nullptr );
	// save file
	pDrumkitCopied->set_name( "COPY" );
	CPPUNIT_ASSERT( pDrumkitCopied->save( sDrumkitPath ) );

	pDrumkitReloaded = nullptr;

	// Check whether blank drumkits are valid.
	pDrumkitNew = std::make_shared<H2Core::Drumkit>();
	CPPUNIT_ASSERT( pDrumkitNew != nullptr );
	CPPUNIT_ASSERT( pDrumkitNew->save( sDrumkitPath ) );
	CPPUNIT_ASSERT( doc.read( H2Core::Filesystem::drumkit_file( sDrumkitPath ),
							  H2Core::Filesystem::drumkit_xsd_path() ) );
	pDrumkitReloaded = H2Core::Drumkit::load( sDrumkitPath );
	CPPUNIT_ASSERT( pDrumkitReloaded != nullptr );

	// Cleanup
	H2Core::Filesystem::rm( sDrumkitPath, true );
	___INFOLOG( "passed" );
}

void XmlTest::testShippedDrumkits()
{
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	for ( auto ii : H2Core::Filesystem::sys_drumkit_list() ) {
		CPPUNIT_ASSERT( doc.read( QString( "%1%2/drumkit.xml" )
								  .arg( H2Core::Filesystem::sys_drumkits_dir() )
								  .arg( ii ),
								  H2Core::Filesystem::drumkit_xsd_path() ) );

	}
	___INFOLOG( "passed" );
}

// Load drumkit which includes instrument with invalid ADSR values.
void XmlTest::testDrumkit_invalidADSRValues()
{
	___INFOLOG( "" );
	auto pTestHelper = TestHelper::get_instance();
	std::shared_ptr<H2Core::Drumkit> pDrumkit = nullptr;

	//1. Check, if the drumkit has been loaded
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );

	//2. Make sure that the instruments of the drumkit have been loaded correctly (see GH issue #839)
	auto pInstruments = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );

	auto pFirstInstrument = pInstruments->get(0);
	CPPUNIT_ASSERT( pFirstInstrument != nullptr );

	auto pLayer = pFirstInstrument->get_components()->front()->get_layer(0);
	CPPUNIT_ASSERT( pLayer != nullptr );

	auto pSample = pLayer->get_sample();
	CPPUNIT_ASSERT( pSample != nullptr );

	CPPUNIT_ASSERT( pSample->get_filename() == QString("snare.wav"));

	___INFOLOG( "passed" );
}

void XmlTest::testDrumkitUpgrade() {
	___INFOLOG( "" );
	// For all drumkits in the legacy folder, check whether there are
	// invalid. Then, we upgrade them to the most recent version and
	// check whether there are valid and if a second upgrade is yields
	// the same result.
	auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();

	QDir legacyDir( H2TEST_FILE( "drumkits/legacyKits" ) );
	QStringList nameFilters;
	nameFilters << "*" + H2Core::Filesystem::drumkit_ext;

	QString sDrumkitPath;

	for ( const auto& ssFile : legacyDir.entryList( nameFilters, QDir::Files ) ) {

		sDrumkitPath = H2TEST_FILE( "drumkits/legacyKits" ) + "/" + ssFile;

		CPPUNIT_ASSERT( ! pCoreActionController->validateDrumkit( sDrumkitPath, false ) );

		// The number of files within the drumkit has to be constant.
		QTemporaryDir contentOriginal( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentOriginal.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sDrumkitPath,
															   contentOriginal.path() ) );
		QDir contentDirOriginal( contentOriginal.path() );
		int nFilesOriginal =
			contentDirOriginal.entryList( QDir::AllEntries |
										  QDir::NoDotAndDotDot ).size();

		// Upgrade the legacy kit and store the result in a temporary
		// folder (they will be automatically removed by Qt as soon as
		// the variable gets out of scope)
		QTemporaryDir firstUpgrade( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		firstUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->upgradeDrumkit( sDrumkitPath,
															   firstUpgrade.path() ) );
		// The upgrade should have yielded a single .h2drumkit file.
		QDir upgradeFolder( firstUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList( QDir::AllEntries |
												 QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sUpgradedKit( firstUpgrade.path() + "/" +
							  upgradeFolder.entryList( QDir::AllEntries |
													   QDir::NoDotAndDotDot )[ 0 ] );
		CPPUNIT_ASSERT( pCoreActionController->validateDrumkit( sUpgradedKit, false ) );
		
		QTemporaryDir contentUpgraded( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentUpgraded.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sUpgradedKit,
															   contentUpgraded.path() ) );
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
		QTemporaryDir secondUpgrade( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		secondUpgrade.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->upgradeDrumkit( sUpgradedKit,
															   secondUpgrade.path() ) );
		upgradeFolder = QDir( secondUpgrade.path() );
		CPPUNIT_ASSERT( upgradeFolder.entryList( QDir::AllEntries |
												 QDir::NoDotAndDotDot ).size() == 1 );
		
		QString sValidationKit( secondUpgrade.path() + "/" +
								upgradeFolder.entryList( QDir::AllEntries |
														 QDir::NoDotAndDotDot )[ 0 ] );

		QTemporaryDir contentValidation( H2Core::Filesystem::tmp_dir() + "-XXXXXX" );
		contentValidation.setAutoRemove( false );
		CPPUNIT_ASSERT( pCoreActionController->extractDrumkit( sUpgradedKit,
															   contentValidation.path() ) );

		// Compare the extracted folders. Attention: in the toplevel
		// temporary folder there is a single directory named
		// according to the drumkit. These ones have to be compared.
		H2TEST_ASSERT_DIRS_EQUAL( QDir( contentUpgraded.path() )
								  .entryList( QDir::Dirs |
											  QDir::NoDotAndDotDot )[ 0 ],
								  QDir( contentValidation.path() )
								  .entryList( QDir::Dirs |
											  QDir::NoDotAndDotDot )[ 0 ] );

		// Only clean up if all checks passed.
		H2Core::Filesystem::rm( contentOriginal.path(), true, true );
		H2Core::Filesystem::rm( contentUpgraded.path(), true, true );
		H2Core::Filesystem::rm( contentValidation.path(), true, true );
		H2Core::Filesystem::rm( firstUpgrade.path(), true, true );
		H2Core::Filesystem::rm( secondUpgrade.path(), true, true );
	}
	___INFOLOG( "passed" );
}

void XmlTest::testPattern()
{
	___INFOLOG( "" );
	QString sPatternPath = H2Core::Filesystem::tmp_dir()+"pat.h2pattern";

	H2Core::Pattern* pPatternLoaded = nullptr;
	H2Core::Pattern* pPatternReloaded = nullptr;
	H2Core::Pattern* pPatternCopied = nullptr;
	H2Core::Pattern* pPatternNew = nullptr;
	std::shared_ptr<H2Core::Drumkit> pDrumkit = nullptr;
	std::shared_ptr<H2Core::InstrumentList> pInstrumentList = nullptr;
	H2Core::XMLDoc doc;

	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( pDrumkit!=nullptr );
	pInstrumentList = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstrumentList->size()==4 );

	pPatternLoaded = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), pInstrumentList );
	CPPUNIT_ASSERT( pPatternLoaded );

	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", H2Core::License(), sPatternPath, true ) );

	// Check for double freeing when destructing both copy and original.
	pPatternCopied = new H2Core::Pattern( pPatternLoaded );

	// Is stored pattern valid?
	CPPUNIT_ASSERT( doc.read( sPatternPath, H2Core::Filesystem::pattern_xsd_path() ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;

	// Check whether the constructor produces valid patterns.
	pPatternNew = new H2Core::Pattern( "test", "ladida", "", 1, 1 );
	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", H2Core::License(), sPatternPath, true ) );
	CPPUNIT_ASSERT( doc.read( sPatternPath, H2Core::Filesystem::pattern_xsd_path() ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;
	delete pPatternLoaded;
	delete pPatternCopied;
	delete pPatternNew;
	___INFOLOG( "passed" );
}

void XmlTest::checkTestPatterns()
{
	___INFOLOG( "" );
	H2Core::XMLDoc doc;
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/pattern/pat.h2pattern" ),
							  H2Core::Filesystem::pattern_xsd_path() ) );
	___INFOLOG( "passed" );
}

void XmlTest::testPlaylist()
{
	___INFOLOG( "" );
	QString sPath = H2Core::Filesystem::tmp_dir()+"playlist.h2playlist";

	H2Core::Playlist::create_instance();
	H2Core::Playlist* pPlaylistCurrent = H2Core::Playlist::get_instance();
	H2Core::Playlist* pPlaylistLoaded = nullptr;
	H2Core::XMLDoc doc;

	CPPUNIT_ASSERT( pPlaylistCurrent->save_file( sPath, "ladida", true, false ) );
	CPPUNIT_ASSERT( doc.read( sPath, H2Core::Filesystem::playlist_xsd_path() ) );
	pPlaylistLoaded = H2Core::Playlist::load_file( sPath, false );
	CPPUNIT_ASSERT( pPlaylistLoaded != nullptr );

	delete pPlaylistLoaded;
	delete pPlaylistCurrent;
	___INFOLOG( "passed" );
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
}
