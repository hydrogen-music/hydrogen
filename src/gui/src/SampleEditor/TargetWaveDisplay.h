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

#ifndef TARGET_WAVE_DISPLAY
#define TARGET_WAVE_DISPLAY

#include "../Widgets/WaveDisplay.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Basics/Sample.h>
#include <core/Object.h>
#include <memory>

class SampleEditor;

namespace H2Core {
class InstrumentLayer;
class EnvelopePoint;
}  // namespace H2Core

/** \ingroup docGUI*/
class TargetWaveDisplay : public WaveDisplay,
						  public H2Core::Object<TargetWaveDisplay> {
	H2_OBJECT( TargetWaveDisplay )
	Q_OBJECT

   public:
        static constexpr int nHeight = 91;
        static constexpr int nWidth = 841;

	explicit TargetWaveDisplay( SampleEditor* pParent );
	~TargetWaveDisplay();

	void paintLocatorEventTargetDisplay( int nPos );
	virtual void paintEvent( QPaintEvent* ev ) override;

   private:
	void mouseMoveEvent( QMouseEvent* ev ) override;
	void mousePressEvent( QMouseEvent* ev ) override;

	void drawPeakData() override;
	/** Since we displaying pan automation on top of the peak data and want to
	 * provide a visual feedback for the corresponding changes applied, we are
	 * bound to render both the left and right channel of the audio. But we
	 * still want to do so within a single widget. Having two instances of
	 * `WaveDisplay` is off the table since we would have to deal with handling
	 * automation nodes between them. */
	void updatePeakData() override;

	void updateMouseSelection( QMouseEvent* ev );
	void updateEnvelope();

	SampleEditor* m_pSampleEditor;

	QString m_sSelectedEnvelopePointValue;
	int m_nSelectedEnvelopePointX;
	int m_nSelectedEnvelopePointY;

	int m_nLocator;

	int m_nSnapRadius;

	std::vector<int> m_peakDataL;
	std::vector<int> m_peakDataR;

	int m_nSelectedEnvelopePoint;
};

#endif
