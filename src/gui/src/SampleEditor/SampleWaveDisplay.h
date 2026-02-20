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

#include "../Widgets/WaveDisplay.h"
#include "SampleEditor.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include <memory>

namespace H2Core {
class Sample;
}

/** \ingroup docGUI*/
class SampleWaveDisplay : public WaveDisplay,
						  public H2Core::Object<SampleWaveDisplay> {
	H2_OBJECT( SampleWaveDisplay )
	Q_OBJECT

   public:
	static constexpr int nWidth = 624;
	static constexpr int nHeight = 132;

	explicit SampleWaveDisplay(
		SampleEditor* pParent,
		WaveDisplay::Channel channel
	);
	~SampleWaveDisplay();

	void updateDisplay( std::shared_ptr<H2Core::Sample> pNewSample );
	void updateDisplayPointer();

	void paintLocatorEvent( int pos, bool last_event );

	void paintEvent( QPaintEvent* ev ) override;

	void testPositionFromSampleeditor();

	int m_nStartFramePosition;
	int m_nLoopFramePosition;
	int m_nEndFramePosition;

	bool m_bStartSliderIsMoved;
	bool m_bLoopSliderIsMoved;
	bool m_bEndSliderIsmoved;

	SampleEditor::Slider m_selectedSlider;

   private:
	void mouseMoveEvent( QMouseEvent* ev ) override;
	void mousePressEvent( QMouseEvent* ev ) override;
	void mouseReleaseEvent( QMouseEvent* ev ) override;
	void testPosition( QMouseEvent* ev );
	void chooseSlider( QMouseEvent* ev );
	void mouseUpdateDone();

	SampleEditor* m_pSampleEditor;

	int m_nSampleLength;
	int m_nLocator;
	bool m_bUpdatePosition;
};

#endif
