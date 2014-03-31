#include "adsr_test.h"

#include <hydrogen/basics/adsr.h>
#include <stdio.h>

CPPUNIT_TEST_SUITE_REGISTRATION( ADSRTest );

using namespace H2Core;

const double delta = 0.00001;

void ADSRTest::setUp()
{
	m_adsr = new ADSR( 1.0, 2.0, 0.8, 256.0 );
}

void ADSRTest::tearDown()
{
	delete m_adsr;
}

void ADSRTest::testAttack()
{
	m_adsr->attack();

	/* Attack */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, m_adsr->get_value( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.774681627750397, m_adsr->get_value( 0.5 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, m_adsr->get_value( 0.1 ), delta );

	/* Decay */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0, m_adsr->get_value( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.830416560173035, m_adsr->get_value( 1.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->get_value( 1.0 ), delta );

	/* Sustain */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->get_value( 4.0 ), delta );
}


void ADSRTest::testRelease()
{
	m_adsr->get_value( 1.1 ); // move past Attack
	m_adsr->get_value( 2.1 ); // move past Decay
	m_adsr->get_value( 0.1 ); // calculate and store sustain

	/* Release note, and check if it was on sustain value */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->release(), delta );

	/* Release */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.8, m_adsr->get_value( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.121666193008423, m_adsr->get_value( 128.0 ), delta );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, m_adsr->get_value( 128.0 ), delta );

	/* Idle */
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, m_adsr->get_value( 2.0 ), delta );
}
