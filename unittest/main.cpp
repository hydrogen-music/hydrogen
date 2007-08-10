#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>


#include "instrument_test.cpp"
#include "note_test.cpp"

int main( int argc, char* argv[] )
{
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(new InstrumentTest() );
	runner.addTest(new NoteTest() );

	// Change the default outputter to a compiler error format outputter
	runner.setOutputter( new CppUnit::CompilerOutputter( &runner.result(), std::cerr ) );


	// Run the tests.
	bool wasSuccessful = runner.run();


	// Return error code 1 if the one of test failed.
	return wasSuccessful ? 0 : 1;
}
