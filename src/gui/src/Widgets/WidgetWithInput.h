/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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

#ifndef WIDGET_WITH_INPUT_H
#define WIDGET_WITH_INPUT_H

#include <QtGui>
#include <QtWidgets>

#include "MidiLearnable.h"
#include "MidiSenseWidget.h"

#include <core/Timehelper.h>

class WidgetWithInput : public QWidget, public MidiLearnable {
	Q_OBJECT

public:
	WidgetWithInput( QWidget* parent, bool bUseIntSteps, QString sBaseTooltip, int nScrollSpeed, int nScrollSpeedFast, float fMin, float fMax );
	~WidgetWithInput();
	void setMin( float fMin );
	float getMin() const;

	void setMax( float fMax );
	float getMax() const;

	virtual void setValue( float fValue );
	float getValue() const;

	void setDefaultValue( float fDefaultValue );
	float getDefaultValue() const;
	void resetValueToDefault();
	
	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	QString getBaseTooltip() const;
	void setBaseTooltip( const QString& sBaseTooltip );

	int getWidgetWidth() const;
	void setWidgetWidth( int nWidgetWidth );
	int getWidgetHeight() const;
	void setWidgetHeight( int nWidgetHeight );

	int getScrollSpeed() const;
	void setScrollSpeed( int nScrollSpeed ) const;
	int getScrollSpeedFast() const;
	void setScrollSpeedFast( int nScrollSpeedFast ) const;

signals:
	void valueChanged(WidgetWithInput *ref);	

protected:
	virtual void mousePressEvent(QMouseEvent *ev);
	virtual void mouseReleaseEvent( QMouseEvent *ev );
	virtual void mouseMoveEvent(QMouseEvent *ev);
	virtual void wheelEvent( QWheelEvent *ev );
	virtual void enterEvent( QEvent *ev );
	virtual void leaveEvent( QEvent *ev );
	virtual void keyPressEvent( QKeyEvent *ev );
	
	bool m_bUseIntSteps;
	QString m_sBaseTooltip;
	
	int m_nWidgetWidth;
	int m_nWidgetHeight;

	int m_nScrollSpeed;
	// Fast version used when the Control modifier is pressed.
	int m_nScrollSpeedFast;

	float m_fMin;
	float m_fMax;
	float m_fValue;
	float m_fDefaultValue;
	
	bool m_bIsActive;
	bool m_bEntered;
	
	bool m_bIgnoreMouseMove;
	float m_fMousePressValue;
	float m_fMousePressY;

	// All key input will be appended to this string.
	QString m_sInputBuffer;
	timeval m_inputBufferTimeval;
	// Number of seconds before #m_sInputBuffer will be flushed
	// (happens asynchronically whenever the next key input occurs.)
	double m_inputBufferTimeout;

};

inline float WidgetWithInput::getValue() const {
	return m_fValue;
}
inline float WidgetWithInput::getMin() const {
	return m_fMin;
}
inline float WidgetWithInput::getMax() const {
	return m_fMax;
}
inline float WidgetWithInput::getDefaultValue() const {
	return m_fDefaultValue;
}
inline bool WidgetWithInput::getIsActive() const {
	return m_bIsActive;
}
inline QString WidgetWithInput::getBaseTooltip() const {
	return m_sBaseTooltip;
}
inline int WidgetWithInput::getWidgetWidth() const {
	return m_nWidgetWidth;
}
inline int WidgetWithInput::getWidgetHeight() const {
	return m_nWidgetHeight;
}
inline int WidgetWithInput::getScrollSpeed() const {
	return m_nScrollSpeed;
}
inline int WidgetWithInput::getScrollSpeedFast() const {
	return m_nScrollSpeedFast;
}

#endif
