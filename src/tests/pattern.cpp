/* Test cases for H2Core::Pattern */

#include <hydrogen/audio_engine.h>
#include <hydrogen/basics/pattern.h>

#include <stdio.h>
#include <stdlib.h>

using namespace H2Core;

/* Assertions */

#define ASSERT(cond) ASSERT_real((cond), #cond, __FILE__, __LINE__)

void ASSERT_real(bool cond, const char* as_string, const char* file, int line)
{
	if ( !cond ) {
		fprintf( stderr, "Assertion `%s' failed at %s line %d\n", as_string, file, line );
		exit( EXIT_FAILURE );
	}
}


/* Tests */

/* Test Pattern::purge_instrument */
void pattern_purge_instrument()
{
	AudioEngine::create_instance();

	Instrument *i = new Instrument();
	Note *n = new Note( i, 1, 1.0, 1.0, 1.0, 1, 1.0 );

	Pattern *pat = new Pattern();
	pat->insert_note( n );
	ASSERT( pat->find_note( 1, -1, i) != NULL );

	pat->purge_instrument( i );
	ASSERT( pat->find_note( 1, -1, i) == NULL );

	delete pat;
}

/* Test runner */
void pattern_run_tests()
{
	pattern_purge_instrument();
}

