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


#ifndef FADER_H
#define FADER_H


#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include "MidiLearnable.h"

///
/// Fader and VuMeter widget
///
class Fader :  public QWidget,  public H2Core::Object<Fader>, public MidiLearnable
{
    H2_OBJECT(Fader)
	Q_OBJECT

	public:
		Fader(QWidget *pParent, bool bUseIntSteps, bool bWithoutKnob );
		~Fader();

		void setMinValue( float fMin );
		void setMaxValue( float fMax );
		float getMinValue() {	return m_fMinValue;	}
		float getMaxValue() {	return m_fMaxValue;	}

		void setValue( float fVal );
		float getValue();

		void setDefaultValue( float fDefaultValue );
		float getDefaultValue() { return m_fDefaultValue; }
		void resetValueToDefault();

		void setMaxPeak( float fMax );
		void setMinPeak( float fMin );

		void setPeak_L( float peak );
		float getPeak_L() {	return m_fPeakValue_L;	}

		void setPeak_R( float peak );
		float getPeak_R() {	return m_fPeakValue_R;	}

		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void wheelEvent( QWheelEvent *ev );
		virtual void paintEvent(QPaintEvent *ev);

	signals:
		void valueChanged(Fader *ref);

	protected:
		bool m_bWithoutKnob;
		bool m_bUseIntSteps;
		bool m_bIgnoreMouseMove;

		float m_fPeakValue_L;
		float m_fPeakValue_R;
		float m_fMinPeak;
		float m_fMaxPeak;

		float m_fValue;
		float m_fMinValue;
		float m_fMaxValue;
		float m_fDefaultValue;

		QPixmap m_back;
		QPixmap m_leds;
		QPixmap m_knob;
};

class VerticalFader : public Fader
{
	Q_OBJECT
	
public:
		VerticalFader(QWidget *pParent, bool bUseIntSteps, bool bWithoutKnob );
		~VerticalFader();
		
		virtual void paintEvent(QPaintEvent *ev) override;
		virtual void mouseMoveEvent(QMouseEvent *ev) override;	
};


class MasterFader :  public QWidget,  public H2Core::Object<MasterFader>, public MidiLearnable
{
    H2_OBJECT(MasterFader)
	Q_OBJECT

	public:
		explicit MasterFader(QWidget *pParent, bool bWithoutKnob = false);
		~MasterFader();

		void setMin( float fMin );
		void setMax( float fMax );
		float getMin() {	return m_fMin;	}
		float getMax() {	return m_fMax;	}

		void setValue( float newValue );
		float getValue();

		void setDefaultValue( float fDefaultValue );
		float getDefaultValue() { return m_fDefaultValue; }
		void resetValueToDefault();

		void setPeak_L( float peak );
		float getPeak_L() {	return m_fPeakValue_L;	}

		void setPeak_R( float peak );
		float getPeak_R() {	return m_fPeakValue_R;	}

		virtual void mousePressEvent( QMouseEvent *ev );
		virtual void mouseMoveEvent( QMouseEvent *ev );
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void paintEvent( QPaintEvent *ev );
		virtual void wheelEvent( QWheelEvent *ev );

	signals:
		void valueChanged( MasterFader *ref );

	private:
		bool m_bWithoutKnob;
		bool m_bIgnoreMouseMove;

		float m_fPeakValue_L;
		float m_fPeakValue_R;

		float m_fValue;
		float m_fMin;
		float m_fMax;
		float m_fDefaultValue;

		QPixmap m_back;
		QPixmap m_leds;
		QPixmap m_knob;


};
#endif
