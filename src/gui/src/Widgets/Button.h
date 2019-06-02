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


#include <hydrogen/object.h>
#include <hydrogen/midi_action.h>

#include "MidiLearnable.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

class PixmapWidget;

/**
 * Generic Button with pixmaps and text.
 *
 * \ingroup docGUI docWidgets
 */
class Button : public QWidget, public H2Core::Object, public MidiLearnable
{
	Q_OBJECT

	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
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

		bool isPressed() {	return m_bPressed;	}
		void setPressed(bool pressed);

		void setText( const QString& sText );
		void setFontSize( int size );

	signals:
		void clicked(Button *pBtn);
		void rightClicked(Button *pBtn);
		void mousePress(Button *pBtn);

	protected slots:
		void buttonPressed_timer_timeout();

	protected:
		bool m_bPressed;

		QFont m_textFont;
		QString m_sText;

		QPixmap m_onPixmap;
		QPixmap m_offPixmap;
		QPixmap m_overPixmap;

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
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

		bool loadImage( const QString& sFilename, QPixmap& pixmap );
};




/**
 * A ToggleButton (On/Off).
 *
 * \ingroup docGUI docWidgets
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
