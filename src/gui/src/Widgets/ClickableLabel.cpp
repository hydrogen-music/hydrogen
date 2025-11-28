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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "ClickableLabel.h"
#include "../HydrogenApp.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Globals.h>

ClickableLabel::ClickableLabel( QWidget *pParent, const QSize& size,
								const QString& sText, const DefaultColor& color,
								bool bIsEditable )
	: QLabel( pParent )
	, m_size( size )
	, m_defaultColor( color )
	, m_bIsEditable( bIsEditable )
	, m_bEntered( false )
{
	if ( ! size.isNull() ) {
		setFixedSize( size );
		resize( size );
	}
	
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	updateFont( pFontTheme->m_sLevel3FontFamily, pFontTheme->m_fontSize );
	updateStyleSheet();

	setAlignment( Qt::AlignCenter );
	setText( sText );
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &ClickableLabel::onPreferencesChanged );

}

void ClickableLabel::updateStyleSheet() {

	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();

	QColor text;
	if ( m_defaultColor == DefaultColor::Bright ) {
		text = pColorTheme->m_windowTextColor;
	} else {
		text = pColorTheme->m_widgetTextColor;
	}

	setStyleSheet( QString( "QLabel { color: %1; }" ).arg( text.name() ) );
}

void ClickableLabel::mousePressEvent( QMouseEvent* pEvent )
{
	// Allow to use the event in the parent widget.
	pEvent->ignore();

	emit labelClicked( this );
}

void ClickableLabel::mouseDoubleClickEvent( QMouseEvent* pEvent ) {
	emit labelDoubleClicked( pEvent );

	QLabel::mouseDoubleClickEvent( pEvent );
}


void ClickableLabel::paintEvent( QPaintEvent *ev ) {

	QLabel::paintEvent( ev );

	if ( ! m_bIsEditable ) {
		return;
	}

	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);

		QColor colorHighlightActive;
		if ( isEnabled() ) {
			colorHighlightActive = pColorTheme->m_highlightColor;
		} else {
			colorHighlightActive = pColorTheme->m_lightColor;
		}

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}

		int nWidth, nHeight;
		if ( ! m_size.isNull() ) {
			nWidth = m_size.width();
			nHeight = m_size.height();
		}
		else {
			nWidth = width();
			nHeight = height();
		}

		QPen pen;
		pen.setColor( colorHighlightActive );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 1, 1, nWidth - 2, nHeight - 2 ), 3, 3 );
	}
}

#ifdef H2CORE_HAVE_QT6
void ClickableLabel::enterEvent( QEnterEvent *ev ) {
#else
void ClickableLabel::enterEvent( QEvent *ev ) {
#endif
	QLabel::enterEvent( ev );
	if ( m_bIsEditable ) {
		m_bEntered = true;
		update();
	}
}

void ClickableLabel::leaveEvent( QEvent* ev ) {
	QLabel::leaveEvent( ev );
	if ( m_bIsEditable ) {
		m_bEntered = false;
		update();
	}
}

void ClickableLabel::updateFont( const QString& sFontFamily,
								 const H2Core::FontTheme::FontSize& fontSize ) {

	int nPixelSize = 0;
	
	if ( ! m_size.isNull() ) {
	
		float fScalingFactor = 1.0;
		switch ( fontSize ) {
		case H2Core::FontTheme::FontSize::Small:
			fScalingFactor = 1.0;
			break;
		case H2Core::FontTheme::FontSize::Medium:
			fScalingFactor = 0.75;
			break;
		case H2Core::FontTheme::FontSize::Large:
			fScalingFactor = 0.5;
			break;
		}

		int nMargin;
		if ( m_size.height() <= 9 ) {
			nMargin = 1;
		} else if ( m_size.height() <= 16 ) {
			nMargin = 2;
		} else {
			nMargin = 8;
		}

		nPixelSize = m_size.height() - std::round( fScalingFactor * nMargin );
	}

	QFont font( sFontFamily );

	if ( ! m_size.isNull() ) {
		font.setPixelSize( nPixelSize );
	}
	font.setBold( true );

	if ( ! m_size.isNull() || width() > height() ) {
		// Check whether the width of the text fits the available frame
		// width of the label
		while ( QFontMetrics( font ).size( Qt::TextSingleLine, text() ).width() >
				width() && nPixelSize > 1 ) {
			nPixelSize--;
			font.setPixelSize( nPixelSize );
		}
	}

	// This method must not be called more than once in this routine. Otherwise,
	// a repaint of the widget is triggered, which calls `updateFont()` again
	// and we are trapped in an infinite loop.
	setFont( font );
}

void ClickableLabel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateFont( pFontTheme->m_sLevel3FontFamily, pFontTheme->m_fontSize );
		updateStyleSheet();
	}
}

void ClickableLabel::setText( const QString& sNewText ) {
	if ( text() == sNewText ) {
		return;
	}

	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	QLabel::setText( sNewText );
	updateFont( pFontTheme->m_sLevel3FontFamily, pFontTheme->m_fontSize );
}
