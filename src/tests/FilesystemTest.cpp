#include "FilesystemTest.h"
#include <core/Preferences.h>

#include <QTest>

using namespace H2Core;

void FilesystemTest::setUp() {
#if !defined(Q_OS_MACX) || !defined(WIN32) 
	m_sNotExistingPath = QDir::homePath().append( "aFunnyNameYouWouldNotExpectInYourHomeFolder.h2song" );
	m_sNoAccessPath = "/etc/shadow";
	m_sReadOnlyPath = "/etc/profile";
	m_sFullAccessPath = Filesystem::usr_data_path().append( "test.h2song"  );
	m_sTmpPath = Filesystem::tmp_file_path( "test.h2song" );
#endif
}

void FilesystemTest::tearDown() {
#if !defined(Q_OS_MACX) || !defined(WIN32) 
	CPPUNIT_ASSERT( Filesystem::rm( m_sTmpPath, false ) );
#endif
}

void FilesystemTest::testPermissions(){
#if !defined(Q_OS_MACX) || !defined(WIN32) 
	CPPUNIT_ASSERT( ! Filesystem::file_exists( m_sNotExistingPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_exists( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::file_readable( m_sNoAccessPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_readable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( ! Filesystem::file_writable( m_sReadOnlyPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_writable( m_sFullAccessPath, true ) );

	CPPUNIT_ASSERT( Filesystem::file_exists( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_readable( m_sTmpPath, true ) );
	CPPUNIT_ASSERT( Filesystem::file_writable( m_sTmpPath, true ) );
#endif
}
