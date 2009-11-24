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

#ifndef TARGET_WAVE_DISPLAY
#define TARGET_WAVE_DISPLAY

#include <QtGui>
#include <hydrogen/Object.h>
#include <hydrogen/sample.h>

class SampleEditor;

namespace H2Core
{
	class InstrumentLayer;
}

class TargetWaveDisplay : public QWidget, public Object
{
	Q_OBJECT

	public:
		TargetWaveDisplay(QWidget* pParent);
		~TargetWaveDisplay();

		void updateDisplay( H2Core::InstrumentLayer *pLayer );
		void updateDisplayPointer();
		void paintLocatorEventTargetDisplay( int pos, bool last_event);
		void paintEvent(QPaintEvent *ev);
//		int m_pFadeOutFramePosition;

	private:
		QPixmap m_background;
		QString m_sSampleName;
		int *m_pPeakDatal;
		int *m_pPeakDatar;
		unsigned m_pSampleLength;
		bool m_pvmove;
		QString m_info;
		int m_x;
		int m_y;
		int m_plocator;
		bool m_pupdateposi;
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		
};


#endif
