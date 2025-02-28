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

#include <QtGui>
#include <QtWidgets>


#include <math.h>
#include <assert.h>
#include <vector>

#include <core/Hydrogen.h>
#include <core/Globals.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
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

	connect( m_pNameLbl, &ClickableLabel::labelClicked, this, [=](){
		auto pSong = Hydrogen::get_instance()->getSong();
		if ( m_pInstrument != nullptr && pSong != nullptr &&
			 pSong->getDrumkit() != nullptr ) {
			MainForm::action_drumkit_renameInstrument(
				pSong->getDrumkit()->getInstruments()->index( m_pInstrument ) );}
	} );

	m_pPitchLCD = new LCDDisplay( m_pInstrumentProp, QSize( 56, 20 ), false, false );
	m_pPitchLCD->move( 24, 213 );
	m_pPitchLbl = new ClickableLabel( m_pInstrumentProp, QSize( 54, 10 ),
									  pCommonStrings->getPitchLabel() );
	m_pPitchLbl->move( 25, 235 );
	
	const float fFinePitch = 0.5;
	m_pPitchCoarseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center,
									   tr( "Pitch offset (Coarse)" ), true,
									   Instrument::fPitchMin + fFinePitch,
									   Instrument::fPitchMax - fFinePitch );
	m_pPitchCoarseRotary->move( 84, 210 );

	connect( m_pPitchCoarseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchCoarseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 82, 235 );

	m_pPitchFineRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center,
									 tr( "Pitch offset (Fine)" ), false,
									 -fFinePitch, fFinePitch );
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

	m_pIsStopNoteCheckBox = new QCheckBox( "", m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 42, 139 );
	m_pIsStopNoteCheckBox->adjustSize();
	m_pIsStopNoteCheckBox->setFixedSize( 14, 14 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, SIGNAL( clicked( bool ) ),
			 this, SLOT( onIsStopNoteCheckBoxClicked( bool ) ) );
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
										   pCommonStrings->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 59, 144 );

	m_pApplyVelocity = new QCheckBox( "", m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 139 );
	m_pApplyVelocity->adjustSize();
	m_pApplyVelocity->setFixedSize( 14, 14 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, SIGNAL( clicked( bool ) ),
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
											tr( "Layer pitch (Coarse)" ), true,
											Instrument::fPitchMin + fFinePitch,
											Instrument::fPitchMax - fFinePitch );
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
	connect( m_pSampleSelectionCombo, SIGNAL( activated( int ) ),
			 this, SLOT( sampleSelectionChanged( int ) ) );
	m_pSampleSelectionLbl = new ClickableLabel( m_pLayerProp, QSize( 70, 10 ),
												pCommonStrings->getSampleSelectionLabel() );
	m_pSampleSelectionLbl->move( 7, 436 );

	// ~ Layer properties

	//component handling
	m_pComponentMenu = new QMenu( this );
	populateComponentMenu();
	// ~ component handling

	showLayers( false );

	HydrogenApp::get_instance()->addEventListener(this);

	updateEditor();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentEditor::onPreferencesChanged );
}



InstrumentEditor::~InstrumentEditor()
{
}

