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

	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();
    m_defaultBackgroundColor = pColorTheme->m_widgetColor;

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
    const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();
	const QColor borderColor = Qt::black;

	const QColor checkedColor( m_baseColor );
	const QColor checkedTextColor( m_baseTextColor );
	const auto defaultColor = m_defaultBackgroundColor;
    const auto defaultTextColor = m_baseColor;

	QColor hoveredColor, hoveredBorder;
	if ( Skin::moreBlackThanWhite( defaultColor ) ) {
		hoveredColor = defaultColor.lighter( Skin::nToolBarHoveredScaling );
		hoveredBorder = borderColor.darker( Skin::nToolBarHoveredScaling );
	}
	else {
		hoveredColor = defaultColor.darker( Skin::nToolBarHoveredScaling );
		hoveredBorder = borderColor.lighter( Skin::nToolBarHoveredScaling );
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
QPushButton:enabled:checked, QPushButton::enabled:checked:hover { \
    color: %7; \
    background: %8; \
} \
QPushButton:disabled { \
    color: %9; \
    background: %10; \
    border: %5; \
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
					   .arg( sBorderHovered )
					   .arg( checkedTextColor.name() )
					   .arg( checkedColor.name() )
					   .arg( disabledTextColor.name() )
					   .arg( disabledColor.name() )
					   .arg( disabledCheckedTextColor.name() )
					   .arg( disabledCheckedColor.name() ) );
}
