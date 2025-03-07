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

InstrumentEditor::InstrumentEditor( QWidget* pParent,
									InstrumentEditorPanel* pPanel )
	: QWidget( pParent )
	, m_pInstrumentEditorPanel( pPanel )
	, m_fPreviousMidiOutChannel( -1.0 )
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
	connect( m_pMidiOutNoteLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setMidiOutNote(
			static_cast<int>(m_pMidiOutNoteLCD->value()) );
	});
	m_pMidiOutNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											pCommonStrings->getMidiOutNoteLabel() );
	m_pMidiOutNoteLbl->move( 159, 281 );

	/////////////

	connect( m_pNameLbl, &ClickableLabel::labelClicked, this, [=](){
		auto pSong = Hydrogen::get_instance()->getSong();
		auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
		if ( pInstrument != nullptr && pSong != nullptr &&
			 pSong->getDrumkit() != nullptr ) {
			MainForm::action_drumkit_renameInstrument(
				pSong->getDrumkit()->getInstruments()->index( pInstrument ) );}
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
	connect( m_pPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		m_pInstrumentEditorPanel->getInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchCoarseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 82, 235 );

	m_pPitchFineRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center,
									 tr( "Pitch offset (Fine)" ), false,
									 -fFinePitch, fFinePitch );
	//it will have resolution of 100 steps between Min and Max => quantum delta = 0.01
	m_pPitchFineRotary->move( 138, 210 );
	connect( m_pPitchFineRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		m_pInstrumentEditorPanel->getInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchFineLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										  pCommonStrings->getPitchFineLabel() );
	m_pPitchFineLbl->move( 136, 235 );

	

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									   tr( "Random pitch factor" ), false );
	m_pRandomPitchRotary->move( 194, 210 );
	connect( m_pRandomPitchRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setRandomPitchFactor(
			m_pRandomPitchRotary->getValue() );
	});
	m_pPitchRandomLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchRandomLabel() );
	m_pPitchRandomLbl->move( 192, 235 );

	// Filter
	m_pFilterBypassBtn = new Button( m_pInstrumentProp, QSize( 36, 15 ), Button::Type::Toggle,
									 "", pCommonStrings->getBypassButton(), true,
									 QSize( 0, 0 ), "", false, true );
	connect( m_pFilterBypassBtn, &Button::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterActive(
			! m_pFilterBypassBtn->isChecked() );
	});
	m_pFilterBypassBtn->move( 67, 169 );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Filter Cutoff" ), false );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	connect( m_pCutoffRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterCutoff(
			m_pCutoffRotary->getValue() );
	});
	m_pCutoffLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getCutoffLabel() );
	m_pCutoffLbl->move( 107, 189 );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									 tr( "Filter resonance" ), false );
	connect( m_pResonanceRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterResonance(
			std::min( 0.95f, m_pResonanceRotary->getValue() ) );
	});
	m_pResonanceLbl = new ClickableLabel( m_pInstrumentProp, QSize( 56, 10 ),
										  pCommonStrings->getResonanceLabel() );
	m_pResonanceLbl->move( 157, 189 );

	m_pCutoffRotary->move( 109, 164 );
	m_pResonanceRotary->move( 163, 164 );
	// ~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Length of Attack phase.\n\nValue" ), false );
	connect( m_pAttackRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setAttack(
			100000 * m_pAttackRotary->getValue() * m_pAttackRotary->getValue() );
	});
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								 tr( "Length of Decay phase.\n\nValue" ), false );
	connect( m_pDecayRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setDecay(
			100000 * m_pDecayRotary->getValue() * m_pDecayRotary->getValue() );
	});
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Sample volume in Sustain phase.\n\nValue" ), false );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	connect( m_pSustainRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setSustain(
			m_pSustainRotary->getValue() );
	});
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Length of Release phase.\n\nValue" ), false );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	connect( m_pReleaseRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setRelease(
			256.0 +
			100000 * m_pReleaseRotary->getValue() * m_pReleaseRotary->getValue() );
	});
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
	connect( m_pInstrumentGain, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setGain(
			m_pInstrumentGain->getValue() );
		updateEditor(); // LCD update
	});
	m_pInstrumentGainLCD->move( 62, 103 );
	m_pInstrumentGain->move( 109, 100 );
	m_pGainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									 pCommonStrings->getGainLabel() );
	m_pGainLbl->move( 107, 125 );


	m_pMuteGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									  LCDSpinBox::Type::Int, -1, 100,
									  true, true );
	m_pMuteGroupLCD->move( 160, 101 );
	connect( m_pMuteGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setMuteGroup(
			static_cast<int>(m_pMuteGroupLCD->value()) );
	});
	m_pMuteGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
										  pCommonStrings->getMuteGroupLabel() );
	m_pMuteGroupLbl->move( 159, 125 );

	m_pIsStopNoteCheckBox = new QCheckBox( "", m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 42, 139 );
	m_pIsStopNoteCheckBox->adjustSize();
	m_pIsStopNoteCheckBox->setFixedSize( 14, 14 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, &QCheckBox::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setStopNotes(
			static_cast<int>(m_pIsStopNoteCheckBox->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
										   pCommonStrings->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 59, 144 );

	m_pApplyVelocity = new QCheckBox( "", m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 139 );
	m_pApplyVelocity->adjustSize();
	m_pApplyVelocity->setFixedSize( 14, 14 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, &QCheckBox::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setApplyVelocity(
			static_cast<int>(m_pApplyVelocity->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pApplyVelocityLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
											  pCommonStrings->getApplyVelocityLabel() );
	m_pApplyVelocityLbl->move( 170, 144 );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									   LCDSpinBox::Type::Int, -1, 32,
									   true, true );
	m_pHihatGroupLCD->move( 28, 303 );
	connect( m_pHihatGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setHihatGrp(
			static_cast<int>(m_pHihatGroupLCD->value()) );
	});
	m_pHihatGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 69, 10 ),
										   pCommonStrings->getHihatGroupLabel() );
	m_pHihatGroupLbl->move( 22, 327 );

	m_pHihatMinRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMinRangeLCD->move( 138, 303 );
	connect( m_pHihatMinRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setLowerCc(
			static_cast<int>(m_pHihatMinRangeLCD->value()) );
		m_pHihatMaxRangeLCD->setMinimum(
			static_cast<int>(m_pHihatMinRangeLCD->value()) );
	});
	m_pHihatMinRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											  pCommonStrings->getHihatMinRangeLabel() );
	m_pHihatMinRangeLbl->move( 136, 327 );

	m_pHihatMaxRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMaxRangeLCD->move( 203, 303 );
	connect( m_pHihatMaxRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setHigherCc(
			static_cast<int>(m_pHihatMaxRangeLCD->value()) );
		m_pHihatMinRangeLCD->setMaximum(
			static_cast<int>(m_pHihatMaxRangeLCD->value()) );
	});
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
	m_pLayerPreview = new LayerPreview( nullptr, pPanel );

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
	connect( m_pLayerGainRotary, &Rotary::valueChanged, [&]() {
		auto pComponent = m_pInstrumentEditorPanel->getInstrument()->getComponent(
			m_pInstrumentEditorPanel->getSelectedComponent() );
		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer(
				m_pInstrumentEditorPanel->getSelectedLayer() );
			if ( pLayer != nullptr ) {
				pLayer->setGain( m_pLayerGainRotary->getValue() );
				updateEditor(); // LCD update
			}
		}
	});
	m_pLayerGainLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
										  pCommonStrings->getLayerGainLabel() );
	m_pLayerGainLbl->move( 50, 360 );

	m_pCompoGainLCD = new LCDDisplay( m_pLayerProp, QSize( 36, 16 ), false, false );
	m_pCompoGainRotary = new Rotary( m_pLayerProp, Rotary::Type::Normal,
									 tr( "Component volume" ), false, 0.0, 5.0 );
	m_pCompoGainRotary->setDefaultValue ( 1.0 );
	connect( m_pCompoGainRotary, &Rotary::valueChanged, [&]() {
		auto pComponent = m_pInstrumentEditorPanel->getInstrument()->getComponent(
			m_pInstrumentEditorPanel->getSelectedComponent() );
		if ( pComponent != nullptr ) {
			pComponent->setGain( m_pCompoGainRotary->getValue() );
			updateEditor(); // LCD update
		}
	});
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
	connect( m_pLayerPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		auto pComponent = m_pInstrumentEditorPanel->getInstrument()->getComponent(
			m_pInstrumentEditorPanel->getSelectedComponent() );
		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer(
				m_pInstrumentEditorPanel->getSelectedLayer() );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updateEditor(); // LCD update
			}
		}
	});
	m_pLayerPitchCoarseLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 10 ),
												 pCommonStrings->getPitchCoarseLabel() );
	m_pLayerPitchCoarseLbl->move( 61, 412 );

	m_pLayerPitchFineRotary = new Rotary( m_pLayerProp, Rotary::Type::Center,
										  tr( "Layer pitch (Fine)" ), true, -50.0, 50.0 );
	connect( m_pLayerPitchFineRotary, &Rotary::valueChanged, [&]() {
		const float fNewPitch = round( m_pLayerPitchCoarseRotary->getValue() ) +
			m_pLayerPitchFineRotary->getValue() / 100.0;
		auto pComponent = m_pInstrumentEditorPanel->getInstrument()->getComponent(
			m_pInstrumentEditorPanel->getSelectedComponent() );
		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer(
				m_pInstrumentEditorPanel->getSelectedLayer() );
			if ( pLayer != nullptr ) {
				pLayer->setPitch( fNewPitch );
				updateEditor(); // LCD update
			}
		}
	});
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

	m_pSampleSelectionCombo = new LCDCombo(
		m_pLayerProp, QSize( width() - 76 - 7, 18 ), true );
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
	updateEditor();
}

