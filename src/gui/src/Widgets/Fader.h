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


#ifndef FADER_H
#define FADER_H


#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>

#include "WidgetWithInput.h"

#include <core/Object.h>

///
/// Fader and VuMeter widget
///
class Fader : public WidgetWithInput, public H2Core::Object
{
    H2_OBJECT

public:
	enum class Type {
		Normal,
		Vertical,
		Master
	};
	
	Fader( QWidget *pParent, Type type, QString sBaseTooltip, bool bUseIntSteps = false, bool bWithoutKnob = false, float fMin = 0.0, float fMax = 1.0 );
	~Fader();

	void setMaxPeak( float fMax );
	void setMinPeak( float fMin );

	void setPeak_L( float peak );
	float getPeak_L() const {	return m_fPeakValue_L;	}

	void setPeak_R( float peak );
	float getPeak_R() const {	return m_fPeakValue_R;	}

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );

private:
	bool m_bWithoutKnob;
	Type m_type;
	QSvgRenderer* m_pBackground;
	QSvgRenderer* m_pKnob;

	float m_fPeakValue_L;
	float m_fPeakValue_R;
	float m_fMinPeak;
	float m_fMaxPeak;

	QColor m_lastHighlightColor;
	QColor m_lastLightColor;
	
	virtual void mouseMoveEvent(QMouseEvent *ev);
	virtual void mousePressEvent(QMouseEvent *ev);
	virtual void paintEvent(QPaintEvent *ev);
};
#endif
