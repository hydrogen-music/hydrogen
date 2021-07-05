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
	setEnabled( false );
	setFocusPolicy( Qt::NoFocus );
	setAlignment( Qt::AlignCenter );
	setLocale( QLocale( QLocale::C, QLocale::AnyCountry ) );

	auto pPref = H2Core::Preferences::get_instance();
	
	// Derive a set of scaling-dependent font sizes on the basis of
	// the default font size determined by Qt itself.
	QFont currentFont = font();
	int nStepSize = 2;

	m_fontPointSizes.resize( 3 );
	switch ( pPref->getFontSize() ) {
    case H2Core::Preferences::FontSize::Small:
		m_fontPointSizes[ 0 ] = currentFont.pointSize();
		break;
    case H2Core::Preferences::FontSize::Large:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - 2 * nStepSize;
		break;
    default:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - nStepSize;
	}
	
	m_fontPointSizes[ 1 ] = m_fontPointSizes[ 0 ] + nStepSize;
	m_fontPointSizes[ 2 ] = m_fontPointSizes[ 0 ] + 2 * nStepSize;
	
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
	
	if ( m_bFixedFont ) {
		return;
	}
	
	auto pPref = H2Core::Preferences::get_instance();

	int nIndex = 1;
	if ( pPref->getFontSize() == H2Core::Preferences::FontSize::Small ) {
		nIndex = 0;
	} else if ( pPref->getFontSize() == H2Core::Preferences::FontSize::Large ) {
		nIndex = 2;
	}

	QFont newFont = font();
	newFont.setFamily( pPref->getLevel3FontFamily() );
	newFont.setPointSize( m_fontPointSizes[ nIndex ] );
	setFont( newFont );
}

void LCDDisplay::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	
	QColor textColor;
	if ( m_bUseRedFont ) {
		textColor = pPref->getDefaultUIStyle()->m_buttonRedColor;
	} else {
		textColor = pPref->getDefaultUIStyle()->m_windowTextColor;
	}

	QString sStyleSheet = QString( "\
QLineEdit { \
    color: %1; \
    background-color: %2;" )
		.arg( textColor.name() )
		.arg( pPref->getDefaultUIStyle()->m_windowColor.name() );

	// For fixed font displays we have to add the current font
	// parameters as well to avoid any inherited changes.
	if ( m_bFixedFont && font().pixelSize() > 0 ) {
		sStyleSheet.append( QString( "\
    font-size: %1px; \
    font-family: %2;" )
							.arg( font().pixelSize() )
							.arg( font().family() ) );
	}

	setStyleSheet( sStyleSheet.append( "}" ) );
}

void LCDDisplay::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateFont();
		updateStyleSheet();
	}
}

void LCDDisplay::paintEvent( QPaintEvent *ev ) {

	QLineEdit::paintEvent( ev );
	updateFont();
}
