#ifndef MEMORY_LEAKAGE_TEST_H
#define MEMORY_LEAKAGE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class MemoryLeakageTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(MemoryLeakageTest);
	CPPUNIT_TEST(testConstructors);
	CPPUNIT_TEST(testLoading);
	CPPUNIT_TEST_SUITE_END();

public:
	void tearDown();
	/** Creates and destroys all basic classes using their
	 * destructors and checks whether there are some objects alive
	 * afterwards.
	 */
	void testConstructors();
	
	/** Creates and destroys all basic classes using their various
	 * loading routines and checks whether there are some objects
	 * alive afterwards.
	 */
	void testLoading();

};


#endif
