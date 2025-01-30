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

Button::Button( QWidget *pParent, QSize size, Type type, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, QString sBaseTooltip, bool bColorful, bool bModifyOnChange, int nBorderRadius )
	: QPushButton( pParent )
	, m_size( size )
	, m_type( type )
	, m_iconSize( iconSize )
	, m_sBaseTooltip( sBaseTooltip )
	, m_bColorful( bColorful )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
	, m_bIsActive( true )
	, m_bUseRedBackground( bUseRedBackground )
	, m_nFixedFontSize( -1 )
	, m_bModifyOnChange( bModifyOnChange )
	, m_nBorderRadius( nBorderRadius )
{
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

	if ( m_nBorderRadius == -1 ) {
		if ( size.width() <= 12 || size.height() <= 12 ) {
			m_nBorderRadius = 0;
		} else if ( size.width() <= 20 || size.height() <= 20 ) {
			m_nBorderRadius = 3;
		} else {
			m_nBorderRadius = 5;
		}
	}
	
	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	if ( type == Type::Icon ) {
		setFlat( true );
	}

	updateFont();
	updateStyleSheet();
	updateTooltip();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Button::onPreferencesChanged );

	connect( this, SIGNAL(clicked()), this, SLOT(onClick()));
}

Button::~Button() {
}

