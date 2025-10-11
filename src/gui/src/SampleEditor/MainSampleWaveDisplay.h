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

#ifndef MAIN_SAMPLE_WAVE_DISPLAY
#define MAIN_SAMPLE_WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include "SampleEditor.h"
class SampleEditor;

/** \ingroup docGUI*/
class MainSampleWaveDisplay :  public QWidget,  public H2Core::Object<MainSampleWaveDisplay>
{
    H2_OBJECT(MainSampleWaveDisplay)
	Q_OBJECT

	public:

		enum Slider {
			NONE,
			START,
			LOOP,
			END
		};

		explicit MainSampleWaveDisplay(QWidget* pParent);
		~MainSampleWaveDisplay();

		void updateDisplay( const QString& sFileName );
		void updateDisplayPointer();

		void paintLocatorEvent( int pos, bool last_event);
		virtual void paintEvent(QPaintEvent *ev) override;
		
		void testPositionFromSampleeditor();
		
		int		m_nStartFramePosition;
		int		m_nLoopFramePosition;
		int		m_nEndFramePosition;

		bool	m_bStartSliderIsMoved;
		bool	m_bLoopSliderIsMoved;
		bool	m_bEndSliderIsmoved;

		Slider  m_SelectedSlider;


	private:
		virtual void mouseMoveEvent(QMouseEvent *ev) override;
		virtual void mousePressEvent(QMouseEvent *ev) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
		void testPosition( QMouseEvent *ev );
		void chooseSlider( QMouseEvent *ev );
		void mouseUpdateDone();
		
		QPixmap m_background;
		int*	m_pPeakDatal;
		int*	m_pPeakDatar;
		
		int		m_nSampleLength;
		int		m_nLocator;
		bool	m_bUpdatePosition;


};


#endif

