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

LCDDisplay::LCDDisplay( QWidget * pParent, QSize size, bool bFixedFont )
 : QLineEdit( pParent )
 , Object( __class_name )
 , m_size( size )
 , m_bFixedFont( bFixedFont )
 , m_bUseRedFont( false )
{
	setReadOnly( true );
	setFocusPolicy( Qt::NoFocus );
	setAlignment( Qt::AlignCenter );
	
	auto pPref = H2Core::Preferences::get_instance();
	
	m_lastUsedFontSize = pPref->getFontSize();
	m_sLastUsedFontFamily = pPref->getLevel3FontFamily();

	m_lastWindowColor = pPref->getDefaultUIStyle()->m_windowColor;
	m_lastButtonRedColor = pPref->getDefaultUIStyle()->m_buttonRedColor;
	m_lastWindowTextColor = pPref->getDefaultUIStyle()->m_windowTextColor;

	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	updateFont();
	updateStyleSheet();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDDisplay::onPreferencesChanged );
}

LCDDisplay::~LCDDisplay() {
}

void LCDDisplay::setUseRedFont( bool bUseRedFont ) {
	m_bUseRedFont = bUseRedFont;
	updateStyleSheet();
}

void LCDDisplay::updateFont() {
	if ( ! m_bFixedFont ) {
		QFont font( m_sLastUsedFontFamily, getPointSize( m_lastUsedFontSize ) );
		setFont( font );
	}
}

void LCDDisplay::updateStyleSheet() {

	QColor textColor;
	if ( m_bUseRedFont ) {
		textColor = m_lastButtonRedColor;
	} else {
		textColor = m_lastWindowTextColor;
	}

	setStyleSheet( QString( "\
QLineEdit { \
    color: %1; \
    background-color: %2; \
}" )
				   .arg( textColor.name() ).arg( m_lastWindowColor.name() ) );
}

void LCDDisplay::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		updateFont();
	}

	if ( m_lastWindowColor != pPref->getDefaultUIStyle()->m_windowColor ||
		 m_lastButtonRedColor != pPref->getDefaultUIStyle()->m_buttonRedColor ||
		 m_lastWindowTextColor != pPref->getDefaultUIStyle()->m_windowTextColor ) {
	
		m_lastWindowColor = pPref->getDefaultUIStyle()->m_windowColor;
		m_lastButtonRedColor = pPref->getDefaultUIStyle()->m_buttonRedColor;
		m_lastWindowTextColor = pPref->getDefaultUIStyle()->m_windowTextColor;

		updateStyleSheet();
	}
}

void LCDDisplay::paintEvent( QPaintEvent *ev ) {

	QLineEdit::paintEvent( ev );
	updateFont();
}
