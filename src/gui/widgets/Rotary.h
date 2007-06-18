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
 * $Id: Rotary.h,v 1.5 2005/05/01 19:51:23 comix Exp $
 *
 */
#ifndef ROTARY_H
#define ROTARY_H

#include <qwidget.h>

#include "LCD.h"

class RotaryTooltip : public QWidget
{
	public:
		RotaryTooltip( QPoint pos );
		~RotaryTooltip();
		void showTip( QPoint pos, QString sText );

	private:
		LCDDisplay *m_pDisplay;
};


class Rotary : public QWidget, public Object
{
	Q_OBJECT
	public:
		enum RotaryType {
			TYPE_NORMAL,
			TYPE_CENTER
		};

		Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseValueTip = true );
		~Rotary();

		void setValue( float fValue );
		float getValue() {	return m_fValue;	}

		void updateRotary();

	signals:
		void valueChanged(Rotary *ref);

	private:
		RotaryType m_type;
		static QPixmap* m_background_normal;
		static QPixmap* m_background_center;
		QPixmap m_temp;

		bool m_bChanged;
		int m_nWidgetWidth;
		int m_nWidgetHeight;

		float m_fValue;
		float m_fMousePressValue;
		float m_fMousePressY;

		int m_nLastFrame;
		RotaryTooltip *m_pValueToolTip;
		bool m_bShowValueToolTip;

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent( QMouseEvent *ev );
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );
};


#endif
