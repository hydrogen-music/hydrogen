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

ClickableLabel::ClickableLabel( QWidget *pParent, QSize size, QString sText, Color color, bool bIsEditable )
	: QLabel( pParent )
	, m_size( size )
	, m_color( color )
	, m_bIsEditable( bIsEditable )
	, m_bEntered( false )
{
	if ( ! size.isNull() ) {
		setFixedSize( size );
		resize( size );
	}
	
	auto pPref = H2Core::Preferences::get_instance();

	updateFont( pPref->getLevel3FontFamily(), pPref->getFontSize() );
	updateStyleSheet();

	setAlignment( Qt::AlignCenter );
	setText( sText );
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &ClickableLabel::onPreferencesChanged );

}

void ClickableLabel::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	
	QColor text;
	if ( m_color == Color::Bright ) {
		text = pPref->getColorTheme()->m_windowTextColor;
	} else {
		text = pPref->getColorTheme()->m_widgetTextColor;
	}

	setStyleSheet( QString( "QLabel { color: %1; }" ).arg( text.name() ) );
}

void ClickableLabel::mousePressEvent( QMouseEvent * e )
{
	UNUSED( e );
	emit labelClicked( this );
}

void ClickableLabel::paintEvent( QPaintEvent *ev ) {

	QLabel::paintEvent( ev );

	if ( ! m_bIsEditable ) {
		return;
	}

	auto pPref = H2Core::Preferences::get_instance();

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);

		QColor colorHighlightActive = pPref->getColorTheme()->m_highlightColor;

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}

		QPen pen;
		pen.setColor( colorHighlightActive );
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.drawRoundedRect( QRect( 1, 1, m_size.width() - 2, m_size.height() - 2 ), 3, 3 );
	}
}

void ClickableLabel::enterEvent( QEvent* ev ) {
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

void ClickableLabel::updateFont( QString sFontFamily, H2Core::FontTheme::FontSize fontSize ) {

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

void ClickableLabel::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateFont( pPref->getLevel3FontFamily(), pPref->getFontSize() );
		updateStyleSheet();
	}
}

void ClickableLabel::setText( const QString& sNewText ) {
	if ( text() == sNewText ) {
		return;
	}
	
	auto pPref = H2Core::Preferences::get_instance();
	
	QLabel::setText( sNewText );
	updateFont( pPref->getLevel3FontFamily(), pPref->getFontSize() );
}
