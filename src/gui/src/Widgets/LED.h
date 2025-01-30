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


#ifndef LED_H
#define LED_H

#include <chrono>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>

#include "../EventListener.h"

/**
 * LED identicating a user selection.
 */
/** \ingroup docGUI docWidgets*/
class LED : public QWidget, public H2Core::Object<LED>
{
    H2_OBJECT(LED)
	Q_OBJECT

public:
	LED( QWidget *pParent, QSize size );
	virtual ~LED();
	
	LED(const LED&) = delete;
	LED& operator=( const LED& rhs ) = delete;
 
	bool getActivated() const;
	void setActivated( bool bActivated );

protected:
	QSvgRenderer* m_background;
	
	bool m_bActivated;
	virtual void paintEvent( QPaintEvent* ev) override;

};

inline bool LED::getActivated() const {
	return m_bActivated;
}

/** Custom LED that comes with its own timer.*/
/** \ingroup docGUI docWidgets*/
class MetronomeLED : public LED, public EventListener, public H2Core::Object<MetronomeLED>
{
    H2_OBJECT(MetronomeLED)
	Q_OBJECT

public:
	MetronomeLED( QWidget *pParent, QSize size );
	virtual ~MetronomeLED();

public slots:
	virtual void metronomeEvent( int nValue ) override;
						   
private slots:
	void turnOff();
	
private:
	bool m_bFirstBar;
	QTimer* m_pTimer;
	std::chrono::milliseconds m_activityTimeout;
	
	virtual void paintEvent( QPaintEvent* ev) override; 
};

#endif
