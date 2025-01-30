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

#ifndef DETAIL_WAVE_DISPLAY
#define DETAIL_WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

namespace H2Core
{
	class Sample;
}

/** \ingroup docGUI*/
class DetailWaveDisplay :  public QWidget,  public H2Core::Object<DetailWaveDisplay>
{
    H2_OBJECT(DetailWaveDisplay)
	Q_OBJECT

	public:
		explicit DetailWaveDisplay(QWidget* pParent);
		~DetailWaveDisplay();

		void updateDisplay( QString filename );

		virtual void paintEvent(QPaintEvent *ev) override;
		void setDetailSamplePosition( unsigned posi, float zoomfactor, QString type);

	private:
		QPixmap m_background;
		QString m_sSampleName;
		int *m_pPeakDatal;
		int *m_pPeakDatar;
		int m_pDetailSamplePosition;
		int m_pNormalImageDetailFrames;
		float m_pZoomFactor;
		QString m_pType;
};


#endif
