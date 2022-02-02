/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Globals.h>


LCDCombo::LCDCombo( QWidget *pParent, QSize size, bool bModifyOnChange )
	: QComboBox( pParent )
	, m_size( size )
	, m_bEntered( false )
	, m_bModifyOnChange( bModifyOnChange )
{
	setFocusPolicy( Qt::ClickFocus );

	if ( ! size.isNull() ) {
		adjustSize();
		setFixedSize( size );
	}

	updateStyleSheet();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LCDCombo::onPreferencesChanged );
	connect( this, SIGNAL( currentIndexChanged(int) ), this,
			 SLOT( onCurrentIndexChanged(int) ) );
}

LCDCombo::~LCDCombo() {
}

void LCDCombo::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	
	setStyleSheet( QString( "\
QComboBox { \
    background-color: %1; \
    font-family: %2; \
    font-size: %3; \
} \
QComboBox QAbstractItemView { \
    background-color: #babfcf; \
}")
				   .arg( pPref->getColorTheme()->m_widgetColor.name() )
				   .arg( pPref->getLevel3FontFamily() )
				   .arg( getPointSize( pPref->getFontSize() ) ) );
}

void LCDCombo::onPreferencesChanged( H2Core::Preferences::Changes changes ) {

	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		updateStyleSheet();
	}
}

void LCDCombo::paintEvent( QPaintEvent *ev ) {

	auto pPref = H2Core::Preferences::get_instance();
	
	QComboBox::paintEvent( ev );

	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = pPref->getColorTheme()->m_highlightColor;

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

void LCDCombo::onCurrentIndexChanged( int ) {
	if ( m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void LCDCombo::setSize( QSize size ) {
	m_size = size;
	
	setFixedSize( size );
	adjustSize();
}
