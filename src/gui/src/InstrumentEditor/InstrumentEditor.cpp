/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QtGui>
#include <QtWidgets>


#include <math.h>
#include <assert.h>
#include <vector>

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Globals.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/EventQueue.h>
using namespace H2Core;

#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../UndoActions.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/WidgetWithInput.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/Fader.h"
#include "InstrumentEditor.h"
#include "InstrumentEditorPanel.h"
#include "WaveDisplay.h"
#include "LayerPreview.h"
#include "AudioFileBrowser/AudioFileBrowser.h"

InstrumentEditor::InstrumentEditor( QWidget* pParent )
	: QWidget( pParent )
	, m_pInstrument( nullptr )
	, m_nSelectedLayer( 0 )
	, m_fPreviousMidiOutChannel( -1.0 )
	, m_nSelectedComponent( -1 )
	, m_bIsActive( true )
{
	setFixedWidth( 290 );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	// Instrument properties top
	m_pInstrumentPropTop = new PixmapWidget( this );
	m_pInstrumentPropTop->setPixmap( "/instrumentEditor/instrumentTab_top.png" );

	m_pShowInstrumentBtn = new Button( m_pInstrumentPropTop, QSize( 141, 22 ), Button::Type::Toggle, "",
									   pCommonStrings->getGeneralButton(), false, QSize(),
									   tr( "Show instrument properties" ) );
	m_pShowInstrumentBtn->move( 4, 4 );
	
	connect( m_pShowInstrumentBtn, &QPushButton::clicked,
			 [=]() { showLayers( false ); } );

	m_pShowLayersBtn = new Button( m_pInstrumentPropTop, QSize( 140, 22 ), Button::Type::Toggle, "",
								   pCommonStrings->getLayersButton(), false, QSize(),
								   tr( "Show layers properties" ) );
	m_pShowLayersBtn->move( 145, 4 );
	
	connect( m_pShowLayersBtn, &QPushButton::clicked,
			 [=]() { showLayers( true ); } );


	// Instrument properties
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move(0, 31);
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp, QSize( 279, 27 ), "",
									 ClickableLabel::Color::Bright, true );
	m_pNameLbl->move( 5, 4 );
	m_pNameLbl->setScaledContents( true );

	/////////////
	//Midi Out

	ClickableLabel* pMidiOutLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
													  pCommonStrings->getMidiOutLabel() );
	pMidiOutLbl->move( 22, 281 );

	m_pMidiOutChannelLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										   LCDSpinBox::Type::Int, -1, 16,
										   true, true );
	m_pMidiOutChannelLCD->move( 98, 257 );
	m_pMidiOutChannelLCD->setToolTip(QString(tr("Midi out channel")));
	connect( m_pMidiOutChannelLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( midiOutChannelChanged( double ) ) );
	m_pMidiOutChannelLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											   pCommonStrings->getMidiOutChannelLabel() );
	m_pMidiOutChannelLbl->move( 96, 281 );

	///
	m_pMidiOutNoteLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										LCDSpinBox::Type::Int, 0, 127, true );
	m_pMidiOutNoteLCD->move( 161, 257 );
	m_pMidiOutNoteLCD->setToolTip(QString(tr("Midi out note")));
	connect( m_pMidiOutNoteLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( midiOutNoteChanged( double ) ) );
	m_pMidiOutNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											pCommonStrings->getMidiOutNoteLabel() );
	m_pMidiOutNoteLbl->move( 159, 281 );

	/////////////

	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ),
			 this, SLOT( labelClicked(ClickableLabel*) ) );
	
	m_pPitchLCD = new LCDDisplay( m_pInstrumentProp, QSize( 56, 20 ), false, false );
	m_pPitchLCD->move( 24, 213 );
	m_pPitchLbl = new ClickableLabel( m_pInstrumentProp, QSize( 54, 10 ),
									  pCommonStrings->getPitchLabel() );
	m_pPitchLbl->move( 25, 235 );
	

	m_pPitchCoarseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center,
									   tr( "Pitch offset (Coarse)" ), true, -24, 24 );
	m_pPitchCoarseRotary->move( 84, 210 );

	connect( m_pPitchCoarseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchCoarseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 82, 235 );

	m_pPitchFineRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center,
									 tr( "Pitch offset (Fine)" ), false, -0.5, 0.5 );
	//it will have resolution of 100 steps between Min and Max => quantum delta = 0.01
	m_pPitchFineRotary->move( 138, 210 );
	connect( m_pPitchFineRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchFineLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										  pCommonStrings->getPitchFineLabel() );
	m_pPitchFineLbl->move( 136, 235 );

	

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									   tr( "Random pitch factor" ), false );
	m_pRandomPitchRotary->move( 194, 210 );
	connect( m_pRandomPitchRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchRandomLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchRandomLabel() );
	m_pPitchRandomLbl->move( 192, 235 );

	// Filter
	m_pFilterBypassBtn = new Button( m_pInstrumentProp, QSize( 36, 15 ), Button::Type::Toggle,
									 "", pCommonStrings->getBypassButton(), true,
									 QSize( 0, 0 ), "", false, true );
	connect( m_pFilterBypassBtn, SIGNAL( clicked() ),
			 this, SLOT( filterActiveBtnClicked() ) );
	m_pFilterBypassBtn->move( 67, 169 );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Filter Cutoff" ), false );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	connect( m_pCutoffRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pCutoffLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getCutoffLabel() );
	m_pCutoffLbl->move( 107, 189 );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									 tr( "Filter resonance" ), false );
	connect( m_pResonanceRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pResonanceLbl = new ClickableLabel( m_pInstrumentProp, QSize( 56, 10 ),
										  pCommonStrings->getResonanceLabel() );
	m_pResonanceLbl->move( 157, 189 );

	m_pCutoffRotary->move( 109, 164 );
	m_pResonanceRotary->move( 163, 164 );
	// ~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Length of Attack phase.\n\nValue" ), false );
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								 tr( "Length of Decay phase.\n\nValue" ), false );
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Sample volume in Sustain phase.\n\nValue" ), false );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Length of Release phase.\n\nValue" ), false );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	connect( m_pAttackRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pDecayRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pSustainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pReleaseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pAttackRotary->move( 45, 52 );
	m_pDecayRotary->move( 97, 52 );
	m_pSustainRotary->move( 149, 52 );
	m_pReleaseRotary->move( 201, 52 );

	m_pAttackLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getAttackLabel() );
	m_pAttackLbl->move( 43, 78 );
	m_pDecayLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									  pCommonStrings->getDecayLabel() );
	m_pDecayLbl->move( 95, 78 );
	m_pSustainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getSustainLabel() );
	m_pSustainLbl->move( 147, 78 );
	m_pReleaseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getReleaseLabel() );
	m_pReleaseLbl->move( 199, 78 );
	// ~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, QSize( 43, 20 ), false, false );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									tr( "Instrument gain" ), false, 0.0, 5.0 );
	m_pInstrumentGain->setDefaultValue( 1.0 );
	connect( m_pInstrumentGain, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pInstrumentGainLCD->move( 62, 103 );
	m_pInstrumentGain->move( 109, 100 );
	m_pGainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									 pCommonStrings->getGainLabel() );
	m_pGainLbl->move( 107, 125 );


	m_pMuteGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									  LCDSpinBox::Type::Int, -1, 100,
									  true, true );
	m_pMuteGroupLCD->move( 160, 101 );
	connect( m_pMuteGroupLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( muteGroupChanged( double ) ) );
	m_pMuteGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
										  pCommonStrings->getMuteGroupLabel() );
	m_pMuteGroupLbl->move( 159, 125 );

	m_pIsStopNoteCheckBox = new QCheckBox ( tr( "" ), m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 42, 139 );
	m_pIsStopNoteCheckBox->adjustSize();
	m_pIsStopNoteCheckBox->setFixedSize( 14, 14 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, SIGNAL( toggled( bool ) ),
			 this, SLOT( onIsStopNoteCheckBoxClicked( bool ) ) );
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
										   pCommonStrings->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 59, 144 );

	m_pApplyVelocity = new QCheckBox ( "", m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 139 );
	m_pApplyVelocity->adjustSize();
	m_pApplyVelocity->setFixedSize( 14, 14 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, SIGNAL( toggled( bool ) ),
			 this, SLOT( onIsApplyVelocityCheckBoxClicked( bool ) ) );
	m_pApplyVelocityLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
											  pCommonStrings->getApplyVelocityLabel() );
	m_pApplyVelocityLbl->move( 170, 144 );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									   LCDSpinBox::Type::Int, -1, 32,
									   true, true );
	m_pHihatGroupLCD->move( 28, 303 );
	connect( m_pHihatGroupLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( hihatGroupChanged( double ) ) );
	m_pHihatGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 69, 10 ),
										   pCommonStrings->getHihatGroupLabel() );
	m_pHihatGroupLbl->move( 22, 327 );

	m_pHihatMinRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMinRangeLCD->move( 138, 303 );
	connect( m_pHihatMinRangeLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( hihatMinRangeChanged( double ) ) );
	m_pHihatMinRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											  pCommonStrings->getHihatMinRangeLabel() );
	m_pHihatMinRangeLbl->move( 136, 327 );

	m_pHihatMaxRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMaxRangeLCD->move( 203, 303 );
	connect( m_pHihatMaxRangeLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( hihatMaxRangeChanged( double ) ) );
	m_pHihatMaxRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											  pCommonStrings->getHihatMaxRangeLabel() );
	m_pHihatMaxRangeLbl->move( 201, 327 );
	// ~ Instrument properties

	// LAYER properties
	m_pLayerProp = new PixmapWidget( this );
	m_pLayerProp->setObjectName( "LayerProperties" );
	m_pLayerProp->move( 0, 31 );
	m_pLayerProp->hide();
	m_pLayerProp->setPixmap( "/instrumentEditor/layerTabsupernew.png" );

	// Component
	m_pCompoNameLbl = new ClickableLabel( m_pLayerProp, QSize( 279, 27 ), "",
										  ClickableLabel::Color::Bright, true );
	m_pCompoNameLbl->move( 5, 4 );
	connect( m_pCompoNameLbl, SIGNAL( labelClicked(ClickableLabel*) ),
			 this, SLOT( renameComponentAction() ) );

	m_DropDownCompoBtn = new Button( m_pLayerProp, QSize( 18, 18 ),
										Button::Type::Push, "dropdown.svg", "",
										false, QSize( 12, 12 ) );
	m_DropDownCompoBtn->setObjectName( "InstrumentEditorComponentNameDropDown" );
	m_DropDownCompoBtn->move( 263, 8 );
	connect( m_DropDownCompoBtn, SIGNAL( clicked() ),
			 this, SLOT( onDropDownCompoClicked() ) );

	// Layer preview
	m_pLayerPreview = new LayerPreview( nullptr );

	m_pLayerScrollArea = new QScrollArea( m_pLayerProp);
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->move( 6, 44 );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( InstrumentComponent::getMaxLayers() > 16 ) {
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	}
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setMaximumHeight( 182 );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( m_pLayerProp );
	m_pWaveDisplay->resize( 277, 58 );
	m_pWaveDisplay->updateDisplay( nullptr );
	m_pWaveDisplay->move( 5, 241 );
	connect( m_pWaveDisplay, SIGNAL( doubleClicked(QWidget*) ),
			 this, SLOT( waveDisplayDoubleClicked(QWidget*) ) );

	m_pLoadLayerBtn = new Button( m_pLayerProp, QSize( 92, 18 ), Button::Type::Push,
								  "", pCommonStrings->getLoadLayerButton() );
	m_pLoadLayerBtn->setObjectName( "LoadLayerButton" );
	m_pLoadLayerBtn->move( 5, 304 );

	m_pRemoveLayerBtn = new Button( m_pLayerProp, QSize( 94, 18 ), Button::Type::Push,
									"", pCommonStrings->getDeleteLayerButton() );
	m_pRemoveLayerBtn->setObjectName( "RemoveLayerButton" );
	m_pRemoveLayerBtn->move( 97, 304 );

	m_pSampleEditorBtn = new Button( m_pLayerProp, QSize( 92, 18 ), Button::Type::Push,
									 "", pCommonStrings->getEditLayerButton() );
	m_pSampleEditorBtn->setObjectName( "SampleEditorButton" );
	m_pSampleEditorBtn->move( 191, 304 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( loadLayerBtnClicked() ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked() ),
			 this, SLOT( removeLayerButtonClicked() ) );
	connect( m_pSampleEditorBtn, SIGNAL( clicked() ),
			 this, SLOT( showSampleEditor() ) );
	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( m_pLayerProp, QSize( 36, 16 ), false, false );
	m_pLayerGainRotary = new Rotary( m_pLayerProp, Rotary::Type::Normal,
									 tr( "Layer gain" ), false , 0.0, 5.0);
	m_pLayerGainRotary->setDefaultValue( 1.0 );
	connect( m_pLayerGainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerGainLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
										  pCommonStrings->getLayerGainLabel() );
	m_pLayerGainLbl->move( 50, 360 );

	m_pCompoGainLCD = new LCDDisplay( m_pLayerProp, QSize( 36, 16 ), false, false );
	m_pCompoGainRotary = new Rotary( m_pLayerProp, Rotary::Type::Normal,
									 tr( "Component volume" ), false, 0.0, 5.0 );
	m_pCompoGainRotary->setDefaultValue ( 1.0 );
	connect( m_pCompoGainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pCompoGainLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
										  pCommonStrings->getComponentGainLabel() );
	m_pCompoGainLbl->move( 147, 360 );

	m_pLayerPitchCoarseLCD = new LCDDisplay( m_pLayerProp, QSize( 28, 16 ), false, false );
	m_pLayerPitchFineLCD = new LCDDisplay( m_pLayerProp, QSize( 28, 16 ), false, false );
	m_pLayerPitchLbl = new ClickableLabel( m_pLayerProp, QSize( 45, 10 ),
										   pCommonStrings->getPitchLabel() );
	m_pLayerPitchLbl->move( 17, 412 );

	m_pLayerPitchCoarseRotary = new Rotary( m_pLayerProp, Rotary::Type::Center,
											tr( "Layer pitch (Coarse)" ), true, -24.0, 24.0 );
	connect( m_pLayerPitchCoarseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerPitchCoarseLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
												 pCommonStrings->getPitchCoarseLabel() );
	m_pLayerPitchCoarseLbl->move( 61, 412 );

	m_pLayerPitchFineRotary = new Rotary( m_pLayerProp, Rotary::Type::Center,
										  tr( "Layer pitch (Fine)" ), true, -50.0, 50.0 );
	connect( m_pLayerPitchFineRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerPitchFineLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
											   pCommonStrings->getPitchFineLabel() );
	m_pLayerPitchFineLbl->move( 147, 412 );

	m_pLayerGainLCD->move( 53, 343 );
	m_pLayerGainRotary->move( 94, 341 );

	m_pCompoGainLCD->move( 151, 343 );
	m_pCompoGainRotary->move( 191, 341 );

	m_pLayerPitchCoarseLCD->move( 70, 393 );
	m_pLayerPitchCoarseRotary->move( 105, 391 );

	m_pLayerPitchFineLCD->move( 155, 393 );
	m_pLayerPitchFineRotary->move( 191, 391 );

	m_pSampleSelectionCombo = new LCDCombo(m_pLayerProp, QSize( width() - 76 - 7, 18 ), true );
	m_pSampleSelectionCombo->move( 76, 432 );

	m_pSampleSelectionCombo->setToolTip( tr( "Select selection algorithm" ) );
	setupSampleSelectionCombo();
	connect( m_pSampleSelectionCombo, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( sampleSelectionChanged( int ) ) );
	m_pSampleSelectionLbl = new ClickableLabel( m_pLayerProp, QSize( 70, 10 ),
												pCommonStrings->getSampleSelectionLabel() );
	m_pSampleSelectionLbl->move( 7, 436 );

	// ~ Layer properties

	//component handling
	m_pComponentMenu = new QMenu( this );
	updateComponentLabels();
	populateComponentMenu();
	// ~ component handling

	showLayers( false );

	HydrogenApp::get_instance()->addEventListener(this);

	selectedInstrumentChangedEvent(); 	// force an update
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentEditor::onPreferencesChanged );
}



