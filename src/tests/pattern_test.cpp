#include "pattern_test.h"

#include <hydrogen/audio_engine.h>
#include <hydrogen/basics/pattern.h>

CPPUNIT_TEST_SUITE_REGISTRATION( PatternTest );

using namespace H2Core;

void PatternTest::setUp()
{
	AudioEngine::create_instance();
}


void PatternTest::testPurgeInstrument()
{
	Instrument *i = new Instrument();
	Note *n = new Note( i, 1, 1.0, 1.0, 1.0, 1, 1.0 );

	Pattern *pat = new Pattern();
	pat->insert_note( n );
	CPPUNIT_ASSERT( pat->find_note( 1, -1, i) != NULL );

	pat->purge_instrument( i );
	CPPUNIT_ASSERT( pat->find_note( 1, -1, i) == NULL );

	delete pat;
}
