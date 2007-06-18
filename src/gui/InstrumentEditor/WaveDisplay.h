/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: WaveDisplay.h,v 1.6 2005/05/09 18:11:47 comix Exp $
 *
 */

#ifndef WAVE_DISPLAY
#define WAVE_DISPLAY

#include <qwidget.h>
#include <qpixmap.h>

#include "lib/Object.h"

class InstrumentLayer;

class WaveDisplay : public QWidget, public Object
{
	Q_OBJECT

	public:
		WaveDisplay(QWidget* pParent);
		~WaveDisplay();

		void updateDisplay( InstrumentLayer *pLayer );

		void paintEvent(QPaintEvent *ev);

	private:
		QPixmap m_background;
		QPixmap m_temp;
		bool m_bChanged;

		string m_sSampleName;

		int *m_pPeakData;
};


#endif
