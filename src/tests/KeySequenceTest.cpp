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

#include "KeySequenceTest.h"

#include "TestHelper.h"

#include <core/Preferences/KeySequence.h>
#include <core/Object.h>

#include "Skin.h"

#include <QFile>
#include <QKeySequence>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

void KeySequenceTest::testValueSemantics()
{
	___INFOLOG( "" );

	// Default construction: empty, zero chords.
	const H2Core::KeySequence empty;
	CPPUNIT_ASSERT( empty.isEmpty() );
	CPPUNIT_ASSERT( empty.count() == 0 );
	CPPUNIT_ASSERT( empty.toPortableText() == "" );

	// Raw int construction: one chord, the rest zero-padded.
	const H2Core::KeySequence single( 0x4000053 );  // Ctrl+S
	CPPUNIT_ASSERT( ! single.isEmpty() );
	CPPUNIT_ASSERT( single.count() == 1 );
	CPPUNIT_ASSERT( single[ 0 ] == 0x4000053 );
	CPPUNIT_ASSERT( single[ 1 ] == 0 );
	// Out-of-range reads are zero, not an error.
	CPPUNIT_ASSERT( single[ 4 ] == 0 );
	CPPUNIT_ASSERT( single[ -1 ] == 0 );

	// setKey: raw storage, count stays derived (first zero ends the
	// sequence — a hole at chord 2 hides chord 3).
	H2Core::KeySequence multi;
	multi.setKey( 0, 0x41 );
	multi.setKey( 1, 0x42 );
	CPPUNIT_ASSERT( multi.count() == 2 );
	multi.setKey( 3, 0x44 );
	CPPUNIT_ASSERT( multi.count() == 2 );
	CPPUNIT_ASSERT( multi[ 3 ] == 0x44 );
	// Out-of-range writes are ignored.
	multi.setKey( 9, 0x45 );
	CPPUNIT_ASSERT( multi[ 3 ] == 0x44 );

	// Equality and ordering compare the raw four-int storage, so they
	// stay byte-stable for serialization (like QKeySequence).
	CPPUNIT_ASSERT( single == H2Core::KeySequence( 0x4000053 ) );
	CPPUNIT_ASSERT( single != empty );
	CPPUNIT_ASSERT( empty == H2Core::KeySequence() );
	CPPUNIT_ASSERT( H2Core::KeySequence( 0x41 ) < H2Core::KeySequence( 0x42 ) );
	CPPUNIT_ASSERT( H2Core::KeySequence( 0x41, 0x42, 0, 0 )
					< H2Core::KeySequence( 0x41, 0x43, 0, 0 ) );
	CPPUNIT_ASSERT( ! ( H2Core::KeySequence( 0x41, 0x42, 0, 0 )
						< H2Core::KeySequence( 0x41, 0x42, 0, 0 ) ) );

	___INFOLOG( "passed" );
}

void KeySequenceTest::testPortableTextRoundTrip()
{
	___INFOLOG( "" );

	// Every pin must survive fromPortableText( toPortableText( x ) )
	// byte-identically — the preferences format depends on it.
	const QStringList pins = {
		"",                       // canonical empty
		"A", "5", "F5", "F12",    // plain keys from the shipped config
		"Space", "Backspace", "Del", "PgUp",
		"+", ",", "-", "\\",      // punctuation keys from the shipped config
		"Ctrl+S", "Ctrl+Alt+D", "Ctrl+Shift+Backspace", "Ctrl+Space",
		"Ctrl+?",                 // Ctrl + question key
		"Ctrl++",                 // Ctrl + plus key
		"Meta+Ctrl+Alt+Shift+P",  // canonical modifier order
		"Ctrl+",                  // modifier-only chord (key 0)
		"Ctrl+X, Ctrl+C",         // multi-chord join
		"unknown",                // Key_unknown round-trips (total bijection)
	};
	for ( const QString& sPin : pins ) {
		const H2Core::KeySequence sequence =
			H2Core::KeySequence::fromPortableText( sPin );
		CPPUNIT_ASSERT( sequence.toPortableText() == sPin );
	}

	// Spot-check the combined ints against the Qt::KeyboardModifier /
	// Qt::Key values (QtCore, stable across Qt 5 and 6).
	CPPUNIT_ASSERT( H2Core::KeySequence::fromPortableText( "Ctrl+S" )[ 0 ]
					== ( int( Qt::ControlModifier ) | int( Qt::Key_S ) ) );
	CPPUNIT_ASSERT( H2Core::KeySequence::fromPortableText(
						"Meta+Ctrl+Alt+Shift+P" )[ 0 ]
					== ( int( Qt::MetaModifier ) | int( Qt::ControlModifier )
						 | int( Qt::AltModifier ) | int( Qt::ShiftModifier )
						 | int( Qt::Key_P ) ) );
	CPPUNIT_ASSERT( H2Core::KeySequence::fromPortableText( "Ctrl+" )[ 0 ]
					== int( Qt::ControlModifier ) );

	___INFOLOG( "passed" );
}