void Button::setIsActive( bool bIsActive ) {
	m_bIsActive = bIsActive;
	
	setEnabled( bIsActive );

	update();
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

void Button::setUseRedBackground( bool bUseRedBackground ) {
	m_bUseRedBackground	= bUseRedBackground;

	updateStyleSheet();
	update();
}

void Button::updateStyleSheet() {

	if ( m_type == Type::Icon ) {
		// Make background transparent
		setStyleSheet( "QPushButton { background-color: none; }" );
		return;
	}

	auto pPref = H2Core::Preferences::get_instance();
	
	int nFactorGradient = 126;
	int nFactorGradientShadow = 225;
	int nHover = 12;
	float fStop1 = 0.2;
	float fStop2 = 0.85;
	float x1 = 0;
	float x2 = 1;
	float y1 = 0;
	float y2 = 1;

	QColor baseColorBackground = pPref->getColorTheme()->m_widgetColor;
	QColor backgroundLight = baseColorBackground.lighter( nFactorGradient );
	QColor backgroundDark = baseColorBackground.darker( nFactorGradient );
	QColor backgroundLightHover = baseColorBackground.lighter( nFactorGradient + nHover );
	QColor backgroundDarkHover = baseColorBackground.darker( nFactorGradient + nHover );
	QColor backgroundShadowLight = baseColorBackground.lighter( nFactorGradientShadow );
	QColor backgroundShadowDark = baseColorBackground.darker( nFactorGradientShadow );
	QColor backgroundShadowLightHover = baseColorBackground.lighter( nFactorGradientShadow + nHover );
	QColor backgroundShadowDarkHover = baseColorBackground.darker( nFactorGradientShadow + nHover );
	QColor border = Qt::black;

	QColor baseColorBackgroundChecked, textChecked;
	if ( ! m_bUseRedBackground ) {
		baseColorBackgroundChecked = pPref->getColorTheme()->m_accentColor;
		textChecked = pPref->getColorTheme()->m_accentTextColor;
	} else {
		baseColorBackgroundChecked = pPref->getColorTheme()->m_buttonRedColor;
		textChecked = pPref->getColorTheme()->m_buttonRedTextColor;
	}
	
	QColor backgroundCheckedLight = baseColorBackgroundChecked.lighter( nFactorGradient );
	QColor backgroundCheckedDark = baseColorBackgroundChecked.darker( nFactorGradient );
	QColor backgroundCheckedLightHover = baseColorBackgroundChecked.lighter( nFactorGradient + nHover );
	QColor backgroundCheckedDarkHover = baseColorBackgroundChecked.darker( nFactorGradient + nHover );
	QColor backgroundShadowCheckedLight = baseColorBackgroundChecked.lighter( nFactorGradientShadow );
	QColor backgroundShadowCheckedDark = baseColorBackgroundChecked.darker( nFactorGradientShadow );
	QColor backgroundShadowCheckedLightHover = baseColorBackgroundChecked.lighter( nFactorGradientShadow + nHover );
	QColor backgroundShadowCheckedDarkHover = baseColorBackgroundChecked.darker( nFactorGradientShadow + nHover );

	QColor textColor = pPref->getColorTheme()->m_widgetTextColor;
	
	QColor backgroundInactiveLight =
		Skin::makeWidgetColorInactive( backgroundLight );
	QColor backgroundInactiveLightHover = backgroundInactiveLight;
	QColor backgroundInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundCheckedLight );
	QColor backgroundInactiveCheckedLightHover = backgroundInactiveCheckedLight;
	QColor backgroundInactiveDark =
		Skin::makeWidgetColorInactive( backgroundDark );
	QColor backgroundInactiveDarkHover = backgroundInactiveDark;
	QColor backgroundInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundCheckedDark );
	QColor backgroundInactiveCheckedDarkHover = backgroundInactiveCheckedDark;
	QColor backgroundShadowInactiveLight =
		Skin::makeWidgetColorInactive( backgroundShadowLight );
	QColor backgroundShadowInactiveLightHover = backgroundShadowInactiveLight;
	QColor backgroundShadowInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedLight );
	QColor backgroundShadowInactiveCheckedLightHover = backgroundShadowInactiveCheckedLight;
	QColor backgroundShadowInactiveDark =
		Skin::makeWidgetColorInactive( backgroundShadowDark );
	QColor backgroundShadowInactiveDarkHover = backgroundShadowInactiveDark;
	QColor backgroundShadowInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedDark );
	QColor backgroundShadowInactiveCheckedDarkHover = backgroundShadowInactiveCheckedDark;
	QColor textInactiveColor = Skin::makeTextColorInactive( textColor );

	setStyleSheet( QString( "\
QPushButton:enabled { \
    color: %1; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %23, stop: %39 %3, \
                                      stop: %40 %4, stop: 1 %24); \
} \
QPushButton:enabled:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %25, stop: %39 %5, \
                                      stop: %40 %6, stop: 1 %26); \
} \
QPushButton:enabled:checked { \
    color: %7; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %27, stop: %39 %8, \
                                      stop: %40 %9, stop: 1 %28); \
} \
QPushButton:enabled:checked:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %29, stop: %39 %10, \
                                      stop: %40 %11, stop: 1 %30); \
} \
QPushButton:disabled { \
    color: %13; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %31, stop: %39 %14, \
                                      stop: %40 %15, stop: 1 %32); \
} \
QPushButton:disabled:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %33, stop: %39 %16, \
                                      stop: %40 %17, stop: 1 %34); \
} \
QPushButton:disabled:checked { \
    color: %18; \
    border: 1px solid %12; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %35, stop: %39 %19, \
                                      stop: %40 %20, stop: 1 %36); \
} \
QPushButton:disabled:checked:hover { \
    background-color: qlineargradient(x1: %41, y1: %43, x2: %42, y2: %44, \
                                      stop: 0 %37, stop: %39 %21, \
                                      stop: %40 %22, stop: 1 %38); \
}"
							)
				   .arg( textColor.name() )
				   .arg( m_nBorderRadius )
				   .arg( backgroundLight.name() )
				   .arg( backgroundDark.name() )
				   .arg( backgroundLightHover.name() )
				   .arg( backgroundDarkHover.name() )
				   .arg( textChecked.name() )
				   .arg( backgroundCheckedLight.name() )
				   .arg( backgroundCheckedDark.name() )
				   .arg( backgroundCheckedLightHover.name() )
				   .arg( backgroundCheckedDarkHover.name() )
				   .arg( border.name() )
				   .arg( textInactiveColor.name() )
				   .arg( backgroundInactiveLight.name() )
				   .arg( backgroundInactiveDark.name() )
				   .arg( backgroundInactiveLightHover.name() )
				   .arg( backgroundInactiveDarkHover.name() )
				   .arg( textChecked.name() )
				   .arg( backgroundInactiveCheckedLight.name() )
				   .arg( backgroundInactiveCheckedDark.name() )
				   .arg( backgroundInactiveCheckedLightHover.name() )
				   .arg( backgroundInactiveCheckedDarkHover.name() )
				   .arg( backgroundShadowLight.name() )
				   .arg( backgroundShadowDark.name() )
				   .arg( backgroundShadowLightHover.name() )
				   .arg( backgroundShadowDarkHover.name() )
				   .arg( backgroundShadowCheckedLight.name() )
				   .arg( backgroundShadowCheckedDark.name() )
				   .arg( backgroundShadowCheckedLightHover.name() )
				   .arg( backgroundShadowCheckedDarkHover.name() )
				   .arg( backgroundShadowInactiveLight.name() )
				   .arg( backgroundShadowInactiveDark.name() )
				   .arg( backgroundShadowInactiveLightHover.name() )
				   .arg( backgroundShadowInactiveDarkHover.name() )
				   .arg( backgroundShadowInactiveCheckedLight.name() )
				   .arg( backgroundShadowInactiveCheckedDark.name() )
				   .arg( backgroundShadowInactiveCheckedLightHover.name() )
				   .arg( backgroundShadowInactiveCheckedDarkHover.name() )
				   .arg( fStop1 ).arg( fStop2 ).arg( x1 ).arg( x2 )
				   .arg( y1 ).arg( y2 ) );
}

