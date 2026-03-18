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
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

Button::Button( QWidget *pParent, const QSize& size, const Type& type,
				const QString& sIcon, const QString& sText, const QSize& iconSize,
				const QString& sBaseToolTip, bool bModifyOnChange,
				int nBorderRadius )
	: QPushButton( pParent )
	, m_size( size )
	, m_type( type )
	, m_iconSize( iconSize )
	, m_bLastCheckedState( false )
	, m_sIcon( sIcon )
	, m_bIsActive( true )
	, m_nFixedFontSize( -1 )
	, m_bModifyOnChange( bModifyOnChange )
	, m_nBorderRadius( nBorderRadius )
	, m_bUseCustomBackgroundColors( false )
{
	auto pPref = H2Core::Preferences::get_instance();
	m_checkedBackgroundColor = pPref->getColorTheme()->m_accentColor;
	m_checkedBackgroundTextColor = pPref->getColorTheme()->m_accentTextColor;

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
	if ( H2Core::Preferences::get_instance()->
		 getInterfaceTheme()->m_iconColor ==
		 H2Core::InterfaceTheme::IconColor::White ) {
		setIcon( QIcon( Skin::getSvgImagePath() + "/icons/white/" + m_sIcon ) );
	}
	else {
		setIcon( QIcon( Skin::getSvgImagePath() + "/icons/black/" + m_sIcon ) );
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

	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();
	
	const QColor backgroundColor = pColorTheme->m_widgetColor;
	const QColor backgroundHoverColor = backgroundColor.lighter( Skin::nToolButtonHoveredScaling );
	const QColor borderColor = Qt::black;

	QColor backgroundCheckedColor, backgroundCheckedTextColor;
	if ( m_bUseCustomBackgroundColors ) {
		backgroundCheckedColor = m_checkedBackgroundColor;
		backgroundCheckedTextColor = m_checkedBackgroundTextColor;
	} else {
		backgroundCheckedColor = pColorTheme->m_accentColor;
		backgroundCheckedTextColor = pColorTheme->m_accentTextColor;
	}

	const QColor textColor = pColorTheme->m_widgetTextColor;
	
	const QColor backgroundInactiveColor =
		Skin::makeWidgetColorInactive( backgroundColor );
	const QColor backgroundInactiveHoverColor = backgroundInactiveColor;
	const QColor backgroundInactiveCheckedColor =
		Skin::makeWidgetColorInactive( backgroundCheckedColor );
	const QColor backgroundInactiveCheckedHoverColor = backgroundInactiveCheckedColor;
	const QColor textInactiveColor = Skin::makeTextColorInactive( textColor );

	setStyleSheet( QString( "\
QPushButton:enabled { \
    color: %5; \
    border: 1px solid %1; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: %3; \
} \
QPushButton:enabled:hover { \
    background-color: %4; \
} \
QPushButton:enabled:checked { \
    color: %7; \
    border: 1px solid %1; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: %6; \
} \
QPushButton:enabled:checked:hover { \
    background-color: %6; \
} \
QPushButton:disabled { \
    color: %12; \
    border: 1px solid %1; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: %8; \
} \
QPushButton:disabled:hover { \
    background-color: %9; \
} \
QPushButton:disabled:checked { \
    color: %12; \
    border: 1px solid %1; \
    border-radius: %2px; \
    padding: 0px; \
    background-color: %10; \
} \
QPushButton:disabled:checked:hover { \
    background-color: %11; \
}" )
					   .arg( borderColor.name() )
					   .arg( m_nBorderRadius )
					   .arg( backgroundColor.name() )
					   .arg( backgroundHoverColor.name() )
					   .arg( textColor.name() )
					   .arg( backgroundCheckedColor.name() )
					   .arg( backgroundCheckedTextColor.name() )
					   .arg( backgroundInactiveColor.name() )
					   .arg( backgroundInactiveHoverColor.name() )
					   .arg( backgroundInactiveCheckedColor.name() )
					   .arg( backgroundInactiveCheckedHoverColor.name() )
					   .arg( textInactiveColor.name() ) );
}

void Button::mousePressEvent(QMouseEvent*ev) {
	if ( ev->button() == Qt::RightButton ) {
		emit rightClicked();
	}

	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getMidiAction() );
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
		} else {
			m_nBorderRadius = 2;
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

	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();
	
	float fScalingFactor = 1.0;
    switch ( pFontTheme->m_fontSize ) {
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

	QFont font( pFontTheme->m_sLevel3FontFamily );
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

	setFont( font );
	update();
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
