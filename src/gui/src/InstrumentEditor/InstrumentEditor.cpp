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

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <math.h>
#include <assert.h>

#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/basics/adsr.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;

#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../widgets/Rotary.h"
#include "../widgets/ClickableLabel.h"
#include "../widgets/Button.h"
#include "../widgets/LCD.h"
#include "../widgets/LCDCombo.h"
#include "../widgets/Fader.h"
#include "InstrumentEditor.h"
#include "WaveDisplay.h"
#include "LayerPreview.h"
#include "AudioFileBrowser/AudioFileBrowser.h"

const char* InstrumentEditor::__class_name = "InstrumentEditor";

InstrumentEditor::InstrumentEditor( QWidget* pParent )
	: QWidget( pParent )
	, Object( __class_name )
	, m_pInstrument( NULL )
	, m_nSelectedLayer( 0 )
{
	setFixedWidth( 290 );

	// Instrument properties top
	m_pInstrumentPropTop = new PixmapWidget( this );
	m_pInstrumentPropTop->setPixmap( "/instrumentEditor/instrumentTab_top.png" );

	m_pShowInstrumentBtn = new ToggleButton(
							   m_pInstrumentPropTop,
							   "/skin_btn_on.png",
							   "/skin_btn_off.png",
							   "/skin_btn_over.png",
							   QSize( 100, 17 ),
							   true
							   );
	m_pShowInstrumentBtn->setText(trUtf8("General"));
	m_pShowInstrumentBtn->setToolTip( trUtf8( "Show instrument properties" ) );
	connect( m_pShowInstrumentBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowInstrumentBtn->move( 40, 7 );
	m_pShowInstrumentBtn->setPressed( true );


	m_pShowLayersBtn = new ToggleButton(
						   m_pInstrumentPropTop,
						   "/skin_btn_on.png",
						   "/skin_btn_off.png",
						   "/skin_btn_over.png",
						   QSize( 100, 17 ),
						   true
						   );
	m_pShowLayersBtn->setText( trUtf8("Layers") );
	m_pShowLayersBtn->setToolTip( trUtf8( "Show layers properties" ) );
	connect( m_pShowLayersBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowLayersBtn->move( 144, 7 );


	// Instrument properties
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move(0, 31);
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab_new.png" );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp );
	m_pNameLbl->setGeometry( 8, 5, 275, 28 );

	/////////////
	//Midi Out

	m_pMidiOutChannelLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMidiOutChannelLCD->move( 67, 261 );
	m_pMidiOutChannelLCD->setToolTip(QString(trUtf8("Midi out channel")));


	m_pAddMidiOutChannelBtn = new Button(
								  m_pInstrumentProp,
								  "/lcd/LCDSpinBox_up_on.png",
								  "/lcd/LCDSpinBox_up_off.png",
								  "/lcd/LCDSpinBox_up_over.png",
								  QSize( 16, 8 )
								  );

	m_pAddMidiOutChannelBtn->move( 109, 260 );
	connect( m_pAddMidiOutChannelBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutChannelBtnClicked(Button*) ) );


	m_pDelMidiOutChannelBtn = new Button(
								  m_pInstrumentProp,
								  "/lcd/LCDSpinBox_down_on.png",
								  "/lcd/LCDSpinBox_down_off.png",
								  "/lcd/LCDSpinBox_down_over.png",
								  QSize(16,8)
								  );
	m_pDelMidiOutChannelBtn->move( 109, 269 );
	connect( m_pDelMidiOutChannelBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutChannelBtnClicked(Button*) ) );


	///
	m_pMidiOutNoteLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMidiOutNoteLCD->move( 160, 261 );

	m_pAddMidiOutNoteBtn = new Button(
							   m_pInstrumentProp,
							   "/lcd/LCDSpinBox_up_on.png",
							   "/lcd/LCDSpinBox_up_off.png",
							   "/lcd/LCDSpinBox_up_over.png",
							   QSize( 16, 8 ),
							   false,
							   true
							   );
	m_pMidiOutNoteLCD->setToolTip(QString(trUtf8("Midi out note")));


	m_pAddMidiOutNoteBtn->move( 202, 260 );
	connect( m_pAddMidiOutNoteBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutNoteBtnClicked(Button*) ) );


	m_pDelMidiOutNoteBtn = new Button(
							   m_pInstrumentProp,
							   "/lcd/LCDSpinBox_down_on.png",
							   "/lcd/LCDSpinBox_down_off.png",
							   "/lcd/LCDSpinBox_down_over.png",
							   QSize(16,8),
							   false,
							   true
							   );
	m_pDelMidiOutNoteBtn->move( 202, 269 );
	connect( m_pDelMidiOutNoteBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutNoteBtnClicked(Button*) ) );

	/////////////

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );
	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelClicked(ClickableLabel*) ) );

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Random pitch factor" ), false, true );
	m_pRandomPitchRotary->move( 117, 210 );
	connect( m_pRandomPitchRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	// Filter
	m_pFilterBypassBtn = new ToggleButton(
							 m_pInstrumentProp,
							 "/instrumentEditor/bypass_on.png",
							 "/instrumentEditor/bypass_off.png",
							 "/instrumentEditor/bypass_over.png",
							 QSize( 30, 13 )
							 );
	connect( m_pFilterBypassBtn, SIGNAL( clicked(Button*) ), this, SLOT( filterActiveBtnClicked(Button*) ) );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Filter Cutoff" ), false, true );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	connect( m_pCutoffRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Filter resonance" ), false, true );
	connect( m_pResonanceRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pFilterBypassBtn->move( 70, 170 );
	m_pCutoffRotary->move( 117, 164 );
	m_pResonanceRotary->move( 170, 164 );
	//~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Attack" ), false, true );
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Decay" ), false, true );
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Sustain" ), false, true );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Release" ), false, true );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	connect( m_pAttackRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pDecayRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pSustainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pReleaseRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	m_pAttackRotary->move( 53, 52 );
	m_pDecayRotary->move( 105, 52 );
	m_pSustainRotary->move( 157, 52 );
	m_pReleaseRotary->move( 209, 52 );
	//~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Instrument gain" ), false, false );
	m_pInstrumentGain->setDefaultValue( 0.2 ); // gain is multiplied with 5, so default is 1.0 from users view
	connect( m_pInstrumentGain, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	m_pInstrumentGainLCD->move( 67, 105 );
	m_pInstrumentGain->move( 117, 100 );


	m_pMuteGroupLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMuteGroupLCD->move( 160, 105 );

	m_pAddMuteGroupBtn = new Button(
							 m_pInstrumentProp,
							 "/lcd/LCDSpinBox_up_on.png",
							 "/lcd/LCDSpinBox_up_off.png",
							 "/lcd/LCDSpinBox_up_over.png",
							 QSize( 16, 8 )
							 );

	m_pAddMuteGroupBtn->move( 202, 104 );
	connect( m_pAddMuteGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteGroupBtnClicked(Button*) ) );


	m_pDelMuteGroupBtn = new Button(
							 m_pInstrumentProp,
							 "/lcd/LCDSpinBox_down_on.png",
							 "/lcd/LCDSpinBox_down_off.png",
							 "/lcd/LCDSpinBox_down_over.png",
							 QSize(16,8)
							 );
	m_pDelMuteGroupBtn->move( 202, 113 );
	connect( m_pDelMuteGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteGroupBtnClicked(Button*) ) );

	m_pIsStopNoteCheckBox = new QCheckBox ( trUtf8( "" ), m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 63, 138 );
	m_pIsStopNoteCheckBox->setToolTip( trUtf8( "Stop the current playing instrument-note before trigger the next note sample." ) );
	connect( m_pIsStopNoteCheckBox, SIGNAL( toggled( bool ) ), this, SLOT( onIsStopNoteCheckBoxClicked( bool ) ) );

	m_pApplyVelocity = new QCheckBox ( trUtf8( "" ), m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 138 );
	m_pApplyVelocity->setToolTip( trUtf8( "Don't change the layers' gain based on velocity" ) );
	connect( m_pApplyVelocity, SIGNAL( toggled( bool ) ), this, SLOT( onIsApplyVelocityCheckBoxClicked( bool ) ) );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pHihatGroupLCD->move( 27, 307 );

	m_pAddHihatGroupBtn = new Button(
					m_pInstrumentProp,
					"/lcd/LCDSpinBox_up_on.png",
					"/lcd/LCDSpinBox_up_off.png",
					"/lcd/LCDSpinBox_up_over.png",
					QSize( 16, 8 )
					);
	m_pAddHihatGroupBtn->move( 69, 306 );
	connect( m_pAddHihatGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatGroupClicked(Button*) ) );

	m_pDelHihatGroupBtn = new Button(
					m_pInstrumentProp,
					"/lcd/LCDSpinBox_down_on.png",
					"/lcd/LCDSpinBox_down_off.png",
					"/lcd/LCDSpinBox_down_over.png",
					QSize(16,8)
					);
	m_pDelHihatGroupBtn->move( 69, 315 );
	connect( m_pDelHihatGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatGroupClicked(Button*) ) );

	m_pHihatMinRangeLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pHihatMinRangeLCD->move( 137, 307 );

	m_pAddHihatMinRangeBtn = new Button(
								 m_pInstrumentProp,
								 "/lcd/LCDSpinBox_up_on.png",
								 "/lcd/LCDSpinBox_up_off.png",
								 "/lcd/LCDSpinBox_up_over.png",
								 QSize( 16, 8 ),
								 false,
								 true
								 );
	m_pAddHihatMinRangeBtn->move( 179, 306 );
	connect( m_pAddHihatMinRangeBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatMinRangeBtnClicked(Button*) ) );

	m_pDelHihatMinRangeBtn = new Button(
								 m_pInstrumentProp,
								 "/lcd/LCDSpinBox_down_on.png",
								 "/lcd/LCDSpinBox_down_off.png",
								 "/lcd/LCDSpinBox_down_over.png",
								 QSize(16,8),
								 false,
								 true
								 );
	m_pDelHihatMinRangeBtn->move( 179, 315 );
	connect( m_pDelHihatMinRangeBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatMinRangeBtnClicked(Button*) ) );


	m_pHihatMaxRangeLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pHihatMaxRangeLCD->move( 202, 307 );

	m_pAddHihatMaxRangeBtn = new Button(
								 m_pInstrumentProp,
								 "/lcd/LCDSpinBox_up_on.png",
								 "/lcd/LCDSpinBox_up_off.png",
								 "/lcd/LCDSpinBox_up_over.png",
								 QSize( 16, 8 ),
								 false,
								 true
								 );
	m_pAddHihatMaxRangeBtn->move( 244, 306 );
	connect( m_pAddHihatMaxRangeBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatMaxRangeBtnClicked(Button*) ) );

	m_pDelHihatMaxRangeBtn = new Button(
								 m_pInstrumentProp,
								 "/lcd/LCDSpinBox_down_on.png",
								 "/lcd/LCDSpinBox_down_off.png",
								 "/lcd/LCDSpinBox_down_over.png",
								 QSize(16,8),
								 false,
								 true
								 );
	m_pDelHihatMaxRangeBtn->move( 244, 315 );
	connect( m_pDelHihatMaxRangeBtn, SIGNAL( clicked(Button*) ), this, SLOT( hihatMaxRangeBtnClicked(Button*) ) );

	//


	//~ Instrument properties





	// LAYER properties
	m_pLayerProp = new PixmapWidget( this );
	m_pLayerProp->move( 0, 31 );
	m_pLayerProp->hide();
	m_pLayerProp->setPixmap( "/instrumentEditor/layerTabsupernew.png" );

	// Component
	m_pCompoNameLbl = new ClickableLabel( m_pLayerProp );
	m_pCompoNameLbl->setGeometry( 8, 5, 275, 28 );
	m_pCompoNameLbl->setFont( boldFont );
	connect( m_pCompoNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelCompoClicked(ClickableLabel*) ) );

	m_buttonDropDownCompo = new Button( m_pLayerProp,
										"/instrumentEditor/btn_dropdown_on.png",
										"/instrumentEditor/btn_dropdown_off.png",
										"/instrumentEditor/btn_dropdown_over.png",
										QSize(13, 13)
										);
	m_buttonDropDownCompo->move( 272, 10 );
	connect( m_buttonDropDownCompo, SIGNAL( clicked( Button* ) ), this, SLOT( onClick( Button* ) ) );

	// Layer preview
	m_pLayerPreview = new LayerPreview( NULL );

	m_pLayerScrollArea = new QScrollArea( m_pLayerProp);
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->move( 6, 44 );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( MAX_LAYERS > 16)
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setMaximumHeight( 182 );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( m_pLayerProp );
	m_pWaveDisplay->resize( 277, 58 );
	m_pWaveDisplay->updateDisplay( NULL );
	m_pWaveDisplay->move( 5, 241 );
	connect( m_pWaveDisplay, SIGNAL( doubleClicked(QWidget*) ), this, SLOT( waveDisplayDoubleClicked(QWidget*) ) );

	m_pLoadLayerBtn = new Button(
						  m_pLayerProp,
						  "/instrumentEditor/loadLayer_on.png",
						  "/instrumentEditor/loadLayer_off.png",
						  "/instrumentEditor/loadLayer_over.png",
						  QSize( 90, 13 )
						  );

	m_pRemoveLayerBtn = new Button(
							m_pLayerProp,
							"/instrumentEditor/deleteLayer_on.png",
							"/instrumentEditor/deleteLayer_off.png",
							"/instrumentEditor/deleteLayer_over.png",
							QSize( 90, 13 )
							);

	m_pSampleEditorBtn = new Button(
							 m_pLayerProp,
							 "/instrumentEditor/editLayer_on.png",
							 "/instrumentEditor/editLayer_off.png",
							 "/instrumentEditor/editLayer_over.png",
							 QSize( 90, 13 )
							 );
	m_pLoadLayerBtn->move( 48, 267 );
	m_pRemoveLayerBtn->move( 145, 267 );



	m_pLoadLayerBtn->move( 6, 306 );
	m_pRemoveLayerBtn->move( 99, 306 );
	m_pSampleEditorBtn->move( 191, 306 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pSampleEditorBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerGainRotary = new Rotary( m_pLayerProp,  Rotary::TYPE_NORMAL, trUtf8( "Layer gain" ), false, false );
	m_pLayerGainRotary->setDefaultValue ( 0.2 ); // gain is multiplied with 5, so default is 1.0 from users view
	connect( m_pLayerGainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pCompoGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pCompoGainRotary = new Rotary( m_pLayerProp,  Rotary::TYPE_NORMAL, trUtf8( "Component volume" ), false, false );
	m_pCompoGainRotary->setDefaultValue ( 0.2 ); // gain is multiplied with 5, so default is 1.0 from users view
	connect( m_pCompoGainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerPitchCoarseLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerPitchFineLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );

	m_pLayerPitchCoarseRotary = new Rotary( m_pLayerProp, Rotary::TYPE_CENTER, trUtf8( "Layer pitch (Coarse)" ), true, false );
	m_pLayerPitchCoarseRotary->setMin( -24.0 );
	m_pLayerPitchCoarseRotary->setMax( 24.0 );
	connect( m_pLayerPitchCoarseRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerPitchFineRotary = new Rotary( m_pLayerProp, Rotary::TYPE_CENTER, trUtf8( "Layer pitch (Fine)" ), true, false );
	m_pLayerPitchFineRotary->setMin( -50.0 );
	m_pLayerPitchFineRotary->setMax( 50.0 );
	connect( m_pLayerPitchFineRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerGainLCD->move( 54, 341 + 3 );
	m_pLayerGainRotary->move( 102, 341 );

	m_pCompoGainLCD->move( 151, 341 + 3 );
	m_pCompoGainRotary->move( 199, 341 );


	m_pLayerPitchCoarseLCD->move( 54, 391 + 3 );
	m_pLayerPitchCoarseRotary->move( 102, 391 );

	m_pLayerPitchFineLCD->move( 151, 391 + 3 );
	m_pLayerPitchFineRotary->move( 199, 391 );

	m_sampleSelectionAlg = new LCDCombo(m_pLayerProp, 25);
	m_sampleSelectionAlg->move( 60, 434 );
	m_sampleSelectionAlg->setToolTip( trUtf8("Select pattern size") );

	m_sampleSelectionAlg->addItem( QString( "First in Velocity" ) );
	m_sampleSelectionAlg->addItem( QString( "Round Robin" ) );
	m_sampleSelectionAlg->addItem( QString( "Random" ) );

	m_sampleSelectionAlg->update();
	connect( m_sampleSelectionAlg, SIGNAL( valueChanged( QString ) ), this, SLOT( pSampleSelectionChanged( QString ) ) );

	//~ Layer properties

	//component handling
	QStringList itemsCompo;
	popCompo = new QMenu( this );
	itemsCompo.clear();

	std::vector<DrumkitComponent*>* compoList = Hydrogen::get_instance()->getSong()->get_components();
	for (std::vector<DrumkitComponent*>::iterator it = compoList->begin() ; it != compoList->end(); ++it) {
		DrumkitComponent* p_compo = *it;
		if( !itemsCompo.contains( p_compo->get_name() ) )
			itemsCompo.append( p_compo->get_name() );
	}
	itemsCompo.append("--sep--");
	itemsCompo.append("add");
	itemsCompo.append("delete");
	itemsCompo.append("rename");

	m_nSelectedComponent = compoList->front()->get_id();

	connect( popCompo, SIGNAL( triggered(QAction*) ), this, SLOT( compoChangeAddDelete(QAction*) ) );
	update();
	//~component handling


	selectLayer( m_nSelectedLayer );

	HydrogenApp::get_instance()->addEventListener(this);

	selectedInstrumentChangedEvent(); 	// force an update

	// this will force an update...
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



InstrumentEditor::~InstrumentEditor()
{
	//INFOLOG( "DESTROY" );
}



void InstrumentEditor::selectedInstrumentChangedEvent()
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= (int)pInstrList->size() ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			m_pInstrument = NULL;
		}
		else {
			m_pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + m_pInstrument->m_sName );
		}
	}
	else {
		m_pInstrument = NULL;
	}
	AudioEngine::get_instance()->unlock();

	// update layer list
	if (m_pInstrument) {
		m_pNameLbl->setText( m_pInstrument->get_name() );

		// ADSR
		m_pAttackRotary->setValue( sqrtf(m_pInstrument->get_adsr()->get_attack() / 100000.0) );
		m_pDecayRotary->setValue( sqrtf(m_pInstrument->get_adsr()->get_decay() / 100000.0) );
		m_pSustainRotary->setValue( m_pInstrument->get_adsr()->get_sustain() );
		float fTmp = m_pInstrument->get_adsr()->get_release() - 256.0;
		if( fTmp < 0.0 ) {
			fTmp = 0.0;
		}
		m_pReleaseRotary->setValue( sqrtf(fTmp / 100000.0) );
		//~ ADSR

		// filter
		m_pFilterBypassBtn->setPressed( !m_pInstrument->is_filter_active());
		m_pCutoffRotary->setValue( m_pInstrument->get_filter_cutoff());
		m_pResonanceRotary->setValue( m_pInstrument->get_filter_resonance());
		//~ filter

		// random pitch
		m_pRandomPitchRotary->setValue( m_pInstrument->get_random_pitch_factor());

		//Stop Note
		m_pIsStopNoteCheckBox->setChecked( m_pInstrument->is_stop_notes() );

		//Ignore Velocity
		m_pApplyVelocity->setChecked( m_pInstrument->get_apply_velocity() );

		// instr gain
		char tmp[20];
		sprintf( tmp, "%#.2f", m_pInstrument->get_gain());
		m_pInstrumentGainLCD->setText( tmp );
		m_pInstrumentGain->setValue( m_pInstrument->get_gain()/ 5.0 );

		// instr mute group
		QString sMuteGroup = QString("%1").arg( m_pInstrument->get_mute_group() );
		if (m_pInstrument->get_mute_group() == -1 ) {
			sMuteGroup = "Off";
		}
		m_pMuteGroupLCD->setText( sMuteGroup );

		// midi out channel
		QString sMidiOutChannel = QString("%1").arg( m_pInstrument->get_midi_out_channel()+1);
		if (m_pInstrument->get_midi_out_channel() == -1 ) {
			sMidiOutChannel = "Off";
		}
		m_pMidiOutChannelLCD->setText( sMidiOutChannel );

		//midi out note
		QString sMidiOutNote = QString("%1").arg( m_pInstrument->get_midi_out_note() );
		m_pMidiOutNoteLCD->setText( sMidiOutNote );

		// hihat
		QString sHHGroup = QString("%1").arg( m_pInstrument->get_hihat_grp() );
		if (m_pInstrument->get_hihat_grp() == -1 ) {
			sHHGroup = "Off";
		}
		m_pHihatGroupLCD->setText( sHHGroup );
		QString sHiHatMinRange = QString("%1").arg( m_pInstrument->get_lower_cc() );
		m_pHihatMinRangeLCD->setText( sHiHatMinRange );
		QString sHiHatMaxRange = QString("%1").arg( m_pInstrument->get_higher_cc() );
		m_pHihatMaxRangeLCD->setText( sHiHatMaxRange );

		/*
		 * m_sampleSelectionAlg->addItem( QString( "First in Velocity" ) );
		 * m_sampleSelectionAlg->addItem( QString( "Round Robin" ) );
		 * m_sampleSelectionAlg->addItem( QString( "Random" ) );
		 **/
		switch ( m_pInstrument->sample_selection_alg() ) {
			case Instrument::VELOCITY:
				m_sampleSelectionAlg->set_text( "First in Velocity" );
				break;
			case Instrument::RANDOM:
				m_sampleSelectionAlg->set_text( "Random" );
				break;
			case Instrument::ROUND_ROBIN:
				m_sampleSelectionAlg->set_text( "Round Robin" );
				break;
		}

		itemsCompo.clear();
		std::vector<DrumkitComponent*>* compoList = Hydrogen::get_instance()->getSong()->get_components();
		for (std::vector<DrumkitComponent*>::iterator it = compoList->begin() ; it != compoList->end(); ++it) {
			DrumkitComponent* p_compo = *it;
			if( !itemsCompo.contains( p_compo->get_name() ) )
				itemsCompo.append( p_compo->get_name() );
		}
		itemsCompo.append("--sep--");
		itemsCompo.append("add");
		itemsCompo.append("delete");
		itemsCompo.append("rename");

		update();

		bool p_found = false;
		for (std::vector<DrumkitComponent*>::iterator it = compoList->begin() ; it != compoList->end(); ++it) {
			DrumkitComponent* p_compo = *it;
			if ( p_compo->get_id() == m_nSelectedComponent ) {
				p_found = true;
				break;
			}
		}
		if ( !p_found )
			m_nSelectedComponent = compoList->front()->get_id();

		DrumkitComponent* p_tmpCompo = Hydrogen::get_instance()->getSong()->get_component( m_nSelectedComponent );

		assert(p_tmpCompo);

		m_pCompoNameLbl->setText( p_tmpCompo->get_name() );

		if(m_nSelectedLayer >= 0){
			InstrumentComponent* component = m_pInstrument->get_component( m_nSelectedComponent );
			if(component) {

				char tmp[20];
				sprintf( tmp, "%#.2f", component->get_gain());
				m_pCompoGainLCD->setText( tmp );

				m_pCompoGainRotary->setValue( component->get_gain() / 5.0 );

				InstrumentLayer* p_layer = component->get_layer( m_nSelectedLayer );
				if(p_layer) {
					m_pWaveDisplay->updateDisplay( p_layer );
				}
				else {
					m_pWaveDisplay->updateDisplay( NULL );
				}
			}
			else {
				m_pWaveDisplay->updateDisplay( NULL );
			}
		}
		else{
			m_pWaveDisplay->updateDisplay( NULL );
		}
	}
	else {
		m_pNameLbl->setText( QString( "NULL Instrument..." ) );
		m_pWaveDisplay->updateDisplay( NULL );
		m_nSelectedLayer = 0;
	}
	selectLayer( m_nSelectedLayer );

}



