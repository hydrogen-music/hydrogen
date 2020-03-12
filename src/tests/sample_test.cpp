#include "pattern_test.h"
#include "test_helper.h"

#include <hydrogen/basics/sample.h>

class SampleTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( SampleTest );
	CPPUNIT_TEST( testLoadInvalidSample );

	CPPUNIT_TEST_SUITE_END();

	void testLoadInvalidSample()
	{
		H2Core::Sample* pSample = nullptr;
		
		//TC1: Sample does not exist
		QString SamplePath("PathDoesNotExist");
		pSample = H2Core::Sample::load( SamplePath );
	
		CPPUNIT_ASSERT(pSample == nullptr);
	
		//TC2: Sample does exist, but is not a valid sample
		pSample = H2Core::Sample::load( H2TEST_FILE("drumkit/drumkit.xml") );
		CPPUNIT_ASSERT(pSample == nullptr);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( SampleTest );