InstrumentEditor::~InstrumentEditor()
{
	//INFOLOG( "DESTROY" );
}

void InstrumentEditor::activate( bool bActivate ) {
	if ( m_bIsActive == bActivate ) {
		return;
	}

	m_bIsActive = bActivate;

	if ( bActivate ) {
		m_pNameLbl->setEnabled( true );
		m_pCompoNameLbl->setEnabled( true );

		m_pAttackRotary->setIsActive( true );
		m_pDecayRotary->setIsActive( true );
		m_pSustainRotary->setIsActive( true );
		m_pReleaseRotary->setIsActive( true );

		m_pPitchCoarseRotary->setIsActive( true );
		m_pPitchFineRotary->setIsActive( true );
		m_pRandomPitchRotary->setIsActive( true );

		m_pFilterBypassBtn->setIsActive( true );
		m_pCutoffRotary->setIsActive( true );
		m_pResonanceRotary->setIsActive( true );

		m_pInstrumentGain->setIsActive( true );

		m_pApplyVelocity->setEnabled( true );
		m_pMuteGroupLCD->setIsActive( true );

		m_pMidiOutChannelLCD->setIsActive( true );
		m_pMidiOutChannelLCD->update();
		m_pMidiOutNoteLCD->setIsActive( true );

		m_pHihatGroupLCD->setIsActive( true );
		m_pHihatMinRangeLCD->setIsActive( true );
		m_pHihatMaxRangeLCD->setIsActive( true );

		m_pLayerGainRotary->setIsActive( true );
		m_pLayerPitchCoarseRotary->setIsActive( true );
		m_pLayerPitchFineRotary->setIsActive( true );

		m_pSampleSelectionCombo->setIsActive( true );
		setupSampleSelectionCombo();

		m_pLoadLayerBtn->setIsActive( true );
		m_pRemoveLayerBtn->setIsActive( true );
		m_pSampleEditorBtn->setIsActive( true );
		m_pIsStopNoteCheckBox->setEnabled( true );

		m_DropDownCompoBtn->setIsActive( true );
		m_pCompoGainRotary->setIsActive( true );
	}
	else {
		m_pNameLbl->setEnabled( false );
		m_pCompoNameLbl->setEnabled( false );

		m_pAttackRotary->setIsActive( false );
		m_pDecayRotary->setIsActive( false );
		m_pSustainRotary->setIsActive( false );
		m_pReleaseRotary->setIsActive( false );

		m_pPitchCoarseRotary->setIsActive( false );
		m_pPitchFineRotary->setIsActive( false );
		m_pPitchLCD->clear();
		m_pRandomPitchRotary->setIsActive( false );

		m_pFilterBypassBtn->setIsActive( false );
		m_pFilterBypassBtn->setChecked( false );
		m_pCutoffRotary->setIsActive( false );
		m_pResonanceRotary->setIsActive( false );

		m_pInstrumentGainLCD->clear();
		m_pInstrumentGain->setIsActive( false );

		m_pApplyVelocity->setEnabled( false );
		m_pApplyVelocity->setChecked( false );
		m_pMuteGroupLCD->setIsActive( false );
		m_pMuteGroupLCD->clear();

		m_pMidiOutChannelLCD->setIsActive( false );
		m_pMidiOutChannelLCD->clear();
		m_pMidiOutNoteLCD->setIsActive( false );
		m_pMidiOutNoteLCD->clear();

		m_pHihatGroupLCD->setIsActive( false );
		m_pHihatGroupLCD->clear();
		m_pHihatMinRangeLCD->setIsActive( false );
		m_pHihatMinRangeLCD->clear();
		m_pHihatMaxRangeLCD->setIsActive( false );
		m_pHihatMaxRangeLCD->clear();

		m_pLayerGainRotary->setIsActive( false );
		m_pLayerGainLCD->clear();
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setIsActive( false );
		m_pLayerPitchCoarseLCD->clear();
		m_pLayerPitchFineLCD->clear();

		m_pSampleSelectionCombo->setIsActive( false );
		m_pSampleSelectionCombo->clear();

		// m_pWaveDisplay;

		m_pLoadLayerBtn->setIsActive( false );
		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );
		m_pIsStopNoteCheckBox->setChecked( false );
		m_pIsStopNoteCheckBox->setEnabled( false );

		m_DropDownCompoBtn->setIsActive( false );
		m_pCompoGainRotary->setIsActive( false );
		m_pCompoGainLCD->clear();
	}
}

