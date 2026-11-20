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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "ShortcutsTest.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <core/Helpers/Xml.h>
#include <core/Preferences/KeySequence.h>
#include <core/Preferences/Shortcuts.h>

using namespace H2Core;

static bool containsAction( const std::vector<Shortcuts::Action>& actions,
							Shortcuts::Action action ) {
	return std::find( actions.begin(), actions.end(), action ) != actions.end();
}

void ShortcutsTest::testCreateDefaultShortcutsUsesInjectedSequences() {
	___INFOLOG( "" );

	// Headless on purpose: the platform-dependent Undo/Redo defaults
	// are resolved by the GUI (QKeySequence::keyBindings() requires a
	// QGuiApplication and would segfault here) and injected as
	// arguments.
	auto pShortcuts = std::make_shared<Shortcuts>();
	pShortcuts->createDefaultShortcuts(
		KeySequence::fromPortableText( "Ctrl+Z" ),
		KeySequence::fromPortableText( "Ctrl+Shift+Z" ) );

	const auto undoActions =
		pShortcuts->getActions( KeySequence::fromPortableText( "Ctrl+Z" ) );
	CPPUNIT_ASSERT( containsAction( undoActions, Shortcuts::Action::Undo ) );

	const auto redoActions = pShortcuts->getActions(
		KeySequence::fromPortableText( "Ctrl+Shift+Z" ) );
	CPPUNIT_ASSERT( containsAction( redoActions, Shortcuts::Action::Redo ) );

	// Locale-independent defaults are created alongside.
	const auto panicActions =
		pShortcuts->getActions( KeySequence::fromPortableText( "F12" ) );
	CPPUNIT_ASSERT( containsAction( panicActions, Shortcuts::Action::Panic ) );

	const auto saveActions =
		pShortcuts->getActions( KeySequence::fromPortableText( "Ctrl+S" ) );
	CPPUNIT_ASSERT(
		containsAction( saveActions, Shortcuts::Action::SaveSong ) );

	___INFOLOG( "passed" );
}

void ShortcutsTest::testLoadFromSkipsMalformedRows() {
	___INFOLOG( "" );

	XMLDoc doc;
	XMLNode root = doc.set_root( "hydrogen" );
	XMLNode shortcutsNode = root.createNode( "shortcuts" );

	XMLNode validNode = shortcutsNode.createNode( "shortcut" );
	validNode.write_string( "keySequence", "Ctrl+Alt+Shift+P" );
	validNode.write_int( "action",
						 static_cast<int>( Shortcuts::Action::Panic ) );

	// Not part of the canonical portable-text format (lowercase
	// modifiers) - must be skipped, not silently reinterpreted.
	XMLNode lowercaseNode = shortcutsNode.createNode( "shortcut" );
	lowercaseNode.write_string( "keySequence", "ctrl+s" );
	lowercaseNode.write_int( "action",
							 static_cast<int>( Shortcuts::Action::Play ) );

	XMLNode emptyNode = shortcutsNode.createNode( "shortcut" );
	emptyNode.write_string( "keySequence", "" );
	emptyNode.write_int( "action",
						 static_cast<int>( Shortcuts::Action::Play ) );

	const auto pShortcuts = Shortcuts::loadFrom( root, true /*bSilent*/ );
	CPPUNIT_ASSERT( pShortcuts != nullptr );

	// The valid row survived ...
	const auto panicActions =
		pShortcuts->getActions( KeySequence::fromPortableText(
			"Ctrl+Alt+Shift+P" ) );
	CPPUNIT_ASSERT( containsAction( panicActions, Shortcuts::Action::Panic ) );

	// ... the malformed ones were skipped without taking the valid
	// one down with them. Unmapped actions report the empty
	// placeholder sequence (the PreferencesDialog renders it as a
	// blank row).
	const auto playSequences =
		pShortcuts->getKeySequences( Shortcuts::Action::Play );
	CPPUNIT_ASSERT_EQUAL( 1, static_cast<int>( playSequences.size() ) );
	CPPUNIT_ASSERT( playSequences.front().isEmpty() );

	// At least one usable shortcut was found: no defaults needed.
	CPPUNIT_ASSERT( ! pShortcuts->requiresDefaults() );

	___INFOLOG( "passed" );
}

