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

#include "ColorSelectionButton.h"

#include <QColorDialog>
#include <core/Globals.h>

const char* ColorSelectionButton::__class_name = "ColorSelectionButton";

ColorSelectionButton::ColorSelectionButton( QWidget* pParent, H2Core::H2RGBColor sInitialColor, int nSize )
 : QPushButton( pParent )
 , Object( __class_name )
 , m_sColor( sInitialColor )
 , m_bMouseOver( false )
{
	setFlat( true );
	QSize size( nSize, nSize );
	setFixedSize( size );
	resize( size );
}

ColorSelectionButton::~ColorSelectionButton() {
}

void ColorSelectionButton::mousePressEvent(QMouseEvent*ev) {

	QColor newColor = QColorDialog::getColor( QColor( m_sColor.toStringFmt() ), this, tr( "Pick a pattern color" ) );

	int r, g, b;
	newColor.getRgb( &r, &g, &b );
	H2Core::H2RGBColor sNewColor( r, g, b );

	if ( m_sColor.toStringFmt() != sNewColor.toStringFmt() ) {
		m_sColor = sNewColor;
		update();
		emit clicked();
	}
}

void ColorSelectionButton::enterEvent(QEvent *ev) {
	UNUSED( ev );
	m_bMouseOver = true;
	update();
}



void ColorSelectionButton::leaveEvent(QEvent *ev) {
	UNUSED( ev );
	m_bMouseOver = false;
	update();
}

void ColorSelectionButton::paintEvent( QPaintEvent* ev) {
	QPainter painter(this);
	QColor color( m_sColor.toStringFmt() );
	if ( m_bMouseOver ) {
		color.setAlpha( 0.2 );
	}
	
	painter.setPen( QColor( "#000" ) );
	painter.drawRect( 0, 0, width(), height() );
	painter.setPen( color );
	painter.fillRect( 4, 4, width() - 8, height() - 8, color );
}
