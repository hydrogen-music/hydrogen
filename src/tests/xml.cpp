
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/pattern.h>

#define BASE_DIR    "./src/tests/data"

int xml_drumkit( int log_level ) {

    ___INFOLOG( "test xml drumkit validation, read and write" );

    H2Core::Drumkit* dk0 = 0;
    H2Core::Drumkit* dk1 = 0;
    H2Core::Drumkit* dk2 = 0;

    dk0 = H2Core::Drumkit::load( BASE_DIR"/drumkit" );
    if( !dk0 ) { return EXIT_FAILURE; }
    dk0->dump();
    if( !dk0->load_samples() ) { return EXIT_FAILURE; }
    if( !dk0->save_samples( BASE_DIR"/dk2" ) ) { return EXIT_FAILURE; }
    if( !dk0->unload_samples() ) { return EXIT_FAILURE; }
    dk0->save_file( BASE_DIR"/drumkit1.xml", true );
    dk1 = H2Core::Drumkit::load_file( BASE_DIR"/drumkit1.xml" );
    dk1->dump();
    if( !dk1 ) { return EXIT_FAILURE; }

    dk2 = new H2Core::Drumkit( dk1 );
    dk2->dump();
    dk2->set_name("COPY");
    if( !dk2 ) { return EXIT_FAILURE; }
    dk2->save_file( BASE_DIR"/drumkit2.xml", true );

    delete dk0;
    delete dk1;
    dk2->dump();
    delete dk2;

    H2Core::Filesystem::rm( BASE_DIR"/dk2", true );
    H2Core::Filesystem::rm( BASE_DIR"/drumkit1.xml" );
    H2Core::Filesystem::rm( BASE_DIR"/drumkit2.xml" );
    
    return EXIT_SUCCESS;
}
