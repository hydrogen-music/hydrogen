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

#ifndef ROTARY_H
#define ROTARY_H

#include "config.h"
#include "MidiLearnable.h"
#include "MidiSenseWidget.h"

#include <QtGui>

class LCDDisplay;

#include <hydrogen/Object.h>

class RotaryTooltip : public QWidget
{
	public:
		RotaryTooltip( QPoint pos );
		~RotaryTooltip();
		void showTip( QPoint pos, QString sText );

	private:
		LCDDisplay *m_pDisplay;
};



class Rotary : public QWidget, public Object, public MidiLearnable
{
	Q_OBJECT
	public:
		enum RotaryType {
			TYPE_NORMAL,
			TYPE_CENTER
		};

		Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseIntSteps, bool bUseValueTip );
		~Rotary();

		void setMin( float fMin );
		float getMin();

		void setMax( float fMax );
		float getMax();

		void setValue( float fValue );
		float getValue() {
			if ( m_bUseIntSteps ) {
				int val = (int)m_fValue;
				return val;
			}
			else
				return m_fValue;
		}

	signals:
		void valueChanged(Rotary *ref);

	private:
		bool m_bUseIntSteps;
		RotaryType m_type;
		static QPixmap* m_background_normal;
		static QPixmap* m_background_center;

		int m_nWidgetWidth;
		int m_nWidgetHeight;

		float m_fMin;
		float m_fMax;
		float m_fValue;

		float m_fMousePressValue;
		float m_fMousePressY;

		RotaryTooltip *m_pValueToolTip;
		bool m_bShowValueToolTip;

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent( QMouseEvent *ev );
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );
};


#endif