void InstrumentEditor::updateEditor()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pInstrument = pHydrogen->getSelectedInstrument();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pSong != nullptr && m_pInstrument != nullptr ) {

		// As each instrument can have an arbitrary compoments, we have to
		// ensure to select a valid one.
		if ( m_nSelectedComponent >= m_pInstrument->getComponents()->size() ) {
			m_nSelectedComponent = std::clamp(
				m_nSelectedComponent, 0,
				static_cast<int>(m_pInstrument->getComponents()->size()) - 1 );
		}

		activate( true );

		m_pNameLbl->setText( m_pInstrument->getName() );

		// ADSR
		m_pAttackRotary->setValue(
			sqrtf(m_pInstrument->getAdsr()->getAttack() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pDecayRotary->setValue(
			sqrtf(m_pInstrument->getAdsr()->getDecay() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pSustainRotary->setValue( m_pInstrument->getAdsr()->getSustain(),
									false, Event::Trigger::Suppress );
		const float fRelease =
			std::max( m_pInstrument->getAdsr()->getRelease() - 256.0, 0.0 );
		m_pReleaseRotary->setValue( sqrtf( fRelease / 100000.0 ), false,
									Event::Trigger::Suppress );
		// ~ ADSR

		// filter
		m_pFilterBypassBtn->setChecked( !m_pInstrument->isFilterActive() );
		m_pCutoffRotary->setValue( m_pInstrument->getFilterCutoff(), false,
								   Event::Trigger::Suppress );
		m_pResonanceRotary->setValue( m_pInstrument->getFilterResonance(),
									  false, Event::Trigger::Suppress );
		// ~ filter

		updateInstrumentPitch();

		// pitch random
		m_pRandomPitchRotary->setValue( m_pInstrument->getRandomPitchFactor(),
										false, Event::Trigger::Suppress );


		//Stop Note
		m_pIsStopNoteCheckBox->setChecked( m_pInstrument->isStopNotes() );

		//Ignore Velocity
		m_pApplyVelocity->setChecked( m_pInstrument->getApplyVelocity() );

		// instr gain
		m_pInstrumentGainLCD->setText(
			QString( "%1" ).arg( m_pInstrument->getGain(), -2, 'f', 2, '0' ) );
		m_pInstrumentGain->setValue( m_pInstrument->getGain(), false,
									 Event::Trigger::Suppress );

		// instr mute group
		m_pMuteGroupLCD->setValue(
			m_pInstrument->getMuteGroup(), Event::Trigger::Suppress );

		// midi out channel
		if ( m_pInstrument->getMidiOutChannel() == -1 ) {
			// turn off
			m_pMidiOutChannelLCD->setValue( -1, Event::Trigger::Suppress );
		}
		else {
			// The MIDI channels start at 1 instead of zero.
			m_pMidiOutChannelLCD->setValue(
				m_pInstrument->getMidiOutChannel() + 1,
				Event::Trigger::Suppress );
		}

		//midi out note
		m_pMidiOutNoteLCD->setValue( m_pInstrument->getMidiOutNote(),
									 Event::Trigger::Suppress );

		// hihat
		m_pHihatGroupLCD->setValue( m_pInstrument->getHihatGrp(),
									Event::Trigger::Suppress );
		m_pHihatMinRangeLCD->setValue( m_pInstrument->getLowerCc(),
									   Event::Trigger::Suppress );
		m_pHihatMaxRangeLCD->setValue( m_pInstrument->getHigherCc(),
									   Event::Trigger::Suppress );

		// see instrument.h
		m_pSampleSelectionCombo->setCurrentIndex(
			m_pInstrument->sampleSelectionAlg() );

		populateComponentMenu();

		const auto pComponent =
			m_pInstrument->getComponent( m_nSelectedComponent );
		if ( pComponent != nullptr ) {
			m_pCompoNameLbl->setText( pComponent->getName() );
			m_pCompoGainLCD->setText(
				QString( "%1" ).arg( pComponent->getGain(),
									 -2, 'f', 2, '0' ) );
			m_pCompoGainRotary->setValue( pComponent->getGain(), false,
										  Event::Trigger::Suppress );
			if ( ! m_pCompoGainRotary->getIsActive() ) {
				m_pCompoGainRotary->setIsActive( true );
			}

			if ( m_nSelectedLayer >= 0 ) {
				const auto pLayer = pComponent->getLayer( m_nSelectedLayer );
				if ( pLayer != nullptr ) {
					m_pWaveDisplay->updateDisplay( pLayer );
				} else {
					m_pWaveDisplay->updateDisplay( nullptr );
				}
			}
			else {
				m_pWaveDisplay->updateDisplay( nullptr );
			}
		}
		else {
			m_pCompoNameLbl->setText( "" );
			m_pCompoGainLCD->setText( "" );
			m_pCompoGainRotary->setValue( 0, false, Event::Trigger::Suppress );
			m_pCompoGainRotary->setIsActive( false );
			m_pWaveDisplay->updateDisplay( nullptr );
		}
	}
	else {
		activate( false );
		m_pNameLbl->setText( "" );
		m_pCompoNameLbl->setText( "" );
		m_pCompoGainLCD->setText( "" );
		m_pCompoGainRotary->setValue( 0, false, Event::Trigger::Suppress );
		m_pCompoGainRotary->setIsActive( false );
		m_pWaveDisplay->updateDisplay( nullptr );
		m_nSelectedLayer = 0;
		m_nSelectedComponent = 0;
	}

	selectLayer( m_nSelectedLayer );
	selectComponent( m_nSelectedComponent );
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
		updateEditor();
	}
}

void InstrumentEditor::drumkitLoadedEvent() {
	updateEditor();
}

void InstrumentEditor::selectedInstrumentChangedEvent() {
	updateEditor();
}

void InstrumentEditor::instrumentParametersChangedEvent( int nInstrumentNumber )
{
	auto pSong = Hydrogen::get_instance()->getSong();

	// Check if either this particular line or all lines should be updated.
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 m_pInstrument != nullptr && nInstrumentNumber != -1 &&
		 m_pInstrument !=
		 pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber ) ) {
		// In case nInstrumentNumber does not belong to the currently
		// selected instrument we don't have to do anything.
	}
	else {
		updateEditor();
	}
}

