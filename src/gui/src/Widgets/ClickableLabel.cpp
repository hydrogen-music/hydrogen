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

ClickableLabel::ClickableLabel( QWidget *pParent, QSize size, QString sText  )
	: QLabel( pParent )
	, Object( __class_name )
	, m_size( size )
{
	auto pPref = H2Core::Preferences::get_instance();
	
	m_lastUsedFontSize = pPref->getFontSize();
	m_sLastUsedFontFamily = pPref->getApplicationFontFamily();
	updateFont( m_sLastUsedFontFamily, m_lastUsedFontSize );
	
	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	defaultPalette.setColor( QPalette::WindowText, QColor( 230, 230, 230 ) );
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
	if ( m_size.height() <= 10 ) {
		nMargin = 2;
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
	
	if ( m_sLastUsedFontFamily != pPref->getApplicationFontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getApplicationFontFamily();
		
		updateFont( m_sLastUsedFontFamily, m_lastUsedFontSize );
	}
}
