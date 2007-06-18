/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: InstrumentEditor.h,v 1.12 2005/05/09 18:11:45 comix Exp $
 *
 */

#ifndef INSTRUMENT_EDITOR_DIALOG_H
#define INSTRUMENT_EDITOR_DIALOG_H

#include <qlistbox.h>

#include "lib/Object.h"

#include "gui/EventListener.h"

class Fader;
class LCDDisplay;
class Button;
class ToggleButton;
class ClickableLabel;
class Rotary;
class WaveDisplay;
class LayerPreview;
class Instrument;

///
/// Instrument Editor
///
class InstrumentEditor : public QWidget, public Object, public EventListener
{
	Q_OBJECT

	public:
		InstrumentEditor( QWidget* parent );
		~InstrumentEditor();

		void selectLayer( int nLayer );

	public slots:
		void rotaryChanged(Rotary *ref);
		void filterActiveBtnClicked(Button *ref);
		void buttonClicked(Button*);
		void labelClicked( ClickableLabel* pRef );

	private:
		Instrument *m_pInstrument;
		int m_nSelectedLayer;

		ToggleButton *m_pShowInstrumentBtn;
		ToggleButton *m_pShowLayersBtn;

		// Instrument properties
		QWidget *m_pInstrumentProp;
		ClickableLabel *m_pNameLbl;

		// ADSR
		Rotary *m_pAttackRotary;
		Rotary *m_pDecayRotary;
		Rotary *m_pSustainRotary;
		Rotary *m_pReleaseRotary;

		// Random pitch
		Rotary *m_pRandomPitchRotary;

		// Low pass filter
		ToggleButton *m_pFilterBypassBtn;
		Rotary *m_pCutoffRotary;
		Rotary *m_pResonanceRotary;

		// Instrument gain
		LCDDisplay *m_pInstrumentGainLCD;
		Rotary *m_pInstrumentGain;

		//~ Instrument properties

		// Layer properties
		LayerPreview *m_pLayerPreview;

		QWidget *m_pLayerProp;
		Rotary *m_pLayerGainRotary;
		LCDDisplay *m_pLayerGainLCD;

		Rotary *m_pLayerPitchRotary;
		LCDDisplay *m_pLayerPitchLCD;

		WaveDisplay *m_pWaveDisplay;

		Button *m_pLoadLayerBtn;
		Button *m_pRemoveLayerBtn;
		//~ Layer properties


		// implements EventListener interface
		virtual void selectedInstrumentChangedEvent();
		//~ implements EventListener interface

		void loadLayer();
};


#endif


