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

#include "ColoredButton.h"

#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>
#include <qnamespace.h>
#include <qpainter.h>

#include "../Skin.h"

ColoredButton::ColoredButton(
	QWidget* pParent,
	const QSize& size,
	const QString& sBaseToolTip,
	int flag
)
	: Button(
		  pParent,
		  size,
		  ColoredButton::Type::Toggle,
		  "",
		  "",
		  QSize( 0, 0 ),
		  sBaseToolTip,
		  flag & Flag::ModifyOnChange,
		  -1
	  ),
      m_bBorderless( false ),
      m_flag( static_cast<Flag>(flag) )
{
	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();
	m_defaultBackgroundColor = pColorTheme->m_widgetColor;

	setFixedFontSize(
		std::min( width(), height() ) - 2 * ColoredButton::nTextMargin
	);

	setFlat( true );
}

ColoredButton::~ColoredButton()
{
}

void ColoredButton::setBorderless( bool bBorderless )
{
	if ( m_bBorderless != bBorderless ) {
		m_bBorderless = bBorderless;
        updateStyleSheet();
	}
}

void ColoredButton::paintEvent( QPaintEvent* pEvent )
{
	if ( !isVisible() ) {
		return;
	}

	Button::paintEvent( pEvent );

	if ( ! ( m_flag & Flag::CustomRendering ) ) {
		return;
	}

	QColor textColor;
	if ( isChecked() ) {
		textColor = m_baseTextColor;
	}
	else {
		textColor = m_baseColor;
	}

	if ( !isEnabled() ) {
		textColor = Skin::makeTextColorInactive( textColor );
	}

	QColor outlineColor;
	if ( H2Core::Preferences::get_instance()
			 ->getInterfaceTheme()
			 ->m_iconColor == H2Core::InterfaceTheme::IconColor::White ) {
		outlineColor = textColor.lighter( 235 );
	}
	else {
		outlineColor = textColor.darker( 235 );
	}

	QPainter p( this );
	p.setRenderHints(
		QPainter::Antialiasing | QPainter::TextAntialiasing |
		QPainter::VerticalSubpixelPositioning
	);
	p.setPen( QPen( outlineColor, 1 ) );
	p.setBrush( textColor );

	const auto fontMetrics = QFontMetrics( font() );
	const auto fontRect = fontMetrics.boundingRect( m_sText );

	QPainterPath path;
	path.addText(
		( width() - fontRect.width() ) / 2 - fontRect.x(),
		( height() - fontRect.height() ) / 2 - fontRect.y(), font(), m_sText
	);
	p.drawPath( path );
}

void ColoredButton::resizeEvent( QResizeEvent* pEvent )
{
	Button::resizeEvent( pEvent );

	setFixedFontSize(
		std::min( width(), height() ) - 2 * ColoredButton::nTextMargin
	);
}

void ColoredButton::updateStyleSheet()
{
	const auto pPref = H2Core::Preferences::get_instance();
    const auto pColorTheme = pPref->getColorTheme();
	QColor borderColor;
	if ( pPref->getInterfaceTheme()->m_iconColor ==
		 H2Core::InterfaceTheme::IconColor::White ) {
		borderColor = Qt::white;
	}
	else {
		borderColor = Qt::black;
	}
	const QColor borderInactiveColor =
		Skin::makeBackgroundColorInactive( borderColor );

	const QColor checkedColor( m_baseColor );
	const QColor checkedTextColor( m_baseTextColor );
	const auto defaultColor = m_defaultBackgroundColor;
    const auto defaultTextColor = m_baseColor;

	QColor hoveredColor;
	if ( Skin::moreBlackThanWhite( defaultColor ) ) {
		hoveredColor = defaultColor.lighter( Skin::nToolButtonHoveredScaling );
	}
	else {
		hoveredColor = defaultColor.darker( Skin::nToolButtonHoveredScaling );
	}
	const auto hoveredTextColor = defaultTextColor;

	QString sBorder;
	if ( m_bBorderless ) {
		sBorder = "none";
	}
	else {
		sBorder = "1px solid #000";
	}

	QString sBorderInteraction, sBorderInactive;
	if ( !m_bBorderless || ( m_flag & Flag::AsToolButton ) ) {
		sBorderInteraction =
			QString( "1px solid %1" ).arg( borderColor.name() );
		sBorderInactive =
			QString( "1px solid %1" ).arg( borderInactiveColor.name() );
	}
	else {
		sBorderInteraction = "none";
		sBorderInactive = "none";
	}

	QColor disabledColor;
	if ( m_flag & Flag::AsToolButton ) {
		// When behaving like a tool button in the tool bar, we will change the
		// icon / text color instead of the background.
		disabledColor = defaultColor;
	}
	else {
		disabledColor = Skin::makeWidgetColorInactive( defaultColor );
	}
	const auto disabledTextColor =
		Skin::makeTextColorInactive( defaultTextColor );
	const auto disabledCheckedColor =
		Skin::makeWidgetColorInactive( checkedColor );
	const auto disabledCheckedTextColor =
		Skin::makeTextColorInactive( checkedTextColor );

	setStyleSheet( QString( "\
QPushButton:enabled { \
    color: %1; \
    font-weight: bold;\
    background: %2; \
    border: %3; \
    padding: 0px; \
} \
QPushButton:enabled:hover { \
    color: %4; \
    background: %5; \
    border: %6; \
} \
QPushButton:pressed { \
    border: %6; \
} \
QPushButton:enabled:checked, QPushButton::enabled:checked:hover { \
    color: %7; \
    background: %8; \
    border: %6; \
} \
QPushButton:disabled { \
    color: %9; \
    background: %10; \
    border: %13; \
    padding: 0px; \
} \
QPushButton:disabled:checked { \
    color: %11; \
    background: %12; \
} \
" )
					   .arg( defaultTextColor.name() )
					   .arg( defaultColor.name() )
					   .arg( sBorder )
					   .arg( hoveredTextColor.name() )
					   .arg( hoveredColor.name() )
					   .arg( sBorderInteraction )
					   .arg( checkedTextColor.name() )
					   .arg( checkedColor.name() )
					   .arg( disabledTextColor.name() )
					   .arg( disabledColor.name() )
					   .arg( disabledCheckedTextColor.name() )
					   .arg( disabledCheckedColor.name() )
					   .arg( sBorderInactive ) );
}
