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

	static constexpr int nTextMargin = 2;

	enum Flag {
		None = 0x000,
		ModifyOnChange = 0x001,
		/** Depending on the background color of the widget the colored button
		 * is residing in - and given that it adopts the same color in unchecked
		 * state - the default text color might be illegible. To circumvent
		 * this problem, we support rendering its text with a custom routine
		 * that uses either a darker or lighter outline (depending on the icon
		 * color set in the preferences). However, the default Qt text rendering
		 * is far superior and works a lot better for small button. So, this
		 * option should be used with care. We also do not provide the "M" or
		 * "S" for mute or solo button as SVG icons because we want to support
		 * internationalization. */
		CustomRendering = 0x002,
		/** Display borders during hovering and pressing as well as in checked
		 * state on borderless buttons. This yields the same UI/UX as for tool
		 * buttons in a tool bar. */
		BordersOnInteraction = 0x004
	};

	ColoredButton(
		QWidget* pParent,
		const QSize& size,
		const QString& sBaseToolTip,
        int flag = Flag::None
	);
	~ColoredButton();

	ColoredButton( const ColoredButton& ) = delete;
	ColoredButton& operator=( const ColoredButton& rhs ) = delete;

	void setBorderless( bool bBorderless );
	void setDefaultBackgroundColor( const QColor& color );

	void updateStyleSheet() override;

   protected:
	void paintEvent( QPaintEvent* pEvent ) override;
	void resizeEvent( QResizeEvent* pEvent ) override;

	void setBaseColor( const QColor& color );
	void setBaseTextColor( const QColor& color );

	Flag m_flag;

	/** We do not use the text member of the parent QPushButton since we
	 * want to render the text ourselves. This gives us the ability to add
	 * an outline and make the text more legible for many more color. */
	QString m_sText;

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
