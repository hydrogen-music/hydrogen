#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>

#include <QCoreApplication>

#include "test_helper.h"

void setupEnvironment(unsigned log_level)
{
	/* Logger */
	H2Core::Logger* logger = H2Core::Logger::bootstrap( log_level );
	/* Test helper */
	TestHelper::create_instance();
	TestHelper* test_helper = TestHelper::get_instance();
	/* Object */
	H2Core::Object::bootstrap( logger, logger->should_log( H2Core::Logger::Debug ) );
	/* Filesystem */
	H2Core::Filesystem::bootstrap( logger, test_helper->data_dir() );
	H2Core::Filesystem::info();

	/* Use fake audio driver */
	H2Core::Preferences::create_instance();
	H2Core::Preferences* preferences = H2Core::Preferences::get_instance();
	preferences->m_sAudioDriver = "Fake";

	H2Core::Hydrogen::create_instance();

	H2Core::Filesystem::rm( H2Core::Filesystem::tmp_dir(), true );
}


int main( int argc, char **argv)
{
	QCoreApplication app(argc, argv);

	QCommandLineParser parser;
	QCommandLineOption verboseOption( QStringList() << "V" << "verbose", "Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH","Level");
	parser.addHelpOption();
	parser.addOption( verboseOption );
	parser.process(app);
	QString sVerbosityString = parser.value( verboseOption );
	unsigned logLevelOpt = H2Core::Logger::None;
	if( parser.isSet(verboseOption) ){
		if( !sVerbosityString.isEmpty() )
		{
			logLevelOpt =  H2Core::Logger::parse_log_level( sVerbosityString.toLocal8Bit() );
		} else {
			logLevelOpt = H2Core::Logger::Error|H2Core::Logger::Warning;
		}
	}

	setupEnvironment(logLevelOpt);

	CppUnit::TextUi::TestRunner runner;
	CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
	runner.addTest( registry.makeTest() );
	bool wasSuccessful = runner.run( "", false );

	return wasSuccessful ? 0 : 1;
}
