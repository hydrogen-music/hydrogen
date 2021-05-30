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


#ifndef LED_H
#define LED_H


#include <core/Object.h>
#include <core/Preferences.h>

#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>


/**
 * LED identicating a user selection.
 */
class LED : public QWidget, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

public:
	LED( QWidget *pParent, QSize size );
	virtual ~LED();
	
	LED(const LED&) = delete;
	LED& operator=( const LED& rhs ) = delete;
 
	bool getActivated() const;
	void setActivated( bool bActivated );

private:
	QSvgRenderer* m_background;
	
	bool m_bActivated;
	void paintEvent( QPaintEvent* ev);

};

inline bool LED::getActivated() const {
	return m_bActivated;
}

#endif
