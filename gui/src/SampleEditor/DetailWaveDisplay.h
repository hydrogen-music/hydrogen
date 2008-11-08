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

#ifndef DETAIL_WAVE_DISPLAY
#define DETAIL_WAVE_DISPLAY

#include <QtGui>
#include <hydrogen/Object.h>

namespace H2Core
{
	class Sample;
}

class DetailWaveDisplay : public QWidget, public Object
{
	Q_OBJECT

	public:
		DetailWaveDisplay(QWidget* pParent);
		~DetailWaveDisplay();

		void updateDisplay( QString filename );

		void paintEvent(QPaintEvent *ev);
		void setDetailSamplePosition( unsigned posi, float zoomfactor, QString type);

	private:
		QPixmap m_background;
		QString m_sSampleName;
		int *m_pPeakDatal;
		int *m_pPeakDatar;
		int m_pDetailSamplePosition; 
		int m_pnormalimagedetailframes;
		float m_pzoomFactor;
		QString m_ptype;
};


#endif
