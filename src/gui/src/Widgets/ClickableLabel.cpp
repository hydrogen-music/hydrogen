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

#include "ClickableLabel.h"
#include "../HydrogenApp.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Globals.h>

ClickableLabel::ClickableLabel( QWidget *pParent, QSize size, QString sText, Color color  )
	: QLabel( pParent )
	, m_size( size )
	, m_color( color )
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

	setStyleSheet( QString( "color: %1" ).arg( text.name() ) );
}

void ClickableLabel::mousePressEvent( QMouseEvent * e )
{
	UNUSED( e );
	emit labelClicked( this );
}

void ClickableLabel::updateFont( QString sFontFamily, H2Core::Preferences::FontSize fontSize ) {

	int nPixelSize;
	
	if ( ! m_size.isNull() ) {
	
		float fScalingFactor = 1.0;
		switch ( fontSize ) {
		case H2Core::Preferences::FontSize::Small:
			fScalingFactor = 1.0;
			break;
		case H2Core::Preferences::FontSize::Normal:
			fScalingFactor = 0.75;
			break;
		case H2Core::Preferences::FontSize::Large:
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
	
	setFont( font );

	if ( ! m_size.isNull() ) {
		// Check whether the width of the text fits the available frame
		// width of the label
		while ( fontMetrics().size( Qt::TextSingleLine, text() ).width() > width()
				&& nPixelSize > 1 ) {
			nPixelSize--;
			font.setPixelSize( nPixelSize );
			setFont( font );
		}
	}
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
	auto pPref = H2Core::Preferences::get_instance();
	
	QLabel::setText( sNewText );
	updateFont( pPref->getLevel3FontFamily(), pPref->getFontSize() );
}