InstrumentEditor::~InstrumentEditor() {
}

void InstrumentEditor::updateEditor()
{
	auto pHydrogen = Hydrogen::get_instance();
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	const int nSelectedLayer = m_pInstrumentEditorPanel->getSelectedLayer();

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	bool bLayerActive = false;
	updateActivation();

	if ( pInstrument != nullptr ) {

		m_pNameLbl->setText( pInstrument->getName() );

		// ADSR
		m_pAttackRotary->setValue(
			sqrtf(pInstrument->getAdsr()->getAttack() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pDecayRotary->setValue(
			sqrtf(pInstrument->getAdsr()->getDecay() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pSustainRotary->setValue( pInstrument->getAdsr()->getSustain(),
									false, Event::Trigger::Suppress );
		const float fRelease =
			std::max( pInstrument->getAdsr()->getRelease() - 256.0, 0.0 );
		m_pReleaseRotary->setValue( sqrtf( fRelease / 100000.0 ), false,
									Event::Trigger::Suppress );
		// ~ ADSR

		// filter
		m_pFilterBypassBtn->setChecked( ! pInstrument->isFilterActive() );
		m_pCutoffRotary->setValue( pInstrument->getFilterCutoff(), false,
								   Event::Trigger::Suppress );
		m_pResonanceRotary->setValue( pInstrument->getFilterResonance(),
									  false, Event::Trigger::Suppress );
		// ~ filter

		// pitch
		const QString sNewPitch = QString( "%1" )
			.arg( pInstrument->getPitchOffset(), -2, 'f', 2, '0' );

		if ( m_pPitchLCD->text() != sNewPitch ) {
			m_pPitchLCD->setText( sNewPitch );
		}

		// pitch offset

		/* fCoarsePitch is the closest integer to pitch_offset (represents the
		   pitch shift interval in half steps) while it is an integer number,
		   it's defined float to be used in next lines */
		float fCoarsePitch;

		// Since the 'fine' rotary covers values from -0.5 to 0.5 e.g. 1.5 can
		// be represented by both coarse: 2, fine: -0.5 and coarse: 1, fine:
		// 0.5. We need some extra logic to avoid unexpected jumping of the
		// rotaries.
		if ( m_pPitchFineRotary->getValue() == 0.5 &&
			 pInstrument->getPitchOffset() -
			 trunc( pInstrument->getPitchOffset() ) == 0.5 ) {
			fCoarsePitch = trunc( pInstrument->getPitchOffset() );
		}
		else if ( m_pPitchFineRotary->getValue() == -0.5 &&
				  trunc( pInstrument->getPitchOffset() ) -
				  pInstrument->getPitchOffset() == 0.5 ) {
			fCoarsePitch = ceil( pInstrument->getPitchOffset() );
		}
		else {
			fCoarsePitch = round( pInstrument->getPitchOffset() );
		}

		// fFinePitch represents the fine adjustment (between -0.5 and +0.5) if
		// pitch_offset has decimal part
		const float fFinePitch = pInstrument->getPitchOffset() - fCoarsePitch;

		if ( m_pPitchCoarseRotary->getValue() != fCoarsePitch ) {
			m_pPitchCoarseRotary->setValue( fCoarsePitch, false,
											Event::Trigger::Suppress );
		}
		if ( m_pPitchFineRotary->getValue() != fFinePitch ) {
			m_pPitchFineRotary->setValue( fFinePitch, false,
										  Event::Trigger::Suppress );
		}

		// pitch random
		m_pRandomPitchRotary->setValue( pInstrument->getRandomPitchFactor(),
										false, Event::Trigger::Suppress );
		// ~ pitch

		//Stop Note
		m_pIsStopNoteCheckBox->setChecked( pInstrument->isStopNotes() );

		//Ignore Velocity
		m_pApplyVelocity->setChecked( pInstrument->getApplyVelocity() );

		// instr gain
		m_pInstrumentGainLCD->setText(
			QString( "%1" ).arg( pInstrument->getGain(), -2, 'f', 2, '0' ) );
		m_pInstrumentGain->setValue( pInstrument->getGain(), false,
									 Event::Trigger::Suppress );

		// instr mute group
		m_pMuteGroupLCD->setValue(
			pInstrument->getMuteGroup(), Event::Trigger::Suppress );

		// midi out channel
		if ( pInstrument->getMidiOutChannel() == -1 ) {
			// turn off
			m_pMidiOutChannelLCD->setValue( -1, Event::Trigger::Suppress );
		}
		else {
			// The MIDI channels start at 1 instead of zero.
			m_pMidiOutChannelLCD->setValue(
				pInstrument->getMidiOutChannel() + 1,
				Event::Trigger::Suppress );
		}

		//midi out note
		m_pMidiOutNoteLCD->setValue( pInstrument->getMidiOutNote(),
									 Event::Trigger::Suppress );

		// hihat
		m_pHihatGroupLCD->setValue( pInstrument->getHihatGrp(),
									Event::Trigger::Suppress );
		m_pHihatMinRangeLCD->setValue( pInstrument->getLowerCc(),
									   Event::Trigger::Suppress );
		m_pHihatMaxRangeLCD->setValue( pInstrument->getHigherCc(),
									   Event::Trigger::Suppress );
		m_pHihatMinRangeLCD->setMaximum( pInstrument->getHigherCc() );
		m_pHihatMaxRangeLCD->setMinimum( pInstrument->getLowerCc() );

		// see instrument.h
		m_pSampleSelectionCombo->setCurrentIndex(
			pInstrument->sampleSelectionAlg() );

		const auto pComponent =
			pInstrument->getComponent( nSelectedComponent );
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

			if ( nSelectedLayer >= 0 ) {
				const auto pLayer = pComponent->getLayer( nSelectedLayer );

				if ( pLayer != nullptr ) {
					bLayerActive = true;

					// Layer GAIN
					m_pLayerGainRotary->setIsActive( true );
					m_pLayerGainRotary->setValue( pLayer->getGain(), false,
												  Event::Trigger::Suppress );
					m_pLayerGainLCD->setText(
						QString( "%1" ).arg( pLayer->getGain(), -2, 'f', 2, '0' ) );

					// Component GAIN
					m_pCompoGainRotary->setIsActive( true );
					m_pCompoGainRotary->setValue( pComponent->getGain(), false,
												  Event::Trigger::Suppress );
					m_pCompoGainLCD->setText(
						QString( "%1" ).arg( pComponent->getGain(), -2, 'f', 2, '0' ) );

					// Layer PITCH
					//
					// For most X.5 values we prefer to round the digit before
					// point up and set the fine value to -0.5. But this is not
					// possible for the maximum value and we have to ensure not
					// to introduce sudden jumps in the fine pitch rotary.
					float fCoarseLayerPitch;
					if ( ( m_pLayerPitchFineRotary->getValue() == 50 &&
						   pLayer->getPitch() -
						   trunc( pLayer->getPitch() ) == 0.5  ) ||
						 pLayer->getPitch() == Instrument::fPitchMax ) {
						fCoarseLayerPitch = trunc( pLayer->getPitch() );
					}
					else if ( m_pLayerPitchFineRotary->getValue() == -50 &&
							  trunc( pLayer->getPitch() ) -
							  pLayer->getPitch() == 0.5 ) {
						fCoarseLayerPitch = ceil( pLayer->getPitch() );
					}
					else {
						fCoarseLayerPitch = round( pLayer->getPitch() );
					}

					const float fFineLayerPitch =
						pLayer->getPitch() - fCoarseLayerPitch;
					m_pLayerPitchCoarseRotary->setIsActive( true );
					m_pLayerPitchCoarseRotary->setValue( fCoarseLayerPitch, false,
												 Event::Trigger::Suppress );
					m_pLayerPitchFineRotary->setIsActive( true );
					m_pLayerPitchFineRotary->setValue( fFineLayerPitch * 100, false,
													   Event::Trigger::Suppress );

					m_pLayerPitchCoarseLCD->setText(
						QString( "%1" ).arg( (int) fCoarseLayerPitch ) );
					m_pLayerPitchFineLCD->setText(
						QString( "%1" ).arg( fFineLayerPitch * 100, 0, 'f', 0 ) );

					m_pRemoveLayerBtn->setIsActive( true );
					m_pSampleEditorBtn->setIsActive( true );

					m_pWaveDisplay->updateDisplay( pLayer );
				}
			}
		}
	}
	else {
		m_pNameLbl->setText( "" );
	}

	if ( ! bLayerActive ) {
		// Layer GAIN
		m_pLayerGainRotary->setIsActive( false );
		m_pLayerGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchCoarseRotary->setIsActive( false );
		m_pLayerPitchFineRotary->setIsActive( false );

		m_pLayerPitchCoarseLCD->setText( "" );
		m_pLayerPitchFineLCD->setText( "" );

		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );

		m_pWaveDisplay->updateDisplay( nullptr );
	}

	populateComponentMenu();
	m_pLayerPreview->update();
}

void InstrumentEditor::updateActivation() {
	if ( m_pInstrumentEditorPanel->getInstrument() != nullptr ) {
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

		m_pLoadLayerBtn->setIsActive( false );
		m_pRemoveLayerBtn->setIsActive( false );
		m_pSampleEditorBtn->setIsActive( false );
		m_pIsStopNoteCheckBox->setChecked( false );
		m_pIsStopNoteCheckBox->setEnabled( false );

		m_DropDownCompoBtn->setIsActive( false );
		m_pCompoGainRotary->setIsActive( false );
		m_pCompoGainLCD->clear();

		m_pWaveDisplay->updateDisplay( nullptr );
	}
}

void InstrumentEditor::waveDisplayDoubleClicked( QWidget* pRef )
{		
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	const int nSelectedLayer = m_pInstrumentEditorPanel->getSelectedLayer();
	if ( pInstrument == nullptr ) {
		return;
	}
	
	auto pCompo = pInstrument->getComponent( nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}
			
	auto pLayer = pCompo->getLayer( nSelectedLayer );
	if ( pLayer != nullptr ) {
		showSampleEditor();
	}
	else {
		loadLayerBtnClicked();
	}
}

void InstrumentEditor::showSampleEditor()
{
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	const int nSelectedLayer = m_pInstrumentEditorPanel->getSelectedLayer();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pCompo = pInstrument->getComponent( nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}

	auto pLayer = pCompo->getLayer( nSelectedLayer );
	if ( pLayer == nullptr ) {
		return;
	}

	auto pSample = pLayer->getSample();
	if ( pSample != nullptr ) {
		HydrogenApp::get_instance()->showSampleEditor(
			pSample->getFilepath(), nSelectedComponent, nSelectedLayer );
	}
}

void InstrumentEditor::removeLayerButtonClicked()
{
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	const int nSelectedLayer = m_pInstrumentEditorPanel->getSelectedLayer();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();

	auto pCompo = pInstrument->getComponent( nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	pCompo->setLayer( nullptr, nSelectedLayer );

	pHydrogen->getAudioEngine()->unlock();

	pHydrogen->setIsModified( true );

	// Select next loaded layer - if available - in order to allow for a quick
	// removal of all layers. In case the last layer was removed, the previous
	// one will be selected.
	int nNextLayerIndex = 0;
	int nCount = 0;
	for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
		auto pLayer = pCompo->getLayer( n );
		if ( pLayer != nullptr ){
			nCount++;

			if ( nNextLayerIndex <= nSelectedLayer &&
				 n != nSelectedLayer ) {
				nNextLayerIndex = n;
			}
		}
	}

	m_pInstrumentEditorPanel->setSelectedLayer( nCount );
	m_pInstrumentEditorPanel->updateEditors();
}

void InstrumentEditor::loadLayerBtnClicked()
{
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	const int nSelectedLayer = m_pInstrumentEditorPanel->getSelectedLayer();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();

	QString sPath = Preferences::get_instance()->getLastOpenLayerDirectory();
	QString sFilename = "";
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	// In case the button was pressed while a layer was selected, we
	// try to use path of the associated sample as default one.
	if ( nSelectedLayer > 0 ) {
		auto pComponent = pInstrument->getComponent( nSelectedComponent );

		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer( nSelectedLayer );

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
			Preferences::get_instance()->setLastOpenLayerDirectory(
				pFileBrowser->getSelectedDirectory() );
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

	int nLastInsertedLayer = nSelectedLayer;
	if ( filename.size() > 2 ) {
		for ( int ii = 2; ii < filename.size(); ++ii ) {
			int nnLayer = nSelectedLayer + ii - 2;
			if ( ( ii - 2 >= InstrumentComponent::getMaxLayers() ) ||
				 ( nnLayer + 1  > InstrumentComponent::getMaxLayers() ) ) {
				break;
			}

			auto pNewSample = Sample::load( filename[ii] );

			pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

			// If we're using multiple layers, we start inserting the first
			// layer at nSelectedLayer and the next layer at nSelectedLayer + 1.

			auto pCompo = pInstrument->getComponent( nSelectedComponent );
			if ( pCompo == nullptr ) {
				ERRORLOG( QString( "Selected component [%1] is invalid for the current instrument." )
						  .arg( nSelectedComponent ) );
				return;
			}

			auto pLayer = pCompo->getLayer( nnLayer );
			if ( pLayer != nullptr ) {
				// insert new sample from newInstrument, old sample gets deleted
				// by setSample
				pLayer->setSample( pNewSample );
			}
			else {
				pLayer = std::make_shared<H2Core::InstrumentLayer>( pNewSample );
				pCompo->setLayer( pLayer, nnLayer );
			}
			nLastInsertedLayer = nnLayer;

			if ( fnc ){
				QString newFilename = filename[ii].section( '/', -1 );
				newFilename.replace( "." + newFilename.section( '.', -1 ), "");
				pInstrument->setName( newFilename );
			}

			//set automatic velocity
			if ( filename[1] ==  "true" ){
				setAutoVelocity();
			}

			pHydrogen->getAudioEngine()->unlock();
		}

		pHydrogen->setIsModified( true );
	}

	m_pInstrumentEditorPanel->setSelectedLayer( nLastInsertedLayer );
	m_pInstrumentEditorPanel->updateEditors();
}


void InstrumentEditor::setAutoVelocity()
{
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	if ( pInstrument == nullptr ) {
		return;
	}
	auto pCompo = pInstrument->getComponent( nSelectedComponent );
	if ( pCompo == nullptr ) {
		return;
	}

	const int nLayers = pCompo->getLayers().size();
	if ( nLayers == 0 ) {
		ERRORLOG( QString( "There are no layers in component [%1]" )
				  .arg( nSelectedComponent ) );
		return;
	}

	const float fVelocityRange = 1.0 / nLayers;

	int nLayer = 0;
	for ( auto& ppLayer : pCompo->getLayers() ) {
		if ( ppLayer != nullptr ) {
			ppLayer->setStartVelocity( nLayer * fVelocityRange );
			ppLayer->setEndVelocity( nLayer * fVelocityRange + fVelocityRange );
			
			++nLayer;
		}
	}
}

void InstrumentEditor::renameComponentAction() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	if ( pInstrument == nullptr ) {
		return;
	}

	const auto pComponent = pInstrument->getComponent( nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve selected component [%1]" )
				  .arg( nSelectedComponent ) );
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
				 sNewName, sOldName, nSelectedComponent ) );
		 pHydrogenApp->showStatusBarMessage(
			 QString( "%1: [%2] -> [%3]" )
					 .arg( pCommonStrings->getActionRenameComponent() )
					 .arg( sOldName ).arg( sNewName ) );
	}
}

