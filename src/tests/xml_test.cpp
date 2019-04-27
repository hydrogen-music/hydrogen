#include "xml_test.h"

#include <unistd.h>

#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/sample.h>

#include <hydrogen/helpers/filesystem.h>
#include "test_helper.h"

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
					H2Core::Sample* pSample = pLayer->get_sample();
					if( loaded ) {
						if( pSample->get_data_l()==0 || pSample->get_data_l()==0 ) return false;
					} else {
						if( pSample->get_data_l()!=0 || pSample->get_data_l()!=0 ) return false;
					}
				}

			}
		}
	}
	return ( count==4 );
}



void XmlTest::testDrumkit()
{
	return; //FIXME: skip test
	QString dk_path = H2Core::Filesystem::tmp_dir()+"/dk0";

	H2Core::Drumkit* dk0 = 0;
	H2Core::Drumkit* dk1 = 0;
	H2Core::Drumkit* dk2 = 0;

	// load without samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkit" ) );
	CPPUNIT_ASSERT( dk0!=0 );
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	CPPUNIT_ASSERT_EQUAL( 4, dk0->get_instruments()->size() );
	//dk0->dump();
	// manually load samples
	dk0->load_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	// load with samples
	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkit" ), true );
	CPPUNIT_ASSERT( dk0!=0 );
	CPPUNIT_ASSERT( dk0->samples_loaded()==true );
	CPPUNIT_ASSERT( check_samples_data( dk0, true ) );
	//dk0->dump();
	// unload samples
	dk0->unload_samples();
	CPPUNIT_ASSERT( dk0->samples_loaded()==false );
	CPPUNIT_ASSERT( check_samples_data( dk0, false ) );
	//dk0->dump();
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
	CPPUNIT_ASSERT( dk1!=0 );
	//dk1->dump();
	// copy constructor
	dk2 = new H2Core::Drumkit( dk1 );
	dk2->set_name( "COPY" );
	CPPUNIT_ASSERT( dk2!=0 );
	// save file
	CPPUNIT_ASSERT( dk2->save_file( dk_path+"/drumkit.xml", true ) );;

	delete dk0;
	delete dk1;
	delete dk2;
}


void XmlTest::testPattern()
{
	QString pat_path = H2Core::Filesystem::tmp_dir()+"/pat";

	H2Core::Pattern* pat0 = 0;
	H2Core::Drumkit* dk0 = 0;
	H2Core::InstrumentList* instruments = 0;

	dk0 = H2Core::Drumkit::load( H2TEST_FILE( "/drumkit" ) );
	CPPUNIT_ASSERT( dk0!=0 );
	instruments = dk0->get_instruments();
	CPPUNIT_ASSERT( instruments->size()==4 );

	pat0 = H2Core::Pattern::load_file( H2TEST_FILE( "/pattern/pat.h2pattern" ), instruments );
	CPPUNIT_ASSERT( pat0 );

	pat0->save_file( "dk_name", "author", "license", pat_path );

	delete pat0;
	delete dk0;
}