void InstrumentEditor::rotaryChanged(Rotary *ref)
{
	float fVal = ref->getValue();

	if ( m_pInstrument ) {
		if ( ref == m_pRandomPitchRotary ){
			m_pInstrument->set_random_pitch_factor( fVal );
		}
		else if ( ref == m_pCutoffRotary ) {
			m_pInstrument->set_filter_cutoff( fVal );
		}
		else if ( ref == m_pResonanceRotary ) {
			if ( fVal > 0.95f ) {
				fVal = 0.95f;
			}
			m_pInstrument->set_filter_resonance( fVal );
		}
		else if ( ref == m_pAttackRotary ) {
			m_pInstrument->get_adsr()->set_attack( fVal * fVal * 100000 );
		}
		else if ( ref == m_pDecayRotary ) {
			m_pInstrument->get_adsr()->set_decay( fVal * fVal * 100000 );
		}
		else if ( ref == m_pSustainRotary ) {
			m_pInstrument->get_adsr()->set_sustain( fVal );
		}
		else if ( ref == m_pReleaseRotary ) {
			m_pInstrument->get_adsr()->set_release( 256.0 + fVal * fVal * 100000 );
		}
		else if ( ref == m_pLayerGainRotary ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pLayerGainLCD->setText( tmp );

			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				H2Core::InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					pLayer->set_gain( fVal );
					m_pWaveDisplay->updateDisplay( pLayer );
				}
			}
		}
		else if ( ref == m_pCompoGainRotary ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pCompoGainLCD->setText( tmp );

			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			pCompo->set_gain( fVal );
		}
		else if ( ref == m_pLayerPitchCoarseRotary ) {
			//fVal = fVal * 24.0 - 12.0;
			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int)fVal ) );

			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				H2Core::InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
					float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
					pLayer->set_pitch( nCoarse + fFine );
					INFOLOG( QString("pitch: %1").arg( pLayer->get_pitch() ) );
				}
			}
		}
		else if ( ref == m_pLayerPitchFineRotary ) {
			m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal ) );
			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				H2Core::InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
					float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
					pLayer->set_pitch( nCoarse + fFine );
					INFOLOG( QString("pitch: %1").arg( pLayer->get_pitch()) );
				}
			}
		}
		else if ( ref == m_pInstrumentGain ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pInstrumentGainLCD->setText( tmp );
			m_pInstrument->set_gain( fVal );
		}
		else {
			ERRORLOG( "[rotaryChanged] unhandled rotary" );
		}
	}
}