void InstrumentEditor::renameComponent( int nComponentId, const QString& sNewName ) {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	auto pComponent = pInstrument->getComponent( nComponentId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nComponentId ) );
		return;
	}

	pComponent->setName( sNewName );

	Hydrogen::get_instance()->setIsModified( true );

	updateEditor();
}

void InstrumentEditor::midiOutChannelChanged( double fValue ) {
	auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	 if ( pInstrument == nullptr ) {
		 return;
	 }

	if ( fValue != 0.0 ) {
		pInstrument->setMidiOutChannel(
			std::max( static_cast<int>(fValue) - 1, -1 ) );
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

void InstrumentEditor::onDropDownCompoClicked() {
	m_pComponentMenu->popup( m_pCompoNameLbl->mapToGlobal(
								 QPoint( m_pCompoNameLbl->width() - 40,
										 m_pCompoNameLbl->height() / 2 ) ) );
}

void InstrumentEditor::populateComponentMenu() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	if ( pInstrument == nullptr ) {
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pComponentMenu->clear();

	// Actions to switch between the drumkits
	for ( int ii = 0; ii < pInstrument->getComponents()->size(); ++ii ) {
		const auto ppComponent = pInstrument->getComponent( ii );
		if ( ppComponent != nullptr ) {
			auto pAction = m_pComponentMenu->addAction(
				ppComponent->getName(), this,
				[=](){ switchComponentAction( ii ); } );;
			if ( ii == nSelectedComponent ) {
				m_pComponentMenu->setDefaultAction( pAction );
			}
		}
	}
	m_pComponentMenu->addSeparator();
	m_pComponentMenu->addAction( pCommonStrings->getMenuActionAdd(), this,
								 SLOT( addComponentAction() ) );
	auto pDeleteAction = m_pComponentMenu->addAction(
		pCommonStrings->getMenuActionDelete(), this, SLOT( deleteComponentAction() ) );
	if ( pInstrument->getComponents()->size() < 2 ) {
		// If there is just a single component present, it must not be removed.
		pDeleteAction->setEnabled( false );
	}

	m_pComponentMenu->addAction( pCommonStrings->getMenuActionRename(), this,
								 SLOT( renameComponentAction() ) );
}

void InstrumentEditor::addComponentAction() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
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

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );

	const auto pNewComponent = std::make_shared<InstrumentComponent>( sNewName );
	pNewInstrument->addComponent( pNewComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::AddComponent, sNewName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionAddComponent() )
		.arg( sNewName ) );

	// New components will be appended and should be selected.
	m_pInstrumentEditorPanel->setSelectedComponent(
		pInstrument->getComponents()->size() );
	m_pInstrumentEditorPanel->updateEditors();
}

