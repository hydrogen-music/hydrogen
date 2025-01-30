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

#include <cppunit/extensions/HelperMacros.h>
#include "TestHelper.h"
#include "assertions/File.h"

class AssertionTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( AssertionTest );
	CPPUNIT_TEST( testAssertions );
	CPPUNIT_TEST_SUITE_END();

	public:
	
	/* Test whether the helper assertion do work as intended */
	void testAssertions()
	{
	___INFOLOG( "" );

	H2TEST_ASSERT_FILES_EQUAL( H2TEST_FILE( "/drumkits/baseKit/hh.wav" ),
							   H2TEST_FILE( "/drumkits/baseKit/hh.wav" ) );
	H2TEST_ASSERT_FILES_UNEQUAL( H2TEST_FILE( "/drumkits/baseKit/hh.wav" ),
								 H2TEST_FILE( "/drumkits/baseKit/snare.wav" ) );
	H2TEST_ASSERT_XML_FILES_EQUAL( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml" ),
							   H2TEST_FILE( "/drumkits/baseKit/drumkit.xml" ) );
	H2TEST_ASSERT_XML_FILES_UNEQUAL( H2TEST_FILE( "/drumkits/baseKit/drumkit.xml" ),
									 H2TEST_FILE( "/drumkits/sampleKit/drumkit.xml" ) );
	H2TEST_ASSERT_DIRS_EQUAL( H2TEST_FILE( "/drumkits/baseKit" ),
							   H2TEST_FILE( "/drumkits/baseKit" ) );
	H2TEST_ASSERT_DIRS_UNEQUAL( H2TEST_FILE( "/drumkits/instrument-type-ref" ),
								H2TEST_FILE( "/drumkits/instrument-type-ref-duplicate" ) );

	___INFOLOG( "passed" );
	}
};