void InstrumentEditor::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateComponentLabels();
		selectedInstrumentChangedEvent();
	}
}

void InstrumentEditor::drumkitLoadedEvent() {
	updateComponentLabels();
	selectedInstrumentChangedEvent();
}

void InstrumentEditor::selectedInstrumentChangedEvent()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pCompoList = pSong->getDrumkit()->getComponents();
	
	m_pInstrument = pHydrogen->getSelectedInstrument();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// update layer list
	if ( pSong != nullptr && m_pInstrument != nullptr &&
		 pCompoList != nullptr && pCompoList->size() > 0 ) {

		activate( true );

		m_pNameLbl->setText( m_pInstrument->get_name() );

		// ADSR
		m_pAttackRotary->setValue( sqrtf(m_pInstrument->get_adsr()->getAttack() / 100000.0) );
		m_pDecayRotary->setValue( sqrtf(m_pInstrument->get_adsr()->getDecay() / 100000.0) );
		m_pSustainRotary->setValue( m_pInstrument->get_adsr()->getSustain() );
		float fTmp = m_pInstrument->get_adsr()->getRelease() - 256.0;
		if( fTmp < 0.0 ) {
			fTmp = 0.0;
		}
		m_pReleaseRotary->setValue( sqrtf(fTmp / 100000.0) );
		// ~ ADSR

		// filter
		m_pFilterBypassBtn->setChecked( !m_pInstrument->is_filter_active() );
		m_pCutoffRotary->setValue( m_pInstrument->get_filter_cutoff() );
		m_pResonanceRotary->setValue( m_pInstrument->get_filter_resonance() );
		// ~ filter

		// pitch offset
		char tmp[7];
		sprintf( tmp, "%#.2f", m_pInstrument->get_pitch_offset() );
		m_pPitchLCD->setText( tmp );
		
		/* fCoarsePitch is the closest integer to pitch_offset (represents the pitch shift interval in half steps)
		while it is an integer number, it's defined float to be used in next lines */
		float fCoarsePitch = round( m_pInstrument->get_pitch_offset() );

		//fFinePitch represents the fine adjustment (between -0.5 and +0.5) if pitch_offset has decimal part
		float fFinePitch = m_pInstrument->get_pitch_offset() - fCoarsePitch;

		m_pPitchCoarseRotary->setValue( fCoarsePitch );
		m_pPitchFineRotary->setValue( fFinePitch );
		
		// pitch random
		m_pRandomPitchRotary->setValue( m_pInstrument->get_random_pitch_factor() );

		//Stop Note
		m_pIsStopNoteCheckBox->setChecked( m_pInstrument->is_stop_notes() );

		//Ignore Velocity
		m_pApplyVelocity->setChecked( m_pInstrument->get_apply_velocity() );

		// instr gain
		sprintf( tmp, "%#.2f", m_pInstrument->get_gain() );
		m_pInstrumentGainLCD->setText( tmp );
		m_pInstrumentGain->setValue( m_pInstrument->get_gain() );

		// instr mute group
		m_pMuteGroupLCD->setValue( m_pInstrument->get_mute_group());

		// midi out channel
		if ( m_pInstrument->get_midi_out_channel() == -1 ) {
			m_pMidiOutChannelLCD->setValue( -1 ); // turn off
		} else {
			// The MIDI channels start at 1 instead of zero.
			m_pMidiOutChannelLCD->setValue( m_pInstrument->get_midi_out_channel() + 1 );
		}

		//midi out note
		m_pMidiOutNoteLCD->setValue( m_pInstrument->get_midi_out_note() );

		// hihat
		m_pHihatGroupLCD->setValue( m_pInstrument->get_hihat_grp() );
		m_pHihatMinRangeLCD->setValue( m_pInstrument->get_lower_cc() );
		m_pHihatMaxRangeLCD->setValue( m_pInstrument->get_higher_cc() );

		// see instrument.h
		m_pSampleSelectionCombo->setCurrentIndex( m_pInstrument->sample_selection_alg() );

		populateComponentMenu();

		bool bFound = false;
		for ( const auto& ppComponent : *pCompoList ) {
			if ( ppComponent != nullptr &&
				 ppComponent->get_id() == m_nSelectedComponent ) {
				bFound = true;
				break;
			}
		}
		if ( ! bFound ){
			selectComponent( pCompoList->front()->get_id() );
		}

		auto pTmpComponent = pSong->getDrumkit()->getComponent( m_nSelectedComponent );
		if ( pTmpComponent == nullptr ) {
			ERRORLOG( QString( "Unable to get component [%1]" )
					  .arg( m_nSelectedComponent ) );
			return;
		}

		m_pCompoNameLbl->setText(
			m_uniqueComponentLabels[ m_nSelectedComponent ] );

		if ( m_nSelectedLayer >= 0 ) {
			
			auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
			if ( pComponent != nullptr ) {

				char tmp[20];
				sprintf( tmp, "%#.2f", pComponent->get_gain());
				m_pCompoGainLCD->setText( tmp );

				m_pCompoGainRotary->setValue( pComponent->get_gain() );

				auto pLayer = pComponent->get_layer( m_nSelectedLayer );
				if ( pLayer != nullptr ) {
					m_pWaveDisplay->updateDisplay( pLayer );
				}
				else {
					m_pWaveDisplay->updateDisplay( nullptr );
				}
			}
			else {
				m_pWaveDisplay->updateDisplay( nullptr );
			}
		}
		else {
			m_pWaveDisplay->updateDisplay( nullptr );
		}
	}
	else {
		activate( false );
		m_pNameLbl->setText( "" );
		m_pCompoNameLbl->setText( "" );
		m_pWaveDisplay->updateDisplay( nullptr );
		m_nSelectedLayer = 0;
	}

	selectLayer( m_nSelectedLayer );
}

