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

#ifndef H2C_COLOR_H
#define H2C_COLOR_H

#include <QString>

namespace H2Core
{

/**
\ingroup H2CORE
*/
/**
 * Minimal RGB color value type used by the headless core.
 *
 * Drop-in replacement for the QColor usages core used to carry: same
 * (r,g,b) construction and validation semantics, same "#rrggbb" name()
 * output. The parity is load-bearing — Theme::toQString() logging and
 * the "r,g,b" XML wire format depend on byte-identical behavior.
 *
 * Deliberately no HSV, alpha, or lighter/darker: the core must stay
 * free of Qt::Gui, and the GUI converts to QColor at its boundary
 * (Skin::toQColor/Skin::fromQColor) whenever it needs those.
 */
class Color
{
	public:
		Color() : m_nRed( 0 ), m_nGreen( 0 ), m_nBlue( 0 ),
				  m_bValid( false ) {}

		/**
		 * Out-of-range components invalidate the color (which then
		 * reads back as black) instead of clamping — the XML reader
		 * relies on this to reject malformed "r,g,b" values.
		 */
		Color( int nRed, int nGreen, int nBlue ) {
			if ( nRed < 0 || nRed > 255 || nGreen < 0 || nGreen > 255
				 || nBlue < 0 || nBlue > 255 ) {
				m_nRed = 0;
				m_nGreen = 0;
				m_nBlue = 0;
				m_bValid = false;
				return;
			}
			m_nRed = nRed;
			m_nGreen = nGreen;
			m_nBlue = nBlue;
			m_bValid = true;
		}

		int red() const { return m_nRed; }
		int green() const { return m_nGreen; }
		int blue() const { return m_nBlue; }
		bool isValid() const { return m_bValid; }

		/** "#rrggbb" lowercase hex; "#000000" when invalid
		 * (QColor::name() parity). */
		QString name() const {
			return QString( "#%1%2%3" )
				.arg( m_nRed, 2, 16, QChar( '0' ) )
				.arg( m_nGreen, 2, 16, QChar( '0' ) )
				.arg( m_nBlue, 2, 16, QChar( '0' ) );
		}

		bool operator==( const Color& other ) const {
			return m_bValid == other.m_bValid
				&& m_nRed == other.m_nRed
				&& m_nGreen == other.m_nGreen
				&& m_nBlue == other.m_nBlue;
		}
		bool operator!=( const Color& other ) const {
			return ! ( *this == other );
		}

	private:
		int m_nRed;
		int m_nGreen;
		int m_nBlue;
		bool m_bValid;
};

}

#endif
