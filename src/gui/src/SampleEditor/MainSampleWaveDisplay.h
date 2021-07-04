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

#ifndef MAIN_SAMPLE_WAVE_DISPLAY
#define MAIN_SAMPLE_WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include "SampleEditor.h"

class MainSampleWaveDisplay : public QWidget, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

	public:
		explicit MainSampleWaveDisplay(QWidget* pParent);
		~MainSampleWaveDisplay();

		std::shared_ptr<H2Core::Sample> loadSampleAndUpdateDisplay( const QString& filename );
		void updateDisplayPointer();

		void paintLocatorEvent( int pos, bool last_event);
		void paintEvent(QPaintEvent *ev);

		void testPositionFromSampleeditor();

		int		m_nStartFramePosition;
		int		m_nLoopFramePosition;
		int		m_nEndFramePosition;


		SampleEditor::Slider m_SelectedSlider;

		std::shared_ptr<H2Core::Sample> getEditedSample() { return m_pEditedSample; }

	signals:
		void doneEditingSlider ( SampleEditor::Slider slider );
		void sliderEdited ( SampleEditor::Slider slider );

	private:
		virtual void mouseMoveEvent(QMouseEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		void testPosition( QMouseEvent *ev );
		void chooseSlider( QMouseEvent *ev );
		void mouseUpdateDone();

		std::shared_ptr<H2Core::Sample> m_pEditedSample;
		QPixmap m_background;
		int*	m_pPeakDatal;
		int*	m_pPeakDatar;

		int		m_nSampleLength;
		int		m_nLocator;
		bool	m_bUpdatePosition;


};


#endif

