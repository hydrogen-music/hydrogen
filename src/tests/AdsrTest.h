#ifndef ADSR_TEST_H
#define ADSR_TEST_H

#include <cppunit/extensions/HelperMacros.h>

namespace H2Core {
	class ADSR;
};

class ADSRTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( ADSRTest );
	CPPUNIT_TEST( testAttack );
	CPPUNIT_TEST( testRelease );
	CPPUNIT_TEST_SUITE_END();

	private:
	H2Core::ADSR* m_adsr;

	public:
	virtual void setUp();
	virtual void tearDown();
	
	void testAttack();
	void testRelease();
};

#endif
