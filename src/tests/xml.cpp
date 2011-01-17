
#include <hydrogen/helpers/filesystem.h>

#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/sample.h>

#define BASE_DIR    "./src/tests/data"

static void spec( bool cond, const char* msg ) {
    if( !cond ) {
        fprintf(stderr, "\033[31m  ** %s\n", msg );
        sleep( 1 );
        exit( EXIT_FAILURE);
    }
}

static bool check_samples_data( H2Core::Drumkit* dk, bool loaded ) {
    H2Core::InstrumentList* instruments = dk->get_instruments();
    for( int i=0; i<instruments->size(); i++ ) {
        H2Core::Instrument* instrument = ( *instruments )[i];
        for ( int n = 0; n < MAX_LAYERS; n++ ) {
            H2Core::InstrumentLayer* layer = instrument->get_layer( n );
            if( layer ) {
                H2Core::Sample* sample = layer->get_sample();
                if( loaded ) {
                    if( sample->get_data_l()==0 || sample->get_data_l()==0 ) return false;
                } else {
                    if( sample->get_data_l()!=0 || sample->get_data_l()!=0 ) return false;
                }
            }
        }
    }
    return true;
}

int xml_drumkit( int log_level ) {

    ___INFOLOG( "test xml drumkit validation, read and write" );

    H2Core::Drumkit* dk0 = 0;
    H2Core::Drumkit* dk1 = 0;
    H2Core::Drumkit* dk2 = 0;

    // load without samples
    dk0 = H2Core::Drumkit::load( BASE_DIR"/drumkit" );
    spec( dk0!=0, "dk0 should not be null" );
    spec( dk0->samples_loaded()==false, "samples should NOT be loaded" );
    spec( check_samples_data( dk0, false ), "sample data should be NULL" );
    // manually load samples
    spec( dk0->load_samples()==true, "should be able to load sample" );
    spec( dk0->samples_loaded()==true, "samples should be loaded" );
    spec( check_samples_data( dk0, true ), "sample data should NOT be NULL" );
    // load with samples
    dk0 = H2Core::Drumkit::load( BASE_DIR"/drumkit", true );
    spec( dk0!=0, "dk0 should not be null" );
    spec( dk0->samples_loaded()==true, "samples should be loaded" );
    spec( check_samples_data( dk0, true ), "sample data should NOT be NULL" );
    dk0->dump();
    // save samples elsewhere
    spec( dk0->save_samples( BASE_DIR"/dk2" ), "should be able to save the samples" );
    spec( H2Core::Filesystem::file_readable( BASE_DIR"/dk2/crash.wav"), "crash.wav should exists and be readable" );
    spec( H2Core::Filesystem::file_readable( BASE_DIR"/dk2/hh.wav"), "hh.wav should exists and be readable" );
    spec( H2Core::Filesystem::file_readable( BASE_DIR"/dk2/kick.wav"), "kick.wav should exists and be readable" );
    spec( H2Core::Filesystem::file_readable( BASE_DIR"/dk2/snare.wav"), "snare.wav should exists and be readable" );
    H2Core::Filesystem::rm( BASE_DIR"/dk2", true );
    // unload samples
    spec( dk0->unload_samples(), "should be able to unload samples" );
    spec( dk0->samples_loaded()==false, "samples should NOT be loaded" );
    spec( check_samples_data( dk0, false ), "sample data should be NULL" );
    // save sample elsewhere
    spec( dk0->save_file( BASE_DIR"/drumkit1.xml", false ), "should be able to save drumkit" );
    dk1 = H2Core::Drumkit::load_file( BASE_DIR"/drumkit1.xml" );
    spec( dk1!=0, "should be able to reload drumkit" );
    dk1->dump();
    // copy constructor
    dk2 = new H2Core::Drumkit( dk1 );
    dk2->set_name("COPY");
    if( !dk2 ) { return EXIT_FAILURE; }
    dk2->save_file( BASE_DIR"/drumkit2.xml", true );

    delete dk0;
    delete dk1;
    delete dk2;

    H2Core::Filesystem::rm( BASE_DIR"/drumkit1.xml" );
    H2Core::Filesystem::rm( BASE_DIR"/drumkit2.xml" );
    
    return EXIT_SUCCESS;
}