void InstrumentEditor::updateInstrumentPitch() {
	const QString sNewPitch = QString( "%1" )
		.arg( m_pInstrument->getPitchOffset(), -2, 'f', 2, '0' );

	if ( m_pPitchLCD->text() != sNewPitch ) {
		m_pPitchLCD->setText( sNewPitch );
	}

		// pitch offset

	/* fCoarsePitch is the closest integer to pitch_offset (represents the pitch
	   shift interval in half steps) while it is an integer number, it's defined
	   float to be used in next lines */
	float fCoarsePitch;

	// Since the 'fine' rotary covers values from -0.5 to 0.5 e.g. 1.5 can be
	// represented by both coarse: 2, fine: -0.5 and coarse: 1, fine: 0.5. We
	// need some extra logic to avoid unexpected jumping of the rotaries.
	if ( m_pPitchFineRotary->getValue() == 0.5 &&
		 m_pInstrument->getPitchOffset() -
		 trunc( m_pInstrument->getPitchOffset() ) == 0.5 ) {
		fCoarsePitch = trunc( m_pInstrument->getPitchOffset() );
	}
	else if ( m_pPitchFineRotary->getValue() == -0.5 &&
			    trunc( m_pInstrument->getPitchOffset() ) -
					 m_pInstrument->getPitchOffset() == 0.5 ) {
		fCoarsePitch = ceil( m_pInstrument->getPitchOffset() );
	}
	else {
		fCoarsePitch = round( m_pInstrument->getPitchOffset() );
	}

	// fFinePitch represents the fine adjustment (between -0.5 and +0.5) if
	// pitch_offset has decimal part
	const float fFinePitch = m_pInstrument->getPitchOffset() - fCoarsePitch;

	if ( m_pPitchCoarseRotary->getValue() != fCoarsePitch ) {
		m_pPitchCoarseRotary->setValue( fCoarsePitch, false,
										Event::Trigger::Suppress );
	}
	if ( m_pPitchFineRotary->getValue() != fFinePitch ) {
		m_pPitchFineRotary->setValue( fFinePitch, false,
									  Event::Trigger::Suppress );
	}
}

