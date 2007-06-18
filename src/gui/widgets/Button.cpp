/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Button.cpp,v 1.11 2005/05/09 18:12:12 comix Exp $
 *
 */

#include <iostream>
#include <qpainter.h>

#include "Button.h"


Button::Button(QWidget * parent, QSize size, string onImage, string offImage, string overImage)
 : QWidget(parent, 0)
 , Object( "Button" )
 , m_bPressed( false )
 , m_bMouseOver( false )
{
	setMinimumSize(size.width(), size.height());
	setMaximumSize(size.width(), size.height());
	resize(size.width(), size.height());

	bool ok = m_onPixmap.load( onImage.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" + onImage );
		m_onPixmap.resize( width(), height() );
		m_onPixmap.fill( QColor( 0, 255, 0 ) );
	}

	ok = m_offPixmap.load( offImage.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" + offImage );
		m_offPixmap.resize( width(), height() );
		m_offPixmap.fill( QColor( 0, 100, 0 ) );
	}

	ok = m_overPixmap.load( overImage.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" + overImage );
		m_overPixmap.resize( width(), height() );
		m_overPixmap.fill( QColor( 0, 180, 0 ) );
	}
	setPaletteBackgroundPixmap( m_offPixmap );
}



Button::~Button() {
}



void Button::mousePressEvent(QMouseEvent*) {
	m_bPressed = true;
	drawButton();

	emit mousePress(this);
}



void Button::mouseReleaseEvent(QMouseEvent* ev)
{
	m_bPressed = false;
	drawButton();

	if (ev->button() == LeftButton) {
		emit clicked(this);
	}
	else if (ev->button() == RightButton) {
		emit rightClicked(this);
	}

}



void Button::setPressed(bool pressed)
{
	if (pressed != m_bPressed) {
		m_bPressed = pressed;
		drawButton();
	}
}



void Button::drawButton() {
	if (m_bPressed) {
		setPaletteBackgroundPixmap(m_onPixmap);
	}
	else {
		if (m_bMouseOver) {
			setPaletteBackgroundPixmap(m_overPixmap);
		}
		else {
			setPaletteBackgroundPixmap(m_offPixmap);
		}
	}
}



void Button::enterEvent(QEvent *ev) {
	m_bMouseOver = true;
	drawButton();
}



void Button::leaveEvent(QEvent *ev) {
	m_bMouseOver = false;
	drawButton();
}


// :::::::::::::::::::::::::



ToggleButton::ToggleButton(QWidget * parent, QSize size, string onImg, string offImg, string overImg)
 : Button( parent, size, onImg, offImg, overImg )
{

}



ToggleButton::~ToggleButton() {
}



void ToggleButton::mousePressEvent(QMouseEvent*) {
	if (m_bPressed) {
		m_bPressed = false;
		setPaletteBackgroundPixmap(m_offPixmap);
	}
	else {
		m_bPressed = true;
		setPaletteBackgroundPixmap(m_onPixmap);
	}

	emit clicked(this);
}



void ToggleButton::mouseReleaseEvent(QMouseEvent*) {
	// do nothing, this method MUST override Button's one
}