void InstrumentEditor::filterActiveBtnClicked(Button *ref)
{
	if ( m_pInstrument ) {
		m_pInstrument->set_filter_active( !ref->isPressed() );
	}
}


void InstrumentEditor::waveDisplayDoubleClicked( QWidget* pRef )
{		
	if ( !m_pInstrument ) {
		return;
	}
	
	InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
	if( !pCompo ) {
		return;
	}
			
	H2Core::InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
	if ( pLayer ) {
		Sample* pSample = pLayer->get_sample();
		
		if( pSample ) {
			QString name = pSample->get_filepath();
			HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
		}
	}
	else {
		loadLayer();
	}
}

void InstrumentEditor::buttonClicked( Button* pButton )
{

	if ( pButton == m_pShowInstrumentBtn ) {
		m_pShowInstrumentBtn->setPressed( true );
		m_pShowLayersBtn->setPressed( false );
		m_pInstrumentProp->show();
		m_pLayerProp->hide();

		m_pShowLayersBtn->show();
		m_pShowInstrumentBtn->show();
	}
	else if ( pButton == m_pShowLayersBtn ) {
		m_pShowLayersBtn->setPressed( true );
		m_pShowInstrumentBtn->setPressed( false );
		m_pLayerProp->show();
		m_pInstrumentProp->hide();

		m_pShowLayersBtn->show();
		m_pShowInstrumentBtn->show();
	}
	else if ( pButton == m_pLoadLayerBtn ) {
		loadLayer();
	}
	else if ( pButton == m_pRemoveLayerBtn ) {
		//Hydrogen *pEngine = Hydrogen::get_instance();
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		if ( m_pInstrument ) {
			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				H2Core::InstrumentLayer *pLayer = m_pInstrument->get_component(m_nSelectedComponent)->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					m_pInstrument->get_component(m_nSelectedComponent)->set_layer( NULL, m_nSelectedLayer );
					delete pLayer;
				}

				int p_count = 0;
				for( int n = 0; n < MAX_LAYERS; n++ ) {
					InstrumentLayer* layer = m_pInstrument->get_component(m_nSelectedComponent)->get_layer( n );
					if( layer )
						p_count++;
				}

				if( p_count == 0 )
					m_pInstrument->get_components()->erase( m_pInstrument->get_components()->begin() + m_nSelectedComponent );
			}
		}
		AudioEngine::get_instance()->unlock();
		selectedInstrumentChangedEvent();    // update all
		m_pLayerPreview->updateAll();
	}
	else if ( pButton == m_pSampleEditorBtn ){
		if ( m_pInstrument ) {
			InstrumentComponent* pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				H2Core::InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					Sample* pSample = pLayer->get_sample();
					if( pSample == NULL) return;
					QString name = pSample->get_filepath();
					HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
				}
			}
		}

	}
	else {
		ERRORLOG( "[buttonClicked] unhandled button" );
	}
}