void InstrumentEditor::rotaryChanged( WidgetWithInput *ref)
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	assert( ref );
	Rotary* pRotary = static_cast<Rotary*>( ref );

	float fVal = pRotary->getValue();

	if ( pRotary == m_pRandomPitchRotary ){
		m_pInstrument->setRandomPitchFactor( fVal );
	}
	else if ( pRotary == m_pPitchCoarseRotary ) {
		//round fVal, since Coarse is the integer number of half steps
		float fNewPitch = round( fVal ) + m_pPitchFineRotary->getValue();
		CoreActionController::setInstrumentPitch(
			pSong->getDrumkit()->getInstruments()->index( m_pInstrument ),
			fNewPitch );
	}
	else if ( pRotary == m_pPitchFineRotary ) {
		float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) + fVal;
		CoreActionController::setInstrumentPitch(
			pSong->getDrumkit()->getInstruments()->index( m_pInstrument ),
			fNewPitch );
	}
	else if ( pRotary == m_pCutoffRotary ) {
		m_pInstrument->setFilterCutoff( fVal );
	}
	else if ( pRotary == m_pResonanceRotary ) {
		if ( fVal > 0.95f ) {
			fVal = 0.95f;
		}
		m_pInstrument->setFilterResonance( fVal );
	}
	else if ( pRotary == m_pAttackRotary ) {
		m_pInstrument->getAdsr()->setAttack( fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pDecayRotary ) {
		m_pInstrument->getAdsr()->setDecay( fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pSustainRotary ) {
		m_pInstrument->getAdsr()->setSustain( fVal );
	}
	else if ( pRotary == m_pReleaseRotary ) {
		m_pInstrument->getAdsr()->setRelease( 256.0 + fVal * fVal * 100000 );
	}
	else if ( pRotary == m_pLayerGainRotary ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", fVal );
		m_pLayerGainLCD->setText( tmp );

		auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				pLayer->setGain( fVal );
				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
	}
	else if ( pRotary == m_pCompoGainRotary ) {
		char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pCompoGainLCD->setText( tmp );

			auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
			if ( pCompo != nullptr ) {
				pCompo->setGain( fVal );
			}
	}
	else if ( pRotary == m_pLayerPitchCoarseRotary ) {
		m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int) round( fVal ) ) );

		auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->setPitch( fCoarse + fFine );
			}
		}
	}
	else if ( pRotary == m_pLayerPitchFineRotary ) {
		m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal, 0, 'f', 0 ) );
		auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
		if ( pCompo != nullptr ) {
			auto pLayer = pCompo->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->setPitch( fCoarse + fFine );
			}
		}
	}
	else if ( pRotary == m_pInstrumentGain ) {
		fVal = fVal;
		char tmp[20];
		sprintf( tmp, "%#.2f", fVal );
		m_pInstrumentGainLCD->setText( tmp );
		m_pInstrument->setGain( fVal );
	}
	else {
		ERRORLOG( "[rotaryChanged] unhandled rotary" );
	}
}


void InstrumentEditor::filterActiveBtnClicked()
{
	if ( m_pInstrument != nullptr ) {
		m_pInstrument->setFilterActive( ! m_pFilterBypassBtn->isChecked() );
	}
}


