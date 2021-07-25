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

#ifndef CPU_LOAD_WIDGET_H
#define CPU_LOAD_WIDGET_H


#include <iostream>

#include "../EventListener.h"
#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

///
/// Shows CPU load
///
class CpuLoadWidget : public QWidget, public EventListener, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

	public:
		explicit CpuLoadWidget(QWidget *pParent );
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
