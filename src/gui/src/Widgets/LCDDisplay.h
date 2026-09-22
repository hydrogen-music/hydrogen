/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef LCDDisplay_H
#define LCDDisplay_H


#include <QtGui>
#include <QtWidgets>
#include <QLineEdit>

#include <core/config.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <vector>

/** Non-interactive display used for both numerical values and the
	status display.*/
/** \ingroup docGUI docWidgets*/
class LCDDisplay : public QLineEdit, public H2Core::Object<LCDDisplay>
{
    H2_OBJECT(LCDDisplay)
	Q_OBJECT

public:
	LCDDisplay( QWidget* pParent, const QSize& size = QSize( 0, 0 ),
				bool bFixedFont = false, bool bIsActive = true );
	~LCDDisplay();

	/** Reserves the cell-based advance of the current text instead of
	 *	its natural one (see #setTabularDigits). */
	virtual QSize sizeHint() const override;

	void setUseRedFont( bool bUseRedFont );

	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	bool getIsHovered() const;

	/** Renders digits in fixed-width cells — each glyph centered in
	 *	the advance of the widest digit of the current font — while all
	 *	other characters keep their natural advance.
	 *
	 *	For rapidly ticking numerical values, like the elapsed time in
	 *	the main toolbar, this keeps the rendered width constant no
	 *	matter which digits are shown: in a proportional font every
	 *	digit swap would otherwise change the string's advance and make
	 *	the centered text pump. The font family is taken from the
	 *	"Item font" selected in the Preferences (like for all other
	 *	LCDDisplay widgets) and follows its changes live; the font size
	 *	set on the widget is preserved. */
	void setTabularDigits( bool bTabular );

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

protected:
	QSize m_size;
	virtual void paintEvent( QPaintEvent *ev ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;

private:
	void updateFont();
	void updateStyleSheet();
	/** Widest digit advance of the current font; a negative result
	 *	marks degenerate metrics (a font without digits) and makes the
	 *	widget fall back to the natural rendering. */
	void measureDigitCellWidth();
	void paintTabularText();

	bool m_bFixedFont;
	bool m_bUseRedFont;
	bool m_bEntered;
	bool m_bIsActive;

	std::vector<int> m_fontPointSizes;

	bool m_bTabularDigits;
	int m_nDigitCellWidth;
	/** Pixel size captured when the tabular mode is enabled — the
	 *	size is owned by the mode, not by whatever font state later
	 *	trickles into the widget. A negative value marks "no explicit
	 *	size set" (the current font's size is preserved instead). */
	int m_nTabularPixelSize;
};
inline bool LCDDisplay::getIsActive() const {
	return m_bIsActive;
}
inline bool LCDDisplay::getIsHovered() const {
	return m_bEntered;
}

#endif
