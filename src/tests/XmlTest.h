#ifndef XML_TEST_H
#define XML_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class XmlTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(XmlTest);
	CPPUNIT_TEST(testDrumkit);
	CPPUNIT_TEST(testDrumkit_UpgradeInvalidADSRValues);
	CPPUNIT_TEST(testPattern);
	CPPUNIT_TEST(testShippedDrumkits);
	CPPUNIT_TEST_SUITE_END();

	public:
		// Removes all .bak backup files from the test data folder.
		void tearDown();
		void testDrumkit();
		void testDrumkit_UpgradeInvalidADSRValues();
		void testPattern();
		// Check whether the drumkits provided alongside this repo can
		// be validated against the drumkit XSD.
		void testShippedDrumkits();
	
};


#endif
