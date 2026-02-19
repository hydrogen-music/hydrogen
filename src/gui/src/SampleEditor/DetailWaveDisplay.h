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

#include "SampleEditor.h"
#include "../Widgets/WaveDisplay.h"

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include <memory>
#include <vector>

/** \ingroup docGUI*/
class DetailWaveDisplay
	: public WaveDisplay,
	  public H2Core::Object<DetailWaveDisplay> {
	H2_OBJECT( DetailWaveDisplay )
	Q_OBJECT

   public:
	static constexpr int nWidth = 180;
	static constexpr int nHeight = 132;

	enum class Channel { Left, Right };

	explicit DetailWaveDisplay( SampleEditor* pParent, Channel channel );
	~DetailWaveDisplay();

	void paintEvent( QPaintEvent* ev ) override;

   private:
	void drawPeakData() override;
	void updatePeakData() override;

	SampleEditor* m_pSampleEditor;

	Channel m_channel;
};

#endif
