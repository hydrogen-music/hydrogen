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

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

ColoredButton::ColoredButton(
	QWidget* pParent,
	const QSize& size,
	const QString& sBaseToolTip,
	bool bModifyOnChange
)
	: Button(
		  pParent,
		  size,
		  ColoredButton::Type::Toggle,
		  "",
		  "M",
		  QSize( 0, 0 ),
		  sBaseToolTip,
		  bModifyOnChange,
		  -1
	  ),
      m_bBorderless( false )
{
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

void ColoredButton::updateStyleSheet()
{
	const QColor borderColor = Qt::black;

	const QColor checkedColor( m_baseColor );
	const QColor checkedTextColor( m_baseTextColor );

	// We try to toggle the text color as much as possible in order to make the
	// state change more evident.
	QColor defaultTextColor, defaultColor;
	if ( Skin::moreBlackThanWhite( m_baseTextColor ) ) {
		defaultTextColor = Qt::white;
		defaultColor = Qt::black;
	}
	else {
		defaultTextColor = Qt::black;
		defaultColor = Qt::white;
	}

	const int nOuterGradientScaling = 100;
    const QString sGradient = "0.7";

	QColor hoveredColor, hoveredBorder, gradientOuterColor;
	if ( Skin::moreBlackThanWhite( defaultColor ) ) {
		defaultTextColor = Qt::white;
		hoveredColor = defaultColor.lighter( Skin::nToolBarHoveredScaling );
		hoveredBorder = borderColor.darker( Skin::nToolBarHoveredScaling );
        gradientOuterColor = m_baseColor.lighter( nOuterGradientScaling );
	}
	else {
		defaultTextColor = Qt::black;
		hoveredColor = defaultColor.darker( Skin::nToolBarHoveredScaling );
		hoveredBorder = borderColor.lighter( Skin::nToolBarHoveredScaling );
        gradientOuterColor = m_baseColor.darker( nOuterGradientScaling );
	}
	const auto hoveredTextColor = defaultTextColor;

	QString sBorder, sBorderHovered;
	if ( m_bBorderless ) {
		sBorder = "none";
		sBorderHovered = "none";
	}
	else {
		sBorder = "1px solid #000";
		sBorderHovered = QString( "1px solid %1" ).arg( hoveredBorder.name() );
	}

	const auto disabledColor = Skin::makeWidgetColorInactive( defaultColor );
	const auto disabledTextColor =
		Skin::makeTextColorInactive( defaultTextColor );
	const auto disabledCheckedColor =
		Skin::makeWidgetColorInactive( checkedColor );
	const auto disabledCheckedTextColor =
		Skin::makeTextColorInactive( checkedTextColor );

	setStyleSheet( QString( "\
QPushButton:enabled { \
    color: %1; \
    background: qradialgradient(cx:0.5, cy:0.5, radius: %2, fx:0.5, fy:0.5,\
                                stop:0 %3, stop:1 %4); \
    border: %5; \
    padding: 0px; \
} \
QPushButton:enabled:hover { \
    color: %6; \
    background: qradialgradient(cx:0.5, cy:0.5, radius: %2, fx:0.5, fy:0.5,\
                                stop:0 %7, stop:1 %4); \
    border: %8; \
} \
QPushButton:enabled:checked, QPushButton::enabled:checked:hover { \
    color: %9; \
    background: %10; \
} \
QPushButton:disabled { \
    color: %11; \
    background: qradialgradient(cx:0.5, cy:0.5, radius: %2, fx:0.5, fy:0.5,\
                                stop:0 %12, stop:1 %10); \
    border: %5; \
    padding: 0px; \
} \
QPushButton:disabled:checked { \
    color: %13; \
    background: %14; \
} \
" )
					   .arg( defaultTextColor.name() )
					   .arg( sGradient )
					   .arg( defaultColor.name() )
					   .arg( gradientOuterColor.name() )
					   .arg( sBorder )
					   .arg( hoveredTextColor.name() )
					   .arg( hoveredColor.name() )
					   .arg( sBorderHovered )
					   .arg( checkedTextColor.name() )
					   .arg( checkedColor.name() )
					   .arg( disabledTextColor.name() )
					   .arg( disabledColor.name() )
					   .arg( disabledCheckedTextColor.name() )
					   .arg( disabledCheckedColor.name() ) );
}
