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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "StatusLED.h"

StatusLED::StatusLED( QWidget* pParent, const QSize& size )
	: QWidget( pParent ), m_state( State::Unchecked )
{
	setFixedSize( size );
}

StatusLED::~StatusLED()
{
}

void StatusLED::setState( State state )
{
	if ( m_state == state ) {
		return;
	}
	m_state = state;
	update();
}

void StatusLED::paintEvent( QPaintEvent* )
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );

	const int w = width();
	const int h = height();

	// Determine LED color based on state
	QColor ledColor;
	switch ( m_state ) {
		case State::Online:
			ledColor = QColor( 0, 180, 0 );
			break;
		case State::Offline:
			ledColor = QColor( 200, 0, 0 );
			break;
		case State::Unchecked:
		default:
			ledColor = QColor( 128, 128, 128 );
			break;
	}

	// Draw oval LED
	const qreal radius = std::min( w, h ) / 2.0 - 0.5;
	const QPointF center( w / 2.0, h / 2.0 );

	QLinearGradient gradient(
		center.x(), center.y() - radius, center.x(), center.y() + radius
	);
	gradient.setColorAt( 0, ledColor.lighter( 160 ) );
	gradient.setColorAt( 1, ledColor.darker( 130 ) );

	QPen pen( Qt::black );
	pen.setWidth( 1 );

	painter.setBrush( gradient );
	painter.setPen( pen );
	painter.drawEllipse( center, radius, radius );
}