void InstrumentEditor::loadLayer()
{
	static QString lastUsedDir = QDir::homePath();

	Hydrogen *engine = Hydrogen::get_instance();

	AudioFileBrowser *fb = new AudioFileBrowser( NULL );
	QStringList filename;
	filename << "false" << "false" << "";

	if (fb->exec() == QDialog::Accepted) {
		filename = fb->selectedFile();
	}

	delete fb;

	if ( filename[2].isEmpty() ) return;

	bool fnc = false;
	if ( filename[0] ==  "true" ){
		fnc = true;
	}

	//use auto velocity if we want to work with multiple filenames
	if ( filename.size() > 3) filename[1] = "true";

	int selectedLayer =  m_nSelectedLayer;
	int firstSelection = selectedLayer;



	if (filename.size() > 2) {

		for(int i=2;i < filename.size();++i)
		{
			selectedLayer = m_nSelectedLayer + i - 2;
			if( ( i-2 >= MAX_LAYERS ) || ( selectedLayer + 1  > MAX_LAYERS ) ) break;

			Sample *newSample = Sample::load( filename[i] );

			H2Core::Instrument *pInstr = NULL;

			AudioEngine::get_instance()->lock( RIGHT_HERE );
			Song *song = engine->getSong();
			InstrumentList *instrList = song->get_instrument_list();
			pInstr = instrList->get( engine->getSelectedInstrumentNumber() );

			/*
				if we're using multiple layers, we start inserting the first layer
				at m_nSelectedLayer and the next layer at m_nSelectedLayer+1
			*/

			InstrumentComponent *pCompo = pInstr->get_component(m_nSelectedComponent);
			if( !pCompo ) {
				pCompo = new InstrumentComponent( m_nSelectedComponent );
				pInstr->get_components()->push_back( pCompo );
			}

			H2Core::InstrumentLayer *pLayer = pInstr->get_component(m_nSelectedComponent)->get_layer( selectedLayer );

			if (pLayer != NULL) {
				// delete old sample
				Sample *oldSample = pLayer->get_sample();
				delete oldSample;

				// insert new sample from newInstrument
				pLayer->set_sample( newSample );
			}
			else {
				pLayer = new H2Core::InstrumentLayer(newSample);
				pInstr->get_component(m_nSelectedComponent)->set_layer( pLayer, selectedLayer );
			}

			if ( fnc ){
				QString newFilename = filename[i].section( '/', -1 );
				newFilename.replace( "." + newFilename.section( '.', -1 ), "");
				m_pInstrument->set_name( newFilename );
			}

			//set automatic velocity
			if ( filename[1] ==  "true" ){
				setAutoVelocity();
			}

			//pInstr->set_drumkit_name( "" );   // external sample, no drumkit info

			AudioEngine::get_instance()->unlock();

		}
	}

	selectedInstrumentChangedEvent();    // update all
	selectLayer( firstSelection );
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::setAutoVelocity()
{
	int layerInUse[ MAX_LAYERS ] = {0};
	int layers = 0;
	for ( int i = 0; i < MAX_LAYERS ; i++ ) {
		InstrumentLayer *pLayers = m_pInstrument->get_component(m_nSelectedComponent)->get_layer( i );
		if ( pLayers ) {
			layers++;
			layerInUse[i] = i;
		}
	}

	float velocityrange = 1.0 / layers;

	for ( int i = 0; i < MAX_LAYERS ; i++ ) {
		if ( layerInUse[i] == i ){
			layers--;
			InstrumentLayer *pLayer = m_pInstrument->get_component(m_nSelectedComponent)->get_layer( i );
			if ( pLayer ) {
				pLayer->set_start_velocity( layers * velocityrange);
				pLayer->set_end_velocity( layers * velocityrange + velocityrange );
			}
		}
	}
}

void InstrumentEditor::labelCompoClicked( ClickableLabel* pRef )
{
	UNUSED( pRef );

	DrumkitComponent* p_compo = Hydrogen::get_instance()->getSong()->get_component( m_nSelectedComponent );

	QString sOldName = p_compo->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", trUtf8( "New component name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );

	if ( bIsOkPressed  ) {
		p_compo->set_name( sNewName );

		selectedInstrumentChangedEvent();

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
}

void InstrumentEditor::selectComponent( int nComponent )
{
	if (!m_pInstrument) {
		return;
	}

	m_nSelectedComponent = nComponent;
	m_pLayerPreview->set_selected_component(m_nSelectedComponent);
}

void InstrumentEditor::labelClicked( ClickableLabel* pRef )
{
	UNUSED( pRef );

	if (m_pInstrument) {
		QString sOldName = m_pInstrument->get_name();
		bool bIsOkPressed;
		QString sNewName = QInputDialog::getText( this, "Hydrogen", trUtf8( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
		if ( bIsOkPressed  ) {
			m_pInstrument->set_name( sNewName );
			selectedInstrumentChangedEvent();

#ifdef H2CORE_HAVE_JACK
			AudioEngine::get_instance()->lock( RIGHT_HERE );
			Hydrogen *engine = Hydrogen::get_instance();
			engine->renameJackPorts(engine->getSong());
			AudioEngine::get_instance()->unlock();
#endif

			// this will force an update...
			EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

		}
		else {
			// user entered nothing or pressed Cancel
		}
	}
}


void InstrumentEditor::selectLayer( int nLayer )
{
	if (!m_pInstrument) {
		return;
	}
	m_nSelectedLayer = nLayer;

	H2Core::InstrumentComponent *pComponent = m_pInstrument->get_component( m_nSelectedComponent );
	if(pComponent && nLayer >= 0 ){
		H2Core::InstrumentLayer *pLayer = pComponent->get_layer( nLayer );
		m_pWaveDisplay->updateDisplay( pLayer );
		if (pLayer) {
			char tmp[20];

			// Layer GAIN
			m_pLayerGainRotary->setValue( pLayer->get_gain() / 5.0 );
			sprintf( tmp, "%#.2f", pLayer->get_gain() );
			m_pLayerGainLCD->setText( tmp );

			//Component GAIN
			char tmp2[20];
			sprintf( tmp2, "%#.2f", pComponent->get_gain());
			m_pCompoGainRotary->setValue( pComponent->get_gain() / 5.0);
			m_pCompoGainLCD->setText( tmp2 );

			// Layer PITCH
			int nCoarsePitch = (int) ::round(pLayer->get_pitch());
			float fFinePitch = pLayer->get_pitch() - nCoarsePitch;
			//INFOLOG( "fine pitch: " + to_string( fFinePitch ) );
			m_pLayerPitchCoarseRotary->setValue( nCoarsePitch );
			m_pLayerPitchFineRotary->setValue( fFinePitch * 100 );

			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( nCoarsePitch ) );
			m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fFinePitch * 100 ) );
		}
		else {
			// Layer GAIN
			m_pLayerGainRotary->setValue( 1.0 );
			m_pLayerGainLCD->setText( "" );

			//Component GAIN
			m_pCompoGainRotary->setValue( 1.0 );
			m_pCompoGainLCD->setText( "" );

			// Layer PITCH
			m_pLayerPitchCoarseRotary->setValue( 0.0 );
			m_pLayerPitchFineRotary->setValue( 0.0 );

			m_pLayerPitchCoarseLCD->setText( "" );
			m_pLayerPitchFineLCD->setText( "" );
		}
	}
	else {
		m_pWaveDisplay->updateDisplay( NULL );

		// Layer GAIN
		m_pLayerGainRotary->setValue( 1.0 );
		m_pLayerGainLCD->setText( "" );

		m_pCompoGainRotary->setValue( 1.0 );
		m_pCompoGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setValue( 0.0 );
		m_pLayerPitchFineRotary->setValue( 0.0 );

		m_pLayerPitchCoarseLCD->setText( "" );
		m_pLayerPitchFineLCD->setText( "" );
	}
}



