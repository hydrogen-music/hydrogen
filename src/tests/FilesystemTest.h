#include <cppunit/extensions/HelperMacros.h>

#include <core/Helpers/Filesystem.h>

class FilesystemTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( FilesystemTest );
	CPPUNIT_TEST( testPermissions );
	CPPUNIT_TEST_SUITE_END();
	
	
public:
	void setUp();
	void tearDown();
	
	void testPermissions();

private:
	QString m_sNotExistingPath;
	QString m_sNoAccessPath;
	QString m_sReadOnlyPath;
	QString m_sFullAccessPath;
	// To ensure Qt does handle the temporary files consistently.
	QString m_sTmpPath;
	QString m_sDemoSongSysPath;
};
CPPUNIT_TEST_SUITE_REGISTRATION( FilesystemTest );
