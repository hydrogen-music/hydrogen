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
#ifndef MIXER_LINE_H
#define MIXER_LINE_H

#include <QtGui>
#include <QtWidgets>

#include <memory.h>
#include <vector>

#include "../Widgets/PixmapWidget.h"

#include <core/EventQueue.h>
#include <core/Globals.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

class Button;
class Fader;
class InstrumentNameWidget;
class LCDDisplay;
class LED;
class Rotary;
class WidgetWithInput;

namespace H2Core {
	class Instrument;
}

///
/// A mixer strip
///
/** \ingroup docGUI*/
class MixerLine: public PixmapWidget, public H2Core::Object<MixerLine>
{
	H2_OBJECT(MixerLine)
	Q_OBJECT

public:
		static constexpr int nWidth = 56;
		static constexpr int nHeight = 254;
		/** How many peak update cycles the sample activation LED will be
		 * activated / flash on incoming NoteOn events. */
		static constexpr int nCyclesSampleActivationLED = 2;

	MixerLine( QWidget* pParent, std::shared_ptr<H2Core::Instrument> pInstrument );
	~MixerLine();

		void updateLine();
		void updatePeaks();
		void updateSelected();

		std::shared_ptr<H2Core::Instrument> getInstrument() const;
		void setInstrument( std::shared_ptr<H2Core::Instrument> pInstrument );

		/** Activates the corresponding LED widget for a short amount of time. */
		void triggerSampleLED();

private:
		int retrieveLineNumber() const;
		void updateActions();

		std::shared_ptr<H2Core::Instrument> m_pInstrument;

		/** For how many more peak update cycles to flash the sample activate
		 * LED. */
		int m_nCycleSampleActivation;
		/** For how many more peak update cycles to keep the same text in the
		 * peak level display. */
		int m_nCycleKeepPeakText;
		float	m_fOldMaxPeak;

	Fader *					m_pFader;
	Rotary*					m_pPanRotary;
	InstrumentNameWidget *	m_pNameWidget;
	Button *			m_pMuteBtn;
	Button *			m_pSoloBtn;
	Button *				m_pPlaySampleBtn;
	LED*				m_pTriggerSampleLED;
	LED*				m_pSelectionLED;
	std::vector<Rotary*> m_fxRotaries;

	LCDDisplay *			m_pPeakLCD;
};

inline std::shared_ptr<H2Core::Instrument> MixerLine::getInstrument() const {
	return m_pInstrument;
}

#endif
