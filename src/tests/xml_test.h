#ifndef XML_TEST_H
#define XML_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class XmlTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(XmlTest);
	CPPUNIT_TEST(testDrumkit);
	CPPUNIT_TEST(testDrumkit_UpgradeInvalidADSRValues);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST_SUITE_END();

	public:
		void testDrumkit();
		void testDrumkit_UpgradeInvalidADSRValues();
		void testPattern();
	
};


#endif