void InstrumentEditor::waveDisplayDoubleClicked( QWidget* pRef )
{		
	if ( m_pInstrument == nullptr ) {
		return;
	}
	
	auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
	if ( pCompo != nullptr ) {
		return;
	}
			
	auto pLayer = pCompo->getLayer( m_nSelectedLayer );
	if ( pLayer != nullptr ) {
		auto pSample = pLayer->getSample();
		
		if( pSample != nullptr ) {
			QString name = pSample->getFilepath();
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

	auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
	if ( pCompo != nullptr ) {
		auto pLayer = pCompo->getLayer( m_nSelectedLayer );
		if ( pLayer != nullptr ) {
			auto pSample = pLayer->getSample();
			if ( pSample != nullptr ) {
				QString name = pSample->getFilepath();
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

	auto pCompo = m_pInstrument->getComponent( m_nSelectedComponent );
	if ( pCompo != nullptr ) {
		pCompo->setLayer( nullptr, m_nSelectedLayer );

		pHydrogen->setIsModified( true );

		// Select next loaded layer - if available - in order to
		// allow for a quick removal of all layers. In case the
		// last layer was removed, the previous one will be
		// selected.
		int nNextLayerIndex = 0;
		int nCount = 0;
		for( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
			auto pLayer = pCompo->getLayer( n );
			if ( pLayer != nullptr ){
				nCount++;

				if ( nNextLayerIndex <= m_nSelectedLayer &&
					 n != m_nSelectedLayer ) {
					nNextLayerIndex = n;
				}
			}
		}

		if ( nCount != 0 ){
			m_pLayerPreview->setSelectedLayer( nNextLayerIndex );
			InstrumentEditorPanel::get_instance()->selectLayer( nNextLayerIndex );
		}
	}

	pHydrogen->getAudioEngine()->unlock();
	updateEditor();
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
		auto pComponent = m_pInstrument->getComponent( m_nSelectedComponent );

		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer( m_nSelectedLayer );

			if ( pLayer != nullptr ) {
				auto pSample = pLayer->getSample();

				if ( pSample != nullptr ) {
					if ( ! pSample->getFilepath().isEmpty() ) {
						QFileInfo fileInfo( pSample->getFilepath() );
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

			auto pCompo = m_pInstrument->getComponent(m_nSelectedComponent);
			if( !pCompo ) {
				pCompo = std::make_shared<InstrumentComponent>();
				m_pInstrument->getComponents()->push_back( pCompo );
			}

			auto pLayer = pCompo->getLayer( selectedLayer );

			if ( pLayer != nullptr ) {
				// insert new sample from newInstrument, old sample gets deleted
				// by setSample
				pLayer->setSample( pNewSample );
			}
			else {
				pLayer = std::make_shared<H2Core::InstrumentLayer>( pNewSample );
				m_pInstrument->getComponent(m_nSelectedComponent)->setLayer( pLayer, selectedLayer );
			}

			if ( fnc ){
				QString newFilename = filename[i].section( '/', -1 );
				newFilename.replace( "." + newFilename.section( '.', -1 ), "");
				m_pInstrument->setName( newFilename );
			}

			//set automatic velocity
			if ( filename[1] ==  "true" ){
				setAutoVelocity();
			}

			pHydrogen->getAudioEngine()->unlock();
		}

		pHydrogen->setIsModified( true );
	}

	updateEditor();
	selectLayer( firstSelection );
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::setAutoVelocity()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}
	auto pCompo = m_pInstrument->getComponent( m_nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}

	int nLayers = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers() ; i++ ) {

		auto pLayer = pCompo->getLayer( i );
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
		auto pLayer = pCompo->getLayer( i );
		if ( pLayer != nullptr ) {
			pLayer->setStartVelocity( nLayer * velocityrange);
			pLayer->setEndVelocity( nLayer * velocityrange + velocityrange );
			
			++nLayer;
		}
	}
}

void InstrumentEditor::renameComponentAction()
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	const auto pComponent = m_pInstrument->getComponent( m_nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve selected component [%1]" )
				  .arg( m_nSelectedComponent ) );
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const QString sOldName = pComponent->getName();
	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", tr( "New component name" ), QLineEdit::Normal,
		sOldName, &bIsOkPressed );

	if ( bIsOkPressed && sOldName != sNewName ) {
		 pHydrogenApp->pushUndoCommand(
			 new SE_renameComponentAction(
				 sNewName, sOldName, m_nSelectedComponent ) );
		 pHydrogenApp->showStatusBarMessage(
			 QString( "%1: [%2] -> [%3]" )
					 .arg( pCommonStrings->getActionRenameComponent() )
					 .arg( sOldName ).arg( sNewName ) );
	}
}

void InstrumentEditor::renameComponent( int nComponentId, const QString& sNewName ) {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	auto pComponent = m_pInstrument->getComponent( nComponentId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nComponentId ) );
		return;
	}

	pComponent->setName( sNewName );
	m_pCompoNameLbl->setText( sNewName );
	populateComponentMenu();

	Hydrogen::get_instance()->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::SelectedInstrumentChanged, -1 );
}

void InstrumentEditor::selectComponent( int nComponent )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	m_nSelectedComponent = nComponent;
	m_pLayerPreview->set_selected_component( m_nSelectedComponent );
}

