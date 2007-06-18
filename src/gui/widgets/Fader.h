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
 * $Id: Fader.h,v 1.12 2005/07/11 10:29:55 comix Exp $
 *
 */


#ifndef FADER_H
#define FADER_H

#include <string>
#include <iostream>

#include "config.h"

#include <qwidget.h>
#include <qpixmap.h>

#include "lib/Object.h"

///
/// Fader and VuMeter widget
///
class Fader : public QWidget, public Object
{
	Q_OBJECT

	public:
		Fader(QWidget * parent, bool bWithoutKnob = false);
		~Fader();

		void setValue(int newValue);
		uint getValue();

		void updateFader();

		void setPeak_L( float peak );
		float getPeak_L() {	return peakValue_L;	}

		void setPeak_R( float peak );
		float getPeak_R() {	return peakValue_R;	}

		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );
		virtual void paintEvent(QPaintEvent *ev);

	signals:
		void valueChanged(Fader *ref);

	private:
		bool m_bWithoutKnob;
		float peakValue_L;
		float peakValue_R;

		int m_nValue;

		QPixmap temp;
		QPixmap back;
		QPixmap leds;
		QPixmap knob;

		bool changed;

};




class MasterFader : public QWidget, public Object
{
	Q_OBJECT

	public:
		MasterFader(QWidget * parent, bool bWithoutKnob = false);
		~MasterFader();

		void setValue(int newValue);
		int getValue();

		void updateFader();

		void setPeak_L( float peak );
		float getPeak_L() {	return peakValue_L;	}

		void setPeak_R( float peak );
		float getPeak_R() {	return peakValue_R;	}

		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void paintEvent(QPaintEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );

	signals:
		void valueChanged(MasterFader *ref);

	private:
		bool m_bWithoutKnob;
		float peakValue_L;
		float peakValue_R;

		int m_nValue;

		QPixmap temp;
		QPixmap back;
		QPixmap leds;
		QPixmap knob;

		bool changed;

};



/*
class PanFader : public QWidget, public Object
{
	Q_OBJECT

	public:
		PanFader(QWidget *parent);
		~PanFader();

		void updateFader();

		void setValue(float newValue);
		float getValue() {	return value;	}

	signals:
		void valueChanged(PanFader *ref);

	private:
		float value;

		QPixmap back;
		QPixmap temp;

		bool changed;

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);

};
*/


class Knob : public QWidget, public Object
{
	Q_OBJECT
	public:
		Knob( QWidget* parent );
		~Knob();

		void setValue( float fValue );
		float getValue() {	return m_fValue;	}

		void updateKnob();

	signals:
		void valueChanged(Knob *ref);

	private:
		static QPixmap* m_background;
		QPixmap m_temp;

		bool m_bChanged;
		int m_nWidgetWidth;
		int m_nWidgetHeight;

		float m_fValue;
		float m_fMousePressValue;
		float m_fMousePressY;

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent( QMouseEvent *ev );
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );
};


#endif
