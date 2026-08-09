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

#include "LoggerInstanceTest.h"

#include <core/Logger.h>
#include <core/Object.h>
#include <core/Helpers/Filesystem.h>

#include <memory>

#include <QFile>
#include <QTextStream>

using namespace H2Core;

static QString readFile( const QString& sPath ) {
	QFile file( sPath );
	if ( ! file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
		return QString();
	}
	QTextStream in( &file );
	const QString sContent = in.readAll();
	file.close();
	return sContent;
}

void LoggerInstanceTest::setUp() {
	// Ensure Info-level messages are emitted regardless of the suite's
	// configured log level; restored in tearDown().
	m_nPreviousBitMask = Logger::bit_mask();
	Logger::set_bit_mask( m_nPreviousBitMask | Logger::Info );
}

void LoggerInstanceTest::tearDown() {
	Logger::set_bit_mask( m_nPreviousBitMask );
}

void LoggerInstanceTest::testPerInstanceFiles() {
	const QString sPathA = Filesystem::tmpDir().append( "loggerInstanceA.log" );
	const QString sPathB = Filesystem::tmpDir().append( "loggerInstanceB.log" );

	const QString sMarkerA = "LoggerInstanceTest-MARKER-ALPHA";
	const QString sMarkerB = "LoggerInstanceTest-MARKER-BETA";

	auto pLoggerA = Logger::createInstanceLogger( sPathA, false, false, false );
	auto pLoggerB = Logger::createInstanceLogger( sPathB, false, false, false );

	{
		// Within this scope the ambient context resolves to pLoggerA, so the
		// macro routes there.
		Logger::Scope scope( pLoggerA );
		___INFOLOG( sMarkerA );
	}
	{
		Logger::Scope scope( pLoggerB );
		___INFOLOG( sMarkerB );
	}

	// Destroying the loggers joins their worker threads, flushing + closing
	// each file deterministically before we read it.
	delete pLoggerA;
	delete pLoggerB;

	const QString sContentA = readFile( sPathA );
	const QString sContentB = readFile( sPathB );

	CPPUNIT_ASSERT( sContentA.contains( sMarkerA ) );
	CPPUNIT_ASSERT( ! sContentA.contains( sMarkerB ) );
	CPPUNIT_ASSERT( sContentB.contains( sMarkerB ) );
	CPPUNIT_ASSERT( ! sContentB.contains( sMarkerA ) );

	Filesystem::rm( sPathA, false, true );
	Filesystem::rm( sPathB, false, true );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testUnscopedHitsDefault() {
	const QString sPath = Filesystem::tmpDir().append( "loggerInstanceC.log" );
	const QString sMarker = "LoggerInstanceTest-MARKER-UNSCOPED";

	auto pLogger = Logger::createInstanceLogger( sPath, false, false, false );

	// No active Scope: the macro must resolve to the process-default logger,
	// NOT to the instance logger we just created.
	___INFOLOG( sMarker );

	delete pLogger;

	const QString sContent = readFile( sPath );
	CPPUNIT_ASSERT( ! sContent.contains( sMarker ) );

	Filesystem::rm( sPath, false, true );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testTeardownFlushesOwnQueue() {
	const QString sPathKept = Filesystem::tmpDir().append( "loggerInstanceKeep.log" );
	const QString sPathGone = Filesystem::tmpDir().append( "loggerInstanceGone.log" );

	const QString sMarkerKept = "LoggerInstanceTest-MARKER-KEEP";
	const QString sMarkerGone = "LoggerInstanceTest-MARKER-GONE";

	auto pLoggerKept = Logger::createInstanceLogger( sPathKept, false, false, false );
	auto pLoggerGone = Logger::createInstanceLogger( sPathGone, false, false, false );

	{
		Logger::Scope scope( pLoggerGone );
		___INFOLOG( sMarkerGone );
	}
	{
		Logger::Scope scope( pLoggerKept );
		___INFOLOG( sMarkerKept );
	}

	// Tear down only the "gone" instance: its queue must be flushed to its own
	// file, and the surviving instance must be unaffected.
	delete pLoggerGone;

	const QString sContentGone = readFile( sPathGone );
	CPPUNIT_ASSERT( sContentGone.contains( sMarkerGone ) );
	CPPUNIT_ASSERT( ! sContentGone.contains( sMarkerKept ) );

	delete pLoggerKept;
	const QString sContentKept = readFile( sPathKept );
	CPPUNIT_ASSERT( sContentKept.contains( sMarkerKept ) );
	CPPUNIT_ASSERT( ! sContentKept.contains( sMarkerGone ) );

	Filesystem::rm( sPathKept, false, true );
	Filesystem::rm( sPathGone, false, true );

	___INFOLOG( "passed" );
}
