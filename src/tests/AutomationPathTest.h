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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <cppunit/extensions/HelperMacros.h>

namespace CppUnit {
template <>
struct assertion_traits<std::pair<const float, float> > {
	static bool equal(
		const std::pair<const float, float>& lhs,
		const std::pair<const float, float>& rhs
	)
	{
		return lhs == rhs;
	}

	static std::string toString( const std::pair<const float, float>& p )
	{
		std::stringstream o;
		o << "(" << p.first << "," << p.second << ")";
		return o.str();
	}
};
}  // namespace CppUnit

class AutomationPathTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( AutomationPathTest );
	CPPUNIT_TEST( testConstruction );
	CPPUNIT_TEST( testEmptyPath );
	CPPUNIT_TEST( testOnePoint );
	CPPUNIT_TEST( testValueBeforeFirstPoint );
	CPPUNIT_TEST( testValueAfterLastPoint );
	CPPUNIT_TEST( testMidpointValue );
	CPPUNIT_TEST( testEmptyPathsEqual );
	CPPUNIT_TEST( testPathsEqual );
	CPPUNIT_TEST( testEmptyPathsNotEqual );
	CPPUNIT_TEST( testPathsNotEqual );
	CPPUNIT_TEST( testIterator );
	CPPUNIT_TEST( testFindPointInEmptyPath );
	CPPUNIT_TEST( testFindPoint );
	CPPUNIT_TEST( testFindNotFound );
	CPPUNIT_TEST( testMovePoint );
	CPPUNIT_TEST( testRemovePoint );
	CPPUNIT_TEST(testRead);
	CPPUNIT_TEST(testWrite);
	CPPUNIT_TEST(testRoundtripReadWrite);
	CPPUNIT_TEST_SUITE_END();

	const double delta = 0.0001;

   public:
	/** Test whether AutomationPaths are constructed correctly */
	void testConstruction();
	/** Empty automation path should always return
	   default value */
	void testEmptyPath();
	/** Test getting value of an anchor point */
	void testOnePoint();
	/** Test getting value before first point,
	   i.e if returned value is defined by first point */
	void testValueBeforeFirstPoint();
	/** Test whether value past the last point
	   is defined by that value */
	void testValueAfterLastPoint();
	/** Test getting value between two anchor points */
	void testMidpointValue();
	/** Test operator== and operator!= */
	void testEmptyPathsEqual();
	void testPathsEqual();
	void testEmptyPathsNotEqual();
	void testPathsNotEqual();
	void testIterator();
	void testFindPointInEmptyPath();
	void testFindPoint();
	void testFindNotFound();
	void testMovePoint();
	void testRemovePoint();
	void testRead();
	void testWrite();
	void testRoundtripReadWrite();
};
