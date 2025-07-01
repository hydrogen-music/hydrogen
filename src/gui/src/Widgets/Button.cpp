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

Button::Button( QWidget *pParent, const QSize& size, const Type& type,
				const QString& sIcon, const QString& sText, const QSize& iconSize,
				const QString& sBaseToolTip, bool bColorful,
				bool bModifyOnChange, int nBorderRadius )
	: QPushButton( pParent )
	, m_size( size )
	, m_type( type )
	, m_iconSize( iconSize )
	, m_bColorful( bColorful )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
	, m_bIsActive( true )
	, m_nFixedFontSize( -1 )
	, m_bModifyOnChange( bModifyOnChange )
	, m_nBorderRadius( nBorderRadius )
	, m_bUseCustomBackgroundColors( false )
{
	auto pPref = H2Core::Preferences::get_instance();
	m_checkedBackgroundColor = pPref->getTheme().m_color.m_accentColor;
	m_checkedBackgroundTextColor = pPref->getTheme().m_color.m_accentTextColor;

	setFocusPolicy( Qt::NoFocus );
	setSize( size );

	if ( ! sIcon.isEmpty() ) {
		updateIcon();
	} else {
		setText( sText );
	}

	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	if ( type == Type::Icon ) {
		setFlat( true );
	}

	updateStyleSheet();
	setBaseToolTip( sBaseToolTip );
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &Button::onPreferencesChanged );

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
		if ( H2Core::Preferences::get_instance()->
			 getTheme().m_interface.m_iconColor ==
			 H2Core::InterfaceTheme::IconColor::White ) {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/white/" + m_sIcon ) );
		} else {
			setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + m_sIcon ) );
		}
	}

	if ( ! m_iconSize.isNull() && ! m_iconSize.isEmpty() ) {
		setIconSize( m_iconSize );
	}
}

void Button::setCheckedBackgroundColor( const QColor& color ) {
	if ( color != m_checkedBackgroundColor ) {
		m_checkedBackgroundColor = color;
		m_bUseCustomBackgroundColors = true;

		updateStyleSheet();
		update();
	}
}

void Button::setCheckedBackgroundTextColor( const QColor& color ) {
	if ( color != m_checkedBackgroundTextColor ) {
		m_checkedBackgroundTextColor = color;
		m_bUseCustomBackgroundColors = true;

		updateStyleSheet();
		update();
	}
}

