
#include "hydrogen/logger.h"
#include "hydrogen/object.h"
#include "hydrogen/helpers/filesystem.h"

void rubberband_test();
int xml_drumkit( int log_level );

int main( int argc, char* argv[] )
{

    int log_level = H2Core::Logger::Debug | H2Core::Logger::Info | H2Core::Logger::Warning | H2Core::Logger::Error;
    /* Logger */
    H2Core::Logger* logger = H2Core::Logger::bootstrap( log_level );
    /* Object */
    H2Core::Object::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );
    /* Filesystem */
    H2Core::Filesystem::bootstrap( logger, "./data" );
    H2Core::Filesystem::info();

    rubberband_test();
    xml_drumkit( log_level );

    delete logger;

    return EXIT_SUCCESS;
}
