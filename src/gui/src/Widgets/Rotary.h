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

#include "MidiLearnable.h"
#include "MidiSenseWidget.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

class LCDDisplay;

#include <hydrogen/object.h>

/** \ingroup docGUI docWidgets */
class RotaryTooltip : public QWidget
{
	public:
		RotaryTooltip( QPoint pos );
		~RotaryTooltip();
		void showTip( QPoint pos, QString sText );

	private:
		LCDDisplay *m_pDisplay;
};



/** \ingroup docGUI docWidgets */
class Rotary : public QWidget, public H2Core::Object, public MidiLearnable
{
	Q_OBJECT
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
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

		void setDefaultValue( float fDefaultValue );
		float getDefaultValue();
		void resetValueToDefault();

	signals:
		void valueChanged(Rotary *ref);

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		bool m_bUseIntSteps;
		bool m_bIgnoreMouseMove;

		RotaryType m_type;
		static QPixmap* m_background_normal;
		static QPixmap* m_background_center;

		int m_nWidgetWidth;
		int m_nWidgetHeight;

		float m_fMin;
		float m_fMax;
		float m_fValue;
		float m_fDefaultValue;

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