// In here we just check those parameters that can be altered by MIDI
// or OSC messages or other parts of Hydrogen.
void InstrumentEditor::instrumentParametersChangedEvent( int nInstrumentNumber )
{
	auto pInstrumentList = Hydrogen::get_instance()->getSong()->getDrumkit()->getInstruments();
	
	// Check if either this particular line or all lines should be updated.
	if ( m_pInstrument != nullptr ) {
		activate( true );

		if ( m_pInstrument == pInstrumentList->get( nInstrumentNumber ) ||
			 nInstrumentNumber == -1 ) {

			if ( m_pNameLbl->text() != m_pInstrument->get_name() ) {
				m_pNameLbl->setText( m_pInstrument->get_name() );
			}

			// filter
			m_pFilterBypassBtn->setChecked( !m_pInstrument->is_filter_active() );
			m_pCutoffRotary->setValue( m_pInstrument->get_filter_cutoff() );
			m_pResonanceRotary->setValue( m_pInstrument->get_filter_resonance() );
			// ~ filter
		}
		// In case nInstrumentNumber does not belong to the currently
		// selected instrument we don't have to do anything.
	}
	else {
		activate( false );

		m_pNameLbl->setText( "" );
		m_pCompoNameLbl->setText( "" );
		m_pWaveDisplay->updateDisplay( nullptr );
		m_nSelectedLayer = 0;
	}

	selectLayer( m_nSelectedLayer );
}

