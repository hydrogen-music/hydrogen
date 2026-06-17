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

#include <core/Object.h>
#include <core/Preferences/PluginConfig.h>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtXml/QDomDocument>

#include <thread>

using namespace H2Core;

namespace {

// Two independent base fields, A and B (neither is in the override layer).
QByteArray makeConfig( int nA, int nB ) {
	return QString( "<hydrogen_preferences>"
					"<fieldA>%1</fieldA><fieldB>%2</fieldB>"
					"</hydrogen_preferences>" )
		.arg( nA ).arg( nB ).toUtf8();
}

QString leaf( const QByteArray& xml, const QString& sTag ) {
	QDomDocument doc;
	if ( ! doc.setContent( xml ) ) {
		return QString( "<parse-error>" );
	}
	return doc.documentElement().firstChildElement( sTag ).text();
}

QByteArray readFile( const QString& sPath ) {
	QFile f( sPath );
	if ( ! f.open( QIODevice::ReadOnly ) ) {
		return QByteArray();
	}
	const QByteArray data = f.readAll();
	f.close();
	return data;
}

void writeFile( const QString& sPath, const QByteArray& data ) {
	QFile f( sPath );
	f.open( QIODevice::WriteOnly );
	f.write( data );
	f.close();
}

} // namespace

void ConfigConcurrencyTest::testDifferentFieldsBothSurvive() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";

	const QByteArray original = makeConfig( 1, 1 );
	writeFile( sPath, original );

	// Two instances loaded the same baseline; each changes a *different* field.
	// Instance 1 persists A=2.
	CPPUNIT_ASSERT( PluginConfig::persist( sPath, original, makeConfig( 2, 1 ) ) );
	// Instance 2 (baseline still the original) persists B=2; persist() re-reads
	// disk (now A=2) and merges only its own change.
	CPPUNIT_ASSERT( PluginConfig::persist( sPath, original, makeConfig( 1, 2 ) ) );

	const QByteArray result = readFile( sPath );
	CPPUNIT_ASSERT_EQUAL( std::string( "2" ),
						  leaf( result, "fieldA" ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "2" ),
						  leaf( result, "fieldB" ).toStdString() );

	___INFOLOG( "passed" );
}

void ConfigConcurrencyTest::testSameFieldLastWriterWins() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";

	const QByteArray original = makeConfig( 1, 1 );
	writeFile( sPath, original );

	// Both instances change the *same* field A. Bounded last-writer-wins, never
	// corruption.
	CPPUNIT_ASSERT( PluginConfig::persist( sPath, original, makeConfig( 2, 1 ) ) );
	CPPUNIT_ASSERT( PluginConfig::persist( sPath, original, makeConfig( 3, 1 ) ) );

	const QByteArray result = readFile( sPath );
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( result ) ); // valid XML, not corrupted
	CPPUNIT_ASSERT_EQUAL( std::string( "3" ),
						  leaf( result, "fieldA" ).toStdString() );

	___INFOLOG( "passed" );
}

void ConfigConcurrencyTest::testParallelPersistNoCorruption() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";

	const QByteArray original = makeConfig( 1, 1 );
	writeFile( sPath, original );

	// Two threads hammer the same config under the cross-process lock, each
	// persisting a change to its own field. The lock + atomic write must keep
	// the file consistent; both changes must end up present.
	const int nIterations = 200;
	auto worker = []( const QString& path, const QByteArray& base,
					  const QByteArray& mine, int n ) {
		for ( int i = 0; i < n; ++i ) {
			PluginConfig::persist( path, base, mine );
		}
	};

	std::thread t1( worker, sPath, original, makeConfig( 2, 1 ), nIterations );
	std::thread t2( worker, sPath, original, makeConfig( 1, 2 ), nIterations );
	t1.join();
	t2.join();

	const QByteArray result = readFile( sPath );
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( result ) ); // never corrupted
	CPPUNIT_ASSERT_EQUAL( std::string( "2" ),
						  leaf( result, "fieldA" ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "2" ),
						  leaf( result, "fieldB" ).toStdString() );

	___INFOLOG( "passed" );
}
