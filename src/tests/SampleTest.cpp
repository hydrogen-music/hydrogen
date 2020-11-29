#include "PatternTest.h"
#include "TestHelper.h"

#include <core/Basics/Sample.h>

class SampleTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( SampleTest );
	CPPUNIT_TEST( testLoadInvalidSample );

	CPPUNIT_TEST_SUITE_END();

	void testLoadInvalidSample()
	{
		std::shared_ptr<H2Core::Sample> pSample;
		
		//TC1: Sample does not exist
		QString SamplePath("PathDoesNotExist");
		pSample = H2Core::Sample::load( SamplePath );
	
		CPPUNIT_ASSERT(pSample == nullptr);
	
		//TC2: Sample does exist, but is not a valid sample
		pSample = H2Core::Sample::load( H2TEST_FILE("drumkits/baseKit/drumkit.xml") );
		CPPUNIT_ASSERT(pSample == nullptr);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SampleTest );
