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

#include "LCDSpinBox.h"
#include "../Skin.h"
#include <core/Globals.h>

const char* LCDSpinBox::__class_name = "LCDSpinBox";

// used in PlayerControl
LCDSpinBox::LCDSpinBox( QWidget *pParent, QSize size, Type type, double fMin, double fMax )
 : QDoubleSpinBox( pParent )
 , Object( __class_name )
 , m_size( size )
 , m_type( type )
 , m_bEntered( false )
{
	setFocusPolicy( Qt::ClickFocus );
	
	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	setMaximum( fMax );
	setMinimum( fMin );
	setValue( fMin );
}

LCDSpinBox::~LCDSpinBox() {
}

QString LCDSpinBox::textFromValue( double fValue ) const {
	QString result;
	if ( fValue == -1.0 ) {
		result = "off";
	} else {
		if ( m_type == Type::Int ) {
			result = QString( "%1" ).arg( fValue, 0, 'f', 0 );
		} else {
			result = QString( "%1" ).arg( fValue, 0, 'f', 2 );
		}
	}

	return result;
}

double LCDSpinBox::valueFromText( const QString& sText ) const {

	double fResult;
	
	if ( sText == "off" ){
		fResult = -1.0;
	} else {
		fResult = QDoubleSpinBox::valueFromText( sText );
	}

	return fResult;
}

void LCDSpinBox::paintEvent( QPaintEvent *ev ) {

	QDoubleSpinBox::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = Skin::getHighlightColor();

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}
	
		painter.fillRect( 0, m_size.height() - 2, m_size.width(), 2, colorHighlightActive );
	}
}

void LCDSpinBox::enterEvent( QEvent* ev ) {
	QDoubleSpinBox::enterEvent( ev );
	m_bEntered = true;
}

void LCDSpinBox::leaveEvent( QEvent* ev ) {
	QDoubleSpinBox::leaveEvent( ev );
	m_bEntered = false;
}
