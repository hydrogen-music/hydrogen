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

#ifndef INSTRUMENT_EDITOR_DIALOG_H
#define INSTRUMENT_EDITOR_DIALOG_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <core/Basics/Instrument.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

class Fader;
class LCDDisplay;
class LCDSpinBox;
class Button;
class ClickableLabel;
class Rotary;
class LCDCombo;
class WaveDisplay;
class LayerPreview;
class WidgetWithInput;

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
	virtual void drumkitLoadedEvent() override;
	virtual void updateSongEvent( int ) override;
	virtual void instrumentParametersChangedEvent( int ) override;
		// ~ implements EventListener interface
		void update();
		static int findFreeDrumkitComponentId( int startingPoint = 0 );


	public slots:
	/** Used by #Shotlist */
	void showLayers( bool bShow );
		void showSampleEditor();
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private slots:
		void rotaryChanged(WidgetWithInput *ref);
		void loadLayerBtnClicked();
		void filterActiveBtnClicked();
		void removeLayerButtonClicked();
		void labelClicked( ClickableLabel* pRef );
		void labelCompoClicked( ClickableLabel* pRef );
		void compoChangeAddDelete(QAction*);
		void onDropDownCompoClicked();

		void muteGroupChanged( double fValue );
		void onIsStopNoteCheckBoxClicked( bool on );
		void onIsApplyVelocityCheckBoxClicked( bool on);
		void midiOutChannelChanged( double fValue );
		void midiOutNoteChanged( double fValue );

		void hihatGroupChanged( double fValue );
		void hihatMinRangeChanged( double fValue );
		void hihatMaxRangeChanged( double fValue );

		void sampleSelectionChanged( int );

		void waveDisplayDoubleClicked( QWidget *pRef );

	private:
		std::shared_ptr<H2Core::Instrument> m_pInstrument;
		int m_nSelectedLayer;
		int m_nSelectedComponent;

		Button *m_pShowInstrumentBtn;
		Button *m_pShowLayersBtn;

		// Instrument properties
		PixmapWidget *m_pInstrumentProp;
		PixmapWidget *m_pInstrumentPropTop;
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
		void setInstrumentPitch();

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
		ClickableLabel *m_pIsStopNoteLbl;

		// Instrument mute group
		LCDSpinBox *m_pMuteGroupLCD;
		ClickableLabel *m_pMuteGroupLbl;

		// Instrument midi out
		LCDSpinBox *m_pMidiOutChannelLCD;
		ClickableLabel* m_pMidiOutChannelLbl;
	/** In order to allow for enumerations starting at 1 while using
		-1 to turn off the LCD.*/
	double m_fPreviousMidiOutChannel;

		LCDSpinBox *m_pMidiOutNoteLCD;
		ClickableLabel* m_pMidiOutNoteLbl;

		// Instrument hihat

		LCDSpinBox *m_pHihatGroupLCD;
		ClickableLabel* m_pHihatGroupLbl;

		LCDSpinBox *m_pHihatMinRangeLCD;
		ClickableLabel* m_pHihatMinRangeLbl;

		LCDSpinBox *m_pHihatMaxRangeLCD;
		ClickableLabel* m_pHihatMaxRangeLbl;

		// ~ Instrument properties

		// Layer properties
		LayerPreview *m_pLayerPreview;
		QScrollArea *m_pLayerScrollArea;


		PixmapWidget *m_pLayerProp;
		Rotary *m_pLayerGainRotary;
		LCDDisplay *m_pLayerGainLCD;
		ClickableLabel* m_pLayerGainLbl;
		ClickableLabel* m_pCompoGainLbl;
		ClickableLabel* m_pLayerPitchLbl;
		ClickableLabel* m_pLayerPitchCoarseLbl;
		ClickableLabel* m_pLayerPitchFineLbl;

		Rotary *m_pLayerPitchCoarseRotary;
		Rotary *m_pLayerPitchFineRotary;

		LCDDisplay *m_pLayerPitchCoarseLCD;
		LCDDisplay *m_pLayerPitchFineLCD;

		//LCDCombo *__pattern_size_combo;
		LCDCombo *m_sampleSelectionAlg;
		ClickableLabel* m_pSampleSelectionLbl;

		WaveDisplay *m_pWaveDisplay;

		Button *m_pLoadLayerBtn;
		Button *m_pRemoveLayerBtn;
		Button *m_pSampleEditorBtn;
		QCheckBox *m_pIsStopNoteCheckBox;
		// ~ Layer properties


		// Component
		ClickableLabel *m_pCompoNameLbl;
		Button *m_buttonDropDownCompo;
		QStringList itemsCompo;
		QMenu *popCompo;

		Rotary *m_pCompoGainRotary;
		LCDDisplay *m_pCompoGainLCD;
		// ~ Component

		void setAutoVelocity();
};


#endif
