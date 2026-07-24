/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2PLAYER_TEST_H
#define H2PLAYER_TEST_H

#include <QString>

#include <cppunit/extensions/HelperMacros.h>

class H2PlayerTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( H2PlayerTest );
	CPPUNIT_TEST( testHelpOption );
	CPPUNIT_TEST( testDefaultIpcMode );
	CPPUNIT_TEST( testNoIpcMode );
	CPPUNIT_TEST( testInteractiveMode );
	CPPUNIT_TEST( testMissingSongFile );
	CPPUNIT_TEST( testInvalidSongFile );
	CPPUNIT_TEST( testLogFileOption );
	CPPUNIT_TEST( testLogTimestampsOption );
	CPPUNIT_TEST( testLogColorsOption );
	CPPUNIT_TEST( testNoLogColorsOption );
	CPPUNIT_TEST( testVerboseOption );
	CPPUNIT_TEST( testConfigOption );
	CPPUNIT_TEST( testUserDataOption );
	CPPUNIT_TEST( testSystemDataOption );
#ifdef H2CORE_HAVE_OSC
	CPPUNIT_TEST( testOscPortOption );
#endif
	CPPUNIT_TEST_SUITE_END();

   public:
	/** Note that since h2player is a runtime dependency, we don't have to add
	 * it to CMakeLists.txt of the test folder but just check its present
	 * when running the unit tests.*/
	void setUp();
	void testHelpOption();
	void testDefaultIpcMode();
	void testNoIpcMode();
	void testInteractiveMode();
	void testMissingSongFile();
	void testInvalidSongFile();
	void testLogFileOption();
	void testLogTimestampsOption();
	void testLogColorsOption();
	void testNoLogColorsOption();
	void testVerboseOption();
	void testConfigOption();
	void testUserDataOption();
	void testSystemDataOption();
	void testOscPortOption();

   private:
	QString m_sH2PlayerPath;
	QString m_sTestSongPath;

	/** Start h2player with @a args (plus `-L` pointing at a temp log file and
	 * `--no-ipc`), wait, kill it, and return the log file content. */
	QString runPlayerAndReadLog( const QStringList& args,
								unsigned nTimeoutMs = 2000 );
	/** Copy the shipped default config to @a sDestPath and replace the
	 * `<oscServerPort>` element value with @a nNewPort. */
	QString prepareCustomConfig( const QString& sDestDir, int nNewPort );
};

#endif