void InstrumentEditor::rotaryChanged( WidgetWithInput *ref)
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	assert( ref );
	Rotary* pRotary = static_cast<Rotary*>( ref );
	
	float fVal = pRotary->getValue();

	if ( pRotary == m_pRandomPitchRotary ){
		m_pInstrument->set_random_pitch_factor( fVal );
	}
	else if ( pRotary == m_pPitchCoarseRotary ) {
		//round fVal, since Coarse is the integer number of half steps
		float newPitch = round( fVal ) + m_pPitchFineRotary->getValue();
		m_pInstrument->set_pitch_offset( newPitch );
		char tmp[7];
		sprintf( tmp, "%#.2f", newPitch);
		m_pPitchLCD->setText( tmp );
	}
	else if ( pRotary == m_pPitchFineRotary ) {
		float newPitch = round( m_pPitchCoarseRotary->getValue() ) + fVal;
		m_pInstrument->set_pitch_offset( newPitch );
		char tmp[7];
		sprintf( tmp, "%#.2f", newPitch);
		m_pPitchLCD->setText( tmp );
	}
	else if ( pRotary == m_pCutoffRotary ) {
		m_pInstrument->set_filter_cutoff( fVal );
	}
	else if ( pRotary == m_pResonanceRotary ) {
		if ( fVal > 0.95f ) {
			fVal = 0.95f;
		}
		m_pInstrument->set_filter_resonance( fVal );
	}
	else if ( pRotary == m_pAttackRotary ) {
		m_pInstrument->get_adsr()->setAttack( fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pDecayRotary ) {
		m_pInstrument->get_adsr()->setDecay( fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pSustainRotary ) {
		m_pInstrument->get_adsr()->setSustain( fVal );
	}
	else if ( pRotary == m_pReleaseRotary ) {
		m_pInstrument->get_adsr()->setRelease( 256.0 + fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pLayerGainRotary ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", fVal );
		m_pLayerGainLCD->setText( tmp );

		auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->get_layer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->set_gain( fVal );
				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
	}
	else if ( pRotary == m_pCompoGainRotary ) {
		char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pCompoGainLCD->setText( tmp );

			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			pCompo->set_gain( fVal );
	}
	else if ( pRotary == m_pLayerPitchCoarseRotary ) {
		m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int) round( fVal ) ) );

		auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->get_layer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->set_pitch( fCoarse + fFine );
			}
		}
	}
	else if ( pRotary == m_pLayerPitchFineRotary ) {
		m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal, 0, 'f', 0 ) );
		auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->get_layer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->set_pitch( fCoarse + fFine );
			}
		}
	}
	else if ( pRotary == m_pInstrumentGain ) {
		fVal = fVal;
		char tmp[20];
		sprintf( tmp, "%#.2f", fVal );
		m_pInstrumentGainLCD->setText( tmp );
		m_pInstrument->set_gain( fVal );
	}
	else {
		ERRORLOG( "[rotaryChanged] unhandled rotary" );
	}
}


void InstrumentEditor::filterActiveBtnClicked()
{
	if ( m_pInstrument != nullptr ) {
		m_pInstrument->set_filter_active( ! m_pFilterBypassBtn->isChecked() );
	}
}


