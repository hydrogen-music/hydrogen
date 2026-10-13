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
#include <core/Preferences/Shortcuts.h>
#include <core/Version.h>

#include <QtCore/QFile>
#include <QtCore/QLockFile>
#include <QtCore/QTemporaryDir>
#include <QtXml/QDomDocument>

#include <algorithm>
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

// Edit a top-level base-layer leaf directly on disk, simulating a concurrent
// edit by another process.
void editLeafOnDisk( const QString& sPath, const QString& sTag,
					 const QString& sValue ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement root = doc.documentElement();
	QDomElement leaf = root.firstChildElement( sTag );
	// This helper edits an existing leaf; a null one means the caller picked
	// a tag that is not a top-level element (silent no-op would follow).
	CPPUNIT_ASSERT( ! leaf.isNull() );
	while ( leaf.hasChildNodes() ) {
		leaf.removeChild( leaf.firstChild() );
	}
	leaf.appendChild( doc.createTextNode( sValue ) );
	writeFile( sPath, doc.toByteArray() );
}

// Materialise a complete, valid hydrogen.conf at sPath: a copy of the
// shipped default config with the version header normalized to the current
// build - identical on every machine, unlike a snapshot of whatever the
// ambient user config happens to contain (create_instance()).
void seedConfig( const QString& sPath ) {
	CPPUNIT_ASSERT( QFile::copy( Filesystem::systemConfigPath(), sPath ) );
	// The shipped file is stamped by the release that wrote it; a no-op
	// save from this build must not register the version header as a
	// change.
	editLeafOnDisk( sPath, "version", QString( get_version().c_str() ) );
}

// Read a top-level leaf element's text directly from disk.
QString readLeafOnDisk( const QString& sPath, const QString& sTag ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement leaf = doc.documentElement().firstChildElement( sTag );
	// Like editLeafOnDisk: a null leaf means the caller picked a tag that
	// is not a top-level element.
	CPPUNIT_ASSERT( ! leaf.isNull() );
	return leaf.text();
}

// Remove a top-level leaf element directly on disk, simulating a legacy
// config that never contained it (or a concurrent process having dropped it).
void removeLeafOnDisk( const QString& sPath, const QString& sTag ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement root = doc.documentElement();
	QDomElement leaf = root.firstChildElement( sTag );
	while ( ! leaf.isNull() ) {
		root.removeChild( leaf );
		leaf = root.firstChildElement( sTag );
	}
	writeFile( sPath, doc.toByteArray() );
}

// Append a top-level leaf element directly on disk, simulating a concurrent
// process having created it after our load.
void addLeafOnDisk( const QString& sPath, const QString& sTag,
					const QString& sValue ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement root = doc.documentElement();
	QDomElement leaf = doc.createElement( sTag );
	leaf.appendChild( doc.createTextNode( sValue ) );
	root.appendChild( leaf );
	writeFile( sPath, doc.toByteArray() );
}

// Canonical form (parsed, then re-serialized): immune to Qt's textual
// serialization quirks (empty elements, processing instructions), but
// sensitive to element order and content.
QByteArray canonicalize( const QByteArray& xml ) {
	QDomDocument doc;
	CPPUNIT_ASSERT( doc.setContent( xml ) );
	return doc.toByteArray( 1 );
}

