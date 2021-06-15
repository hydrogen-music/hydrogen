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

#include "LCDDisplay.h"
#include "../HydrogenApp.h"

#include <core/Globals.h>

const char* LCDDisplay::__class_name = "LCDDisplay";

LCDDisplay::LCDDisplay( QWidget * pParent, QSize size )
 : QLineEdit( pParent )
 , Object( __class_name )
 , m_size( size )
 , m_bEntered( false )
{
	setReadOnly( true );
	setFocusPolicy( Qt::NoFocus );
	setAlignment( Qt::AlignCenter );
	
	m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
	m_sLastUsedFontFamily = H2Core::Preferences::get_instance()->getLevel3FontFamily();

	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	updateFont();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDDisplay::onPreferencesChanged );

	// QPalette defaultPalette;
	// defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	// this->setPalette( defaultPalette );

}

LCDDisplay::~LCDDisplay() {
}

void LCDDisplay::setDefaultFont() {
}

void LCDDisplay::setRedFont() {
}

// void LCDDisplay::setText( float fValue ) {
// {
// 	QLineEdit::setText( QString( "%1" ).arg( fValue, 0, 'f', 2 ) );
// }

void LCDDisplay::updateFont() {
	QFont font( m_sLastUsedFontFamily, getPointSize( m_lastUsedFontSize ) );
	// setFont( font );
}

void LCDDisplay::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		updateFont();
	}
}

void LCDDisplay::paintEvent( QPaintEvent *ev ) {

	QLineEdit::paintEvent( ev );

	if ( m_bEntered ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = QColor( 97, 167, 251);
		colorHighlightActive.setAlpha( 150 );
	
		painter.fillRect( 0, m_size.height() - 2, m_size.width(), 2, colorHighlightActive );
	}
}

void LCDDisplay::enterEvent( QEvent* ev ) {
	QLineEdit::enterEvent( ev );
	m_bEntered = true;
	update();
}

void LCDDisplay::leaveEvent( QEvent* ev ) {
	QLineEdit::leaveEvent( ev );
	m_bEntered = false;
	update();
}
