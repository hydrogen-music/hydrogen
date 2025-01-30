/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef ASSERTIONS_FILE_H
#define ASSERTIONS_FILE_H

#include <cppunit/extensions/HelperMacros.h>
#include <QString>
#include <QFile>

namespace H2Test {

	enum class FileType {
		/** Generic XML file */
		Xml = 0,
		Song = 1,
		Preferences = 2,
		Drumkit = 3,
		Theme = 4
	};

	void checkFilesEqual( const QString& sExpected, const QString& sActual,
						  bool bEquality, CppUnit::SourceLine sourceLine );
	void checkXmlFilesEqual( const QString& sExpected, const QString& sActual,
							 const bool bEquality, const FileType& fileType,
							 CppUnit::SourceLine sourceLine );
	void checkDirsEqual( const QString& sDirExpected, const QString& sDirActual,
						 bool bEquality, CppUnit::SourceLine sourceLine );

	void checkFileArgs( const QString& sExpected, QFile& f1,
						const QString& sActual, QFile& f2,
						const bool bEquality, const FileType& fileType,
						CppUnit::SourceLine sourceLine );
}

/**
 * \brief Assert that two files' contents are the same
 **/
#define H2TEST_ASSERT_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkFilesEqual( sExpected, sActual, true, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_FILES_UNEQUAL( sExpected, sActual ) \
	H2Test::checkFilesEqual( sExpected, sActual, false, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_XML_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, true, H2Test::FileType::Xml, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_XML_FILES_UNEQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, false, H2Test::FileType::Xml, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_H2SONG_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, true, H2Test::FileType::Song, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_H2SONG_FILES_UNEQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, false, H2Test::FileType::Song, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_DRUMKIT_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, true, H2Test::FileType::Drumkit, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_DRUMKIT_FILES_UNEQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, false, H2Test::FileType::Drumkit, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_PREFERENCES_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, true, H2Test::FileType::Preferences, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_THEME_FILES_EQUAL( sExpected, sActual ) \
	H2Test::checkXmlFilesEqual( sExpected, sActual, true, H2Test::FileType::Theme, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_DIRS_EQUAL( sExpected, sActual ) \
	H2Test::checkDirsEqual( sExpected, sActual, true, CPPUNIT_SOURCELINE() )
#define H2TEST_ASSERT_DIRS_UNEQUAL( sExpected, sActual ) \
	H2Test::checkDirsEqual( sExpected, sActual, false, CPPUNIT_SOURCELINE() )

#endif
