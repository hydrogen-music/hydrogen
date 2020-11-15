#include "hydrogen/config.h"

#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>

class TimeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( TimeTest );
	CPPUNIT_TEST( testElapsedTime );
	CPPUNIT_TEST_SUITE_END();
	
private:
	QString m_sValidPath;
	
public:
	void setUp();
	void tearDown();

	/**
	 * \param nPatternPos Index of the pattern group to locate to
	 *
	 * \return float Time in seconds passed at @a nPatternPos since
	 * the beginning of the song.
	 */
	float locateAndLookupTime( int nPatternPos );

	/**
	 * Adds a couple of tempo markers and moves to various places
	 * within the song to check the calculation of the elapsed time.
	 */
	void testElapsedTime();
};
CPPUNIT_TEST_SUITE_REGISTRATION( TimeTest );
