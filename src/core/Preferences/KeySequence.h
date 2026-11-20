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

#ifndef H2C_KEY_SEQUENCE_H
#define H2C_KEY_SEQUENCE_H

#include <QString>

namespace H2Core
{

/**
\ingroup H2CORE
*/
/**
 * Keyboard shortcut value type used by the headless core.
 *
 * Drop-in replacement for the QKeySequence usages core used to carry:
 * up to four chords, each stored as one combined int (Qt::KeyboardModifier
 * bits OR Qt::Key value), zero-padded. The count is derived — the first
 * zero chord ends the sequence — so the type serializes byte-stable
 * (plain lexicographic operator< over the four ints, like QKeySequence).
 *
 * The portable-text format is owned by this class, not by Qt:
 *
 * - Chords are joined with ", " (e.g. "Ctrl+X, Ctrl+C").
 * - Within a chord, modifier prefixes are emitted in canonical order
 *   Meta, Ctrl, Alt, Shift, followed by the key name (e.g.
 *   "Meta+Ctrl+Alt+Shift+P").
 * - Key names are the portable names QKeySequence::toString() emits,
 *   except for the keys Qt formats as non-ASCII or garbage (the
 *   Latin-1 supplement, dead keys, IME keys, the modifier key-codes),
 *   which use their Qt::Key enum names instead.
 * - A chord with modifiers but no key ("Ctrl+") round-trips with key 0.
 *
 * Parsing is strict, by design: case-sensitive, exact ", " separation,
 * each modifier at most once, and anything unknown — key names,
 * modifiers, empty chords, more than four chords — yields the empty
 * sequence, which callers skip and warn about. Qt's reader is lenient
 * (case-insensitive, trims, fills garbage with Key_unknown); we keep it
 * strict and human-editable.
 *
 * Deliberately no NativeText, no StandardKey resolution, no
 * QKeySequence::matches(): the core must stay free of Qt::Gui, and the
 * GUI converts to QKeySequence at its boundary
 * (Skin::toQKeySequence/Skin::fromQKeySequence) whenever it needs
 * those.
 */
class KeySequence
{
	public:
		KeySequence() {}

		/** Stores one raw combined chord; the rest stays zero. */
		explicit KeySequence( int nKey ) {
			m_nKeys[ 0 ] = nKey;
		}

		/** Stores four raw combined chords. */
		KeySequence( int nKey1, int nKey2, int nKey3, int nKey4 );

		/** Parses strict portable text; unknown or malformed input
		 * yields the empty sequence. */
		static KeySequence fromPortableText( const QString& sText );

		/** Formats the sequence as portable text; "" when empty. */
		QString toPortableText() const;

		bool isEmpty() const;

		/** Number of chords: the first zero chord ends the sequence. */
		int count() const;

		/** Combined int of chord \a nIndex (zero-padded: out-of-range
		 * and trailing chords read 0). */
		int operator[]( int nIndex ) const;

		/** Stores \a nValue at chord \a nIndex; out-of-range indexes
		 * are ignored. */
		void setKey( int nIndex, int nValue );

		bool operator==( const KeySequence& other ) const;
		bool operator!=( const KeySequence& other ) const;

		/** Plain lexicographic comparison over the four stored ints
		 * (byte-stable, like QKeySequence). */
		bool operator<( const KeySequence& other ) const;

	private:
		int m_nKeys[ 4 ] = { 0, 0, 0, 0 };
};

}

#endif
