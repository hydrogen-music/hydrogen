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

#include "PatternTest.h"
#include "TestHelper.h"

#include <core/Basics/Sample.h>

class SampleTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( SampleTest );
	CPPUNIT_TEST( testLoadInvalidSample );

	CPPUNIT_TEST_SUITE_END();

	void testLoadInvalidSample()
	{
	___INFOLOG( "" );
		std::shared_ptr<H2Core::Sample> pSample;
		
		//TC1: Sample does not exist
		QString SamplePath("PathDoesNotExist");
		pSample = H2Core::Sample::load( SamplePath );
	
		CPPUNIT_ASSERT(pSample == nullptr);
	
		//TC2: Sample does exist, but is not a valid sample
		pSample = H2Core::Sample::load( H2TEST_FILE("drumkits/baseKit/drumkit.xml") );
		CPPUNIT_ASSERT(pSample == nullptr);
	___INFOLOG( "passed" );
	}
};
