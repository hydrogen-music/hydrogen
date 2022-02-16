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
#include <core/CoreActionController.h>

#include <QDir>
#include <QTemporaryDir>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "TestHelper.h"
#include "assertions/File.h"

CPPUNIT_TEST_SUITE_REGISTRATION( XmlTest );


static bool check_samples_data( H2Core::Drumkit* dk, bool loaded )
{
	int count = 0;
	H2Core::InstrumentComponent::setMaxLayers( 16 );
	H2Core::InstrumentList* instruments = dk->get_instruments();
	for( int i=0; i<instruments->size(); i++ ) {
		count++;
		H2Core::Instrument* pInstr = ( *instruments )[i];
		for (std::vector<H2Core::InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			H2Core::InstrumentComponent* pComponent = *it;
			for ( int nLayer = 0; nLayer < H2Core::InstrumentComponent::getMaxLayers(); nLayer++ ) {
				H2Core::InstrumentLayer* pLayer = pComponent->get_layer( nLayer );
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
	QString dk_path = H2Core::Filesystem::tmp_dir()+"/dk0";

	H2Core::Drumkit* dk0 = nullptr;
	H2Core::Drumkit* dk1 = nullptr;
	H2Core::Drumkit* dk2 = nullptr;

	// load without samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit") );
	CPPUNIT_ASSERT( dk0!=nullptr );
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, dk0->get_instruments()->size() );
	//dk0->dump();

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( TestHelper::get_instance()->findDrumkitBackupFiles( "drumkits/baseKit/" )
					.size() == 0 );
	
	// manually load samples
	dk0->load_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	
	// load with samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ), true );
	CPPUNIT_ASSERT( dk0!=nullptr );
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	
	// unload samples
	dk0->unload_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	//dk0->dump();
	
	/*
	// save drumkit elsewhere
	dk0->set_name( "dk0" );
	CPPUNIT_ASSERT( dk0->save( dk_path, false ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( dk_path+"/snare.wav" ) );
	// load file
	dk1 = H2Core::Drumkit::load_file( dk_path+"/drumkit.xml" );
	CPPUNIT_ASSERT( dk1!=nullptr );
	//dk1->dump();
	// copy constructor
	dk2 = new H2Core::Drumkit( dk1 );
	dk2->set_name( "COPY" );
	CPPUNIT_ASSERT( dk2!=nullptr );
	// save file
	CPPUNIT_ASSERT( dk2->save_file( dk_path+"/drumkit.xml", true ) );
	*/
	
	delete dk0;
	//delete dk1;
	//delete dk2;
}

void XmlTest::testShippedDrumkits()
{
	H2Core::XMLDoc doc;
	for ( auto ii : H2Core::Filesystem::sys_drumkit_list() ) {
		CPPUNIT_ASSERT( doc.read( QString( "%1%2/drumkit.xml" )
								  .arg( H2Core::Filesystem::sys_drumkits_dir() )
								  .arg( ii ),
								  H2Core::Filesystem::drumkit_xsd_path() ) );

	}
}

//Load drumkit which includes instrument with invalid ADSR values.
// Expected behavior: The drumkit will be loaded successfully. 
//					  In addition, the drumkit file will be saved with 
//					  correct ADSR values.
void XmlTest::testDrumkit_UpgradeInvalidADSRValues()
{
	auto pTestHelper = TestHelper::get_instance();
	H2Core::Drumkit* pDrumkit = nullptr;

	//1. Check, if the drumkit has been loaded
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	
	//2. Make sure that the instruments of the drumkit have been loaded correctly (see GH issue #839)
	H2Core::InstrumentList* pInstruments = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstruments != nullptr );
	
	H2Core::Instrument* pFirstInstrument = pInstruments->get(0);
	CPPUNIT_ASSERT( pFirstInstrument != nullptr );
	
	H2Core::InstrumentLayer* pLayer = pFirstInstrument->get_components()->front()->get_layer(0);
	CPPUNIT_ASSERT( pLayer != nullptr );
	
	auto pSample = pLayer->get_sample();
	CPPUNIT_ASSERT( pSample != nullptr );
	
	CPPUNIT_ASSERT( pSample->get_filename() == QString("snare.wav"));
	
	//3. Make sure that the original (invalid) file has been saved as
	//a backup
	QStringList backupFiles = pTestHelper->findDrumkitBackupFiles( "drumkits/invAdsrKit" );
	CPPUNIT_ASSERT( backupFiles.size() == 1 );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_exists( backupFiles[ 0 ] ) );
		
	if( pDrumkit ) {
		delete pDrumkit;
	}

	//4. Load the drumkit again to assure updated file is valid
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/invAdsrKit") );
	backupFiles = pTestHelper->findDrumkitBackupFiles( "drumkits/invAdsrKit" );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( backupFiles.size() == 1 );
		 
	if ( pDrumkit ) {
		delete pDrumkit;
	}
	
	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::file_copy( backupFiles[ 0 ],
												   H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml" ),
												   true ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( backupFiles[ 0 ], false ) );
}

void XmlTest::testDrumkitUpgrade() {
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

		CPPUNIT_ASSERT( ! pCoreActionController->validateDrumkit( sDrumkitPath ) );

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
		CPPUNIT_ASSERT( pCoreActionController->validateDrumkit( sUpgradedKit ) );
		
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
}

void XmlTest::testPattern()
{
	QString pat_path = H2Core::Filesystem::tmp_dir()+"/pat";

	H2Core::Pattern* pat0 = nullptr;
	H2Core::Drumkit* dk0 = nullptr;
	H2Core::InstrumentList* instruments = nullptr;

	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( dk0!=nullptr );
	instruments = dk0->get_instruments();
	CPPUNIT_ASSERT( instruments->size()==4 );

	pat0 = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), instruments );
	CPPUNIT_ASSERT( pat0 );

	pat0->save_file( "dk_name", "author", "license", pat_path );

	delete pat0;
	delete dk0;
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
