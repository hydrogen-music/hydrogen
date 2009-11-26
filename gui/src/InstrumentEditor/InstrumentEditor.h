/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 */

#ifndef INSTRUMENT_EDITOR_DIALOG_H
#define INSTRUMENT_EDITOR_DIALOG_H

#include "config.h"

#include <QtGui>

#include <hydrogen/instrument.h>
#include <hydrogen/Object.h>

#include "../EventListener.h"
#include "../widgets/PixmapWidget.h"

class Fader;
class LCDDisplay;
class Button;
class ToggleButton;
class ClickableLabel;
class Rotary;
class WaveDisplay;
class LayerPreview;


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
		void setFileforLayer(QString filename );

		// implements EventListener interface
		virtual void selectedInstrumentChangedEvent();
		virtual void rubberbandbpmchangeEvent();
		//~ implements EventListener interface

	private slots:
		void rotaryChanged(Rotary *ref);
		void filterActiveBtnClicked(Button *ref);
		void buttonClicked(Button*);
		void labelClicked( ClickableLabel* pRef );

		void muteGroupBtnClicked(Button *pRef);
		void onIsStopNoteCheckBoxClicked( bool on );
		void midiOutChannelBtnClicked(Button *pRef);
		void midiOutNoteBtnClicked(Button *pRef);

	private:
		H2Core::Instrument *m_pInstrument;
		int m_nSelectedLayer;

		ToggleButton *m_pShowInstrumentBtn;
		ToggleButton *m_pShowLayersBtn;

		// Instrument properties
		PixmapWidget *m_pInstrumentProp;
		PixmapWidget *m_pInstrumentPropTop;
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

		// Instrument mute group
		LCDDisplay *m_pMuteGroupLCD;
		Button *m_pAddMuteGroupBtn;
		Button *m_pDelMuteGroupBtn;
		
		// Instrument midi out
		LCDDisplay *m_pMidiOutChannelLCD;
		Button *m_pAddMidiOutChannelBtn;
		Button *m_pDelMidiOutChannelBtn;
		
		LCDDisplay *m_pMidiOutNoteLCD;
		Button *m_pAddMidiOutNoteBtn;
		Button *m_pDelMidiOutNoteBtn;		


		//~ Instrument properties

		// Layer properties
		LayerPreview *m_pLayerPreview;
		QScrollArea *m_pLayerScrollArea;
		

		PixmapWidget *m_pLayerProp;
		Rotary *m_pLayerGainRotary;
		LCDDisplay *m_pLayerGainLCD;

		Rotary *m_pLayerPitchCoarseRotary;
		Rotary *m_pLayerPitchFineRotary;

		LCDDisplay *m_pLayerPitchCoarseLCD;
		LCDDisplay *m_pLayerPitchFineLCD;

		WaveDisplay *m_pWaveDisplay;

		Button *m_pLoadLayerBtn;
		Button *m_pRemoveLayerBtn;
		Button *m_pSamleEditorBtn;
		QCheckBox *m_pIsStopNoteCheckBox;
		//~ Layer properties




		void loadLayer();
		void setAutoVelocity();
};


#endif
