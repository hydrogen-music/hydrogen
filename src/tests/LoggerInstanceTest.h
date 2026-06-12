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

#ifndef LOGGER_INSTANCE_TEST_H
#define LOGGER_INSTANCE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/** Validates the per-instance #H2Core::Logger + ambient-context mechanism
 * (ADR 0015, T1.6). */
class LoggerInstanceTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( LoggerInstanceTest );
	CPPUNIT_TEST( testPerInstanceFiles );
	CPPUNIT_TEST( testUnscopedHitsDefault );
	CPPUNIT_TEST( testTeardownFlushesOwnQueue );
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() override;
	void tearDown() override;

	/** Two instance loggers, each entered via its own Logger::Scope, write to
	 * two distinct files with no cross-writing. */
	void testPerInstanceFiles();
	/** A log emitted outside any scope routes to the process default, not to
	 * either instance logger's file. */
	void testUnscopedHitsDefault();
	/** Destroying one instance logger flushes its own queue (its messages land
	 * in its file) without affecting the other. */
	void testTeardownFlushesOwnQueue();

private:
	unsigned m_nPreviousBitMask;
};

#endif
