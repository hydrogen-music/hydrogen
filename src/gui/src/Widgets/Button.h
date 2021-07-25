/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef BUTTON_H
#define BUTTON_H


#include <core/Object.h>
#include <core/Preferences.h>
#include <core/MidiAction.h>

#include "MidiLearnable.h"

#include <QtGui>
#include <QtWidgets>
#include "WidgetWithScalableFont.h"


class PixmapWidget;

/**
 * Generic Button with pixmaps and text.
 */
class Button : public QWidget, protected WidgetWithScalableFont<6, 8, 10>, public H2Core::Object, public MidiLearnable
{
    H2_OBJECT
	Q_OBJECT

	public:
		Button(
				QWidget *pParent,
				const QString& sOnImg,
				const QString& sOffImg,
				const QString& sOverImg,
				QSize size,
				bool use_skin_style = false,
				bool enable_press_hold = false
		);
		virtual ~Button();
	
		Button(const Button&) = delete;
		Button& operator=( const Button& rhs ) = delete;

		bool isPressed() {	return m_bPressed;	}
		void setPressed(bool pressed);

		void setText( const QString& sText );

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );
	
	signals:
		void clicked(Button *pBtn);
		void rightClicked(Button *pBtn);
		void mousePress(Button *pBtn);

	protected slots:
		void buttonPressed_timer_timeout();

	protected:
		bool m_bPressed;

		QString m_sText;

		QPixmap m_onPixmap;
		QPixmap m_offPixmap;
		QPixmap m_overPixmap;

	private:
		bool m_bMouseOver;
		bool __use_skin_style;
		bool __enable_press_hold;

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

		bool loadImage( const QString& sFilename, QPixmap& pixmap );
};




/**
 * A ToggleButton (On/Off).
 */
class ToggleButton : public Button
{
	Q_OBJECT

	public:
		ToggleButton( QWidget *pParent, const QString& sOnImg, const QString& sOffImg, const QString& sOverImg, QSize size, bool use_skin_style = false );
		~ToggleButton();

	private:
		void mousePressEvent( QMouseEvent *ev );
		void mouseReleaseEvent( QMouseEvent *ev );
};


#endif
