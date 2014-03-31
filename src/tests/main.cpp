#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <hydrogen/helpers/filesystem.h>

void setupEnvironment()
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
}


int main( int argc, char **argv)
{
	setupEnvironment();

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	bool wasSuccessful = runner.run( "", false );

	return wasSuccessful;
}
