#include "PatternTest.h"

#include <core/AudioEngine.h>
#include <core/Basics/Pattern.h>

CPPUNIT_TEST_SUITE_REGISTRATION( PatternTest );

using namespace H2Core;

void PatternTest::setUp()
{
	AudioEngine::create_instance();
}


void PatternTest::testPurgeInstrument()
{
	Instrument *pInstrument = new Instrument();
	Note *pNote = new Note( pInstrument, 1, 1.0, 1.0, 1.0, 1, 1.0 );

	Pattern *pPattern = new Pattern();
	pPattern->insert_note( pNote );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) != nullptr );

	pPattern->purge_instrument( pInstrument );
	CPPUNIT_ASSERT( pPattern->find_note( 1, -1, pInstrument) == nullptr );

	delete pPattern;
}
