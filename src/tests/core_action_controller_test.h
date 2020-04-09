#include <cppunit/extensions/HelperMacros.h>

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/core_action_controller.h>

namespace H2Core {
	class CoreActionController;
};

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST( testIsSongPathValid );
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
	
	// Tests the CoreActionController::newSong(),
	// CoreActionController::openSong(),
	// CoreActionController::saveSong()
	// CoreActionController::saveSongAs() methods.
	void testSessionManagement();
	
	// Tests CoreActionController::isSongPathValid()
	void testIsSongPathValid();
};

CPPUNIT_TEST_SUITE_REGISTRATION( CoreActionControllerTest );
