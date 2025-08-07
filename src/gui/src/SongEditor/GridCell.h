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

#ifndef GRID_CELL_H
#define GRID_CELL_H

#include <core/Basics/GridPoint.h>
#include <core/Object.h>

#include <map>
#include <memory>
#include <vector>

#include <QtGui>
#include <QtWidgets>

/** Basic building block for the grid of the #SongEditor.
 *
 * \ingroup docGUI*/

class GridCell : public H2Core::Object<GridCell>
{
    H2_OBJECT(GridCell)

	public:
		GridCell( const H2Core::GridPoint& gridPoint, bool bActive, float fWidth,
			  bool bDrawnVirtual );
		GridCell( const GridCell& other );
		~GridCell();

		int getColumn() const;
		int getRow() const;
		H2Core::GridPoint getGridPoint() const;
		void setGridPoint( H2Core::GridPoint gridPoint );

		bool getActive() const;
		void setActive( bool bActive );

		float getWidth() const;
		void setWidth( float fWidth );

		bool getDrawnVirtual() const;
		void setDrawnVirtual( bool bDrawnVirtual );

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	private:
		H2Core::GridPoint m_gridPoint;
		bool m_bActive;
		bool m_bDrawnVirtual;
		float m_fWidth;
};

inline int GridCell::getColumn() const {
	return m_gridPoint.getColumn();
}
inline int GridCell::getRow() const {
	return m_gridPoint.getRow();
}
inline H2Core::GridPoint GridCell::getGridPoint() const {
	return m_gridPoint;
}
inline void GridCell::setGridPoint( H2Core::GridPoint gridPoint ) {
	if ( m_gridPoint != gridPoint ) {
		m_gridPoint = gridPoint;
	}
}

inline bool GridCell::getActive() const {
	return m_bActive;
}
inline void GridCell::setActive( bool bActive ) {
	if ( m_bActive != bActive ) {
		m_bActive = bActive;
	}
}
inline float GridCell::getWidth() const {
	return m_fWidth;
}
inline void GridCell::setWidth( float fWidth ) {
	if ( m_fWidth != fWidth ) {
		m_fWidth = fWidth;
	}
}
inline bool GridCell::getDrawnVirtual() const {
	return m_bDrawnVirtual;
}
inline void GridCell::setDrawnVirtual( bool bDrawnVirtual ) {
	if ( m_bDrawnVirtual != bDrawnVirtual ) {
		m_bDrawnVirtual = bDrawnVirtual;
	}
}

#endif
