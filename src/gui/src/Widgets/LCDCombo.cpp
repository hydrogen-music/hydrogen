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

#include "LCDCombo.h"

#include "../HydrogenApp.h"

#include <core/Globals.h>

const char* LCDCombo::__class_name = "LCDCombo";

LCDCombo::LCDCombo( QWidget *pParent, QSize size )
	: QComboBox( pParent )
	, Object( __class_name )
	, m_size( size )
	, m_bEntered( false )
{
	m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
	m_sLastUsedFontFamily = H2Core::Preferences::get_instance()->getLevel3FontFamily();

	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	setStyleSheet("background-color: #8e94a6;" 
				  "color: #0a0a0a;" );

	updateFont();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDCombo::onPreferencesChanged );
}

LCDCombo::~LCDCombo() {
}

void LCDCombo::updateFont() {
	QFont font( m_sLastUsedFontFamily, getPointSize( m_lastUsedFontSize ) );
	setFont( font );
}

void LCDCombo::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		updateFont();
	}
}

void LCDCombo::paintEvent( QPaintEvent *ev ) {

	QComboBox::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = QColor( 97, 167, 251);

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}
	
		painter.fillRect( 0, m_size.height() - 2, m_size.width(), 2, colorHighlightActive );
	}
}

void LCDCombo::enterEvent( QEvent* ev ) {
	QComboBox::enterEvent( ev );
	m_bEntered = true;
}

void LCDCombo::leaveEvent( QEvent* ev ) {
	QComboBox::leaveEvent( ev );
	m_bEntered = false;
}