void InstrumentEditor::waveDisplayDoubleClicked( QWidget* pRef )
{		
	if ( m_pInstrument == nullptr ) {
		return;
	}
	
	auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
	if( !pCompo ) {
		return;
	}
			
	auto pLayer = pCompo->get_layer( m_nSelectedLayer );
	if ( pLayer != nullptr ) {
		auto pSample = pLayer->get_sample();
		
		if( pSample != nullptr ) {
			QString name = pSample->get_filepath();
			HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
		}
	}
	else {
		loadLayerBtnClicked();
	}
}

void InstrumentEditor::showSampleEditor()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
	if ( pCompo != nullptr ) {
		auto pLayer = pCompo->get_layer( m_nSelectedLayer );
		if ( pLayer != nullptr ) {
			auto pSample = pLayer->get_sample();
			if ( pSample != nullptr ) {
				QString name = pSample->get_filepath();
				HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
			}
		}
	}
}

void InstrumentEditor::removeLayerButtonClicked()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	auto pCompo = m_pInstrument->get_component( m_nSelectedComponent );
	if ( pCompo != nullptr ) {
		pCompo->set_layer( nullptr, m_nSelectedLayer );

		pHydrogen->setIsModified( true );

		// Select next loaded layer - if available - in order to
		// allow for a quick removal of all layers. In case the
		// last layer was removed, the previous one will be
		// selected.
		int nNextLayerIndex = 0;
		int nCount = 0;
		for( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
			auto pLayer = pCompo->get_layer( n );
			if( pLayer != nullptr ){
				nCount++;

				if ( nNextLayerIndex <= m_nSelectedLayer &&
					 n != m_nSelectedLayer ) {
					nNextLayerIndex = n;
				}
			}
		}

		if ( nCount == 0 ){
			m_pInstrument->get_components()->erase( m_pInstrument->get_components()->begin() + m_nSelectedComponent );
		} else {
			m_pLayerPreview->setSelectedLayer( nNextLayerIndex );
			InstrumentEditorPanel::get_instance()->selectLayer( nNextLayerIndex );
		}
	}

	pHydrogen->getAudioEngine()->unlock();
	selectedInstrumentChangedEvent();    // update all
	m_pLayerPreview->updateAll();
}



void InstrumentEditor::loadLayerBtnClicked()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	Hydrogen *pHydrogen = Hydrogen::get_instance();

	QString sPath = Preferences::get_instance()->getLastOpenLayerDirectory();
	QString sFilename = "";
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	// In case the button was pressed while a layer was selected, we
	// try to use path of the associated sample as default one.
	if ( m_nSelectedLayer > 0 ) {
		auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );

		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->get_layer( m_nSelectedLayer );

			if ( pLayer != nullptr ) {
				auto pSample = pLayer->get_sample();

				if ( pSample != nullptr ) {
					if ( ! pSample->get_filepath().isEmpty() ) {
						QFileInfo fileInfo( pSample->get_filepath() );
						sPath = fileInfo.absoluteDir().absolutePath();
						sFilename = fileInfo.absoluteFilePath();
					}
				}
			}
		}
	}

	AudioFileBrowser *pFileBrowser =
		new AudioFileBrowser( nullptr, true, true, sPath, sFilename );
	QStringList filename;
	filename << "false" << "false" << "";

	if (pFileBrowser->exec() == QDialog::Accepted) {
		filename = pFileBrowser->getSelectedFiles();
		
		// Only overwrite the default directory if we didn't start
		// from an existing file or the final directory differs from
		// the starting one.
		if ( sFilename.isEmpty() ||
			 sPath != pFileBrowser->getSelectedDirectory() ) {
			Preferences::get_instance()->setLastOpenLayerDirectory( pFileBrowser->getSelectedDirectory() );
		}
	}

	delete pFileBrowser;

	if ( filename[2].isEmpty() ) {
		return;
	}

	bool fnc = false;
	if ( filename[0] ==  "true" ){
		fnc = true;
	}

	int selectedLayer =  m_nSelectedLayer;
	int firstSelection = selectedLayer;

	if (filename.size() > 2) {

		for ( int i=2; i < filename.size(); ++i )
		{
			selectedLayer = m_nSelectedLayer + i - 2;
			if ( ( i-2 >= InstrumentComponent::getMaxLayers() ) ||
				 ( selectedLayer + 1  > InstrumentComponent::getMaxLayers() ) ) {
				break;
			}

			auto pNewSample = Sample::load( filename[i] );

			pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

			/*
				if we're using multiple layers, we start inserting the first layer
				at m_nSelectedLayer and the next layer at m_nSelectedLayer+1
			*/

			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( !pCompo ) {
				pCompo = std::make_shared<InstrumentComponent>( m_nSelectedComponent );
				m_pInstrument->get_components()->push_back( pCompo );
			}

			auto pLayer = pCompo->get_layer( selectedLayer );

			if ( pLayer != nullptr ) {
				// insert new sample from newInstrument, old sample gets deleted by set_sample
				pLayer->set_sample( pNewSample );
			}
			else {
				pLayer = std::make_shared<H2Core::InstrumentLayer>( pNewSample );
				m_pInstrument->get_component(m_nSelectedComponent)->set_layer( pLayer, selectedLayer );
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

			pHydrogen->getAudioEngine()->unlock();
		}

		pHydrogen->setIsModified( true );
	}

	selectedInstrumentChangedEvent();    // update all
	selectLayer( firstSelection );
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::setAutoVelocity()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}
	auto pCompo = m_pInstrument->get_component( m_nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}

	int nLayers = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers() ; i++ ) {

		auto pLayer = pCompo->get_layer( i );
		if ( pLayer != nullptr ) {
			nLayers++;
		}
	}
	
	if( nLayers == 0 ){
		nLayers = 1;
	}

	float velocityrange = 1.0 / nLayers;

	int nLayer = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers() ; i++ ) {
		auto pLayer = pCompo->get_layer( i );
		if ( pLayer != nullptr ) {
			pLayer->set_start_velocity( nLayer * velocityrange);
			pLayer->set_end_velocity( nLayer * velocityrange + velocityrange );
			
			++nLayer;
		}
	}
}