void KeySequenceTest::testStrictParsing()
{
	___INFOLOG( "" );

	// Anything malformed yields the empty sequence, which callers skip
	// and warn about — no silent repair, no Key_unknown carrier.
	const QStringList garbage = {
		"ctrl+s",            // lowercase modifier
		"CTRL+S",            // uppercase modifier
		"Ctrl+shift+A",      // mixed case
		"Ctrl+Ctrl+A",       // duplicate modifier
		"Ctrl+NotAKey",      // unknown key name
		"Ctrl+ A",           // space before the key
		" Ctrl+A",           // leading space
		"Ctrl+A ",           // trailing space
		"A,,B",              // stray commas
		"Ctrl+A, ",          // trailing separator: empty chord
		"Ctrl+A,,Ctrl+C",    // empty middle chord
		"A,B",               // missing space after the comma
		"A,  B",             // double space after the comma
		"A, B, C, D, E",     // more than four chords
		"Num+1",             // unsupported modifier
		"Meta+Ctrl",         // trailing modifier without a key
	};
	for ( const QString& sGarbage : garbage ) {
		const H2Core::KeySequence sequence =
			H2Core::KeySequence::fromPortableText( sGarbage );
		CPPUNIT_ASSERT( sequence.isEmpty() );
		CPPUNIT_ASSERT( sequence.toPortableText() == "" );
	}

	// "" is the canonical empty sequence, not garbage.
	CPPUNIT_ASSERT( H2Core::KeySequence::fromPortableText( "" ).isEmpty() );

	___INFOLOG( "passed" );
}

void KeySequenceTest::testCorpusRoundTrip()
{
	___INFOLOG( "" );

	// Every sequence shipped in the test configuration must round-trip
	// byte-identically — this is the format-compatibility pin for the
	// preferences file.
	QFile configFile( H2TEST_FILE( "preferences/current.conf" ) );
	CPPUNIT_ASSERT( configFile.open( QIODevice::ReadOnly | QIODevice::Text ) );
	const QString sConfig = QTextStream( &configFile ).readAll();

	QRegularExpression re( "<keySequence>([^<]*)</keySequence>" );
	QRegularExpressionMatchIterator it = re.globalMatch( sConfig );
	int nSequences = 0;
	while ( it.hasNext() ) {
		const QString sStored = it.next().captured( 1 );
		const H2Core::KeySequence sequence =
			H2Core::KeySequence::fromPortableText( sStored );
		CPPUNIT_ASSERT( ! sequence.isEmpty() );
		CPPUNIT_ASSERT( sequence.toPortableText() == sStored );
		++nSequences;
	}
	// Exact count: editing the corpus must be a conscious change.
	CPPUNIT_ASSERT( nSequences == 67 );

	___INFOLOG( "passed" );
}

void KeySequenceTest::testQKeySequenceConversion()
{
	___INFOLOG( "" );

	// The GUI boundary converts in both directions; parity must hold
	// for everything the preferences format can store.
	const QStringList pins = {
		"A", "5", "F5", "Space", "Backspace", "Del",
		"+", ",", "-", "\\",
		"Ctrl+S", "Ctrl+Alt+D", "Ctrl+Shift+Backspace", "Ctrl+Space",
		"Ctrl+?", "Ctrl++",
		"Meta+Ctrl+Alt+Shift+P",
		"Ctrl+X, Ctrl+C",
	};
	for ( const QString& sPin : pins ) {
		const H2Core::KeySequence core =
			H2Core::KeySequence::fromPortableText( sPin );
		const QKeySequence qt =
			QKeySequence::fromString( sPin, QKeySequence::PortableText );
		CPPUNIT_ASSERT( Skin::toQKeySequence( core ) == qt );
		CPPUNIT_ASSERT( Skin::fromQKeySequence( qt ) == core );
	}

	// Deliberately no QKeySequence( StandardKey ) round-trip here: on
	// Qt 6 that constructor queries the platform theme and crashes in
	// a headless test binary. This is exactly why the core never
	// resolves StandardKey itself — the GUI constructs the undo/redo
	// defaults after the QApplication exists and hands them over.

	___INFOLOG( "passed" );
}

void KeySequenceTest::testQnamespaceCompleteness()
{
	___INFOLOG( "" );

	// Every literal-valued Qt::Key entry of the Qt version we build
	// against must round-trip through the key table — a Qt upgrade
	// that adds keys fails here loudly instead of silently dropping
	// them.
	const QString sHeaderPath = QString( QT_QNAMESPACE_H );
	QFile headerFile( sHeaderPath );
	CPPUNIT_ASSERT( headerFile.open( QIODevice::ReadOnly | QIODevice::Text ) );
	const QString sHeader = QTextStream( &headerFile ).readAll();

	QRegularExpression re( "Key_([A-Za-z0-9_]+)\\s*=\\s*(0x[0-9A-Fa-f]+|\\d+)" );
	QRegularExpressionMatchIterator it = re.globalMatch( sHeader );
	int nKeys = 0;
	while ( it.hasNext() ) {
		const QRegularExpressionMatch match = it.next();
		const int nValue = match.captured( 2 ).toInt( nullptr, 0 );
		const H2Core::KeySequence sequence( nValue );
		// An untabled key formats as "" — that is the loud failure.
		CPPUNIT_ASSERT( sequence.toPortableText() != "" );
		CPPUNIT_ASSERT( H2Core::KeySequence::fromPortableText(
							sequence.toPortableText() )[ 0 ] == nValue );
		++nKeys;
	}
	// Sanity: the header regex still matches the whole enum.
	CPPUNIT_ASSERT( nKeys > 400 );

	___INFOLOG( "passed" );
}
