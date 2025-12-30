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

#ifndef COLORED_BUTTON_H
#define COLORED_BUTTON_H

#include "Button.h"

/** Common basis for #MuteButton and #SoloButton.
 *
 * \ingroup docGUI docWidgets*/
class ColoredButton : public Button, public H2Core::Object<ColoredButton> {
	H2_OBJECT( ColoredButton )
	Q_OBJECT

   public:
	ColoredButton(
		QWidget* pParent,
		const QSize& size,
		const QString& sBaseToolTip,
		bool bModifyOnChange
	);
	~ColoredButton();

	ColoredButton( const ColoredButton& ) = delete;
	ColoredButton& operator=( const ColoredButton& rhs ) = delete;

	void setBorderless( bool bBorderless );
	void setDefaultBackgroundColor( const QColor& color );

	void updateStyleSheet() override;

   protected:
	void setBaseColor( const QColor& color );
	void setBaseTextColor( const QColor& color );

	QColor m_baseColor;
	QColor m_baseTextColor;
	QColor m_defaultBackgroundColor;

	/** Whether the widget will draw its own border or the parent will
	 * handle this job. */
	bool m_bBorderless;
};

inline void ColoredButton::setDefaultBackgroundColor( const QColor& color )
{
	if ( color != m_defaultBackgroundColor ) {
		m_defaultBackgroundColor = color;
        updateStyleSheet();
	}
}
inline void ColoredButton::setBaseColor( const QColor& color )
{
	if ( color != m_baseColor ) {
		m_baseColor = color;
	}
}
inline void ColoredButton::setBaseTextColor( const QColor& color )
{
	if ( color != m_baseTextColor ) {
		m_baseTextColor = color;
	}
}

#endif
