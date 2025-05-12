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

#include "LCDCombo.h"

#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Globals.h>


LCDCombo::LCDCombo( QWidget *pParent, const QSize& size, bool bModifyOnChange )
	: QComboBox( pParent )
	, m_size( size )
	, m_bEntered( false )
	, m_bModifyOnChange( bModifyOnChange )
	, m_nMaxWidth( 0 )
	, m_bIsActive( true )
{
	setFocusPolicy( Qt::NoFocus );

	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	updateStyleSheet();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDCombo::onPreferencesChanged );
	// Mark the current song modified if there was an user interaction
	// with the widget.
	connect( this, SIGNAL( activated(int) ), this, SLOT( handleIsModified(int) ) );
}

LCDCombo::~LCDCombo() {
}

void LCDCombo::handleIsModified( int ) {
	if ( m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDCombo::addItem( const QString& sText, const QVariant& userData ) {
	int nWidth =
		fontMetrics().size( Qt::TextSingleLine, sText ).width() *
		1.1 + // custom factor to ensure the text does fit
		view()->autoScrollMargin(); // width of the scrollbar

	if ( nWidth > m_nMaxWidth ) {
		m_nMaxWidth = nWidth;
	}

	QComboBox::addItem( sText, userData );
}

void LCDCombo::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	
	update();
	
	setEnabled( bIsActive );
}

void LCDCombo::showPopup() {
	if ( m_nMaxWidth > view()->sizeHint().width() ) {
		view()->setMinimumWidth( m_nMaxWidth );
	}
	
	QComboBox::showPopup();
}

void LCDCombo::updateStyleSheet() {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QColor widgetColor = theme.m_color.m_widgetColor;
	QColor widgetTextColor = theme.m_color.m_widgetTextColor;
	QColor widgetInactiveColor = 
		Skin::makeWidgetColorInactive( widgetColor );
	QColor widgetTextInactiveColor =
		Skin::makeTextColorInactive( widgetTextColor );
	
	setStyleSheet( QString( "\
QComboBox:enabled { \
    color: %1; \
    background-color: %2; \
    font-family: %3; \
    font-size: %4; \
} \
QComboBox:disabled { \
    color: %5; \
    background-color: %6; \
    font-family: %3; \
    font-size: %4; \
} \
QComboBox QAbstractItemView { \
    color: %1; \
    background-color: #babfcf; \
}")
				   .arg( widgetTextColor.name() )
				   .arg( widgetColor.name() )
				   .arg( theme.m_font.m_sLevel3FontFamily )
				   .arg( getPointSize( theme.m_font.m_fontSize ) )
				   .arg( widgetTextInactiveColor.name() )
				   .arg( widgetInactiveColor.name() ) );
}

void LCDCombo::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateStyleSheet();
	}
}

void LCDCombo::paintEvent( QPaintEvent *ev ) {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	QComboBox::paintEvent( ev );

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
		painter.drawRoundedRect( 0, 0, width() - 1, height() - 1, 3,3 );
	}
}

#ifdef H2CORE_HAVE_QT6
void LCDCombo::enterEvent( QEnterEvent *ev ) {
#else
void LCDCombo::enterEvent( QEvent *ev ) {
#endif
	QComboBox::enterEvent( ev );
	m_bEntered = true;
}

void LCDCombo::leaveEvent( QEvent* ev ) {
	QComboBox::leaveEvent( ev );
	m_bEntered = false;
}

void LCDCombo::setSize( const QSize& size ) {
	m_size = size;
	
	setFixedSize( size );
	adjustSize();
}