void Button::setBaseToolTip( const QString& sNewTip ) {
	m_sBaseTooltip = sNewTip;
	updateTooltip();
}

void Button::mousePressEvent(QMouseEvent*ev) {
	if ( ev->button() == Qt::RightButton ) {
		emit rightClicked();
	}

	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
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
		if ( m_registeredMidiEvents.size() > 0 ) {
			for ( const auto& [event, nnParam] : m_registeredMidiEvents ) {
				if ( event == H2Core::MidiMessage::Event::Note ||
					 event == H2Core::MidiMessage::Event::CC ) {
					sTip.append( QString( "\n%1 [%2 : %3]" )
								 .arg( pCommonStrings->getMidiTooltipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) )
								 .arg( nnParam ) );
				}
				else {
					// PC and MMC_x do not have a parameter.
					sTip.append( QString( "\n%1 [%2]" )
								 .arg( pCommonStrings->getMidiTooltipBound() )
								 .arg( H2Core::MidiMessage::EventToQString( event ) ) );
				}
			}
		}
		else {
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

void Button::setType( Type type ) {

	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	if ( type == Type::Icon ) {
		setFlat( true );
	} else {
		setFlat( false );
	}
	
	m_type = type;

	updateStyleSheet();
	update();
}

void Button::updateFont() {

	auto pPref = H2Core::Preferences::get_instance();
	
	float fScalingFactor = 1.0;
    switch ( pPref->getFontSize() ) {
    case H2Core::FontTheme::FontSize::Small:
		fScalingFactor = 1.2;
		break;
    case H2Core::FontTheme::FontSize::Medium:
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

	if ( m_size.width() > m_size.height() ) {
		// Check whether the width of the text fits the available frame
		// width of the button.
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
	
void Button::paintEvent( QPaintEvent* ev )
{
	QPushButton::paintEvent( ev );

	updateFont();

	// Grey-out the widget some more if it is not enabled
	if ( ! isEnabled() ) {
		QPainter( this ).fillRect( ev->rect(), QColor( 128, 128, 128, 48 ) );
	}

}

void Button::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {

		updateFont();
		updateStyleSheet();
	}

	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcon();
	}
}

void Button::onClick() {
	if ( m_bModifyOnChange ) {
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}
}

void Button::setBorderRadius( int nBorderRadius ) {
	m_nBorderRadius = nBorderRadius;
	updateStyleSheet();
}
