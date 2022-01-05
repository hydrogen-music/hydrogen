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

#include "Button.h"

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#include <core/Globals.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>
#include <core/Hydrogen.h>

Button::Button( QWidget *pParent, QSize size, Type type, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, QString sBaseTooltip, bool bColorful )
	: QPushButton( pParent )
	, m_size( size )
	, m_iconSize( iconSize )
	, m_sBaseTooltip( sBaseTooltip )
	, m_sRegisteredMidiEvent( "" )
	, m_nRegisteredMidiParameter( 0 )
	, m_bColorful( bColorful )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
	, m_bIsActive( true )
	, m_bUseRedBackground( bUseRedBackground )
	, m_nFixedFontSize( -1 )
{
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFocusPolicy( Qt::NoFocus );
	
	if ( size.isNull() || size.isEmpty() ) {
		m_size = sizeHint();
	}
	adjustSize();
	setFixedSize( m_size );
	resize( m_size );

	if ( ! sIcon.isEmpty() ) {
		updateIcon();
	} else {
		setText( sText );
	}

	if ( size.width() <= 12 || size.height() <= 12 ) {
		m_sBorderRadius = "0";
	} else if ( size.width() <= 20 || size.height() <= 20 ) {
		m_sBorderRadius = "3";
	} else {
		m_sBorderRadius = "5";
	}
	
	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	updateFont();
	updateStyleSheet();
	updateTooltip();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Button::onPreferencesChanged );
}

Button::~Button() {
}

void Button::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	
	updateStyleSheet();
	update();
	
	setEnabled( bIsActive );
}


void Button::updateIcon() {
	if ( m_bColorful ) {
		setIcon( QIcon( Skin::getSvgImagePath() + "/icons/" + m_sIcon ) );
	} else {
		if ( H2Core::Preferences::get_instance()->getIconColor() ==
			 H2Core::InterfaceTheme::IconColor::White ) {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/white/" + m_sIcon ) );
		} else {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + m_sIcon ) );
		}
	}
	setIconSize( m_iconSize );
}

void Button::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	
	int nFactorGradient = 120;
	int nHover = 10;
	
	QColor backgroundLight = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient );
	QColor backgroundDark = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient );
	QColor backgroundLightHover = pPref->getColorTheme()->m_widgetColor.lighter( nFactorGradient + nHover );
	QColor backgroundDarkHover = pPref->getColorTheme()->m_widgetColor.darker( nFactorGradient + nHover );
	QColor border = Qt::black;

	QColor backgroundCheckedLight, backgroundCheckedDark, backgroundCheckedLightHover,
		backgroundCheckedDarkHover, textChecked;
	if ( ! m_bUseRedBackground ) {
		backgroundCheckedLight = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient );
		backgroundCheckedDark = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient );
		backgroundCheckedLightHover = pPref->getColorTheme()->m_accentColor.lighter( nFactorGradient + nHover );
		backgroundCheckedDarkHover = pPref->getColorTheme()->m_accentColor.darker( nFactorGradient + nHover );
		textChecked = pPref->getColorTheme()->m_accentTextColor;
	} else {
		backgroundCheckedLight = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradient );
		backgroundCheckedDark = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradient );
		backgroundCheckedLightHover = pPref->getColorTheme()->m_buttonRedColor.lighter( nFactorGradient + nHover );
		backgroundCheckedDarkHover = pPref->getColorTheme()->m_buttonRedColor.darker( nFactorGradient + nHover );
		textChecked = pPref->getColorTheme()->m_buttonRedTextColor;
	}

	QColor textColor;
	if ( m_bIsActive ) {
		textColor = pPref->getColorTheme()->m_widgetTextColor;
	} else {
		textColor = pPref->getColorTheme()->m_baseColor;
	}
	
	setStyleSheet( QString( "QPushButton { \
    color: %1; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %3, stop: 1 %4); \
} \
QPushButton:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %5, stop: 1 %6); \
} \
QPushButton:checked { \
    color: %7; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %8, stop: 1 %9); \
} \
QPushButton:checked:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 %10, stop: 1 %11); \
}"
							)
				   .arg( textColor.name() )
				   .arg( m_sBorderRadius )
				   .arg( backgroundLight.name() ).arg( backgroundDark.name() )
				   .arg( backgroundLightHover.name() ).arg( backgroundDarkHover.name() )
				   .arg( textChecked.name() )
				   .arg( backgroundCheckedLight.name() ).arg( backgroundCheckedDark.name() )
				   .arg( backgroundCheckedLightHover.name() ).arg( backgroundCheckedDarkHover.name() )
				   .arg( border.name() ) );
}

void Button::setBaseToolTip( const QString& sNewTip ) {
	m_sBaseTooltip = sNewTip;
	updateTooltip();
}

void Button::setAction( std::shared_ptr<Action> pAction ) {
	m_pAction = pAction;
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
	if ( m_pAction != nullptr ) {
		sTip.append( QString( "\n%1: %2 " ).arg( pCommonStrings->getMidiTooltipHeading() )
					 .arg( m_pAction->getType() ) );
		if ( ! m_sRegisteredMidiEvent.isEmpty() ) {
			sTip.append( QString( "%1 [%2 : %3]" ).arg( pCommonStrings->getMidiTooltipBound() )
						 .arg( m_sRegisteredMidiEvent ).arg( m_nRegisteredMidiParameter ) );
		} else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiTooltipUnbound() ) );
		}
	}
			
	setToolTip( sTip );
}

void Button::setSize( QSize size ) {
	m_size = size;
	
	adjustSize();
	if ( ! size.isNull() ) {
		setFixedSize( size );
		resize( size );
	}

	updateFont();
}

void Button::updateFont() {

	auto pPref = H2Core::Preferences::get_instance();
	
	float fScalingFactor = 1.0;
    switch ( pPref->getFontSize() ) {
    case H2Core::FontTheme::FontSize::Small:
		fScalingFactor = 1.2;
		break;
    case H2Core::FontTheme::FontSize::Normal:
		fScalingFactor = 1.0;
		break;
    case H2Core::FontTheme::FontSize::Large:
		fScalingFactor = 0.75;
		break;
	}

	int nPixelSize;
	if ( m_nFixedFontSize < 0 ) {

		int nMargin;
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
	} else {
		nPixelSize = m_nFixedFontSize;
	}

	QFont font( pPref->getLevel3FontFamily() );
	font.setPixelSize( nPixelSize );
	setFont( font );

	if ( m_size.width() > m_size.height() ) {
		// Check whether the width of the text fits the available frame
		// width of the button.
		while ( fontMetrics().size( Qt::TextSingleLine, text() ).width() > width()
				&& nPixelSize > 1 ) {
			nPixelSize--;
			font.setPixelSize( nPixelSize );
			setFont( font );
		}
	}
}
	
void Button::paintEvent( QPaintEvent* ev )
{
	QPushButton::paintEvent( ev );

	updateFont();

	// Grey-out the widget if it is not enabled
	if ( ! isEnabled() ) {
		QPainter( this ).fillRect( ev->rect(), QColor( 128, 128, 128, 208 ) );
	}
}

void Button::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		updateFont();
		updateStyleSheet();
	}

	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcon();
	}
}
