#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>

#include <QCoreApplication>

#include "test_helper.h"

void setupEnvironment()
{
	TestHelper::create_instance();
	TestHelper* test_helper = TestHelper::get_instance();

    int log_level = H2Core::Logger::Debug | H2Core::Logger::Info | H2Core::Logger::Warning | H2Core::Logger::Error;
    /* Logger */
    H2Core::Logger* logger = H2Core::Logger::bootstrap( log_level );
    /* Object */
    H2Core::Object::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );
    /* Filesystem */
    H2Core::Filesystem::bootstrap( logger, test_helper->data_dir() );
    H2Core::Filesystem::info();
    H2Core::Filesystem::rm( H2Core::Filesystem::tmp_dir(), true );

	/* Use fake audio driver */
	H2Core::Preferences::create_instance();
	H2Core::Preferences* preferences = H2Core::Preferences::get_instance();
	preferences->m_sAudioDriver = "Fake";

	H2Core::Hydrogen::create_instance();
}


int main( int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	setupEnvironment();

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	bool wasSuccessful = runner.run( "", false );

	return wasSuccessful;
}
