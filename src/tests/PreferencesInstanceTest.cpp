/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "PreferencesInstanceTest.h"
#include "TestHelper.h"

#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Shortcuts.h>

#include <algorithm>

using namespace H2Core;

void PreferencesInstanceTest::testIndependentInstances() {
	___INFOLOG( "" );

	// Preferences is instance-ownable: several may coexist, fully independent
	// of each other (ADR 0015).
	auto pA = std::make_shared<Preferences>();
	auto pB = std::make_shared<Preferences>();
	CPPUNIT_ASSERT( pA != pB );

	pA->m_nBufferSize = 256;
	pB->m_nBufferSize = 2048;

	// Mutating one instance never affects the other.
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pA->m_nBufferSize );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )2048, pB->m_nBufferSize );

	// A copy is an independent object, not an alias.
	auto pCopy = std::make_shared<Preferences>( pA );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pCopy->m_nBufferSize );
	pCopy->m_nBufferSize = 512;
	CPPUNIT_ASSERT_EQUAL( ( unsigned )256, pA->m_nBufferSize );
	CPPUNIT_ASSERT_EQUAL( ( unsigned )512, pCopy->m_nBufferSize );

	___INFOLOG( "passed" );
}

void PreferencesInstanceTest::testCopyConstructorIsDeep() {
	___INFOLOG( "" );

	// The members held by pointer are mutable shared state. The copy
	// constructor's contract is a deep copy: fresh objects carrying the
	// source's values, so edits in one instance never leak into the
	// other.
	auto pSource = std::make_shared<Preferences>();

	pSource->m_pShortcuts->insertShortcut(
		QKeySequence( "Ctrl+Alt+Shift+P" ), Shortcuts::Action::Play );
	pSource->m_pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, Midi::parameterFromInt( 0 ),
		std::make_shared<MidiAction>( MidiAction::Type::BeatCounter ),
		Event::Trigger::Suppress, pTestHydrogen() );
	pSource->m_pMidiInstrumentMap->setGlobalInputChannel( Midi::Channel( 9 ) );
	pSource->m_pTheme->m_pInterface->m_patternColors[ 0 ] = QColor( 1, 2, 3 );

	auto pCopy = std::make_shared<Preferences>( pSource );

	// Fresh objects, not aliases of the source's ones.
	CPPUNIT_ASSERT( pCopy->m_pShortcuts.get() != pSource->m_pShortcuts.get() );
	CPPUNIT_ASSERT(
		pCopy->m_pMidiEventMap.get() != pSource->m_pMidiEventMap.get() );
	CPPUNIT_ASSERT( pCopy->m_pMidiInstrumentMap.get() !=
					pSource->m_pMidiInstrumentMap.get() );
	CPPUNIT_ASSERT( pCopy->m_pTheme.get() != pSource->m_pTheme.get() );

	// The source's state was taken over.
	const auto copySequences =
		pCopy->m_pShortcuts->getKeySequences( Shortcuts::Action::Play );
	CPPUNIT_ASSERT( std::find( copySequences.begin(), copySequences.end(),
							   QKeySequence( "Ctrl+Alt+Shift+P" ) ) !=
					copySequences.end() );
	CPPUNIT_ASSERT_EQUAL(
		1, static_cast<int>( pCopy->m_pMidiEventMap->getMidiEvents().size() ) );
	CPPUNIT_ASSERT( pCopy->m_pMidiInstrumentMap->getGlobalInputChannel() ==
					Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( pCopy->m_pTheme->m_pInterface->m_patternColors.at( 0 ) ==
					QColor( 1, 2, 3 ) );

	// The deep copy chains: the copied event's action is a fresh object
	// too, not shared with the source.
	CPPUNIT_ASSERT( pCopy->m_pMidiEventMap->getMidiEvents()
						.at( 0 )
						->getMidiAction()
						.get() !=
					pSource->m_pMidiEventMap->getMidiEvents()
						.at( 0 )
						->getMidiAction()
						.get() );

	// Mutating the copy never leaks into the source.
	pCopy->m_pShortcuts->insertShortcut(
		QKeySequence( "Ctrl+Alt+Shift+O" ), Shortcuts::Action::Play );
	CPPUNIT_ASSERT_EQUAL(
		1, static_cast<int>(
			   pSource->m_pShortcuts->getKeySequences( Shortcuts::Action::Play )
				   .size() ) );

	pCopy->m_pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, Midi::parameterFromInt( 1 ),
		std::make_shared<MidiAction>( MidiAction::Type::BeatCounter ),
		Event::Trigger::Suppress, pTestHydrogen() );
	CPPUNIT_ASSERT_EQUAL(
		1, static_cast<int>( pSource->m_pMidiEventMap->getMidiEvents().size() ) );

	pCopy->m_pMidiInstrumentMap->setGlobalInputChannel( Midi::Channel( 3 ) );
	CPPUNIT_ASSERT( pSource->m_pMidiInstrumentMap->getGlobalInputChannel() ==
					Midi::Channel( 9 ) );

	pCopy->m_pTheme->m_pInterface->m_patternColors[ 0 ] = QColor( 4, 5, 6 );
	CPPUNIT_ASSERT( pSource->m_pTheme->m_pInterface->m_patternColors.at( 0 ) ==
					QColor( 1, 2, 3 ) );

	___INFOLOG( "passed" );
}
