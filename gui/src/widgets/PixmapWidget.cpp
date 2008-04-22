/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "PixmapWidget.h"
#include "../Skin.h"
#include <QPainter>

#include <hydrogen/Object.h>

PixmapWidget::PixmapWidget( QWidget *pParent, const QString& sClassName )
 : Object( sClassName )
 , QWidget( pParent )
 , m_sPixmapPath( "" )
 , __expand_horiz(false)
{
	// draw the background: slower but useful with transparent images!
	//setAttribute(Qt::WA_NoBackground);
	__color = QColor(200, 0, 0);
}



PixmapWidget::~PixmapWidget()
{
}




void PixmapWidget::setColor(const QColor& color)
{
	if (__color == color) {
		return;
	}
	__color = color;
	update();
}



void PixmapWidget::setPixmap( QString sPixmapPath, bool expand_horiz )
{
	if ( m_sPixmapPath == sPixmapPath ) {
		return;
	}
	m_sPixmapPath = sPixmapPath;
	__expand_horiz = expand_horiz;

	bool ok = m_pixmap.load( Skin::getImagePath() + sPixmapPath );
	if ( !ok ) {
		_INFOLOG( QString( "Error loading: %1%2").arg( Skin::getImagePath() ).arg( sPixmapPath ) );
	}

	resize( m_pixmap.width(), m_pixmap.height() );
	update();
}



void PixmapWidget::paintEvent( QPaintEvent* ev)
{
	QWidget::paintEvent(ev);

	QPainter painter(this);
	if ( m_pixmap.isNull() ) {
		painter.fillRect( ev->rect(), __color );
	}
	else {
		if (__expand_horiz) {
			static int w = 10;
			static int h = m_pixmap.height();

			// central section, scaled
			painter.drawPixmap( QRect(w, 0, width() - w * 2, h), m_pixmap, QRect(10, 0, w, h) );

			// left side
			painter.drawPixmap( QRect(0, 0, w, h), m_pixmap, QRect(0, 0, w, h) );

			// right side
			painter.drawPixmap( QRect(width() - w, 0, w, h), m_pixmap, QRect(m_pixmap.width() - w, 0, w, h) );
		}
		else {
			painter.drawPixmap( ev->rect(), m_pixmap, ev->rect() );
		}
	}
}


