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

#include "GridCell.h"

using namespace H2Core;

GridCell::GridCell( const GridPoint& gridPoint, bool bActive, float fWidth,
					bool bDrawnVirtual )
	: m_gridPoint( gridPoint )
	, m_bActive( bActive )
	, m_fWidth( fWidth )
	, m_bDrawnVirtual( bDrawnVirtual )
{
}

GridCell::GridCell( const GridCell& other )
	: m_gridPoint( other.m_gridPoint )
	, m_bActive( other.m_bActive )
	, m_fWidth( other.m_fWidth )
	, m_bDrawnVirtual( other.m_bDrawnVirtual )
{
}

GridCell::~GridCell() {
}

QString GridCell::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[GridCell]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_gridPoint: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_gridPoint.toQString() ) )
			.append( QString( "%1%2m_bActive: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bActive ) )
			.append( QString( "%1%2m_fWidth: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fWidth ) )
			.append( QString( "%1%2m_bDrawnVirtual: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bDrawnVirtual ) );
	}
	else {
		sOutput = QString( "[GridCell]" )
			.append( QString( ", m_gridPoint: %1" ).arg( m_gridPoint.toQString() ) )
			.append( QString( ", m_bActive: %1" ).arg( m_bActive ) )
			.append( QString( ", m_fWidth: %1" ).arg( m_fWidth ) )
			.append( QString( ", m_bDrawnVirtual: %1" ).arg( m_bDrawnVirtual ) );
	}

	return sOutput;
}
