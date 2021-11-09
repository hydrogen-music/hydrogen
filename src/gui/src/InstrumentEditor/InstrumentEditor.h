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

#ifndef INSTRUMENT_EDITOR_DIALOG_H
#define INSTRUMENT_EDITOR_DIALOG_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <core/Basics/Instrument.h>
#include <core/Object.h>
#include <core/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

class Fader;
class LCDDisplay;
class Button;
class ToggleButton;
class ClickableLabel;
class Rotary;
class LCDCombo;
class WaveDisplay;
class LayerPreview;


///
/// Instrument Editor
///
/** \ingroup docGUI*/
class InstrumentEditor :  public QWidget, protected WidgetWithScalableFont<10, 12, 14>,  public H2Core::Object<InstrumentEditor>, public EventListener
{
	H2_OBJECT(InstrumentEditor)
	Q_OBJECT

	public:
		explicit InstrumentEditor( QWidget* parent );
		~InstrumentEditor();

		void selectLayer( int nLayer );
		void setFileforLayer(QString filename );

		void selectComponent( int nComponent );

		// implements EventListener interface
		virtual void selectedInstrumentChangedEvent() override;
		virtual void rubberbandbpmchangeEvent() override;
		//~ implements EventListener interface
		void update();
		static int findFreeDrumkitComponentId( int startingPoint = 0 );


	public slots:
		void showLayers();
		void showInstrument();
		void showSampleEditor();
	void onPreferencesChanged( bool bAppearanceOnly );

	private slots:
		void rotaryChanged(Rotary *ref);
		void filterActiveBtnClicked(Button *ref);
		void buttonClicked(Button*);
		void labelClicked( ClickableLabel* pRef );
		void labelCompoClicked( ClickableLabel* pRef );
		void compoChangeAddDelete(QAction*);
		void onClick(Button*);

		void muteGroupBtnClicked(Button *pRef);
		void onIsStopNoteCheckBoxClicked( bool on );
		void onIsApplyVelocityCheckBoxClicked( bool on);
		void midiOutChannelBtnClicked(Button *pRef);
		void midiOutNoteBtnClicked(Button *pRef);

		void hihatGroupClicked(Button *pRef);
		void hihatMinRangeBtnClicked(Button *pRef);
		void hihatMaxRangeBtnClicked(Button *pRef);

		void pSampleSelectionChanged( int );

		void waveDisplayDoubleClicked( QWidget *pRef );

	private:
		std::shared_ptr<H2Core::Instrument> m_pInstrument;
		int m_nSelectedLayer;
		int m_nSelectedComponent;

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

		// Instrument pitch
		Rotary *m_pPitchCoarseRotary;
		Rotary *m_pPitchFineRotary;
		Rotary *m_pRandomPitchRotary;
		LCDDisplay *m_pPitchLCD;

		// Low pass filter
		ToggleButton *m_pFilterBypassBtn;
		Rotary *m_pCutoffRotary;
		Rotary *m_pResonanceRotary;

		// Instrument gain
		LCDDisplay *m_pInstrumentGainLCD;
		Rotary *m_pInstrumentGain;

		QCheckBox *m_pApplyVelocity;

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

		// Instrument hihat

		LCDDisplay *m_pHihatGroupLCD;
		Button *m_pAddHihatGroupBtn;
		Button *m_pDelHihatGroupBtn;

		LCDDisplay *m_pHihatMinRangeLCD;
		Button *m_pAddHihatMinRangeBtn;
		Button *m_pDelHihatMinRangeBtn;

		LCDDisplay *m_pHihatMaxRangeLCD;
		Button *m_pAddHihatMaxRangeBtn;
		Button *m_pDelHihatMaxRangeBtn;


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

		//LCDCombo *__pattern_size_combo;
		LCDCombo *m_sampleSelectionAlg;

		WaveDisplay *m_pWaveDisplay;

		Button *m_pLoadLayerBtn;
		Button *m_pRemoveLayerBtn;
		Button *m_pSampleEditorBtn;
		QCheckBox *m_pIsStopNoteCheckBox;
		//~ Layer properties


		// Component
		ClickableLabel *m_pCompoNameLbl;
		Button *m_buttonDropDownCompo;
		QStringList itemsCompo;
		QMenu *popCompo;

		Rotary *m_pCompoGainRotary;
		LCDDisplay *m_pCompoGainLCD;
		//~ Component

		void loadLayer();
		void setAutoVelocity();
		/** Converts #m_lastUsedFontSize into a point size used for
			the widget's font.*/
		int getPointSizeButton() const;
		/** Used to detect changed in the font*/
		H2Core::Preferences::FontSize m_lastUsedFontSize;
};


#endif
