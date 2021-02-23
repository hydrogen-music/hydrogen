#ifndef PATTERN_TEST_H
#define PATTERN_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class PatternTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(PatternTest);
	CPPUNIT_TEST(testPurgeInstrument);
	CPPUNIT_TEST_SUITE_END();

	public:
		virtual void setUp();
		void testPurgeInstrument();
};


#endif
