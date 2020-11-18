#include <cppunit/extensions/HelperMacros.h>

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>

class CoreActionControllerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( CoreActionControllerTest );
	CPPUNIT_TEST( testSessionManagement );
	CPPUNIT_TEST( testIsSongPathValid );
	CPPUNIT_TEST_SUITE_END();
	
private:
	H2Core::Hydrogen* m_pHydrogen;
	H2Core::CoreActionController* m_pController;
	
	QString m_sFileNameImproper;
	QString m_sFileName;
	QString m_sFileName2;
	
public:
	// Initializes the private member variables and loads an empty
	// song into Hydrogen.
	void setUp();
	// Loads an empty song into Hydrogen and deletes all temporary
	// files.
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
