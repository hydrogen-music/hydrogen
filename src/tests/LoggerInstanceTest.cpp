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

#include "utils/FakePluginHost.h"

#include <core/Logger.h>
#include <core/Object.h>
#include <core/Helpers/Filesystem.h>

#include <memory>
#include <thread>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

	auto pLoggerA = Logger::createInstanceLogger( sPathA, Logger::Option::None );
	auto pLoggerB = Logger::createInstanceLogger( sPathB, Logger::Option::None );

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

	auto pLogger = Logger::createInstanceLogger( sPath, Logger::Option::None );

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

	auto pLoggerKept = Logger::createInstanceLogger( sPathKept, Logger::Option::None );
	auto pLoggerGone = Logger::createInstanceLogger( sPathGone, Logger::Option::None );

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

void LoggerInstanceTest::testAppendModeAccumulates() {
	const QString sPathAppend = Filesystem::tmpDir().append( "loggerAppend.log" );
	const QString sPathTruncate = Filesystem::tmpDir().append( "loggerTruncate.log" );
	const QString sMarker1 = "LoggerInstanceTest-MARKER-APPEND-FIRST";
	const QString sMarker2 = "LoggerInstanceTest-MARKER-APPEND-SECOND";

	// Two sequential append-mode loggers on the same path must accumulate -
	// a pid-named plugin log can recur after pid reuse.
	auto pAppendFirst = Logger::createInstanceLogger( sPathAppend, Logger::Option::Append );
	{
		Logger::Scope scope( pAppendFirst );
		___INFOLOG( sMarker1 );
	}
	delete pAppendFirst;
	auto pAppendSecond = Logger::createInstanceLogger( sPathAppend, Logger::Option::Append );
	{
		Logger::Scope scope( pAppendSecond );
		___INFOLOG( sMarker2 );
	}
	delete pAppendSecond;

	const QString sContentAppend = readFile( sPathAppend );
	CPPUNIT_ASSERT( sContentAppend.contains( sMarker1 ) );
	CPPUNIT_ASSERT( sContentAppend.contains( sMarker2 ) );

	// The default (truncate) mode must keep its semantics: the second
	// logger starts from scratch.
	auto pTruncateFirst = Logger::createInstanceLogger( sPathTruncate, Logger::Option::None );
	{
		Logger::Scope scope( pTruncateFirst );
		___INFOLOG( sMarker1 );
	}
	delete pTruncateFirst;
	auto pTruncateSecond = Logger::createInstanceLogger( sPathTruncate, Logger::Option::None );
	{
		Logger::Scope scope( pTruncateSecond );
		___INFOLOG( sMarker2 );
	}
	delete pTruncateSecond;

	const QString sContentTruncate = readFile( sPathTruncate );
	CPPUNIT_ASSERT( ! sContentTruncate.contains( sMarker1 ) );
	CPPUNIT_ASSERT( sContentTruncate.contains( sMarker2 ) );

	Filesystem::rm( sPathAppend, false, true );
	Filesystem::rm( sPathTruncate, false, true );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testConcurrentAppendPreservesLines() {
	const QString sPath = Filesystem::tmpDir().append( "loggerConcurrentAppend.log" );
	const int nLines = 200;

	auto pLoggerA = Logger::createInstanceLogger( sPath, Logger::Option::Append );
	auto pLoggerB = Logger::createInstanceLogger( sPath, Logger::Option::Append );

	// Two append-mode writers on the same file: every line must survive
	// whole (the per-write file lock keeps them from interleaving inside a
	// line).
	auto threadA = std::thread( [ pLoggerA, nLines ]() {
		Logger::Scope scope( pLoggerA );
		for ( int ii = 0; ii < nLines; ++ii ) {
			___INFOLOG( QString( "LoggerInstanceTest-MARKER-CONCURRENT-A %1" )
						.arg( ii ) );
		}
	} );
	auto threadB = std::thread( [ pLoggerB, nLines ]() {
		Logger::Scope scope( pLoggerB );
		for ( int ii = 0; ii < nLines; ++ii ) {
			___INFOLOG( QString( "LoggerInstanceTest-MARKER-CONCURRENT-B %1" )
						.arg( ii ) );
		}
	} );
	threadA.join();
	threadB.join();

	// Destroying the loggers joins their worker threads, flushing + closing
	// the file deterministically before we read it.
	delete pLoggerA;
	delete pLoggerB;

	const QString sContent = readFile( sPath );
	for ( int ii = 0; ii < nLines; ++ii ) {
		CPPUNIT_ASSERT( sContent.contains(
			QString( "LoggerInstanceTest-MARKER-CONCURRENT-A %1" ).arg( ii ) ) );
		CPPUNIT_ASSERT( sContent.contains(
			QString( "LoggerInstanceTest-MARKER-CONCURRENT-B %1" ).arg( ii ) ) );
	}

	Filesystem::rm( sPath, false, true );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testThreadBodyMacrosFollowScope() {
	const QString sPath = Filesystem::tmpDir().append( "loggerThreadBody.log" );
	const QString sMarker = "LoggerInstanceTest-MARKER-THREAD-BODY";

	// setUp() already forces Info; the Object<T> announcements additionally
	// need the Constructors bit. tearDown() restores the saved mask even
	// when an assertion throws.
	Logger::set_bit_mask( Logger::bit_mask() | Logger::Constructors );

	// Stands in for the `Base* __object = ( Base* )param;` a driver thread
	// body declares before logging via the __-family.
	struct Stub : public H2Core::Object<Stub> {
		H2_OBJECT( Stub )
	};

	auto pLogger = Logger::createInstanceLogger( sPath, Logger::Option::None );
	{
		Logger::Scope scope( pLogger );
		Stub stub;
		Base* __object = &stub;
		Q_UNUSED( __object );
		__INFOLOG( sMarker );
	}
	delete pLogger;

	const QString sContent = readFile( sPath );
	CPPUNIT_ASSERT( sContent.contains( sMarker ) );
#ifdef H2CORE_HAVE_DEBUG
	CPPUNIT_ASSERT( sContent.contains( "Constructor" ) );
	CPPUNIT_ASSERT( sContent.contains( "Destructor" ) );
#endif

	Filesystem::rm( sPath, false, true );

	___INFOLOG( "passed" );
}

#ifdef H2CORE_HAVE_DEBUG
void LoggerInstanceTest::testConstructionRoutesToInstanceLogger() {
	const QFileInfo logInfo( Filesystem::logFilePath() );
	const QDir logDir( logInfo.absolutePath() );
	const QString sPattern = "hydrogen_" +
		QString::number( QCoreApplication::applicationPid() ) + "_*.log";

	// Other fixtures spawn Plugin-driver instances too; only files that
	// appear while this test runs count as ours.
	const QStringList sBefore = logDir.entryList( { sPattern } );

	{
		// Destroying the host joins the instance logger's worker thread, so
		// its file is flushed + closed (and kept, in debug builds) by the
		// time we read it.
		FakePluginHost host;
	}

	QStringList sNewFiles;
	for ( const QString& sName : logDir.entryList( { sPattern } ) ) {
		if ( ! sBefore.contains( sName ) &&
			 ! sName.endsWith( "_plugin.log" ) ) {
			sNewFiles << sName;
		}
	}
	CPPUNIT_ASSERT( sNewFiles.size() == 1 );

	const QString sContent = readFile( logDir.filePath( sNewFiles.first() ) );
	// Construction work after the instance logger exists must route there
	// (the SoundLibraryDatabase scan, audio driver startup, ...).
	CPPUNIT_ASSERT( sContent.contains( "AudioEngine::startAudioDriver" ) );
	// The spawning announcement itself stays on the process default so the
	// shared log lists the per-instance files.
	CPPUNIT_ASSERT( ! sContent.contains( "Spawning instance logger" ) );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testDestructionRoutesToInstanceLogger() {
	const QFileInfo logInfo( Filesystem::logFilePath() );
	const QDir logDir( logInfo.absolutePath() );
	const QString sPattern = "hydrogen_" +
		QString::number( QCoreApplication::applicationPid() ) + "_*.log";

	// Other fixtures spawn Plugin-driver instances too; only files that
	// appear while this test runs count as ours.
	const QStringList sBefore = logDir.entryList( { sPattern } );

	{
		// Destroying the host joins the instance logger's worker thread, so
		// its file is flushed + closed (and kept, in debug builds) by the
		// time we read it.
		FakePluginHost host;
	}

	QStringList sNewFiles;
	for ( const QString& sName : logDir.entryList( { sPattern } ) ) {
		if ( ! sBefore.contains( sName ) &&
			 ! sName.endsWith( "_plugin.log" ) ) {
			sNewFiles << sName;
		}
	}
	CPPUNIT_ASSERT( sNewFiles.size() == 1 );

	const QString sContent = readFile( logDir.filePath( sNewFiles.first() ) );
	// Teardown work must route to the instance log too: the destructor
	// announcement and everything the destructor body runs.
	CPPUNIT_ASSERT( sContent.contains( "[~Hydrogen]" ) );

	___INFOLOG( "passed" );
}

void LoggerInstanceTest::testProcessDefaultAuditSurface() {
	// The process default writes asynchronously; drain its queue so the
	// offset below marks a quiet point and the delta only contains lines
	// logged during this test's lifecycle.
	Logger::get_instance()->flush();

	// The process default's actual file. Filesystem::logFilePath() is NOT
	// reliable here: its lazy first-call resolution can clobber the
	// bootstrapped custom path (the suite never exercises the pre-bootstrap
	// Reporter flow that normally initializes it), leaving it pointing at the
	// XDG default instead of this suite's shared log.
	QFile processLog( Logger::get_instance()->getLogFile() );
	CPPUNIT_ASSERT( processLog.open( QIODevice::ReadOnly ) );
	const qint64 nOffsetBefore = processLog.size();
	processLog.close();

	{
		// Full lifecycle: construction, a processing cycle with a note, and
		// teardown.
		FakePluginHost host;
		host.addNoteOn( 0, 36, 100 );
		host.process( 1024 );
		host.process( 1024 );
	}

	Logger::get_instance()->flush();

	CPPUNIT_ASSERT( processLog.open( QIODevice::ReadOnly ) );
	processLog.seek( nOffsetBefore );
	QTextStream in( &processLog );
	QStringList sMarkedLines;
	while ( ! in.atEnd() ) {
		const QString sLine = in.readLine();
		if ( sLine.contains( "[unscoped]" ) ) {
			sMarkedLines << sLine;
		}
	}
	processLog.close();

	// The one deliberate process-level line of an instance lifecycle is the
	// spawning announcement (the shared log doubles as the index of
	// per-instance files). Anything else reaching the process default while
	// instance loggers are alive is a routing gap.
	CPPUNIT_ASSERT_MESSAGE(
		QString( "Expected exactly the spawning announcement on the process "
				 "default (offset %1..%2), got %3 marked line(s):\n%4" )
			.arg( nOffsetBefore )
			.arg( processLog.size() )
			.arg( sMarkedLines.size() )
			.arg( sMarkedLines.join( "\n" ) )
			.toStdString(),
		sMarkedLines.size() == 1 );
	CPPUNIT_ASSERT( sMarkedLines.first().contains( "Spawning instance logger" ) );

	___INFOLOG( "passed" );
}
#endif
