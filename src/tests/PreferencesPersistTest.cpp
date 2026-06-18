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

#include "PreferencesPersistTest.h"
#include "TestHelper.h"

#include <core/Helpers/Filesystem.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtXml/QDomDocument>

#include <memory>

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

void writeFile( const QString& sPath, const QByteArray& data ) {
	QFile f( sPath );
	f.open( QIODevice::WriteOnly );
	f.write( data );
	f.close();
}

// Materialise a complete, valid hydrogen.conf at sPath (a plain snapshot write,
// since the persist path keys on userConfigPath()).
void seedConfig( const QString& sPath ) {
	auto pSeed = Preferences::create_instance();
	pSeed->saveCopyAs( sPath, true /*bSilent*/ );
}

// Edit a top-level base-layer leaf directly on disk, simulating a concurrent
// edit by another process.
void editLeafOnDisk( const QString& sPath, const QString& sTag,
					 const QString& sValue ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement root = doc.documentElement();
	QDomElement leaf = root.firstChildElement( sTag );
	while ( leaf.hasChildNodes() ) {
		leaf.removeChild( leaf.firstChild() );
	}
	leaf.appendChild( doc.createTextNode( sValue ) );
	writeFile( sPath, doc.toByteArray() );
}

} // namespace

void PreferencesPersistTest::tearDown() {
	// Never leave userConfigPath() redirected for other suites.
	Filesystem::setPreferencesOverwritePath( "" );
}

void PreferencesPersistTest::testBaselineRetainedOnLoad() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	// The on-disk XML is retained for the diff-against-baseline merge (ADR 0023).
	CPPUNIT_ASSERT( ! pPref->getBaselineXml().isEmpty() );
	CPPUNIT_ASSERT( pPref->getBaselineXml().contains( "hydrogen_preferences" ) );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testConcurrentBaseEditSurvives() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	// Redirect userConfigPath() to the temp file so save() takes the persist path.
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// This instance changes one base field ...
	pPref->setPreferredLanguage( "zz" );
	// ... while another process concurrently changes a *different* base field on
	// disk (after our load, so it is not in our baseline).
	editLeafOnDisk( sPath, "maxBars", "999" );

	// Saving the shared config merges only our own change; the concurrent edit
	// must survive.
	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );
	CPPUNIT_ASSERT_EQUAL( 999, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testOverrideFieldNotWritten() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	const unsigned nOriginalBuffer = pPref->m_nBufferSize;

	// Change a base field (must persist) and an override field (must NOT be
	// written to the shared config - it is host/state-owned, ADR 0022).
	pPref->setMaxBars( 42 );
	pPref->m_nBufferSize = nOriginalBuffer + 137;
	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );
	// The override field on the shared config is unchanged.
	CPPUNIT_ASSERT_EQUAL( nOriginalBuffer, pReloaded->m_nBufferSize );

	___INFOLOG( "passed" );
}
