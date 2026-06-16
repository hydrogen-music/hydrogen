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

#include "AutomationPathTest.h"

#include "TestHelper.h"

#include <core/Basics/AutomationPath.h>
#include <core/Hydrogen.h>

using namespace H2Core;

/* Test whether AutomationPaths are constructed correctly */
void AutomationPathTest::testConstruction()
{
	___INFOLOG( "" );
	AutomationPath p( 0.2f, 0.8f, 0.6f );

	CPPUNIT_ASSERT( p.empty() );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.2, static_cast<double>( p.getMin() ), delta
	);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.8, static_cast<double>( p.getMax() ), delta
	);

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.6, static_cast<double>( p.getDefault() ), delta
	);
	___INFOLOG( "passed" );
}

/* Empty automation path should always return
   default value */
void AutomationPathTest::testEmptyPath()
{
	___INFOLOG( "" );
	AutomationPath p1( 0.0f, 1.0f, 0.0f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.0, static_cast<double>( p1.getValue( 0.0f ) ), delta
	);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.0, static_cast<double>( p1.getValue( 2.0f ) ), delta
	);

	AutomationPath p2( 0.0f, 1.0f, 0.3f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.3, static_cast<double>( p2.getValue( 0.0f ) ), delta
	);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.3, static_cast<double>( p2.getValue( 5.0f ) ), delta
	);

	AutomationPath p3( 0.0f, 1.0f, 1.0f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		1.0, static_cast<double>( p3.getValue( 0.0f ) ), delta
	);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		1.0, static_cast<double>( p3.getValue( 7.0f ) ), delta
	);
	___INFOLOG( "passed" );
}

/* Test getting value of an anchor point */
void AutomationPathTest::testOnePoint()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );

	p.addPoint( 1.0f, 0.5f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.5, static_cast<double>( p.getValue( 1.0f ) ), delta
	);
	___INFOLOG( "passed" );
}

/* Test getting value before first point,
   i.e if returned value is defined by first point */
void AutomationPathTest::testValueBeforeFirstPoint()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );

	p.addPoint( 1.0f, 0.5f );
	p.addPoint( 2.0f, 0.7f );

	CPPUNIT_ASSERT( !p.empty() );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.5, static_cast<double>( p.getValue( 0.0f ) ), delta
	);
	___INFOLOG( "passed" );
}

/* Test whether value past the last point
   is defined by that value */
void AutomationPathTest::testValueAfterLastPoint()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );

	p.addPoint( 1.0f, 0.4f );
	p.addPoint( 2.0f, 0.6f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.6, static_cast<double>( p.getValue( 3.0f ) ), delta
	);
	___INFOLOG( "passed" );
}

/* Test getting value between two anchor points */
void AutomationPathTest::testMidpointValue()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );

	p.addPoint( 1.0f, 0.2f );
	p.addPoint( 2.0f, 0.4f );

	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		0.3, static_cast<double>( p.getValue( 1.5f ) ), delta
	);
	___INFOLOG( "passed" );
}

