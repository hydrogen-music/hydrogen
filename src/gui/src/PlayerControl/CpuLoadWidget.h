/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef CPU_LOAD_WIDGET_H
#define CPU_LOAD_WIDGET_H

#include <chrono>
#include <vector>

#include "../EventListener.h"
#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

/**
 * Shows the current CPU load using a meter similar to the one used in
 * #Fader.
 *
 * All aspects of the widgets are directly drawn.
 *
 * In order to not annoy and to give a better view on the overall CPU
 * load, the widget measures the load at five consecutive points in
 * time and displays the average.
 *
 * In case an XRun event is reported by the JACK server, the outlines
 * of he widget will be painted in red for 1.5 seconds.
 */
/** \ingroup docGUI docWidgets*/
class CpuLoadWidget : public QWidget, public EventListener, public H2Core::Object<CpuLoadWidget>
{
    H2_OBJECT(CpuLoadWidget)
	Q_OBJECT

public:
	explicit CpuLoadWidget( QWidget *pParent );
	~CpuLoadWidget();

private slots:
	void updateCpuLoadWidget();

private:
	std::vector<float> m_recentValues;
	float m_fValue;
	uint m_nXRunValue;
	QSize m_size;
	
	virtual void paintEvent( QPaintEvent *ev ) override;

	virtual void XRunEvent() override;

};


#endif