void InstrumentEditor::renameComponentAction()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	auto pComponent = pDrumkit->getComponent( m_nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve selected component [%1]" )
				  .arg( m_nSelectedComponent ) );
		return;
	}

	QString sOldName = pComponent->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New component name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );

	if ( bIsOkPressed && sOldName != sNewName ) {
		 auto pAction = new SE_renameComponentAction(
			 sNewName, sOldName, m_nSelectedComponent );
		 HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
	}
}

void InstrumentEditor::renameComponent( int nComponentId, const QString& sNewName ) {
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	auto pComponent = pDrumkit->getComponent( nComponentId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nComponentId ) );
		return;
	}

	pComponent->set_name( sNewName );
	m_pCompoNameLbl->setText( sNewName );
	updateComponentLabels();
	populateComponentMenu();

	Hydrogen::get_instance()->setIsModified( true );

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void InstrumentEditor::selectComponent( int nComponent )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	m_nSelectedComponent = nComponent;
	m_pLayerPreview->set_selected_component( m_nSelectedComponent );
}

void InstrumentEditor::labelClicked( ClickableLabel* pRef )
{
	UNUSED( pRef );

	if ( m_pInstrument == nullptr ) {
		return;
	}

	QString sOldName = m_pInstrument->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );

	if ( bIsOkPressed && sNewName != sOldName ) {
		auto pHydrogen = Hydrogen::get_instance();

		m_pInstrument->set_name( sNewName );
		selectedInstrumentChangedEvent();

		pHydrogen->setIsModified( true );

#ifdef H2CORE_HAVE_JACK
		pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pHydrogen->renameJackPorts( pHydrogen->getSong() );
		pHydrogen->getAudioEngine()->unlock();
#endif

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
	}
	else {
		// user entered nothing or pressed Cancel
	}
}


void InstrumentEditor::selectLayer( int nLayer )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	m_nSelectedLayer = nLayer;

	auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
	if(pComponent && nLayer >= 0 ){
		auto pLayer = pComponent->get_layer( nLayer );
		m_pWaveDisplay->updateDisplay( pLayer );
		if ( pLayer != nullptr ) {
			char tmp[20];

			// Layer GAIN
			m_pLayerGainRotary->setIsActive( true );
			m_pLayerGainRotary->setValue( pLayer->get_gain() );
			sprintf( tmp, "%#.2f", pLayer->get_gain() );
			m_pLayerGainLCD->setText( tmp );

			//Component GAIN
			char tmp2[20];
			sprintf( tmp2, "%#.2f", pComponent->get_gain());
			m_pCompoGainRotary->setIsActive( true );
			m_pCompoGainRotary->setValue( pComponent->get_gain());
			m_pCompoGainLCD->setText( tmp2 );

			// Layer PITCH
			float fCoarsePitch = round( pLayer->get_pitch() );
			float fFinePitch = pLayer->get_pitch() - fCoarsePitch;
			m_pLayerPitchCoarseRotary->setIsActive( true );
			m_pLayerPitchCoarseRotary->setValue( fCoarsePitch );
			m_pLayerPitchFineRotary->setIsActive( true );
			m_pLayerPitchFineRotary->setValue( fFinePitch * 100 );

			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int) fCoarsePitch ) );
			m_pLayerPitchFineLCD->setText( QString( "%1" )
										   .arg( fFinePitch * 100, 0, 'f', 0 ) );

			m_pRemoveLayerBtn->setIsActive( true );
			m_pSampleEditorBtn->setIsActive( true );
		}
		else {
			// Layer GAIN
			m_pLayerGainRotary->setIsActive( false );
			m_pLayerGainRotary->setValue( 1.0 );
			m_pLayerGainLCD->setText( "" );

			//Component GAIN
			m_pCompoGainRotary->setIsActive( false );
			m_pCompoGainRotary->setValue( 1.0 );
			m_pCompoGainLCD->setText( "" );

			// Layer PITCH
			m_pLayerPitchCoarseRotary->setIsActive( false );
			m_pLayerPitchCoarseRotary->setValue( 0.0 );
			m_pLayerPitchFineRotary->setIsActive( false );
			m_pLayerPitchFineRotary->setValue( 0.0 );

			m_pLayerPitchCoarseLCD->setText( "" );
			m_pLayerPitchFineLCD->setText( "" );

			m_pRemoveLayerBtn->setIsActive( false );
			m_pSampleEditorBtn->setIsActive( false );
		}
	}
	else {
		m_pWaveDisplay->updateDisplay( nullptr );

		// Layer GAIN
		m_pLayerGainRotary->setIsActive( false );
		m_pLayerGainRotary->setValue( 1.0 );
		m_pLayerGainLCD->setText( "" );

		m_pCompoGainRotary->setIsActive( false );
		m_pCompoGainRotary->setValue( 1.0 );
		m_pCompoGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchCoarseRotary->setValue( 0.0 );
		m_pLayerPitchFineRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setValue( 0.0 );

		m_pLayerPitchCoarseLCD->setText( "" );
		m_pLayerPitchFineLCD->setText( "" );

		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );
	}
}



void InstrumentEditor::muteGroupChanged( double fValue )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	m_pInstrument->set_mute_group( static_cast<int>(fValue) );
	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::onIsStopNoteCheckBoxClicked( bool on )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	if ( m_pInstrument->is_stop_notes() != on ) {
		m_pInstrument->set_stop_notes( on );
		Hydrogen::get_instance()->setIsModified( true );
		selectedInstrumentChangedEvent();	// force an update
	}
}

void InstrumentEditor::onIsApplyVelocityCheckBoxClicked( bool on )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	if ( m_pInstrument->get_apply_velocity() != on ) {
		m_pInstrument->set_apply_velocity( on );
		Hydrogen::get_instance()->setIsModified( true );
		selectedInstrumentChangedEvent();	// force an update
	}
}

void InstrumentEditor::midiOutChannelChanged( double fValue ) {
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	if ( fValue != 0.0 ) {
		m_pInstrument->set_midi_out_channel( std::max( static_cast<int>(fValue) - 1,
													   -1 ) );
	} else {
		if ( m_fPreviousMidiOutChannel == -1.0 ) {
			m_pMidiOutChannelLCD->setValue( 1 );
			return;
		} else {
			m_pMidiOutChannelLCD->setValue( -1 );
			return;
		}
	}

	m_fPreviousMidiOutChannel = fValue;
}

