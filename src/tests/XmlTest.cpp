#include "XmlTest.h"

#include <unistd.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Sample.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include "TestHelper.h"

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
	QString sDrumkitPath = H2Core::Filesystem::tmp_dir()+"dk0";

	H2Core::Drumkit* pDrumkitLoaded = nullptr;
	H2Core::Drumkit* pDrumkitReloaded = nullptr;
	H2Core::Drumkit* pDrumkitCopied = nullptr;
	H2Core::Drumkit* pDrumkitNew = nullptr;
	H2Core::XMLDoc doc;

	// load without samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit") );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, pDrumkitLoaded->get_instruments()->size() );

	// Check if drumkit was valid (what we assume in this test)
	CPPUNIT_ASSERT( ! H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml.bak" ) ) );
	
	// manually load samples
	pDrumkitLoaded->load_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );
	
	// load with samples
	pDrumkitLoaded = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ), true );
	CPPUNIT_ASSERT( pDrumkitLoaded!=nullptr );
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, true ) );
	
	// unload samples
	pDrumkitLoaded->unload_samples();
	CPPUNIT_ASSERT( pDrumkitLoaded->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( pDrumkitLoaded, false ) );
	
	// save drumkit elsewhere
	pDrumkitLoaded->set_name( "pDrumkitLoaded" );
	CPPUNIT_ASSERT( pDrumkitLoaded->save( sDrumkitPath, false ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/drumkit.xml" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/crash.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/hh.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/kick.wav" ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::file_readable( sDrumkitPath+"/snare.wav" ) );

	// Check whether the generated drumkit is valid.
	CPPUNIT_ASSERT( doc.read( sDrumkitPath + "/drumkit.xml",
							  H2Core::Filesystem::drumkit_xsd_path() ) );
	
	// load file
	pDrumkitReloaded = H2Core::Drumkit::load_file( sDrumkitPath+"/drumkit.xml" );
	CPPUNIT_ASSERT( pDrumkitReloaded!=nullptr );
	
	// copy constructor
	pDrumkitCopied = new H2Core::Drumkit( pDrumkitReloaded );
	CPPUNIT_ASSERT( pDrumkitCopied!=nullptr );
	// save file
	pDrumkitCopied->set_name( "COPY" );
	CPPUNIT_ASSERT( pDrumkitCopied->save_file( sDrumkitPath+"/drumkit.xml", true ) );
		CPPUNIT_ASSERT( pDrumkitCopied->save_file( sDrumkitPath+"/drumkit.xml", true ) );

	delete pDrumkitReloaded;

	// Check whether blank drumkits are valid.
	pDrumkitNew = new H2Core::Drumkit();
	CPPUNIT_ASSERT( pDrumkitNew != nullptr );
	CPPUNIT_ASSERT( pDrumkitNew->save_file( sDrumkitPath+"/drumkit.xml", true ) );
	CPPUNIT_ASSERT( doc.read( sDrumkitPath + "/drumkit.xml",
							  H2Core::Filesystem::drumkit_xsd_path() ) );
	pDrumkitReloaded = H2Core::Drumkit::load_file( sDrumkitPath+"/drumkit.xml" );
	CPPUNIT_ASSERT( pDrumkitReloaded!=nullptr );

	delete pDrumkitLoaded;
	delete pDrumkitReloaded;
	delete pDrumkitCopied;
	delete pDrumkitNew;

	// Cleanup
	H2Core::Filesystem::rm( sDrumkitPath, true );
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
	
	//3. Make sure that the original (invalid) file has been saved as a backup
	CPPUNIT_ASSERT( H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak") ) );
		
	if( pDrumkit ) {
		delete pDrumkit;
	}

	//4. Load the drumkit again to assure updated file is valid
	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/invAdsrKit") );
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( ! H2Core::Filesystem::file_exists( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak.1") ) );
		 
	if ( pDrumkit ) {
		delete pDrumkit;
	}
	
	// Cleanup
	CPPUNIT_ASSERT( H2Core::Filesystem::file_copy( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak" ), H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml" ), true ) );
	CPPUNIT_ASSERT( H2Core::Filesystem::rm( H2TEST_FILE( "/drumkits/invAdsrKit/drumkit.xml.bak"), false ) );
}

void XmlTest::testPattern()
{
	QString sPatternPath = H2Core::Filesystem::tmp_dir()+"pat.h2pattern";

	H2Core::Pattern* pPatternLoaded = nullptr;
	H2Core::Pattern* pPatternReloaded = nullptr;
	H2Core::Pattern* pPatternCopied = nullptr;
	H2Core::Pattern* pPatternNew = nullptr;
	H2Core::Drumkit* pDrumkit = nullptr;
	H2Core::InstrumentList* pInstrumentList = nullptr;
	H2Core::XMLDoc doc;

	pDrumkit = H2Core::Drumkit::load( H2TEST_FILE( "/drumkits/baseKit" ) );
	CPPUNIT_ASSERT( pDrumkit!=nullptr );
	pInstrumentList = pDrumkit->get_instruments();
	CPPUNIT_ASSERT( pInstrumentList->size()==4 );

	pPatternLoaded = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), pInstrumentList );
	CPPUNIT_ASSERT( pPatternLoaded );

	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", "license", sPatternPath, true ) );

	// Check for double freeing when destructing both copy and original.
	pPatternCopied = new H2Core::Pattern( pPatternLoaded );

	// Is stored pattern valid?
	CPPUNIT_ASSERT( doc.read( sPatternPath, H2Core::Filesystem::pattern_xsd_path() ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;

	// Check whether the constructor produces valid patterns.
	pPatternNew = new H2Core::Pattern( "test", "ladida", "", 1, 1 );
	CPPUNIT_ASSERT( pPatternLoaded->save_file( "dk_name", "author", "license", sPatternPath, true ) );
	CPPUNIT_ASSERT( doc.read( sPatternPath, H2Core::Filesystem::pattern_xsd_path() ) );
	pPatternReloaded = H2Core::Pattern::load_file( sPatternPath, pInstrumentList );
	CPPUNIT_ASSERT( pPatternReloaded != nullptr );

	delete pPatternReloaded;
	delete pPatternLoaded;
	delete pPatternCopied;
	delete pPatternNew;
	delete pDrumkit;
}

void XmlTest::checkTestPatterns()
{
	H2Core::XMLDoc doc;
	CPPUNIT_ASSERT( doc.read( H2TEST_FILE( "/pattern/pat.h2pattern" ),
							  H2Core::Filesystem::pattern_xsd_path() ) );
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
