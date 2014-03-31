
#include "hydrogen/logger.h"
#include "hydrogen/object.h"
#include "hydrogen/helpers/filesystem.h"

void rubberband_test( const QString& sample_path );
int xml_drumkit( int log_level );
int xml_pattern( int log_level );
void pattern_run_tests();

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
	H2Core::Filesystem::rm( H2Core::Filesystem::tmp_dir(), true );

	rubberband_test( H2Core::Filesystem::drumkit_path_search( "GMkit" )+"/cym_Jazz.flac" );
	xml_drumkit( log_level );
	xml_pattern( log_level );
	
	pattern_run_tests();

	delete logger;

	return EXIT_SUCCESS;
}
