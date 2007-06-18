/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Button.h,v 1.8 2005/05/09 18:12:15 comix Exp $
 *
 */


#ifndef BUTTON_H
#define BUTTON_H

#include <lib/Object.h>

#include <qpixmap.h>
#include <qwidget.h>

/**
 * Generic Button with pixmaps
 */
class Button : public QWidget, public Object
{
	Q_OBJECT

	public:
		Button(QWidget * parent, QSize size, string onImg, string offImg, string overImg);
		~Button();
		bool isPressed() {	return m_bPressed;	}
		void setPressed(bool pressed);
		void drawButton();

	signals:
		void clicked(Button *pBtn);
		void rightClicked(Button *pBtn);

		void mousePress(Button *pBtn);

	protected:
		bool m_bPressed;

		QPixmap m_onPixmap;
		QPixmap m_offPixmap;
		QPixmap m_overPixmap;

	private:
		bool m_bMouseOver;
		void mousePressEvent(QMouseEvent *ev);
		void mouseReleaseEvent(QMouseEvent *ev);
		void enterEvent(QEvent *ev);
		void leaveEvent(QEvent *ev);
};



/**
 * ToggleButton
 */
class ToggleButton : public Button
{
	Q_OBJECT

	public:
		/** Constructor */
		ToggleButton(QWidget * parent, QSize size, string onImg, string offImg, string overImg);
		~ToggleButton();

	private:
		void mousePressEvent(QMouseEvent *ev);
		void mouseReleaseEvent(QMouseEvent *ev);
};


#endif

