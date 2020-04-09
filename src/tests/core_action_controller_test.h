#include <cppunit/extensions/HelperMacros.h>

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/core_action_controller.h>

namespace H2Core {
	class CoreActionController;
};

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testNewSong );
	CPPUNIT_TEST( testOpenSong );
	CPPUNIT_TEST( testSaveSong );
	CPPUNIT_TEST( testSaveSongAs );
	CPPUNIT_TEST( testQuit );
	CPPUNIT_TEST_SUITE_END();
	
private:
	H2Core::Hydrogen* m_pHydrogen;
	H2Core::CoreActionController* m_pController;
	
	
public:
	// Initializes the private member variables and loads an empty
	// song into Hydrogen.
	void setUp();
	// Loads an empty song into Hydrogen.
	void tearDown();
	
	// Tests the CoreActionController::newSong() command by 
	// - providing an improper file name
	// - providing the path to an existing file with proper naming
	// - providing the path to a non-existing file with proper naming
	void testNewSong();
	void testOpenSong();
	void testSaveSong();
	void testSaveSongAs();
	void testQuit();
	
};

CPPUNIT_TEST_SUITE_REGISTRATION( CoreActionControllerTest );
