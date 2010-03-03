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

#include "config.h"

#include <hydrogen/Object.h>
#include <hydrogen/action.h>

#include "MidiLearnable.h"

#include <QtGui>

class PixmapWidget;

/**
 * Generic Button with pixmaps and text.
 */
class Button : public QWidget, public Object, public MidiLearnable
{
	Q_OBJECT

	public:
		Button(
				QWidget *pParent,
				const QString& sOnImg,
				const QString& sOffImg,
				const QString& sOverImg,
				QSize size,
				bool use_skin_style = false
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

	protected:
		bool m_bPressed;

		QFont m_textFont;
		QString m_sText;

		QPixmap m_onPixmap;
		QPixmap m_offPixmap;
		QPixmap m_overPixmap;

	private:
		bool m_bMouseOver;
		bool __use_skin_style;

		void mousePressEvent(QMouseEvent *ev);
		void mouseReleaseEvent(QMouseEvent *ev);
		void enterEvent(QEvent *ev);
		void leaveEvent(QEvent *ev);
		void paintEvent( QPaintEvent* ev);

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
