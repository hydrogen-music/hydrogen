/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PanelSeparator.h"

PanelSeparator::PanelSeparator( QWidget *pParent )
	: QWidget( pParent )
	, m_color( Qt::black )
{
	setFixedWidth( PanelSeparator::nWidth +
				   2 * PanelSeparator::nMarginHorizontal );
}

PanelSeparator::~PanelSeparator() {
}

void PanelSeparator::setColor( const QColor& color ) {
	if ( m_color != color ) {
		m_color = color;
		update();
	}
}

void PanelSeparator::paintEvent( QPaintEvent* pEvent ) {
	QPainter painter( this );

	QPen pen;
	pen.setColor( m_color );
	pen.setWidth( PanelSeparator::nWidth );
	painter.setPen( pen );
	painter.drawLine(
		width() / 2, height() * ( 1 - PanelSeparator::fEffectiveHeight ),
		width() / 2, height() * PanelSeparator::fEffectiveHeight );
}
