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

#ifndef INSTRUMENT_EDITOR_H
#define INSTRUMENT_EDITOR_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

class Button;
class ClickableLabel;
class InstrumentEditorPanel;
class LCDDisplay;
class LCDSpinBox;
class Rotary;

/** Editor for all general #H2core::Instrument properties affecting all
 * components and layers.
 *
 * \ingroup docGUI*/
class InstrumentEditor :  public QWidget,
						  protected WidgetWithScalableFont<10, 12, 14>,
						  public H2Core::Object<InstrumentEditor>
{
	H2_OBJECT(InstrumentEditor)
	Q_OBJECT

	public:
		static constexpr int nMargin = 5;

		explicit InstrumentEditor( InstrumentEditorPanel* pPanel );
		~InstrumentEditor();

		void updateColors();
		void updateEditor();

	private:
		InstrumentEditorPanel* m_pInstrumentEditorPanel;

		void updateActivation();

		PixmapWidget *m_pInstrumentProp;
		ClickableLabel *m_pNameLbl;

		// ADSR
		Rotary *m_pAttackRotary;
		Rotary *m_pDecayRotary;
		Rotary *m_pSustainRotary;
		Rotary *m_pReleaseRotary;
		ClickableLabel* m_pAttackLbl;
		ClickableLabel* m_pDecayLbl;
		ClickableLabel* m_pSustainLbl;
		ClickableLabel* m_pReleaseLbl;

		// Instrument pitch
		Rotary *m_pPitchCoarseRotary;
		Rotary *m_pPitchFineRotary;
		Rotary *m_pRandomPitchRotary;
		LCDDisplay *m_pPitchLCD;
		ClickableLabel* m_pPitchLbl;
		ClickableLabel* m_pPitchCoarseLbl;
		ClickableLabel* m_pPitchFineLbl;
		ClickableLabel* m_pPitchRandomLbl;

		// Low pass filter
		Button *m_pFilterBypassBtn;
		Rotary *m_pCutoffRotary;
		Rotary *m_pResonanceRotary;
		ClickableLabel* m_pCutoffLbl;
		ClickableLabel* m_pResonanceLbl;

		// Instrument gain
		LCDDisplay *m_pInstrumentGainLCD;
		Rotary *m_pInstrumentGain;
		ClickableLabel *m_pGainLbl;

		QCheckBox *m_pApplyVelocity;
		ClickableLabel *m_pApplyVelocityLbl;
		QCheckBox* m_pIsStopNoteCheckBox;
		ClickableLabel *m_pIsStopNoteLbl;

		// Instrument mute group
		LCDSpinBox *m_pMuteGroupLCD;
		ClickableLabel *m_pMuteGroupLbl;

		// Instrument midi out
		LCDSpinBox *m_pMidiOutChannelLCD;
		ClickableLabel* m_pMidiOutChannelLbl;

		LCDSpinBox *m_pMidiOutNoteLCD;
		ClickableLabel* m_pMidiOutNoteLbl;

		// Instrument hihat

		LCDSpinBox *m_pHihatGroupLCD;
		ClickableLabel* m_pHihatGroupLbl;

		LCDSpinBox *m_pHihatMinRangeLCD;
		ClickableLabel* m_pHihatMinRangeLbl;

		LCDSpinBox *m_pHihatMaxRangeLCD;
		ClickableLabel* m_pHihatMaxRangeLbl;
};

#endif
