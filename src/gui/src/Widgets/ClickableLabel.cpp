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

#include "ClickableLabel.h"
#include "../HydrogenApp.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Globals.h>

const char* ClickableLabel::__class_name = "ClickableLabel";

ClickableLabel::ClickableLabel( QWidget *pParent, QSize size, QString sText, Color color  )
	: QLabel( pParent )
	, Object( __class_name )
	, m_size( size )
	, m_color( color )
{
	auto pPref = H2Core::Preferences::get_instance();
	
	m_lastUsedFontSize = pPref->getFontSize();
	m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
	updateFont( m_sLastUsedFontFamily, m_lastUsedFontSize );

	QPalette defaultPalette;
	if ( color == Color::Bright ) {
		defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
		defaultPalette.setColor( QPalette::WindowText, QColor( 230, 230, 230 ) );
	} else if ( color == Color::LCD ) {
		defaultPalette.setColor( QPalette::Window, QColor( 50, 50, 50 ) );
		defaultPalette.setColor( QPalette::WindowText, QColor( 154, 195, 246 ) );
	} else {
		defaultPalette.setColor( QPalette::Window, QColor( 230, 230, 230 ) );
		defaultPalette.setColor( QPalette::WindowText, QColor( 25, 25, 25 ) );
	}
	
	this->setPalette( defaultPalette );

	this->setAlignment( Qt::AlignCenter );
		
	setText( sText );

	setFixedSize( size );
	resize( size );
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &ClickableLabel::onPreferencesChanged );

}


void ClickableLabel::mousePressEvent( QMouseEvent * e )
{
	UNUSED( e );
	emit labelClicked( this );
}

void ClickableLabel::updateFont( QString sFontFamily, H2Core::Preferences::FontSize fontSize ) {
		
	float fScalingFactor = 1.0;
    switch ( fontSize ) {
    case H2Core::Preferences::FontSize::Small:
		fScalingFactor = 1.5;
		break;
    case H2Core::Preferences::FontSize::Normal:
		fScalingFactor = 1.0;
		break;
    case H2Core::Preferences::FontSize::Large:
		fScalingFactor = 0.5;
		break;
	}

	int nMargin, nPixelSize;
	if ( m_size.height() <= 9 ) {
		nMargin = 2;
	} else if ( m_size.height() <= 12 ) {
		nMargin = 3;
	} else {
		nMargin = 8;
	}

	nPixelSize = m_size.height() - std::round( fScalingFactor * nMargin );

	QFont font( sFontFamily );
	font.setPixelSize( nPixelSize );
	font.setBold( true );
	
	setFont( font );

}

void ClickableLabel::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		
		updateFont( m_sLastUsedFontFamily, m_lastUsedFontSize );
	}
}