void InstrumentEditor::muteGroupBtnClicked(Button *pRef)
{
	assert( m_pInstrument );

	int mute_grp = m_pInstrument->get_mute_group();
	if (pRef == m_pAddMuteGroupBtn ) {
		mute_grp += 1;
	}
	else if (pRef == m_pDelMuteGroupBtn ) {
		mute_grp -= 1;
	}
	m_pInstrument->set_mute_group( mute_grp );

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::onIsStopNoteCheckBoxClicked( bool on )
{
	m_pInstrument->set_stop_notes( on );
	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::onIsApplyVelocityCheckBoxClicked( bool on )
{
	assert( m_pInstrument );

	m_pInstrument->set_apply_velocity( on );
	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::midiOutChannelBtnClicked(Button *pRef)
{
	assert( m_pInstrument );

	if (pRef == m_pAddMidiOutChannelBtn ) {
		m_pInstrument->set_midi_out_channel( m_pInstrument->get_midi_out_channel() + 1);
	}
	else if (pRef == m_pDelMidiOutChannelBtn ) {
		m_pInstrument->set_midi_out_channel( m_pInstrument->get_midi_out_channel() - 1);
	}

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::midiOutNoteBtnClicked(Button *pRef)
{
	assert( m_pInstrument );

	if (pRef == m_pAddMidiOutNoteBtn ) {
		m_pInstrument->set_midi_out_note( m_pInstrument->get_midi_out_note() + 1);
	}
	else if (pRef == m_pDelMidiOutNoteBtn ) {
		m_pInstrument->set_midi_out_note( m_pInstrument->get_midi_out_note() - 1);
	}

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::onClick(Button*)
{
	popCompo->popup( m_pCompoNameLbl->mapToGlobal( QPoint( m_pCompoNameLbl->width() - 40, m_pCompoNameLbl->height() / 2 ) ) );
}

void InstrumentEditor::update()
{
	//INFOLOG ( "update: "+toString(items.size()) );
	popCompo->clear();

	for( int i = 0; i < itemsCompo.size(); i++ ) {
		if ( itemsCompo.at(i) != "--sep--" ){
			popCompo->addAction( itemsCompo.at(i) );
		}else{
			popCompo->addSeparator();
		}
	}
}

int InstrumentEditor::findFreeDrumkitComponentId( int startingPoint )
{
	bool bFoundFreeSlot = true;
	std::vector<DrumkitComponent*>* pDrumkitComponentList = Hydrogen::get_instance()->getSong()->get_components();
	for (std::vector<DrumkitComponent*>::iterator it = pDrumkitComponentList->begin() ; it != pDrumkitComponentList->end(); ++it) {
		DrumkitComponent* pDrumkitComponent = *it;
		if( pDrumkitComponent->get_id() == startingPoint ) {
			bFoundFreeSlot = false;
			break;
		}
	}

	if(bFoundFreeSlot)
		return startingPoint;
	else
		return findFreeDrumkitComponentId( startingPoint + 1 );
}

void InstrumentEditor::compoChangeAddDelete(QAction* pAction)
{
	QString sSelectedAction = pAction->text();

	Hydrogen * pEngine = Hydrogen::get_instance();

	if( sSelectedAction.compare("add") == 0 ) {
		if ( m_pInstrument ) {
			bool bIsOkPressed;
			QString sNewName = QInputDialog::getText( this, "Hydrogen", trUtf8( "Component name" ), QLineEdit::Normal, "New Component", &bIsOkPressed );
			if ( bIsOkPressed  ) {
				DrumkitComponent* pDrumkitComponent = new DrumkitComponent( findFreeDrumkitComponentId(), sNewName );
				pEngine->getSong()->get_components()->push_back( pDrumkitComponent );

				//InstrumentComponent* instrument_component = new InstrumentComponent( dm_component->get_id() );
				//instrument_component->set_gain( 1.0f );
				//m_pInstrument->get_components()->push_back( instrument_component );

				m_nSelectedComponent = pDrumkitComponent->get_id();
				m_pLayerPreview->set_selected_component( pDrumkitComponent->get_id() );

				selectedInstrumentChangedEvent();

				// this will force an update...
				EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

#ifdef H2CORE_HAVE_JACK
				pEngine->renameJackPorts(pEngine->getSong());
#endif
			}
			else {
				// user entered nothing or pressed Cancel
			}
		}
	}
	else if( sSelectedAction.compare("delete") == 0 ) {
		std::vector<DrumkitComponent*>* pDrumkitComponents = pEngine->getSong()->get_components();

		if(pDrumkitComponents->size() == 1){
			return;
		}

		DrumkitComponent* pDrumkitComponent = pEngine->getSong()->get_component( m_nSelectedComponent );

		InstrumentList* pInstruments = pEngine->getSong()->get_instrument_list();
		for ( int n = ( int )pInstruments->size() - 1; n >= 0; n-- ) {
			Instrument* pInstrument = pInstruments->get( n );
			for( int o = 0 ; o < pInstrument->get_components()->size() ; o++ ) {
				InstrumentComponent* pInstrumentComponent = pInstrument->get_components()->at( o );
				if( pInstrumentComponent->get_drumkit_componentID() == pDrumkitComponent->get_id() ) {
					for( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
						InstrumentLayer* pLayer = pInstrumentComponent->get_layer( nLayer );
						if( pLayer )
							delete pLayer;
					}
					pInstrument->get_components()->erase( pInstrument->get_components()->begin() + o );;
					break;
				}
			}
		}

		for ( int n = 0 ; n < pDrumkitComponents->size() ; n++ ) {
			DrumkitComponent* pTmpDrumkitComponent = pDrumkitComponents->at( n );
			if( pTmpDrumkitComponent->get_id() == pDrumkitComponent->get_id() ) {
				pDrumkitComponents->erase( pDrumkitComponents->begin() + n );
				break;
			}
		}

		m_nSelectedComponent = pDrumkitComponents->front()->get_id();

		selectedInstrumentChangedEvent();
		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
	else if( sSelectedAction.compare("rename") == 0 ) {
		labelCompoClicked( NULL );
	}
	else {
		m_nSelectedComponent = -1;
		std::vector<DrumkitComponent*>* pDrumkitComponents = pEngine->getSong()->get_components();
		for (std::vector<DrumkitComponent*>::iterator it = pDrumkitComponents->begin() ; it != pDrumkitComponents->end(); ++it) {
			DrumkitComponent* pDrumkitComponent = *it;
			if( pDrumkitComponent->get_name().compare( sSelectedAction ) == 0) {
				m_nSelectedComponent = pDrumkitComponent->get_id();
				m_pCompoNameLbl->setText( pDrumkitComponent->get_name() );
				break;
			}
		}

		if( m_pInstrument && !m_pInstrument->get_component(m_nSelectedComponent)) {
			INFOLOG("Component needs to be added");

			InstrumentComponent* pInstrComponent = new InstrumentComponent( m_nSelectedComponent );
			pInstrComponent->set_gain( 1.0f );

			m_pInstrument->get_components()->push_back( pInstrComponent );



#ifdef H2CORE_HAVE_JACK
			pEngine->renameJackPorts(pEngine->getSong());
#endif
		}

		m_pLayerPreview->set_selected_component(m_nSelectedComponent);

		selectedInstrumentChangedEvent();

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
}


void InstrumentEditor::rubberbandbpmchangeEvent()
{
	if( !Preferences::get_instance()->getRubberBandBatchMode() /*&& Preferences::get_instance()->__usetimeline */){
		//we return also if time-line is activated. this wont work.
		//		INFOLOG( "Tempo change: Recomputing rubberband samples is disabled" );
		return;
	}
	//	INFOLOG( "Tempo change: Recomputing rubberband samples." );
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	assert(song);
	if(song){
		InstrumentList *songInstrList = song->get_instrument_list();
		assert(songInstrList);
		for ( unsigned nInstr = 0; nInstr < songInstrList->size(); ++nInstr ) {
			Instrument *pInstr = songInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				InstrumentComponent* pInstrumentComponent = pInstr->get_component(m_nSelectedComponent);
				if (!pInstrumentComponent) continue; // regular case when you have a new component empty
				for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
					InstrumentLayer *pLayer = pInstrumentComponent->get_layer( nLayer );
					if ( pLayer ) {
						Sample *pSample = pLayer->get_sample();
						if ( pSample ) {
							if( pSample->get_rubberband().use ) {
								//INFOLOG( QString("Instrument %1 Layer %2" ).arg(nInstr).arg(nLayer));
								Sample *newSample = Sample::load(
														pSample->get_filepath(),
														pSample->get_loops(),
														pSample->get_rubberband(),
														*pSample->get_velocity_envelope(),
														*pSample->get_pan_envelope()
														);
								if( !newSample  ){
									continue;
								}
								delete pSample;
								// insert new sample from newInstrument
								AudioEngine::get_instance()->lock( RIGHT_HERE );
								pLayer->set_sample( newSample );
								AudioEngine::get_instance()->unlock();

							}
						}
					}
				}
			}
		}
	}

}

void InstrumentEditor::pSampleSelectionChanged( QString selected )
{
	/*
		"First in Velocity"
		"Round Robin"
		"Random"
	*/

	assert( m_pInstrument );

	if ( selected.compare("First in Velocity") == 0 )
		m_pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
	else if ( selected.compare("Round Robin") == 0 )
		m_pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
	else if ( selected.compare("Random") == 0)
		m_pInstrument->set_sample_selection_alg( Instrument::RANDOM );

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::hihatGroupClicked(Button *pRef)
{
	assert( m_pInstrument );

	if ( pRef == m_pAddHihatGroupBtn && m_pInstrument->get_hihat_grp() < 32 )
		m_pInstrument->set_hihat_grp( m_pInstrument->get_hihat_grp() + 1 );
	else if ( pRef == m_pDelHihatGroupBtn && m_pInstrument->get_hihat_grp() > -1 )
		m_pInstrument->set_hihat_grp( m_pInstrument->get_hihat_grp() - 1 );

	selectedInstrumentChangedEvent();   // force an update
}

void InstrumentEditor::hihatMinRangeBtnClicked(Button *pRef)
{
	assert( m_pInstrument );

	if ( pRef == m_pAddHihatMinRangeBtn && m_pInstrument->get_lower_cc() < 127 )
		m_pInstrument->set_lower_cc( m_pInstrument->get_lower_cc() + 1 );
	else if ( pRef == m_pDelHihatMinRangeBtn && m_pInstrument->get_lower_cc() > 0 )
		m_pInstrument->set_lower_cc( m_pInstrument->get_lower_cc() - 1 );

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::hihatMaxRangeBtnClicked(Button *pRef)
{
	assert( m_pInstrument );

	if ( pRef == m_pAddHihatMaxRangeBtn && m_pInstrument->get_higher_cc() < 127 )
		m_pInstrument->set_higher_cc( m_pInstrument->get_higher_cc() + 1);
	else if ( pRef == m_pDelHihatMaxRangeBtn && m_pInstrument->get_higher_cc() > 0 )
		m_pInstrument->set_higher_cc( m_pInstrument->get_higher_cc() - 1);

	selectedInstrumentChangedEvent();	// force an update
}
