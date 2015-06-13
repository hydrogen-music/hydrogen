#include <cppunit/extensions/HelperMacros.h>
#include <hydrogen/basics/automation_path.h>

using namespace H2Core;

class AutomationPathTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(AutomationPathTest);
	CPPUNIT_TEST(testConstruction);
	CPPUNIT_TEST(testEmptyPath);
	CPPUNIT_TEST(testOnePoint);
	CPPUNIT_TEST(testValueBeforeFirstPoint);
	CPPUNIT_TEST(testValueAfterLastPoint);
	CPPUNIT_TEST(testMidpointValue);
	CPPUNIT_TEST_SUITE_END();

	const double delta = 0.0001;

	public:
	
	/* Test whether AutomationPaths are constructed correctly */
	void testConstruction()
	{
		AutomationPath p(0.2f, 0.8f, 0.6f);


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
	}
	

	/* Empty automation path should always return
	   default value */
	void testEmptyPath()
	{
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
	}


	/* Test getting value of an anchor point */
	void testOnePoint()
	{
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.5f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.5,
				static_cast<double>(p.get_value(1.0f)),
				delta);
	}


	/* Test getting value before first point,
	   i.e if returned value is defined by first point */
	void testValueBeforeFirstPoint()
	{
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.5f);
		p.add_point(2.0f, 0.7f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.5,
				static_cast<double>(p.get_value(0.0f)),
				delta);
	}


	/* Test whether value past the last point
	   is defined by that value */
	void testValueAfterLastPoint()
	{
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.4f);
		p.add_point(2.0f, 0.6f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.6,
				static_cast<double>(p.get_value(3.0f)),
				delta);
	}


	/* Test getting value between two anchor points */
	void testMidpointValue()
	{
		AutomationPath p(0.0f, 1.0f, 1.0f);

		p.add_point(1.0f, 0.2f);
		p.add_point(2.0f, 0.4f);

		CPPUNIT_ASSERT_DOUBLES_EQUAL(
				0.3,
				static_cast<double>(p.get_value(1.5f)),
				delta);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION( AutomationPathTest );
