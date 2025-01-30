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

#ifndef SAMPLE_WAVE_DISPLAY
#define SAMPLE_WAVE_DISPLAY

#include <QtWidgets>

#include <core/Object.h>



/** \ingroup docGUI*/
class SampleWaveDisplay :  public QWidget,  public H2Core::Object<SampleWaveDisplay>
{
	H2_OBJECT(SampleWaveDisplay)
	Q_OBJECT

	public:
		explicit SampleWaveDisplay( QWidget* pParent );
		~SampleWaveDisplay();

		void updateDisplay( const QString& filename );

		virtual void paintEvent(QPaintEvent *ev) override;

	private:
		QPixmap m_Background;
		QString m_sSampleName;
		int *	m_pPeakData;
};


#endif