void InstrumentEditor::midiOutNoteChanged( double fValue ) {
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	m_pInstrument->set_midi_out_note( static_cast<int>(fValue) );
}

void InstrumentEditor::onDropDownCompoClicked()
{
	m_pComponentMenu->popup( m_pCompoNameLbl->mapToGlobal( QPoint( m_pCompoNameLbl->width() - 40, m_pCompoNameLbl->height() / 2 ) ) );
}

void InstrumentEditor::populateComponentMenu()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "invalid drumkit" );
		return;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pComponentMenu->clear();

	// Actions to switch between the drumkits
	for ( const auto& ppComponent : *pDrumkit->getComponents() ) {
		if ( ppComponent != nullptr ) {
			auto pAction = m_pComponentMenu->addAction(
				m_uniqueComponentLabels[ ppComponent->get_id() ], this,
				[=](){ switchComponentAction( ppComponent->get_id() ); } );;
			if ( ppComponent->get_id() == m_nSelectedComponent ) {
				m_pComponentMenu->setDefaultAction( pAction );
			}
		}
	}
	m_pComponentMenu->addSeparator();
	m_pComponentMenu->addAction( pCommonStrings->getMenuActionAdd(), this,
								 SLOT( addComponentAction() ) );
	auto pDeleteAction = m_pComponentMenu->addAction(
		pCommonStrings->getMenuActionDelete(), this, SLOT( deleteComponentAction() ) );
	if ( pDrumkit->getComponents()->size() < 2 ) {
		// If there is just a single component present, it must not be removed.
		pDeleteAction->setEnabled( false );
	}

	m_pComponentMenu->addAction( pCommonStrings->getMenuActionRename(), this,
								 SLOT( renameComponentAction() ) );
}

void InstrumentEditor::updateComponentLabels() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	m_uniqueComponentLabels = pDrumkit->generateUniqueComponentLabels();

}

void InstrumentEditor::addComponentAction() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	bool bIsOkPressed;
	QString sNewName =
		QInputDialog::getText( this, "Hydrogen", tr( "Component name" ),
							   QLineEdit::Normal, "New Component",
							   &bIsOkPressed );
	if ( ! bIsOkPressed ) {
		// Dialog closed using cancel
		return;
	}

	auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
	auto pNewDrumkitComponent = pNewDrumkit->addComponent();
	pNewDrumkitComponent->set_name( sNewName );

	selectComponent( pNewDrumkitComponent->get_id() );

	auto pAction = new SE_switchDrumkitAction(
		pNewDrumkit, pDrumkit, false,
		SE_switchDrumkitAction::Type::AddComponent, sNewName );
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );


#ifdef H2CORE_HAVE_JACK
	pHydrogen->renameJackPorts( pSong );
#endif
}

void InstrumentEditor::deleteComponentAction() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	if ( pDrumkit->getComponents()->size() == 1 ) {
		ERRORLOG( "There is just a single component remaining. This one can not be deleted." );
		return;
	}

	auto pComponent = pDrumkit->getComponent( m_nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to find selected component [%1]" )
				  .arg( m_nSelectedComponent ) );
		return;
	}

	auto sOldName = pComponent->get_name();

	auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
	pNewDrumkit->removeComponent( m_nSelectedComponent );

	// Fall back to the first component.
	selectComponent( pDrumkit->getComponents()->front()->get_id() );

	// Undoing the deletion of a drumkit component is a rather difficult path as
	// it also involves all associated instrument components and their samples.
	// It's both more easy and clean to just switch between the entire drumkits.
	auto pAction = new SE_switchDrumkitAction(
		pNewDrumkit, pDrumkit, false,
		SE_switchDrumkitAction::Type::DeleteComponent, sOldName );
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}

void InstrumentEditor::switchComponentAction( int nId ) {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	int nSelectedComponent = -1;
	auto pDrumkitComponents = pDrumkit->getComponents();
	for ( const auto& pComponent : *pDrumkitComponents ) {
		if ( pComponent->get_id() == nId ) {
			nSelectedComponent = pComponent->get_id();
			break;
		}
	}

	auto pComponent = pDrumkit->getComponent( nId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nId ) );
		return;
	}

	m_pCompoNameLbl->setText( m_uniqueComponentLabels[ nId ] );

	selectComponent( nId );

	selectedInstrumentChangedEvent();

	// this will force an update...
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void InstrumentEditor::sampleSelectionChanged( int selected )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 if ( m_pSampleSelectionCombo->count() < 3 ) {
		 // We flushed the widget while disabling the InstrumentEditor and are
		 // in the process of refilling it.
		 return;
	 }

	if ( selected == 0 ){
		m_pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
	}
	else if ( selected == 1 ){
		m_pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
	}
	else if ( selected == 2){
		m_pInstrument->set_sample_selection_alg( Instrument::RANDOM );
	}

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::hihatGroupChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->set_hihat_grp( static_cast<int>(fValue) );
}

void InstrumentEditor::hihatMinRangeChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->set_lower_cc( static_cast<int>(fValue) );
	 m_pHihatMaxRangeLCD->setMinimum( static_cast<int>(fValue) );
}

void InstrumentEditor::hihatMaxRangeChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->set_higher_cc( static_cast<int>(fValue) );
	 m_pHihatMinRangeLCD->setMaximum( static_cast<int>(fValue) );
}

void InstrumentEditor::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		setStyleSheet( QString( "QLabel { background: %1 }" )
					   .arg( pPref->getColorTheme()->m_windowColor.name() ) );
	}
}

void InstrumentEditor::showLayers( bool bShow ) {
	if ( bShow ) {
		m_pInstrumentProp->hide();
		m_pShowInstrumentBtn->setChecked( false );
		m_pLayerProp->show();
		m_pShowLayersBtn->setChecked( true );
	}
	else {
		m_pInstrumentProp->show();
		m_pShowInstrumentBtn->setChecked( true );
		m_pLayerProp->hide();
		m_pShowLayersBtn->setChecked( false );
	}
}

void InstrumentEditor::setupSampleSelectionCombo() {
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "First in Velocity" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Round Robin" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Random" ) );
}