/* Test operator== and operator!= */
void AutomationPathTest::testEmptyPathsEqual()
{
	___INFOLOG( "" );
	AutomationPath p1( -2.0f, 2.0f, 1.0f );
	AutomationPath p2( -2.0f, 2.0f, 1.0f );

	CPPUNIT_ASSERT( p1 == p2 );
	CPPUNIT_ASSERT( !( p1 != p2 ) );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testPathsEqual()
{
	___INFOLOG( "" );
	AutomationPath p1( -4.0f, 3.0f, 1.5f );
	p1.addPoint( 1.0f, 0.0f );
	p1.addPoint( 2.0f, 2.0f );

	AutomationPath p2( -4.0f, 3.0f, 1.5f );
	p2.addPoint( 1.0f, 0.0f );
	p2.addPoint( 2.0f, 2.0f );

	CPPUNIT_ASSERT( p1 == p2 );
	CPPUNIT_ASSERT( !( p1 != p2 ) );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testEmptyPathsNotEqual()
{
	___INFOLOG( "" );
	AutomationPath p1( -2.0f, 2.0f, 1.0f );
	AutomationPath p2( -1.0f, 1.0f, 0.0f );

	CPPUNIT_ASSERT( p1 != p2 );
	CPPUNIT_ASSERT( !( p1 == p2 ) );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testPathsNotEqual()
{
	___INFOLOG( "" );
	AutomationPath p1( -2.0f, 2.0f, 1.0f );
	p1.addPoint( 1.0f, 0.0f );

	AutomationPath p2( -2.0f, 2.0f, 1.0f );
	p2.addPoint( 2.0f, 2.0f );

	CPPUNIT_ASSERT( p1 != p2 );
	CPPUNIT_ASSERT( !( p1 == p2 ) );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testIterator()
{
	___INFOLOG( "" );
	typedef std::pair<const float, float> pair;
	AutomationPath p( 0.0f, 4.0f, 1.0f );
	p.addPoint( 0.0f, 0.0f );
	p.addPoint( 1.0f, 2.0f );
	p.addPoint( 2.0f, 4.0f );

	auto i = p.begin();
	CPPUNIT_ASSERT( i != p.end() );
	CPPUNIT_ASSERT_EQUAL( pair( 0.0f, 0.0f ), *i );

	i++;
	CPPUNIT_ASSERT( i != p.end() );
	CPPUNIT_ASSERT_EQUAL( pair( 1.0f, 2.0f ), *i );

	i++;
	CPPUNIT_ASSERT( i != p.end() );
	CPPUNIT_ASSERT_EQUAL( pair( 2.0f, 4.0f ), *i );

	i++;
	CPPUNIT_ASSERT( i == p.end() );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testFindPointInEmptyPath()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );

	auto iter = p.find( 0.0f );
	CPPUNIT_ASSERT( iter == p.end() );

	auto iter2 = p.find( 22.0f );
	CPPUNIT_ASSERT( iter2 == p.end() );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testFindPoint()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );
	p.addPoint( 4.0f, 0.5f );

	auto iter = p.find( 4.0f );
	CPPUNIT_ASSERT( iter == p.begin() );

	auto iter2 = p.find( 4.4f );
	CPPUNIT_ASSERT( iter2 == p.begin() );

	auto iter3 = p.find( 3.6f );
	CPPUNIT_ASSERT( iter3 == p.begin() );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testFindNotFound()
{
	___INFOLOG( "" );
	AutomationPath p( 0.0f, 1.0f, 1.0f );
	p.addPoint( 2.0f, 0.2f );

	auto iter = p.find( 1.3f );
	CPPUNIT_ASSERT( iter == p.end() );

	auto iter2 = p.find( 2.6f );
	CPPUNIT_ASSERT( iter2 == p.end() );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testMovePoint()
{
	___INFOLOG( "" );
	typedef std::pair<const float, float> pair;
	AutomationPath p( 0.0f, 1.0f, 1.0f );
	p.addPoint( 5.0f, 0.5f );

	auto in = p.begin();
	auto out = p.move( in, 6.0f, 1.0f );

	CPPUNIT_ASSERT( out == p.begin() );
	CPPUNIT_ASSERT_EQUAL( pair( 6.0f, 1.0f ), *out );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testRemovePoint()
{
	___INFOLOG( "" );
	AutomationPath p( 1.0f, 1.0f, 1.0f );
	p.addPoint( 0.0f, 0.0f );

	p.removePoint( 0.0f );

	CPPUNIT_ASSERT( p.empty() );
	CPPUNIT_ASSERT( p.find( 0.0f ) == p.end() );
	CPPUNIT_ASSERT_DOUBLES_EQUAL(
		1.0, static_cast<double>( p.getValue( 0.0f ) ), delta
	);

	___INFOLOG( "passed" );
}

void AutomationPathTest::testRead()
{
	___INFOLOG( "" );
	QDomDocument doc;
	QString xml = "<path><point x='0.2' y='0.4'/><point x='0.4' y='0.2'/></path>";
	CPPUNIT_ASSERT( doc.setContent( xml, false ) );

	auto pPath = AutomationPath::loadFrom( doc.documentElement(), false );

	auto pExpect = std::make_shared<AutomationPath>();
	pExpect->addPoint( 0.2, 0.4 );
	pExpect->addPoint( 0.4, 0.2 );

	CPPUNIT_ASSERT_EQUAL( *pExpect, *pPath );
	CPPUNIT_ASSERT_EQUAL( 0.4f, pPath->getValue( 0.2f ) );
	CPPUNIT_ASSERT_EQUAL( 0.2f, pPath->getValue( 0.4f ) );
	___INFOLOG( "passed" );
}

void AutomationPathTest::testWrite()
{
	___INFOLOG( "" );
	AutomationPath path;
	path.addPoint( 0.0f, 0.0f );
	path.addPoint( 0.2f, 0.0f );
	path.addPoint( 1.0f, 1.0f );

	QDomDocument doc;
	XMLNode node = doc.createElement( "path" );
	doc.appendChild( node );
	path.saveTo( node, false );

	QDomDocument expect;
	QString expect_xml =
		"<path><point x='0' y='0'/><point x='0.2' "
		"y='0'/><point x='1' y='1'/></path>";
	CPPUNIT_ASSERT( expect.setContent( expect_xml, false ) );

	/* This test may be fragile */
	CPPUNIT_ASSERT_EQUAL(
		expect.toString( 0 ).toStdString(), doc.toString( 0 ).toStdString()
	);
	___INFOLOG( "passed" );
}

void AutomationPathTest::testRoundtripReadWrite()
{
	___INFOLOG( "" );
	auto p1 = std::make_shared<AutomationPath>();
	p1->addPoint( 0.0f, 0.4f );
	p1->addPoint( 0.1f, 0.8f );
	p1->addPoint( 0.3f, 0.6f );

	QDomDocument doc;
	XMLNode node = doc.createElement( "path" );
	doc.appendChild( node );
	p1->saveTo( node, false );

	auto p2 = AutomationPath::loadFrom( node, false );

	CPPUNIT_ASSERT_EQUAL( *p1, *p2 );
	___INFOLOG( "passed" );
}