void InstrumentEditor::selectLayer( int nLayer )
{
	if ( m_pInstrument == nullptr ) {
		return;
	}

	m_nSelectedLayer = nLayer;

	auto pComponent = m_pInstrument->getComponent( m_nSelectedComponent );
	if(pComponent && nLayer >= 0 ){
		auto pLayer = pComponent->getLayer( nLayer );
		m_pWaveDisplay->updateDisplay( pLayer );
		if ( pLayer != nullptr ) {
			// Layer GAIN
			m_pLayerGainRotary->setIsActive( true );
			m_pLayerGainRotary->setValue( pLayer->getGain(), false,
										  Event::Trigger::Suppress );
			m_pLayerGainLCD->setText(
				QString( "%1" ).arg( pLayer->getGain(), -2, 'f', 2, '0' ) );

			//Component GAIN
			m_pCompoGainRotary->setIsActive( true );
			m_pCompoGainRotary->setValue( pComponent->getGain(), false,
										  Event::Trigger::Suppress );
			m_pCompoGainLCD->setText(
				QString( "%1" ).arg( pComponent->getGain(), -2, 'f', 2, '0' ) );

			// Layer PITCH
			const float fCoarsePitch = round( pLayer->getPitch() );
			const float fFinePitch = pLayer->getPitch() - fCoarsePitch;
			m_pLayerPitchCoarseRotary->setIsActive( true );
			m_pLayerPitchCoarseRotary->setValue( fCoarsePitch, false,
												 Event::Trigger::Suppress );
			m_pLayerPitchFineRotary->setIsActive( true );
			m_pLayerPitchFineRotary->setValue( fFinePitch * 100, false,
											   Event::Trigger::Suppress );

			m_pLayerPitchCoarseLCD->setText(
				QString( "%1" ).arg( (int) fCoarsePitch ) );
			m_pLayerPitchFineLCD->setText(
				QString( "%1" ).arg( fFinePitch * 100, 0, 'f', 0 ) );

			m_pRemoveLayerBtn->setIsActive( true );
			m_pSampleEditorBtn->setIsActive( true );
		}
		else {
			// Layer GAIN
			m_pLayerGainRotary->setIsActive( false );
			m_pLayerGainRotary->setValue( 1.0, false,
										  Event::Trigger::Suppress );
			m_pLayerGainLCD->setText( "" );

			//Component GAIN
			m_pCompoGainRotary->setIsActive( false );
			m_pCompoGainRotary->setValue( 1.0, false,
										  Event::Trigger::Suppress );
			m_pCompoGainLCD->setText( "" );

			// Layer PITCH
			m_pLayerPitchCoarseRotary->setIsActive( false );
			m_pLayerPitchCoarseRotary->setValue( 0.0, false,
												 Event::Trigger::Suppress );
			m_pLayerPitchFineRotary->setIsActive( false );
			m_pLayerPitchFineRotary->setValue( 0.0, false,
											   Event::Trigger::Suppress );

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
		m_pLayerGainRotary->setValue( 1.0, false, Event::Trigger::Suppress );
		m_pLayerGainLCD->setText( "" );

		m_pCompoGainRotary->setIsActive( false );
		m_pCompoGainRotary->setValue( 1.0, false, Event::Trigger::Suppress );
		m_pCompoGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchCoarseRotary->setValue( 0.0, false,
											 Event::Trigger::Suppress );
		m_pLayerPitchFineRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setValue( 0.0, false, Event::Trigger::Suppress );

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

	m_pInstrument->setMuteGroup( static_cast<int>(fValue) );
	updateEditor();
}

void InstrumentEditor::onIsStopNoteCheckBoxClicked( bool on )
{
	if ( m_pInstrument == nullptr ) {
		 return;
	 }

	m_pInstrument->setStopNotes( on );
	Hydrogen::get_instance()->setIsModified( true );
}

void InstrumentEditor::onIsApplyVelocityCheckBoxClicked( bool on )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	m_pInstrument->setApplyVelocity( on );
	Hydrogen::get_instance()->setIsModified( true );
}

void InstrumentEditor::midiOutChannelChanged( double fValue ) {
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	if ( fValue != 0.0 ) {
		m_pInstrument->setMidiOutChannel( std::max( static_cast<int>(fValue) - 1,
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

	m_pInstrument->setMidiOutNote( static_cast<int>(fValue) );
}

void InstrumentEditor::onDropDownCompoClicked()
{
	m_pComponentMenu->popup( m_pCompoNameLbl->mapToGlobal( QPoint( m_pCompoNameLbl->width() - 40, m_pCompoNameLbl->height() / 2 ) ) );
}

void InstrumentEditor::populateComponentMenu() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pComponentMenu->clear();

	// Actions to switch between the drumkits
	for ( int ii = 0; ii < m_pInstrument->getComponents()->size(); ++ii ) {
		const auto ppComponent = m_pInstrument->getComponent( ii );
		if ( ppComponent != nullptr ) {
			auto pAction = m_pComponentMenu->addAction(
				ppComponent->getName(), this,
				[=](){ switchComponentAction( ii ); } );;
			if ( ii == m_nSelectedComponent ) {
				m_pComponentMenu->setDefaultAction( pAction );
			}
		}
	}
	m_pComponentMenu->addSeparator();
	m_pComponentMenu->addAction( pCommonStrings->getMenuActionAdd(), this,
								 SLOT( addComponentAction() ) );
	auto pDeleteAction = m_pComponentMenu->addAction(
		pCommonStrings->getMenuActionDelete(), this, SLOT( deleteComponentAction() ) );
	if ( m_pInstrument->getComponents()->size() < 2 ) {
		// If there is just a single component present, it must not be removed.
		pDeleteAction->setEnabled( false );
	}

	m_pComponentMenu->addAction( pCommonStrings->getMenuActionRename(), this,
								 SLOT( renameComponentAction() ) );
}

void InstrumentEditor::addComponentAction() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", tr( "Component name" ), QLineEdit::Normal,
		"New Component", &bIsOkPressed );
	if ( ! bIsOkPressed ) {
		// Dialog closed using cancel
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	auto pNewInstrument = std::make_shared<Instrument>( m_pInstrument );

	const auto pNewComponent = std::make_shared<InstrumentComponent>( sNewName );
	pNewInstrument->addComponent( pNewComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, m_pInstrument,
			SE_replaceInstrumentAction::Type::AddComponent, sNewName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionAddComponent() )
		.arg( sNewName ) );

	// New components will be appended.
	selectComponent( m_pInstrument->getComponents()->size() );
}

void InstrumentEditor::deleteComponentAction() {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	if ( m_pInstrument->getComponents()->size() <= 1 ) {
		ERRORLOG( "There is just a single component remaining. This one can not be deleted." );
		return;
	}

	auto pComponent = m_pInstrument->getComponent( m_nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to find selected component [%1]" )
				  .arg( m_nSelectedComponent ) );
		return;
	}
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const auto sName = pComponent->getName();

	auto pNewInstrument = std::make_shared<Instrument>( m_pInstrument );

	const auto pNewComponent = std::make_shared<InstrumentComponent>( sName );
	pNewInstrument->removeComponent( m_nSelectedComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, m_pInstrument,
			SE_replaceInstrumentAction::Type::DeleteComponent, sName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionDeleteComponent() )
		.arg( sName ) );

	selectComponent(
		std::clamp( m_nSelectedComponent, 0,
					static_cast<int>(m_pInstrument->getComponents()->size()) - 2 ) );
}