void InstrumentEditor::deleteComponentAction() {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	const int nSelectedComponent =
		m_pInstrumentEditorPanel->getSelectedComponent();
	if ( pInstrument == nullptr ) {
		return;
	}

	if ( pInstrument->getComponents()->size() <= 1 ) {
		ERRORLOG( "There is just a single component remaining. This one can not be deleted." );
		return;
	}

	auto pComponent = pInstrument->getComponent( nSelectedComponent );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to find selected component [%1]" )
				  .arg( nSelectedComponent ) );
		return;
	}
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	const auto sName = pComponent->getName();

	auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
	pNewInstrument->removeComponent( nSelectedComponent );

	pHydrogenApp->pushUndoCommand(
		new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::DeleteComponent, sName ) );
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionDeleteComponent() )
		.arg( sName ) );

	m_pInstrumentEditorPanel->setSelectedComponent(
		std::clamp( nSelectedComponent, 0,
					static_cast<int>(pInstrument->getComponents()->size()) - 2 ) );
	m_pInstrumentEditorPanel->updateEditors();
}

void InstrumentEditor::switchComponentAction( int nId ) {
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	if ( pInstrument == nullptr ) {
		return;
	}

	const auto pComponent = pInstrument->getComponent( nId );
	if ( pComponent == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve component [%1]" )
				  .arg( nId ) );
		return;
	}

	m_pInstrumentEditorPanel->setSelectedComponent( nId );
	m_pInstrumentEditorPanel->updateEditors();
}

void InstrumentEditor::sampleSelectionChanged( int selected ) {
	auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	 if ( pInstrument == nullptr ) {
		 return;
	 }

	 if ( m_pSampleSelectionCombo->count() < 3 ) {
		 // We flushed the widget while disabling the InstrumentEditor and are
		 // in the process of refilling it.
		 return;
	 }

	if ( selected == 0 ){
		pInstrument->setSampleSelectionAlg( Instrument::VELOCITY );
	}
	else if ( selected == 1 ){
		pInstrument->setSampleSelectionAlg( Instrument::ROUND_ROBIN );
	}
	else if ( selected == 2){
		pInstrument->setSampleSelectionAlg( Instrument::RANDOM );
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
	m_pSampleSelectionCombo->clear();
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "First in Velocity" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Round Robin" ) );
	/*: Sample selection algorithm available in the instrument editor */
	m_pSampleSelectionCombo->addItem( tr( "Random" ) );
}
