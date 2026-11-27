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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2C_QT_GUI_GATE_TEST_H
#define H2C_QT_GUI_GATE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/**
 * Pins the headless boundary of hydrogen-core: no source file under
 * src/core may include a QtGui/QtWidgets header, and the core
 * library must not link against either module. Everything
 * GUI-flavored (QColor, QKeySequence, ...) is converted at the
 * Skin.h boundary instead.
 */
class QtGuiGateTest : public CPPUNIT_NS::TestCase {
	CPPUNIT_TEST_SUITE( QtGuiGateTest );
	CPPUNIT_TEST( testCoreSourcesFreeOfGuiIncludes );
	CPPUNIT_TEST( testCoreCMakeFreeOfGuiLinkage );
	CPPUNIT_TEST_SUITE_END();

public:
	void testCoreSourcesFreeOfGuiIncludes();
	void testCoreCMakeFreeOfGuiLinkage();
};

#endif
