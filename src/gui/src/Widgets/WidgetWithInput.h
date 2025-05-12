/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <memory>

#include <QtGui>
#include <QtWidgets>

#include "MidiLearnable.h"

#include <core/Basics/Event.h>
#include <core/Timehelper.h>

/** Base class for active user input widget, which are not based on
 * a high-level Qt widget.
 *
 * The widgets can be set by click-drag, wheel event, and by
 * keyboard. For the latter the widget has to be clicked first, in
 * order for it to acquire focus. The derived class must indicate the
 * presence of the focus in its paintEvent() using the
 * H2Core::ColorTheme::m_highlightColor.
 *
 * The widget will be reset to its default value of Ctrl-clicking
 * it. It's MIDI learnable and the MIDI action - added by the parent -
 * can be bound by the user by Shift-clicking it. The derived class
 * must display an available MIDI action and a possible binding in its
 * tooltip.
 *
 * The current value of the derived class has to be displayed in the
 * tooltip and when altering the value via mouse or keyboard a static
 * tooltip must be used to indicate the new value.
 *
 * For keyboard input a buffer is used to accumulate all provided
 * numbers. After 2 seconds the input buffer is flushed and the next
 * key press will fill a fresh buffer. Alternatively, the user can use
 * the ESC key to immediately flush the input buffer.
 */
class WidgetWithInput : public QWidget, public MidiLearnable {
	Q_OBJECT

public:
	WidgetWithInput( QWidget* parent, bool bUseIntSteps,
					 const QString& sBaseTooltip, int nScrollSpeed,
					 int nScrollSpeedFast, float fMin, float fMax,
					 bool bModifyOnChange );
	~WidgetWithInput();
	void setMin( float fMin );
	float getMin() const;

	void setMax( float fMax );
	float getMax() const;

	virtual void setValue( float fValue, bool bTriggeredByUserInteraction = false,
						   H2Core::Event::Trigger trigger =
						      H2Core::Event::Trigger::Default );
	float getValue() const;

	void setDefaultValue( float fDefaultValue );
	float getDefaultValue() const;
	void resetValueToDefault();
	
	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	const QString& getBaseTooltip() const;
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
	virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent(QMouseEvent *ev) override;
	virtual void wheelEvent( QWheelEvent *ev ) override;
	virtual void enterEvent( QEnterEvent *ev ) override;
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void keyPressEvent( QKeyEvent *ev ) override;

	void updateTooltip() override;
	
	bool m_bUseIntSteps;
	QString m_sBaseTooltip;
	
	int m_nWidgetWidth;
	int m_nWidgetHeight;

	int m_nScrollSpeed;
	/** Fast version used when the Control modifier is pressed.*/
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

	/** All key input will be appended to this string.*/
	QString m_sInputBuffer;
	timeval m_inputBufferTimeval;
	/** Number of seconds before #m_sInputBuffer will be flushed
		(happens asynchronically whenever the next key input occurs.)*/
	double m_inputBufferTimeout;

	/** Whether Hydrogen::setIsModified() is invoked with `true` as
		soon as the value of the widget does change.*/
	bool m_bModifyOnChange;
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
inline const QString& WidgetWithInput::getBaseTooltip() const {
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
