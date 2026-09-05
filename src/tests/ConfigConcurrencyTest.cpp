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

#include "ConfigConcurrencyTest.h"

#include <core/Helpers/Filesystem.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Version.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtXml/QDomDocument>

#include <thread>

using namespace H2Core;

namespace {

QByteArray readFile( const QString& sPath ) {
	QFile f( sPath );
	if ( ! f.open( QIODevice::ReadOnly ) ) {
		return QByteArray();
	}
	const QByteArray data = f.readAll();
	f.close();
	return data;
}

// Materialise a complete, valid hydrogen.conf at sPath: a copy of the
// shipped default config with the version header normalized to the current
// build - identical on every machine, unlike a snapshot of whatever the
// ambient user config happens to contain (create_instance()).
void seedConfig( const QString& sPath ) {
	CPPUNIT_ASSERT( QFile::copy( Filesystem::systemConfigPath(), sPath ) );
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement version = doc.documentElement().firstChildElement( "version" );
	CPPUNIT_ASSERT( ! version.isNull() );
	while ( ! version.lastChild().isNull() ) {
		version.removeChild( version.lastChild() );
	}
	version.appendChild(
		doc.createTextNode( QString( get_version().c_str() ) ) );
	QFile f( sPath );
	CPPUNIT_ASSERT( f.open( QIODevice::WriteOnly ) );
	f.write( doc.toByteArray() );
	f.close();
}

} // namespace

void ConfigConcurrencyTest::tearDown() {
	// Never leave userConfigPath() redirected for other suites.
	Filesystem::setPreferencesOverwritePath( "" );
}

void ConfigConcurrencyTest::testDifferentFieldsBothSurvive() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	// Redirect userConfigPath() to the temp file so save() persists there.
	Filesystem::setPreferencesOverwritePath( sPath );

	// Two instances loaded the same baseline; each changes a *different*
	// field.
	auto pA = Preferences::load( sPath, true, nullptr );
	auto pB = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pA != nullptr );
	CPPUNIT_ASSERT( pB != nullptr );

	pA->setMaxBars( 42 );
	CPPUNIT_ASSERT( pA->save( true ) );

	// pB still holds the original baseline; its save must re-read the disk
	// (now maxBars=42) and merge only its own change.
	pB->setPreferredLanguage( "zz" );
	CPPUNIT_ASSERT( pB->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );

	___INFOLOG( "passed" );
}

void ConfigConcurrencyTest::testSameFieldLastWriterWins() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Both instances change the *same* field. Bounded last-writer-wins,
	// never corruption.
	auto pA = Preferences::load( sPath, true, nullptr );
	auto pB = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pA != nullptr );
	CPPUNIT_ASSERT( pB != nullptr );

	pA->setMaxBars( 42 );
	CPPUNIT_ASSERT( pA->save( true ) );
	pB->setMaxBars( 99 );
	CPPUNIT_ASSERT( pB->save( true ) );

	const QByteArray result = readFile( sPath );
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( result ) ); // valid XML, not corrupted

	auto pReloaded = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 99, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void ConfigConcurrencyTest::testParallelPersistNoCorruption() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Two threads hammer the shared config, each persisting a change to its
	// own field. The cross-process lock + atomic write must keep the file
	// consistent; both changes must end up present.
	//
	// One instance per thread: save() never mutates the instance (the load
	// baseline is deliberately not refreshed, ADR 0023), but a Preferences
	// instance is not safe for unsynchronized concurrent use either.
	const int nIterations = 200;
	bool bOkA = true;
	bool bOkB = true;
	auto worker = [ & ]( bool bMaxBars, bool* bOk ) {
		auto pPref = Preferences::load( sPath, true, nullptr );
		if ( pPref == nullptr ) {
			*bOk = false;
			return;
		}
		if ( bMaxBars ) {
			pPref->setMaxBars( 42 );
		}
		else {
			pPref->setPreferredLanguage( "zz" );
		}
		for ( int ii = 0; ii < nIterations; ++ii ) {
			if ( ! pPref->save( true ) ) {
				*bOk = false;
				return;
			}
		}
	};

	std::thread t1( worker, true, &bOkA );
	std::thread t2( worker, false, &bOkB );
	t1.join();
	t2.join();

	CPPUNIT_ASSERT( bOkA );
	CPPUNIT_ASSERT( bOkB );

	const QByteArray result = readFile( sPath );
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( result ) ); // never corrupted

	auto pReloaded = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );

	___INFOLOG( "passed" );
}

void ConfigConcurrencyTest::testMultiProcessHammerNoCorruption() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Three real processes hammer the shared config - the cross-process
	// QLockFile is the only thing standing between their read-merge-write
	// cycles. Two of them contend on the same field (bounded
	// last-writer-wins), the third owns a different one.
	const QString sExe = QCoreApplication::applicationFilePath();
	QProcess children[ 3 ];
	for ( int ii = 0; ii < 3; ++ii ) {
		children[ ii ].start( sExe, { "--config-hammer", sPath,
									  "--hammer-field",
									  QString::number( ii ) } );
	}
	for ( auto& child : children ) {
		CPPUNIT_ASSERT( child.waitForFinished( 60000 ) );
		// A crash would not surface via exitCode() (a signaled child
		// reports 0 or the raw signal number, both below the setup
		// markers) - the exit status is the reliable signal.
		CPPUNIT_ASSERT( child.exitStatus() == QProcess::NormalExit );
		// 253/254 mark setup failures. A small positive count is the
		// bounded retry budget doing its job under contention - each
		// child had 50 attempts, so its change is expected to land at
		// least once (asserted via the file content below).
		CPPUNIT_ASSERT( child.exitCode() < 200 );
	}

	const QByteArray result = readFile( sPath );
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( result ) ); // never corrupted

	auto pReloaded = Preferences::load( sPath, true, nullptr );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	// The uncontended field always lands; the contended one has a bounded
	// winner.
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );
	CPPUNIT_ASSERT( pReloaded->getMaxBars() == 42 ||
					pReloaded->getMaxBars() == 99 );
	// Rows nobody touched survive the hammering - no child fell back to a
	// full snapshot that would clobber its siblings' rows.
	QDomElement audioEngine =
		doc.documentElement().firstChildElement( "audio_engine" );
	CPPUNIT_ASSERT( ! audioEngine.isNull() );
	CPPUNIT_ASSERT( audioEngine.firstChildElement( "samplerate" )
					.text() == "44100" );

	___INFOLOG( "passed" );
}
