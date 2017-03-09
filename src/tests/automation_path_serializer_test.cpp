/*
 * Hydrogen
 * Copyright(c) 2015-2016 by Przemysław Sitek
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <cppunit/extensions/HelperMacros.h>

#include <QDomDocument>

#include <hydrogen/basics/automation_path.h>
#include <hydrogen/automation_path_serializer.h>

using namespace H2Core;

class AutomationPathSerializerTest : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE(AutomationPathSerializerTest);
	CPPUNIT_TEST(testRead);
	CPPUNIT_TEST(testWrite);
	CPPUNIT_TEST(testRoundtripReadWrite);
	CPPUNIT_TEST_SUITE_END();

	public:

	void testRead()
	{
		QDomDocument doc;
		QString xml = "<path><point x='2' y='4'/><point x='4' y='-2'/></path>";
		CPPUNIT_ASSERT(doc.setContent(xml, false));

		AutomationPath path(-10, 10, 0);
		AutomationPathSerializer reader;
		reader.read_automation_path(doc.documentElement(), path);

		AutomationPath expect(-10, 10, 0);
		expect.add_point(2, 4);
		expect.add_point(4, -2);

		CPPUNIT_ASSERT_EQUAL(expect, path);
		CPPUNIT_ASSERT_EQUAL(4.0f, path.get_value(2.0f));
		CPPUNIT_ASSERT_EQUAL(-2.0f, path.get_value(4.0f));
	}


	void testWrite()
	{
		AutomationPath path(-1, 1, 0);
		path.add_point(0.0f, 0.0f);
		path.add_point(1.0f, 1.0f);
		path.add_point(2.0f, 0.0f);
		path.add_point(3.0f,-1.0f);

		AutomationPathSerializer writer;
		QDomDocument doc;
		QDomElement node = doc.createElement("path");
		doc.appendChild(node);
		writer.write_automation_path(node, path);

		QDomDocument expect;
		QString expect_xml = "<path><point x='0' y='0'/><point x='1' y='1'/><point x='2' y='0'/><point x='3' y='-1'/></path>";
		CPPUNIT_ASSERT(expect.setContent(expect_xml, false));

		/* This test may be fragile */
		CPPUNIT_ASSERT_EQUAL(
				expect.toString(0).toStdString(),
				doc.toString(0).toStdString()
		);
	}


	void testRoundtripReadWrite()
	{
		AutomationPath p1(0, 10, 0);
		p1.add_point(0.0f, 4.0f);
		p1.add_point(1.0f, 8.0f);
		p1.add_point(3.0f, 6.0f);

		QDomDocument doc;
		QDomElement node = doc.createElement("path");
		doc.appendChild(node);
		
		AutomationPathSerializer serializer;
		
		serializer.write_automation_path(node, p1);

		AutomationPath p2(0, 10, 0);
		serializer.read_automation_path(node, p2);

		CPPUNIT_ASSERT_EQUAL(p1, p2);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( AutomationPathSerializerTest );
