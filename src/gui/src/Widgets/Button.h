/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef BUTTON_H
#define BUTTON_H


#include <core/Object.h>
#include <core/Preferences.h>
#include <core/MidiAction.h>

#include "MidiLearnable.h"

#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>
#include "WidgetWithScalableFont.h"


/**
 * Generic Button with pixmaps and text.
 */
class Button : public QWidget, public H2Core::Object, public MidiLearnable
{
    H2_OBJECT
	Q_OBJECT

public:
	/**
	 * Either the path to a SVG image or a text to be displayed has to
	 * be provided. If both are given, the icon will be used over the
	 * text. If the text should be used instead, @a sIcon must the
	 * an empty string.
	 */
	Button(
		   QWidget *pParent,
		   QSize size,
		   const QString& sIcon,
		   const QString& sText = "",
		   bool bUseRedBackground = false,
		   QSize iconSize = QSize( 0, 0 ),
		   bool bEnablePressHold = false
		   );
	virtual ~Button();
	
	Button(const Button&) = delete;
	Button& operator=( const Button& rhs ) = delete;

	bool isPressed() const;
	void setPressed(bool pressed);

	void setText( const QString& sText );
	const QString& getText() const ;

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );

signals:
	void clicked(Button *pBtn);
	void rightClicked(Button *pBtn);
	void mousePress(Button *pBtn);
	
protected slots:
	void buttonPressed_timer_timeout();

protected:
	bool m_bIsPressed;
	QString m_sText;

	QSvgRenderer* m_icon;
	QSvgRenderer* m_background;

private:
	bool m_bMouseOver;
	bool m_bUseRedBackground;
	bool m_bEnablePressHold;

	int m_nHeight;
	int m_nWidth;
	QSize m_iconSize;

	void mousePressEvent(QMouseEvent *ev);
	void mouseReleaseEvent(QMouseEvent *ev);
	void enterEvent(QEvent *ev);
	void leaveEvent(QEvent *ev);
	void paintEvent( QPaintEvent* ev);

	QTimer *m_timer;
	int m_timerTimeout;
	/** Used to detect changed in the font*/
	QString m_sLastUsedFontFamily;
	/** Used to detect changed in the font*/
	H2Core::Preferences::FontSize m_lastUsedFontSize;
};

inline bool Button::isPressed() const {
	return m_bIsPressed;
}
inline const QString& Button::getText() const {
	return m_sText;
}

/**
 * A ToggleButton (On/Off).
 */
class ToggleButton : public Button
{
	Q_OBJECT

public:
	ToggleButton( QWidget *pParent, QSize size, const QString& sIcon, const QString& sText = "", bool bUseRedBackground = false, QSize iconSize = QSize( 0, 0 ) );
	~ToggleButton();

private:
	void mousePressEvent( QMouseEvent *ev );
	void mouseReleaseEvent( QMouseEvent *ev );
};

class LEDButton : public Button {
	Q_OBJECT

public:
	LEDButton( QWidget *pParent, QSize size );
	~LEDButton();

private:
	void paintEvent( QPaintEvent* ev );
};
#endif