void Button::updateStyleSheet() {

	if ( m_type == Type::Icon ) {
		// Make background transparent
		setStyleSheet( "QPushButton { background-color: none; }" );
		return;
	}

	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	
	const int nFactorGradient = 126;
	const int nFactorGradientShadow = 225;
	const int nHover = 12;
	const float fStop1 = 0.2;
	const float fStop2 = 0.85;
	const float x1 = 0;
	const float x2 = 1;
	const float y1 = 0;
	const float y2 = 1;

	const QColor baseColorBackground = theme.m_color.m_widgetColor;
	const QColor backgroundLight = baseColorBackground.lighter( nFactorGradient );
	const QColor backgroundDark = baseColorBackground.darker( nFactorGradient );
	const QColor backgroundLightHover = baseColorBackground.lighter( nFactorGradient + nHover );
	const QColor backgroundDarkHover = baseColorBackground.darker( nFactorGradient + nHover );
	const QColor backgroundShadowLight = baseColorBackground.lighter( nFactorGradientShadow );
	const QColor backgroundShadowDark = baseColorBackground.darker( nFactorGradientShadow );
	const QColor backgroundShadowLightHover = baseColorBackground.lighter( nFactorGradientShadow + nHover );
	const QColor backgroundShadowDarkHover = baseColorBackground.darker( nFactorGradientShadow + nHover );
	const QColor border = Qt::black;

	QColor backgroundCheckedColor, backgroundCheckedTextColor;
	if ( m_bUseCustomBackgroundColors ) {
		backgroundCheckedColor = m_checkedBackgroundColor;
		backgroundCheckedTextColor = m_checkedBackgroundTextColor;
	} else {
		backgroundCheckedColor = theme.m_color.m_accentColor;
		backgroundCheckedTextColor = theme.m_color.m_accentTextColor;
	}
	const QColor backgroundCheckedLight =
		backgroundCheckedColor.lighter( nFactorGradient );
	const QColor backgroundCheckedDark =
		backgroundCheckedColor.darker( nFactorGradient );
	const QColor backgroundCheckedLightHover =
		backgroundCheckedColor.lighter( nFactorGradient + nHover );
	const QColor backgroundCheckedDarkHover =
		backgroundCheckedColor.darker( nFactorGradient + nHover );
	const QColor backgroundShadowCheckedLight =
		backgroundCheckedColor.lighter( nFactorGradientShadow );
	const QColor backgroundShadowCheckedDark =
		backgroundCheckedColor.darker( nFactorGradientShadow );
	const QColor backgroundShadowCheckedLightHover =
		backgroundCheckedColor.lighter( nFactorGradientShadow + nHover );
	const QColor backgroundShadowCheckedDarkHover =
		backgroundCheckedColor.darker( nFactorGradientShadow + nHover );

	const QColor textColor = theme.m_color.m_widgetTextColor;
	
	const QColor backgroundInactiveLight =
		Skin::makeWidgetColorInactive( backgroundLight );
	const QColor backgroundInactiveLightHover = backgroundInactiveLight;
	const QColor backgroundInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundCheckedLight );
	const QColor backgroundInactiveCheckedLightHover = backgroundInactiveCheckedLight;
	const QColor backgroundInactiveDark =
		Skin::makeWidgetColorInactive( backgroundDark );
	const QColor backgroundInactiveDarkHover = backgroundInactiveDark;
	const QColor backgroundInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundCheckedDark );
	const QColor backgroundInactiveCheckedDarkHover = backgroundInactiveCheckedDark;
	const QColor backgroundShadowInactiveLight =
		Skin::makeWidgetColorInactive( backgroundShadowLight );
	const QColor backgroundShadowInactiveLightHover = backgroundShadowInactiveLight;
	const QColor backgroundShadowInactiveCheckedLight =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedLight );
	const QColor backgroundShadowInactiveCheckedLightHover = backgroundShadowInactiveCheckedLight;
	const QColor backgroundShadowInactiveDark =
		Skin::makeWidgetColorInactive( backgroundShadowDark );
	const QColor backgroundShadowInactiveDarkHover = backgroundShadowInactiveDark;
	const QColor backgroundShadowInactiveCheckedDark =
		Skin::makeWidgetColorInactive( backgroundShadowCheckedDark );
	const QColor backgroundShadowInactiveCheckedDarkHover = backgroundShadowInactiveCheckedDark;
	const QColor textInactiveColor = Skin::makeTextColorInactive( textColor );

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
				   .arg( backgroundCheckedTextColor.name() )
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
				   .arg( backgroundCheckedTextColor.name() )
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

void Button::updateToolTip() {
	setToolTip( composeToolTip() );
}

void Button::setSize( const QSize& size ) {
	if ( size.isNull() || size.isEmpty() ) {
		m_size = sizeHint();
	} else {
		m_size = size;
	}

	setFixedSize( m_size );
	resize( m_size );
	adjustSize();

	if ( m_nBorderRadius == -1 ) {
		if ( m_size.width() <= 12 || m_size.height() <= 12 ) {
			m_nBorderRadius = 0;
		} else if ( m_size.width() <= 20 || m_size.height() <= 20 ) {
			m_nBorderRadius = 3;
		} else {
			m_nBorderRadius = 5;
		}
	}

	updateFont();
}

void Button::setType( const Type& type ) {

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

	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	
	float fScalingFactor = 1.0;
    switch ( theme.m_font.m_fontSize ) {
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

	QFont font( theme.m_font.m_sLevel3FontFamily );
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

void Button::setIconFileName( const QString& sIcon ) {
	if ( m_sIcon != sIcon ) {
		m_sIcon = sIcon;
		updateIcon();
	}
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

void Button::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
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
