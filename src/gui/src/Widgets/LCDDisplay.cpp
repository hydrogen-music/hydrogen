/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "LCDDisplay.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Globals.h>
#include <core/Preferences/Theme.h>

LCDDisplay::LCDDisplay( QWidget * pParent, const QSize& size, bool bFixedFont,
						bool bIsActive )
 : QLineEdit( pParent )
 , m_size( size )
 , m_bEntered( false )
 , m_bFixedFont( bFixedFont )
 , m_bUseRedFont( false )
 , m_bIsActive( bIsActive )
{
	setReadOnly( ! bIsActive );
	setEnabled( bIsActive );
	if ( ! bIsActive ) {
		setFocusPolicy( Qt::NoFocus );
	}
	setAlignment( Qt::AlignCenter );
	setLocale( QLocale( QLocale::C, QLocale::AnyCountry ) );

	// Derive a set of scaling-dependent font sizes on the basis of
	// the default font size determined by Qt itself.
	QFont currentFont = font();
	int nStepSize = 2;

	m_fontPointSizes.resize( 3 );
	switch ( H2Core::Preferences::get_instance()->getTheme().m_font.m_fontSize ) {
	case H2Core::FontTheme::FontSize::Small:
		m_fontPointSizes[ 0 ] = currentFont.pointSize();
		break;
	case H2Core::FontTheme::FontSize::Large:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - 2 * nStepSize;
		break;
	default:
		m_fontPointSizes[ 0 ] = currentFont.pointSize() - nStepSize;
	}
	
	m_fontPointSizes[ 1 ] = m_fontPointSizes[ 0 ] + nStepSize;
	m_fontPointSizes[ 2 ] = m_fontPointSizes[ 0 ] + 2 * nStepSize;
	
	if ( ! size.isNull() && ! size.isEmpty() ) {
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
	if ( bUseRedFont != m_bUseRedFont ) {
		m_bUseRedFont = bUseRedFont;
		updateStyleSheet();
	}
}

void LCDDisplay::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;;

	setReadOnly( ! bIsActive );
	setEnabled( bIsActive );
	
	if ( ! bIsActive ) {
		setFocusPolicy( Qt::NoFocus );
	}
	else {
		setFocusPolicy( Qt::StrongFocus );
	}

	update();
}

void LCDDisplay::updateFont() {
	
	if ( m_bFixedFont ) {
		return;
	}

	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	int nIndex = 1;
	if ( theme.m_font.m_fontSize == H2Core::FontTheme::FontSize::Small ) {
		nIndex = 0;
	} else if ( theme.m_font.m_fontSize == H2Core::FontTheme::FontSize::Large ) {
		nIndex = 2;
	}

	QFont newFont = font();
	newFont.setFamily( theme.m_font.m_sLevel3FontFamily );
	newFont.setPointSize( m_fontPointSizes[ nIndex ] );
	setFont( newFont );
}

void LCDDisplay::updateStyleSheet() {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QColor textColor, textColorActive;
	if ( m_bUseRedFont ) {
		textColor = theme.m_color.m_buttonRedColor;
		textColorActive = theme.m_color.m_buttonRedColor;
	} else {
		textColor = theme.m_color.m_windowTextColor;
		textColorActive = theme.m_color.m_widgetTextColor;
	}
	QColor backgroundColor = theme.m_color.m_windowColor;

	QColor backgroundColorActive = theme.m_color.m_widgetColor;

	QString sStyleSheet = QString( "\
QLineEdit:enabled { \
    color: %1; \
    background-color: %2; \
} \
QLineEdit:disabled { \
    color: %3; \
    background-color: %4; \
}" )
		.arg( textColorActive.name() )
		.arg( backgroundColorActive.name() )
		.arg( textColor.name() )
		.arg( backgroundColor.name() );

	// For fixed font displays we have to add the current font
	// parameters as well to avoid any inherited changes.
	if ( m_bFixedFont && font().pixelSize() > 0 ) {
		sStyleSheet.append( QString( "\
QLineEdit { \
    font-size: %1px; \
    font-family: %2; \
}" )
							.arg( font().pixelSize() )
							.arg( font().family() ) );
	}

	setStyleSheet( sStyleSheet );
}

void LCDDisplay::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateFont();
		updateStyleSheet();
	}
}

void LCDDisplay::paintEvent( QPaintEvent *ev ) {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QLineEdit::paintEvent( ev );
	updateFont();

	// Hovering highlights
	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);

		QColor colorHighlightActive;
		if ( m_bIsActive ) {
			colorHighlightActive = theme.m_color.m_highlightColor;
		} else {
			colorHighlightActive = theme.m_color.m_lightColor;
		}

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}

		QPen pen;
		pen.setColor( colorHighlightActive );
		pen.setWidth( 3 );
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 0, 0, width() - 1, height() - 1 ), 3, 3 );
	}
}

#ifdef H2CORE_HAVE_QT6
void LCDDisplay::enterEvent( QEnterEvent *ev ) {
#else
void LCDDisplay::enterEvent( QEvent *ev ) {
#endif
	QLineEdit::enterEvent( ev );
	m_bEntered = true;
	update();
}

void LCDDisplay::leaveEvent( QEvent* ev ) {
	QLineEdit::leaveEvent( ev );
	m_bEntered = false;
	update();
}
