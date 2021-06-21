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

#include "Button.h"

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#include <core/Globals.h>
#include <core/Preferences.h>
#include <core/Hydrogen.h>

const char* Button::__class_name = "Button";

Button::Button( QWidget *pParent, QSize size, Type type, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, QString sBaseTooltip, bool bColorful )
	: QPushButton( pParent )
	, Object( __class_name )
	, m_size( size )
	, m_iconSize( iconSize )
	, m_sBaseTooltip( sBaseTooltip )
	, m_sRegisteredMidiEvent( "" )
	, m_nRegisteredMidiParameter( 0 )
	, m_bColorful( bColorful )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
{
	m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
	m_sLastUsedFontFamily = H2Core::Preferences::get_instance()->getLevel3FontFamily();
	
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFocusPolicy( Qt::NoFocus );
	
	adjustSize();
	setFixedSize( size );
	resize( size );

	if ( ! sIcon.isEmpty() ) {
		if ( bColorful ) {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/" + sIcon ) );
		} else {
			// Unchecked version
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + sIcon ) );
		}
		setIconSize( iconSize );
	} else {
		setText( sText );
	}

	updateFont();

	QString sBackgroundCheckedLight, sBackgroundCheckedDark,
		sBackgroundCheckedHoverLight, sBackgroundCheckedHoverDark, sColorChecked;
	if ( bUseRedBackground ) {
		sBackgroundCheckedLight = "#ffb1b1";
		sBackgroundCheckedDark = "#ff6767";
		sBackgroundCheckedHoverLight = "#ffc0c0";
		sBackgroundCheckedHoverDark = "#ff7676";
		sColorChecked = "#0a0a0a";
	} else {
		sBackgroundCheckedLight = "#5276a2";
		sBackgroundCheckedDark = "#3b5574";
		sBackgroundCheckedHoverLight = "#5a81b1";
		sBackgroundCheckedHoverDark = "#436083";
		sColorChecked = "#ffffff";
	}

	QString sRadius;
	if ( size.width() <= 12 || size.height() <= 12 ) {
		sRadius = "0";
	} else if ( size.width() <= 20 || size.height() <= 20 ) {
		sRadius = "3";
	} else {
		sRadius = "5";
	}

	setStyleSheet( QString( "QPushButton { \
    color: #0a0a0a; \
    border: 1px solid #0a0a0a; \
    border-radius: %1px; \
    padding: 0px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #dae0f2, stop: 1 #9298aa); \
} \
QPushButton:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #e8eeff, stop: 1 #9fa6b9); \
} \
QPushButton:checked { \
    color: %2; \
    border: 1px solid #0a0a0a; \
    border-radius: %1px; \
    padding: 0px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %3, stop: 1 %4); \
} \
QPushButton:checked:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %5, stop: 1 %6); \
}"
							).arg( sRadius ).arg( sColorChecked )
				   .arg( sBackgroundCheckedLight ).arg( sBackgroundCheckedDark )
				   .arg( sBackgroundCheckedHoverLight ).arg( sBackgroundCheckedHoverDark ) );
	
	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}
	updateTooltip();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Button::onPreferencesChanged );
}

Button::~Button() {
}

void Button::setBaseToolTip( const QString& sNewTip ) {
	m_sBaseTooltip = sNewTip;
	updateTooltip();
}

void Button::setAction( Action *pAction ) {
	m_action = pAction;
	updateTooltip();
}

void Button::mousePressEvent(QMouseEvent*ev) {
	
	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();

		// Store the registered MIDI event and parameter in order to
		// show them in the tooltip. Looking them up in the MidiMap
		// using the Action associated to the Widget might not yield a
		// unique result since the Action can be registered from the
		// PreferencesDialog as well.
		m_sRegisteredMidiEvent = H2Core::Hydrogen::get_instance()->lastMidiEvent;
		m_nRegisteredMidiParameter = H2Core::Hydrogen::get_instance()->lastMidiEventParameter;
		
		updateTooltip();
		return;
	}

	QPushButton::mousePressEvent( ev );
}

void Button::updateTooltip() {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QString sTip = QString("%1" ).arg( m_sBaseTooltip );

	// Add the associated MIDI action.
	if ( m_action != nullptr ) {
		sTip.append( QString( "\n%1: %2 " ).arg( pCommonStrings->getMidiTooltipHeading() )
					 .arg( m_action->getType() ) );
		if ( ! m_sRegisteredMidiEvent.isEmpty() ) {
			sTip.append( QString( "%1 [%2 : %3]" ).arg( pCommonStrings->getMidiTooltipBound() )
						 .arg( m_sRegisteredMidiEvent ).arg( m_nRegisteredMidiParameter ) );
		} else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiTooltipUnbound() ) );
		}
	}
			
	setToolTip( sTip );
}

void Button::updateFont() {
	
	float fScalingFactor = 1.0;
    switch ( m_lastUsedFontSize ) {
    case H2Core::Preferences::FontSize::Small:
		fScalingFactor = 1.2;
		break;
    case H2Core::Preferences::FontSize::Normal:
		fScalingFactor = 1.0;
		break;
    case H2Core::Preferences::FontSize::Large:
		fScalingFactor = 0.75;
		break;
	}

	int nMargin, nPixelSize;
	if ( m_size.width() <= 12 || m_size.height() <= 12 ) {
		nMargin = 1;
	} else if ( m_size.width() <= 19 || m_size.height() <= 19 ) {
		nMargin = 5;
	} else if ( m_size.width() <= 22 || m_size.height() <= 22 ) {
		nMargin = 7;
	} else {
		nMargin = 9;
	}
	
	if ( m_size.width() >= m_size.height() ) {
		nPixelSize = m_size.height() - std::round( fScalingFactor * nMargin );
	} else {
		nPixelSize = m_size.width() - std::round( fScalingFactor * nMargin );
	}

	QFont font( m_sLastUsedFontFamily );
	font.setPixelSize( nPixelSize );
	setFont( font );
}

void Button::paintEvent( QPaintEvent* ev )
{
	QPushButton::paintEvent( ev );

	updateFont();

	if ( ! m_sIcon.isEmpty() && !m_bColorful && isChecked() != m_bLastCheckedState ) {
		if ( isChecked() ) {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/white/" + m_sIcon ) );
		} else {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + m_sIcon ) );
		}

		m_bLastCheckedState = isChecked();
	}
}

void Button::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		updateFont();
	}
}
