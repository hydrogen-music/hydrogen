/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef ADSR_TEST_H
#define ADSR_TEST_H

#include <cppunit/extensions/HelperMacros.h>

namespace H2Core {
	class ADSR;
};

class ADSRTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( ADSRTest );
	CPPUNIT_TEST( testAttack );
	CPPUNIT_TEST( testRelease );
	CPPUNIT_TEST_SUITE_END();

	private:
	std::shared_ptr<H2Core::ADSR> m_adsr;

	public:
	virtual void setUp();
	
	void testAttack();
	void testRelease();
};

#endif
