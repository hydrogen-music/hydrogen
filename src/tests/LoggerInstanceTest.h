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

#include <core/config.h>

/** Validates the per-instance #H2Core::Logger + ambient-context mechanism
 * (ADR 0015, T1.6). */
class LoggerInstanceTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( LoggerInstanceTest );
	CPPUNIT_TEST( testPerInstanceFiles );
	CPPUNIT_TEST( testUnscopedHitsDefault );
	CPPUNIT_TEST( testTeardownFlushesOwnQueue );
	CPPUNIT_TEST( testAppendModeAccumulates );
	CPPUNIT_TEST( testConcurrentAppendPreservesLines );
	CPPUNIT_TEST( testThreadBodyMacrosFollowScope );
#ifdef H2CORE_HAVE_DEBUG
	CPPUNIT_TEST( testConstructionRoutesToInstanceLogger );
	CPPUNIT_TEST( testDestructionRoutesToInstanceLogger );
	CPPUNIT_TEST( testInstanceFilesStayInTmpDir );
	CPPUNIT_TEST( testProcessDefaultAuditSurface );
#endif
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
	/** Append-mode instance loggers accumulate across sequential runs on the
	 * same path instead of truncating it; the default (truncate) semantics
	 * stay unchanged. */
	void testAppendModeAccumulates();
	/** Two append-mode instance loggers writing the same file concurrently
	 * must not lose or corrupt lines (per-write file lock). */
	void testConcurrentAppendPreservesLines();
	/** The thread-body macro family (a `Base* __object` plus __INFOLOG, as
	 * declared in driver thread bodies) and the Object<T>
	 * constructor/destructor announcements must resolve through the ambient
	 * scope (Logger::currentLogger()), not the process-static
	 * Base::__logger. */
	void testThreadBodyMacrosFollowScope();
	/** A Plugin-driver instance's construction work (SoundLibraryDatabase
	 * scan, audio driver startup) routes to its per-instance log file, not
	 * to the process default. Debug builds only: release builds delete the
	 * instance log again on destruction. */
	void testConstructionRoutesToInstanceLogger();
	/** A Plugin-driver instance's teardown work (the ~Hydrogen announcement,
	 * member destruction) routes to its per-instance log file as well.
	 * Debug builds only (same reason as above). */
	void testDestructionRoutesToInstanceLogger();
	/** The per-instance log files live in the tmp dir (transient per-run
	 * artifacts), not next to the process default's log file: an explicit
	 * log file location (e.g. the suite's `-o <plain name>`) designates
	 * just that one file. Debug builds only (same reason as above). */
	void testInstanceFilesStayInTmpDir();
	/** The process-default log doubles as the audit surface: while
	 * per-instance loggers are alive, every line reaching it is marked
	 * `[unscoped]`. After a full plugin-instance lifecycle (construction,
	 * processing, teardown) the only marked line must be the deliberate
	 * "Spawning instance logger" index announcement. Debug builds only (same
	 * reason as above). */
	void testProcessDefaultAuditSurface();

private:
	unsigned m_nPreviousBitMask;
};

#endif