void ShortcutsTest::testLoadFromGarbageOnlyFallsBackToDefaults() {
	___INFOLOG( "" );

	XMLDoc doc;
	XMLNode root = doc.set_root( "hydrogen" );
	XMLNode shortcutsNode = root.createNode( "shortcuts" );

	XMLNode garbageNode = shortcutsNode.createNode( "shortcut" );
	garbageNode.write_string( "keySequence", "ctrl+s" );
	garbageNode.write_int( "action",
						   static_cast<int>( Shortcuts::Action::Play ) );

	const auto pShortcuts = Shortcuts::loadFrom( root, true /*bSilent*/ );
	CPPUNIT_ASSERT( pShortcuts != nullptr );

	// A configuration whose shortcut rows are all unusable is as good
	// as none: it must not silently leave the user without any
	// bindings.
	CPPUNIT_ASSERT( pShortcuts->requiresDefaults() );

	___INFOLOG( "passed" );
}

void ShortcutsTest::testLoadFromDefersDefaultsWhenNodeMissing() {
	___INFOLOG( "" );

	XMLDoc doc;
	XMLNode root = doc.set_root( "hydrogen" );
	// No <shortcuts> element at all.

	const auto pShortcuts = Shortcuts::loadFrom( root, true /*bSilent*/ );
	CPPUNIT_ASSERT( pShortcuts != nullptr );

	// Headless callers must never trigger default creation themselves
	// - the Undo/Redo defaults require a QGuiApplication. The GUI
	// seeds them after its bootstrap.
	CPPUNIT_ASSERT( pShortcuts->requiresDefaults() );

	___INFOLOG( "passed" );
}

void ShortcutsTest::testSaveToLoadFromRoundTrip() {
	___INFOLOG( "" );

	auto pShortcuts = std::make_shared<Shortcuts>();
	pShortcuts->insertShortcut(
		KeySequence::fromPortableText( "Ctrl+Alt+Shift+P" ),
		Shortcuts::Action::Panic );
	// A modifier-only chord and a multi-chord sequence: both are legal
	// in the owned portable-text format and must survive a round
	// trip.
	pShortcuts->insertShortcut( KeySequence::fromPortableText( "Ctrl+" ),
								Shortcuts::Action::Play );
	pShortcuts->insertShortcut(
		KeySequence::fromPortableText( "Ctrl+X, Ctrl+C" ),
		Shortcuts::Action::Play );
	// Several actions may share one sequence.
	pShortcuts->insertShortcut(
		KeySequence::fromPortableText( "Ctrl+Alt+Shift+P" ),
		Shortcuts::Action::Play );

	XMLDoc doc;
	XMLNode root = doc.set_root( "hydrogen" );
	pShortcuts->saveTo( root );

	const auto pReloaded = Shortcuts::loadFrom( root, true /*bSilent*/ );
	CPPUNIT_ASSERT( pReloaded != nullptr );

	const auto panicSequences =
		pReloaded->getKeySequences( Shortcuts::Action::Panic );
	CPPUNIT_ASSERT_EQUAL( 1, static_cast<int>( panicSequences.size() ) );
	CPPUNIT_ASSERT( panicSequences.front() ==
					KeySequence::fromPortableText( "Ctrl+Alt+Shift+P" ) );

	// m_actionsMap iterates sequences in KeySequence order, so the
	// per-action vectors are deterministic on both sides.
	const auto playSequences =
		pReloaded->getKeySequences( Shortcuts::Action::Play );
	CPPUNIT_ASSERT_EQUAL( 3, static_cast<int>( playSequences.size() ) );
	CPPUNIT_ASSERT( playSequences.at( 0 ) ==
					KeySequence::fromPortableText( "Ctrl+" ) );
	CPPUNIT_ASSERT( playSequences.at( 1 ) ==
					KeySequence::fromPortableText( "Ctrl+X, Ctrl+C" ) );
	CPPUNIT_ASSERT( playSequences.at( 2 ) ==
					KeySequence::fromPortableText( "Ctrl+Alt+Shift+P" ) );

	// Usable shortcuts were found: the reloaded instance must not ask
	// for defaults again.
	CPPUNIT_ASSERT( ! pReloaded->requiresDefaults() );

	___INFOLOG( "passed" );
}