void InstrumentEditor::switchComponentAction( int nId ) {
	if ( m_pInstrument == nullptr ) {
		return;
	}

	const auto pComponent = m_pInstrument->getComponent( nId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nId ) );
		return;
	}

	m_pCompoNameLbl->setText( pComponent->getName() );

	selectComponent( nId );
	updateEditor();
	m_pLayerPreview->updateAll();
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
		m_pInstrument->setSampleSelectionAlg( Instrument::VELOCITY );
	}
	else if ( selected == 1 ){
		m_pInstrument->setSampleSelectionAlg( Instrument::ROUND_ROBIN );
	}
	else if ( selected == 2){
		m_pInstrument->setSampleSelectionAlg( Instrument::RANDOM );
	}

	updateEditor();
}

void InstrumentEditor::hihatGroupChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->setHihatGrp( static_cast<int>(fValue) );
}

void InstrumentEditor::hihatMinRangeChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->setLowerCc( static_cast<int>(fValue) );
	 m_pHihatMaxRangeLCD->setMinimum( static_cast<int>(fValue) );
}

void InstrumentEditor::hihatMaxRangeChanged( double fValue )
{
	 if ( m_pInstrument == nullptr ) {
		 return;
	 }

	 m_pInstrument->setHigherCc( static_cast<int>(fValue) );
	 m_pHihatMinRangeLCD->setMaximum( static_cast<int>(fValue) );
}

void InstrumentEditor::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		setStyleSheet( QString( "QLabel { background: %1 }" )
					   .arg( pPref->getTheme().m_color.m_windowColor.name() ) );
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
