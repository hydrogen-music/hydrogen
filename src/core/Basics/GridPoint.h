/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef GRID_POINT_H
#define GRID_POINT_H

#include <QString>

namespace H2Core {

/**
 * Lightweight definition of a point within one of our editor grids. This could
 * be a pattern cell in #SongEditor or a note in a #PatternEditor instance. It's
 * main purpose is to have a clear distinction of pixel-based QPoint used in the
 * Qt rendering engine and OS interfacing events and a point on our own grid.
 *
 * This class is not derived from #H2Core::Object to keep it lightweight since
 * there will be a lot of them.
 *
 * \ingroup docDataStructure */
class GridPoint {
	public:
		GridPoint( int nColumn = -1, int nRow = -1 )
			: m_nColumn( nColumn )
			, m_nRow( nRow ) {
		}
		GridPoint( const GridPoint& other )
			: m_nColumn( other.m_nColumn )
			, m_nRow( other.m_nRow ) {
		}
		~GridPoint() {};

		int getColumn() const {
			return m_nColumn;
		}
		void setColumn( int nColumn ) {
			m_nColumn = nColumn;
		}

		int getRow() const {
			return m_nRow;
		}
		void setRow( int nRow ) {
			m_nRow = nRow;
		}

		bool operator==( const GridPoint& other ) {
			return m_nRow == other.m_nRow && m_nColumn == other.m_nColumn;
		}

		bool operator!=( const GridPoint& other ) {
			return m_nRow != other.m_nRow || m_nColumn != other.m_nColumn;
		}

		QString toQString() const {
			return QString( "[ column: %1, row: %2 ]" )
				.arg( m_nColumn ).arg( m_nRow );
		}

	private:
		int m_nColumn;
		int m_nRow;
};

// Implement comparison between GridPoint needed for std::set.
inline int operator<( GridPoint a, GridPoint b ) {
	int nColumnA = a.getColumn(), nColumnB = b.getColumn();
	if ( nColumnA != nColumnB ) {
		return nColumnA < nColumnB;
	} else {
		int nRowA = a.getRow(), nRowB = b.getRow();
		return nRowA < nRowB;
	}
}
};

#endif
