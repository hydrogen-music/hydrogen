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

#ifndef CPU_LOAD_WIDGET_H
#define CPU_LOAD_WIDGET_H


#include <iostream>

#include "../EventListener.h"
#include <hydrogen/object.h>

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

///
/// Shows CPU load
///
class CpuLoadWidget : public QWidget, public EventListener, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

	public:
		CpuLoadWidget(QWidget *pParent );
		~CpuLoadWidget();

		void setValue( float newValue );
		float getValue();

		void mousePressEvent(QMouseEvent *ev);
		void paintEvent(QPaintEvent *ev);

		void XRunEvent();

	public slots:
		void updateCpuLoadWidget();

	private:
		float m_fValue;
		uint m_nXRunValue;

		QPixmap m_back;
		QPixmap m_leds;
};


#endif