// Number of child elements of a given tag inside a container element of
// the document root.
int countChildElements( const QString& sPath, const QString& sContainerTag,
						const QString& sChildTag ) {
	QDomDocument doc;
	doc.setContent( readFile( sPath ) );
	QDomElement container = doc.documentElement().firstChildElement(
		sContainerTag );
	if ( container.isNull() ) {
		return 0;
	}
	int nCount = 0;
	QDomElement child = container.firstChildElement( sChildTag );
	while ( ! child.isNull() ) {
		++nCount;
		child = child.nextSiblingElement( sChildTag );
	}
	return nCount;
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
	// A plugin-host guest only owns the base layer (ADR 0022).
	pPref->setFieldOwnership( Preferences::FieldOwnership::BaseLayer );

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
	// A plugin-host guest only owns the base layer (ADR 0022).
	pPref->setFieldOwnership( Preferences::FieldOwnership::BaseLayer );

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

void PreferencesPersistTest::testMissingElementCreatedOnDisk() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// The element for the field we are about to change is absent on disk
	// (legacy config). The merge must create it, not skip it.
	removeLeafOnDisk( sPath, "maxBars" );
	pPref->setMaxBars( 42 );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testCorruptDiskSelfHeals() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// Another process crashed mid-write and left garbage behind. The save
	// must not silently merge onto (or echo back) the corrupt document: it
	// self-heals by writing a full snapshot of its own state.
	writeFile( sPath, "this is <> not valid xml ]]" );
	pPref->setMaxBars( 42 );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testListRowReplacedWholesale() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// Grow the list and persist, so the disk state contains three entries.
	pPref->setRecentFiles( QStringList( { "a", "b", "c" } ) );
	CPPUNIT_ASSERT( pPref->save( true ) );

	// Now shrink it. A container row must replace its elements wholesale —
	// never append to or merge with the on-disk list.
	pPref->setRecentFiles( QStringList( { "only" } ) );
	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	const QStringList recentFiles = pReloaded->getRecentFiles();
	CPPUNIT_ASSERT_EQUAL( 1, static_cast<int>( recentFiles.size() ) );
	CPPUNIT_ASSERT_EQUAL( std::string( "only" ),
						  recentFiles.front().toStdString() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testGuiOwnedMask() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	// An editor mirror only owns the GUI rows; the authoritative headless
	// engine owns the core rows (ADR 0022/0023).
	pPref->setFieldOwnership( Preferences::FieldOwnership::GuiOwned );

	const int nSeedMaxBars = pPref->getMaxBars();

	// Change one GUI-owned row (must persist) and one core-owned row (must
	// NOT be written by this process).
	pPref->setPreferredLanguage( "zz" );
	pPref->setMaxBars( 42 );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );
	CPPUNIT_ASSERT_EQUAL( nSeedMaxBars, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testNoOpSaveChangesNothing() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	// Force a bare-empty row into the seed so the empty-value handling is
	// exercised deterministically, independent of the ambient config.
	editLeafOnDisk( sPath, "maxBars", "" );
	const QByteArray seedBytes = readFile( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// The load-time Rubberband search replaces the shipped placeholder in
	// memory with the binary found on this machine - a real change this
	// test does not want. Restore the on-disk value so the save below is
	// a true no-op on every machine.
	pPref->m_sRubberBandCLIexecutable =
		readLeafOnDisk( sPath, "path_to_rubberband" );

	// Nothing changed in this instance. The merged write must not touch a
	// single row - in particular not the empty-valued ones, whose fresh-built
	// and parsed serializations differ textually (Qt writes `<a></a>` for a
	// fresh empty text node but `<a/>` for a parsed empty element).
	CPPUNIT_ASSERT( pPref->save( true ) );

	CPPUNIT_ASSERT_EQUAL( canonicalize( seedBytes ).toStdString(),
						  canonicalize( readFile( sPath ) ).toStdString() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testConcurrentEmptyRowEditSurvives() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	// The row is empty on disk and in our instance (empty is a legitimate
	// value for this row). Another process fills it in after our load. The
	// save must not mistake the empty value for a change - the fresh-built
	// and parsed serializations of an empty row differ textually - and clobber
	// the concurrent edit.
	editLeafOnDisk( sPath, "preferredLanguage", "" );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT_EQUAL( std::string( "" ),
						  pPref->getPreferredLanguage().toStdString() );

	editLeafOnDisk( sPath, "preferredLanguage", "zz" );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( std::string( "zz" ),
						  pReloaded->getPreferredLanguage().toStdString() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testSaveCreatesMissingConfigDir() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sSeedPath = tmp.path() + "/seed.conf";
	seedConfig( sSeedPath );

	// Fresh install: the user config directory does not exist yet.
	const QString sPath = tmp.path() + "/fresh/dir/hydrogen.conf";
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sSeedPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	pPref->setMaxBars( 42 );

	// The save must create the missing directory instead of failing on the
	// lock file (which cannot be created in a nonexistent directory).
	CPPUNIT_ASSERT( pPref->save( true ) );
	CPPUNIT_ASSERT( QFile::exists( sPath ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testAbsentRowNotRewritten() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Legacy config: maxBars was never written to disk.
	removeLeafOnDisk( sPath, "maxBars" );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// A concurrent process creates the row after our load. Our instance still
	// holds the default value, so the save must treat the row as unchanged
	// (absent from the baseline == loaded as default) and must not clobber
	// the concurrent edit with the default.
	addLeafOnDisk( sPath, "maxBars", "999" );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 999, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testLockExhaustionFailsLoudly() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	pPref->setMaxBars( 42 );

	// Another process holds the lock beyond the bounded retry budget.
	QLockFile lock( sPath + ".lock" );
	lock.setStaleLockTime( 30000 );
	CPPUNIT_ASSERT( lock.tryLock( 100 ) );

	// The save must fail loudly after the bounded retry - never block
	// forever, never silently succeed (ADR 0023).
	CPPUNIT_ASSERT( ! pPref->save( true ) );

	// With the contention gone the very same save goes through.
	lock.unlock();
	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testStandaloneWritesOverrideRow() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Default ownership is All: a standalone instance persists override
	// (driver/session) rows too - the e9fb02150 regression, where the
	// blanket override exclusion dropped driver settings on save.
	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_audioDriver != Preferences::AudioDriver::Jack );

	pPref->m_audioDriver = Preferences::AudioDriver::Jack;
	pPref->setMaxBars( 42 );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT( pReloaded->m_audioDriver ==
					Preferences::AudioDriver::Jack );
	CPPUNIT_ASSERT_EQUAL( 42, pReloaded->getMaxBars() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testShortcutsRowReplacedWholesale() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// The shipped default defers shortcut creation (bare <shortcuts/>
	// element, no children), so the seed count is zero.
	const int nSeedShortcuts =
		countChildElements( sPath, "shortcuts", "shortcut" );

	// The shortcuts subtree is one opaque merge unit: adding a binding
	// replaces the whole container - the on-disk list must end up exactly
	// one entry longer, never appended-to or duplicated.
	pPref->m_pShortcuts->insertShortcut(
		QKeySequence( "Ctrl+Alt+Shift+P" ), Shortcuts::Action::Play );

	CPPUNIT_ASSERT( pPref->save( true ) );

	CPPUNIT_ASSERT_EQUAL( nSeedShortcuts + 1,
						  countChildElements( sPath, "shortcuts",
											  "shortcut" ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	const auto sequences = pReloaded->m_pShortcuts->getKeySequences(
		Shortcuts::Action::Play );
	CPPUNIT_ASSERT( std::find( sequences.begin(), sequences.end(),
							   QKeySequence( "Ctrl+Alt+Shift+P" ) )
					!= sequences.end() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testPatternColorRowPersisted() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	auto& colors = pPref->m_pTheme->m_pInterface->m_patternColors;
	CPPUNIT_ASSERT( colors.size() >= 2 );
	const QColor originalSecond = colors.at( 1 );
	const int nSeedColors = colors.size();

	// Pattern colors are prefix-matched rows: changing one color rewrites
	// the row wholesale; sibling colors and the cardinality are preserved.
	colors[ 0 ] = QColor( 1, 2, 3 );

	CPPUNIT_ASSERT( pPref->save( true ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	const auto& reloadedColors =
		pReloaded->m_pTheme->m_pInterface->m_patternColors;
	CPPUNIT_ASSERT_EQUAL( nSeedColors,
						  static_cast<int>( reloadedColors.size() ) );
	CPPUNIT_ASSERT( reloadedColors.at( 0 ) == QColor( 1, 2, 3 ) );
	CPPUNIT_ASSERT( reloadedColors.at( 1 ) == originalSecond );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testMissingShortcutsElementDefersDefaults() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );

	// Legacy config: no <shortcuts> element at all.
	removeLeafOnDisk( sPath, "shortcuts" );

	Filesystem::setPreferencesOverwritePath( sPath );

	// Headless context: a QCoreApplication (the test runner) but no
	// QGuiApplication - Qt standard keys have no platform theme here.
	// Loading must defer default creation instead of crashing in
	// QKeySequence::keyBindings (Shortcuts::createDefaultShortcuts).
	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_pShortcuts->requiresDefaults() );

	// The deferred (empty) state equals the default footprint, so an
	// unchanged save leaves the row absent on disk - the signal survives
	// for the GUI bootstrap to pick up.
	CPPUNIT_ASSERT( pPref->save( true ) );
	CPPUNIT_ASSERT( ! readFile( sPath ).contains( "<shortcuts" ) );

	auto pReloaded = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pReloaded != nullptr );
	CPPUNIT_ASSERT( pReloaded->m_pShortcuts->requiresDefaults() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testWriteThroughPendingChanges() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// A freshly loaded instance has nothing pending: the clean point is
	// the post-load state, migrations included.
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	const int nLoadedMaxBars = pPref->m_nMaxBars;

	// A user change is pending...
	pPref->m_nMaxBars = nLoadedMaxBars + 17;
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	// ...and stays pending while further edits arrive - the write-through
	// coalesces them, it does not track individual fields.
	pPref->m_nMaxBars = nLoadedMaxBars + 1;
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	// Reverting to the last persisted state before the write-through
	// fires leaves nothing to write.
	pPref->m_nMaxBars = nLoadedMaxBars;
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	// A save - explicit (dialog OK, OSC, teardown) or write-through -
	// persists the state and clears the pending flag.
	pPref->m_nMaxBars = nLoadedMaxBars + 17;
	CPPUNIT_ASSERT( pPref->save( true ) );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );
	CPPUNIT_ASSERT_EQUAL( QString::number( nLoadedMaxBars + 17 ).toStdString(),
						  readLeafOnDisk( sPath, "maxBars" ).toStdString() );

	// Reverting *after* a save is a real change again: the clean point
	// follows the last persisted state, not what was once loaded.
	pPref->m_nMaxBars = nLoadedMaxBars;
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	// The pending change is a revert-to-baseline: the merge deliberately
	// does not write such rows back (ADR 0023), so the save leaves the
	// disk value untouched - but it still clears the pending flag.
	CPPUNIT_ASSERT( pPref->save( true ) );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );
	CPPUNIT_ASSERT_EQUAL( QString::number( nLoadedMaxBars + 17 ).toStdString(),
						  readLeafOnDisk( sPath, "maxBars" ).toStdString() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testWriteThroughOwnershipMask() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	// An editor mirror owns only GUI base rows. Tagging the ownership
	// after load must not by itself register as pending.
	pPref->setFieldOwnership( Preferences::FieldOwnership::GuiOwned );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	// Override-layer rows are outside the mask: changing them is not this
	// instance's business to write through.
	pPref->m_audioDriver = Preferences::AudioDriver::Jack;
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	// GUI-owned base rows are.
	pPref->m_sPreferredLanguage = "zz";
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testSaveCopyAsLeavesChangesPending() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );

	pPref->m_nMaxBars = pPref->m_nMaxBars + 17;
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	// Exporting a copy must not mark the shared-config state persisted.
	const QString sCopyPath = tmp.path() + "/copy.conf";
	CPPUNIT_ASSERT( pPref->saveCopyAs( sCopyPath, true ) );
	CPPUNIT_ASSERT( pPref->hasPendingChanges() );

	// The real save does.
	CPPUNIT_ASSERT( pPref->save( true ) );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	___INFOLOG( "passed" );
}

void PreferencesPersistTest::testWriteThroughIgnoresForeignDiskWrites() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";
	seedConfig( sPath );
	Filesystem::setPreferencesOverwritePath( sPath );

	auto pPref = Preferences::load( sPath, true, pTestHydrogen() );
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	// Another process edits the file. Pending is memory-based: a foreign
	// edit is nothing for this instance's write-through to do.
	// (Row-level reconciliation is the merge's job on the next save.)
	editLeafOnDisk( sPath, "maxBars", "42" );
	CPPUNIT_ASSERT( ! pPref->hasPendingChanges() );

	___INFOLOG( "passed" );
}
