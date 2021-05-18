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
#include <QtWidgets>
#include <QSvgRenderer>
#include <QColor>

class LCDDisplay;

#include <core/Object.h>

class Rotary : public QWidget, public H2Core::Object, public MidiLearnable
{
    H2_OBJECT
	Q_OBJECT
	
	public:
	enum RotaryType {
		TYPE_NORMAL,
		TYPE_CENTER,
		TYPE_SMALL
	};

	Rotary(const Rotary&) = delete;
	Rotary& operator=( const Rotary& rhs ) = delete;
	
	Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseIntSteps, bool bUseValueTip, float fMin = 0.0, float fMax = 1.0, QColor color = QColor( 255, 0, 0 ) );
	~Rotary();

	void setMin( float fMin );
	float getMin() const;

	void setMax( float fMax );
	float getMax() const;

	void setValue( float fValue );
	float getValue() const {
		if ( m_bUseIntSteps ) {
			int val = static_cast<int>(m_fValue);
			return val;
		}
		else
			return m_fValue;
	}

	void setDefaultValue( float fDefaultValue );
	float getDefaultValue() const;
	void resetValueToDefault();
	void setColor( QColor color );
	QColor getColor() const;

signals:
	void valueChanged(Rotary *ref);

private:
	bool m_bUseIntSteps;
	bool m_bIgnoreMouseMove;
	QString m_sBaseTooltip;

	RotaryType m_type;
	QSvgRenderer* m_background;
	QColor m_color;

	int m_nWidgetWidth;
	int m_nWidgetHeight;

	int m_nScrollSpeedSlow;
	int m_nScrollSpeedFast;

	float m_fMin;
	float m_fMax;
	float m_fValue;
	float m_fDefaultValue;

	float m_fMousePressValue;
	float m_fMousePressY;

	virtual void paintEvent(QPaintEvent *ev);
	virtual void mousePressEvent(QMouseEvent *ev);
	virtual void mouseReleaseEvent( QMouseEvent *ev );
	virtual void mouseMoveEvent(QMouseEvent *ev);
	virtual void wheelEvent( QWheelEvent *ev );
};

inline float Rotary::getMin() const {
	return m_fMin;
}
inline float Rotary::getMax() const {
	return m_fMax;
}
inline float Rotary::getDefaultValue() const {
	return m_fDefaultValue;
}
inline QColor Rotary::getColor() const {
	return m_color;
}
#endif
