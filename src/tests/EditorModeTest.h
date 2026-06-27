/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_EDITOR_MODE_TEST_H
#define H2C_EDITOR_MODE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <core/Object.h>

class EditorModeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( EditorModeTest );
	CPPUNIT_TEST( testAttachesToEngineEndpoint );
	CPPUNIT_TEST( testFailedConnectionReported );
	CPPUNIT_TEST( testMirrorUsesPassiveAudioDriver );
	CPPUNIT_TEST( testReceivesEngineState );
	CPPUNIT_TEST( testIssuesCommands );
	CPPUNIT_TEST( testEngineSurvivesEditorDisconnect );
	CPPUNIT_TEST_SUITE_END();

public:
	void testAttachesToEngineEndpoint();
	void testFailedConnectionReported();
	void testMirrorUsesPassiveAudioDriver();
	void testReceivesEngineState();
	void testIssuesCommands();
	void testEngineSurvivesEditorDisconnect();
};

#endif
