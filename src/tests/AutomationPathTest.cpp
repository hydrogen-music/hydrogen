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
#include <core/Basics/AutomationPath.h>

using namespace H2Core;

namespace CppUnit {
template<>
struct assertion_traits<std::pair<const float,float> >
{
	static bool equal(const std::pair<const float,float> &lhs, const std::pair<const float,float> &rhs)
	{
		return lhs == rhs;
	}

	static std::string toString(const std::pair<const float,float> &p)
	{
		std::stringstream o;
		o << "(" << p.first << "," << p.second << ")";
		return o.str();
	}
};
}

class AutomationPathTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(AutomationPathTest);
	CPPUNIT_TEST(testConstruction);
	CPPUNIT_TEST(testEmptyPath);
	CPPUNIT_TEST(testOnePoint);
	CPPUNIT_TEST(testValueBeforeFirstPoint);
	CPPUNIT_TEST(testValueAfterLastPoint);
	CPPUNIT_TEST(testMidpointValue);
	CPPUNIT_TEST(testEmptyPathsEqual);
	CPPUNIT_TEST(testPathsEqual);
	CPPUNIT_TEST(testEmptyPathsNotEqual);
	CPPUNIT_TEST(testPathsNotEqual);
	CPPUNIT_TEST(testIterator);
	CPPUNIT_TEST(testFindPointInEmptyPath);
	CPPUNIT_TEST(testFindPoint);
	CPPUNIT_TEST(testFindNotFound);
	CPPUNIT_TEST(testMovePoint);
	CPPUNIT_TEST(testRemovePoint);
	CPPUNIT_TEST_SUITE_END();

	const double delta = 0.0001;

	public:
	
	/* Test whether AutomationPaths are constructed correctly */
	void testConstruction()
	{
	___INFOLOG( "" );
		AutomationPath p(0.2f, 0.8f, 0.6f);

		CPPUNIT_ASSERT(p.empty());

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.2,
				static_cast<double>(p.get_min()),
				delta);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.8,
				static_cast<double>(p.get_max()),
				delta);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.6,
				static_cast<double>(p.get_default()),
				delta);
	___INFOLOG( "passed" );
	}
	

	/* Empty automation path should always return
	   default value */
	void testEmptyPath()
	{
	___INFOLOG( "" );
		AutomationPath p1(0.0f, 1.0f, 0.0f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.0,
				static_cast<double>(p1.get_value(0.0f)),
				delta);
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.0,
				static_cast<double>(p1.get_value(2.0f)),
				delta);

		AutomationPath p2(0.0f, 1.0f, 0.3f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.3,
				static_cast<double>(p2.get_value(0.0f)),
				delta);
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.3,
				static_cast<double>(p2.get_value(5.0f)),
				delta);

		
		AutomationPath p3(0.0f, 1.0f, 1.0f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				1.0,
				static_cast<double>(p3.get_value(0.0f)),
				delta);
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				1.0,
				static_cast<double>(p3.get_value(7.0f)),
				delta);
	___INFOLOG( "passed" );
	}


	/* Test getting value of an anchor point */
	void testOnePoint()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.5f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.5,
				static_cast<double>(p.get_value(1.0f)),
				delta);
	___INFOLOG( "passed" );
	}


	/* Test getting value before first point,
	   i.e if returned value is defined by first point */
	void testValueBeforeFirstPoint()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.5f);
		p.add_point(2.0f, 0.7f);

		CPPUNIT_ASSERT(! p.empty());

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.5,
				static_cast<double>(p.get_value(0.0f)),
				delta);
	___INFOLOG( "passed" );
	}


	/* Test whether value past the last point
	   is defined by that value */
	void testValueAfterLastPoint()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.4f);
		p.add_point(2.0f, 0.6f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.6,
				static_cast<double>(p.get_value(3.0f)),
				delta);
	___INFOLOG( "passed" );
	}


	/* Test getting value between two anchor points */
	void testMidpointValue()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.2f);
		p.add_point(2.0f, 0.4f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.3,
				static_cast<double>(p.get_value(1.5f)),
				delta);
	___INFOLOG( "passed" );
	}

	
	/* Test operator== and operator!= */
	void testEmptyPathsEqual()
	{
	___INFOLOG( "" );
		AutomationPath p1(-2.0f, 2.0f, 1.0f);
		AutomationPath p2(-2.0f, 2.0f, 1.0f);

		CPPUNIT_ASSERT(p1 == p2);
		CPPUNIT_ASSERT(!(p1 != p2));
	___INFOLOG( "passed" );
	}

	void testPathsEqual()
	{
	___INFOLOG( "" );
		AutomationPath p1(-4.0f, 3.0f, 1.5f);
		p1.add_point(1.0f, 0.0f);
		p1.add_point(2.0f, 2.0f);

		AutomationPath p2(-4.0f, 3.0f, 1.5f);
		p2.add_point(1.0f, 0.0f);
		p2.add_point(2.0f, 2.0f);

		CPPUNIT_ASSERT(p1 == p2);
		CPPUNIT_ASSERT(!(p1 != p2));
	___INFOLOG( "passed" );
	}

	void testEmptyPathsNotEqual()
	{
	___INFOLOG( "" );
		AutomationPath p1(-2.0f, 2.0f, 1.0f);
		AutomationPath p2(-1.0f, 1.0f, 0.0f);

		CPPUNIT_ASSERT(p1 != p2);
		CPPUNIT_ASSERT(!(p1 == p2));
	___INFOLOG( "passed" );
	}

	void testPathsNotEqual()
	{
	___INFOLOG( "" );
		AutomationPath p1(-2.0f, 2.0f, 1.0f);
		p1.add_point(1.0f, 0.0f);

		AutomationPath p2(-2.0f, 2.0f, 1.0f);
		p2.add_point(2.0f, 2.0f);

		CPPUNIT_ASSERT(p1 != p2);
		CPPUNIT_ASSERT(!(p1 == p2));
	___INFOLOG( "passed" );
	}

	void testIterator()
	{
	___INFOLOG( "" );
		typedef std::pair<const float,float> pair;
		AutomationPath p(0.0f, 4.0f, 1.0f);
		p.add_point(0.0f, 0.0f);
		p.add_point(1.0f, 2.0f);
		p.add_point(2.0f, 4.0f);

		auto i = p.begin();
		CPPUNIT_ASSERT(i != p.end());
		CPPUNIT_ASSERT_EQUAL(pair(0.0f,0.0f), *i);

		i++;
		CPPUNIT_ASSERT(i != p.end());
		CPPUNIT_ASSERT_EQUAL(pair(1.0f,2.0f), *i);

		i++;
		CPPUNIT_ASSERT(i != p.end());
		CPPUNIT_ASSERT_EQUAL(pair(2.0f,4.0f), *i);

		i++;
		CPPUNIT_ASSERT(i == p.end());
	___INFOLOG( "passed" );
	}


	void testFindPointInEmptyPath()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);

		auto iter = p.find(0.0f);
		CPPUNIT_ASSERT(iter == p.end());

		auto iter2 = p.find(22.0f);
		CPPUNIT_ASSERT(iter2 == p.end());
	___INFOLOG( "passed" );
	}

	void testFindPoint()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);
		p.add_point(4.0f, 0.5f);

		auto iter = p.find(4.0f);
		CPPUNIT_ASSERT(iter == p.begin());

		auto iter2 = p.find(4.4f);
		CPPUNIT_ASSERT(iter2 == p.begin());

		auto iter3 = p.find(3.6f);
		CPPUNIT_ASSERT(iter3 == p.begin());
	___INFOLOG( "passed" );
	}


	void testFindNotFound()
	{
	___INFOLOG( "" );
		AutomationPath p(0.0f, 1.0f, 1.0f);
		p.add_point(2.0f, 0.2f);

		auto iter = p.find(1.3f);
		CPPUNIT_ASSERT(iter == p.end());

		auto iter2 = p.find(2.6f);
		CPPUNIT_ASSERT(iter2 == p.end());
	___INFOLOG( "passed" );
	}


	void testMovePoint()
	{
	___INFOLOG( "" );
		typedef std::pair<const float,float> pair;
		AutomationPath p(0.0f, 1.0f, 1.0f);
		p.add_point(5.0f, 0.5f);

		auto in = p.begin();
		auto out = p.move(in, 6.0f, 1.0f);

		CPPUNIT_ASSERT(out == p.begin());
		CPPUNIT_ASSERT_EQUAL(
				pair(6.0f, 1.0f),
				*out
		);
	___INFOLOG( "passed" );
	}


	void testRemovePoint()
	{
	___INFOLOG( "" );
		AutomationPath p(1.0f, 1.0f, 1.0f);
		p.add_point(0.0f, 0.0f);

		p.remove_point(0.0f);

		CPPUNIT_ASSERT(p.empty());
		CPPUNIT_ASSERT(p.find(0.0f) == p.end());
		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				1.0,
				static_cast<double>(p.get_value(0.0f)),
				delta);

	___INFOLOG( "passed" );
	}
};
