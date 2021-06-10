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
#include <QtWidgets>


#include <math.h>
#include <assert.h>
#include <vector>

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Globals.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Sample.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/AudioEngine.h>
#include <core/EventQueue.h>
using namespace H2Core;

#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../CommonStrings.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/WidgetWithInput.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCD.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/Fader.h"
#include "InstrumentEditor.h"
#include "WaveDisplay.h"
#include "LayerPreview.h"
#include "AudioFileBrowser/AudioFileBrowser.h"

const char* InstrumentEditor::__class_name = "InstrumentEditor";

InstrumentEditor::InstrumentEditor( QWidget* pParent )
	: QWidget( pParent )
	, Object( __class_name )
	, m_pInstrument( nullptr )
	, m_nSelectedLayer( 0 )
	, m_nPreviousMidiOutChannel( -1 )
{
	setFixedWidth( 290 );
	
	// Instrument properties top
	m_pInstrumentPropTop = new PixmapWidget( this );
	m_pInstrumentPropTop->setPixmap( "/instrumentEditor/instrumentTab_top.png" );

	m_pShowInstrumentBtn = new ToggleButton( m_pInstrumentPropTop, QSize( 100, 17 ), "", HydrogenApp::get_instance()->getCommonStrings()->getGeneralButton() );
	m_pShowInstrumentBtn->setToolTip( tr( "Show instrument properties" ) );
	connect( m_pShowInstrumentBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowInstrumentBtn->move( 40, 7 );
	m_pShowInstrumentBtn->setPressed( true );

	m_pShowLayersBtn = new ToggleButton( m_pInstrumentPropTop, QSize( 100, 17 ), "", HydrogenApp::get_instance()->getCommonStrings()->getLayersButton() );
	m_pShowLayersBtn->setToolTip( tr( "Show layers properties" ) );
	connect( m_pShowLayersBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowLayersBtn->move( 144, 7 );


	// Instrument properties
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move(0, 31);
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp, QSize( 275, 28 ), "" );
	m_pNameLbl->move( 8, 5 );

	/////////////
	//Midi Out

	m_pMidiOutChannelLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, -1, 16 );
	m_pMidiOutChannelLCD->move( 72, 259 );
	m_pMidiOutChannelLCD->setToolTip(QString(tr("Midi out channel")));
	connect( m_pMidiOutChannelLCD, SIGNAL( valueChanged( double ) ), this, SLOT( midiOutChannelChanged( double ) ) );
	m_pMidiOutChannelLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getMidiOutChannelLabel() );
	m_pMidiOutChannelLbl->move( 66, 282 );

	///
	m_pMidiOutNoteLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, 0, 100 );
	m_pMidiOutNoteLCD->move( 165, 259 );
	m_pMidiOutNoteLCD->setToolTip(QString(tr("Midi out note")));
	connect( m_pMidiOutNoteLCD, SIGNAL( valueChanged( double ) ), this, SLOT( midiOutNoteChanged( double ) ) );
	m_pMidiOutNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getMidiOutNoteLabel() );
	m_pMidiOutNoteLbl->move( 159, 282 );

	/////////////

	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelClicked(ClickableLabel*) ) );
	
	m_pPitchLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 6 );
	m_pPitchLCD->move(25, 215 );
	m_pPitchLbl = new ClickableLabel( m_pInstrumentProp, QSize( 54, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchLabel() );
	m_pPitchLbl->move( 25, 236 );
	

	m_pPitchCoarseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Coarse)" ), true, -24, 24 );
	m_pPitchCoarseRotary->move( 84, 210 );

	connect( m_pPitchCoarseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchCoarseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 82, 236 );

	m_pPitchFineRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Fine)" ), false, -0.5, 0.5 );
	//it will have resolution of 100 steps between Min and Max => quantum delta = 0.01
	m_pPitchFineRotary->move( 138, 210 );
	connect( m_pPitchFineRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchFineLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchFineLabel() );
	m_pPitchFineLbl->move( 136, 236 );

	

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Random pitch factor" ), false );
	m_pRandomPitchRotary->move( 194, 210 );
	connect( m_pRandomPitchRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pPitchRandomLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchRandomLabel() );
	m_pPitchRandomLbl->move( 192, 236 );

	// Filter
	m_pFilterBypassBtn = new ToggleButton( m_pInstrumentProp, QSize( 30, 13 ), "", HydrogenApp::get_instance()->getCommonStrings()->getBypassButton(), true );
	connect( m_pFilterBypassBtn, SIGNAL( clicked(Button*) ), this, SLOT( filterActiveBtnClicked(Button*) ) );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Filter Cutoff" ), false );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	connect( m_pCutoffRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pCutoffLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getCutoffLabel() );
	m_pCutoffLbl->move( 107, 190 );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Filter resonance" ), false );
	connect( m_pResonanceRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pResonanceLbl = new ClickableLabel( m_pInstrumentProp, QSize( 56, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getResonanceLabel() );
	m_pResonanceLbl->move( 157, 190 );

	m_pFilterBypassBtn->move( 70, 170 );
	m_pCutoffRotary->move( 109, 164 );
	m_pResonanceRotary->move( 163, 164 );
	//~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Attack" ), false );
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Decay" ), false );
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Sustain" ), false );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Release" ), false );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	connect( m_pAttackRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pDecayRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pSustainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	connect( m_pReleaseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pAttackRotary->move( 45, 52 );
	m_pDecayRotary->move( 97, 52 );
	m_pSustainRotary->move( 149, 52 );
	m_pReleaseRotary->move( 201, 52 );

	m_pAttackLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getAttackLabel() );
	m_pAttackLbl->move( 43, 79 );
	m_pDecayLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getDecayLabel() );
	m_pDecayLbl->move( 95, 79 );
	m_pSustainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getSustainLabel() );
	m_pSustainLbl->move( 147, 79 );
	m_pReleaseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getReleaseLabel() );
	m_pReleaseLbl->move( 199, 79 );
	//~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::Type::Normal, tr( "Instrument gain" ), false, 0.0, 5.0 );
	m_pInstrumentGain->setDefaultValue( 1.0 );
	connect( m_pInstrumentGain, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pInstrumentGainLCD->move( 67, 105 );
	m_pInstrumentGain->move( 109, 100 );
	m_pGainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getGainLabel() );
	m_pGainLbl->move( 107, 126 );


	m_pMuteGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, -1, 100 );
	m_pMuteGroupLCD->move( 164, 103 );
	connect( m_pMuteGroupLCD, SIGNAL( valueChanged( double ) ), this, SLOT( muteGroupChanged( double ) ) );
	m_pMuteGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getMuteGroupLabel() );
	m_pMuteGroupLbl->move( 159, 126 );

	m_pIsStopNoteCheckBox = new QCheckBox ( tr( "" ), m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 53, 138 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, SIGNAL( toggled( bool ) ), this, SLOT( onIsStopNoteCheckBoxClicked( bool ) ) );
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 79, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 68, 140 );

	m_pApplyVelocity = new QCheckBox ( tr( "" ), m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 138 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, SIGNAL( toggled( bool ) ), this, SLOT( onIsApplyVelocityCheckBoxClicked( bool ) ) );
	m_pApplyVelocityLbl = new ClickableLabel( m_pInstrumentProp, QSize( 75, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getApplyVelocityLabel() );
	m_pApplyVelocityLbl->move( 168, 140 );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, -1, 32 );
	m_pHihatGroupLCD->move( 32, 305 );
	connect( m_pHihatGroupLCD, SIGNAL( valueChanged( double ) ), this, SLOT( hihatGroupChanged( double ) ) );
	m_pHihatGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 69, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getHihatGroupLabel() );
	m_pHihatGroupLbl->move( 22, 328 );

	m_pHihatMinRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, 0, 127 );
	m_pHihatMinRangeLCD->move( 142, 305 );
	connect( m_pHihatMinRangeLCD, SIGNAL( valueChanged( double ) ), this, SLOT( hihatMinRangeChanged( double ) ) );
	m_pHihatMinRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getHihatMinRangeLabel() );
	m_pHihatMinRangeLbl->move( 136, 328 );

	m_pHihatMaxRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 51, 20 ), LCDSpinBox::Type::Int, 0, 127 );
	m_pHihatMaxRangeLCD->move( 207, 305 );
	connect( m_pHihatMaxRangeLCD, SIGNAL( valueChanged( double ) ), this, SLOT( hihatMaxRangeChanged( double ) ) );
	m_pHihatMaxRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getHihatMaxRangeLabel() );
	m_pHihatMaxRangeLbl->move( 201, 328 );
	//~ Instrument properties

	// LAYER properties
	m_pLayerProp = new PixmapWidget( this );
	m_pLayerProp->setObjectName( "LayerProperties" );
	m_pLayerProp->move( 0, 31 );
	m_pLayerProp->hide();
	m_pLayerProp->setPixmap( "/instrumentEditor/layerTabsupernew.png" );

	// Component
	m_pCompoNameLbl = new ClickableLabel( m_pLayerProp, QSize( 275, 28 ), "" );
	m_pCompoNameLbl->move( 8, 5 );
	connect( m_pCompoNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelCompoClicked(ClickableLabel*) ) );

	m_buttonDropDownCompo = new Button( m_pLayerProp, QSize( 13, 13 ), "dropdown.svg" );
	m_buttonDropDownCompo->move( 272, 10 );
	connect( m_buttonDropDownCompo, SIGNAL( clicked( Button* ) ), this, SLOT( onClick( Button* ) ) );

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
	connect( m_pWaveDisplay, SIGNAL( doubleClicked(QWidget*) ), this, SLOT( waveDisplayDoubleClicked(QWidget*) ) );

	m_pLoadLayerBtn = new Button( m_pLayerProp, QSize( 90, 13 ), "", HydrogenApp::get_instance()->getCommonStrings()->getLoadLayerButton() );
	m_pLoadLayerBtn->setObjectName( "LoadLayerButton" );
	m_pLoadLayerBtn->move( 6, 306 );

	m_pRemoveLayerBtn = new Button( m_pLayerProp, QSize( 90, 13 ), "", HydrogenApp::get_instance()->getCommonStrings()->getDeleteLayerButton() );
	m_pRemoveLayerBtn->setObjectName( "RemoveLayerButton" );
	m_pRemoveLayerBtn->move( 99, 306 );

	m_pSampleEditorBtn = new Button( m_pLayerProp, QSize( 90, 13 ), "", HydrogenApp::get_instance()->getCommonStrings()->getEditLayerButton() );
	m_pSampleEditorBtn->setObjectName( "SampleEditorButton" );
	m_pSampleEditorBtn->move( 191, 306 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pSampleEditorBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerGainRotary = new Rotary( m_pLayerProp,  Rotary::Type::Normal, tr( "Layer gain" ), false , 0.0, 5.0);
	m_pLayerGainRotary->setDefaultValue( 1.0 );
	connect( m_pLayerGainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerGainLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getLayerGainLabel() );
	m_pLayerGainLbl->move( 50, 361 );

	m_pCompoGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pCompoGainRotary = new Rotary( m_pLayerProp,  Rotary::Type::Normal, tr( "Component volume" ), false, 0.0, 5.0 );
	m_pCompoGainRotary->setDefaultValue ( 1.0 );
	connect( m_pCompoGainRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pCompoGainLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getComponentGainLabel() );
	m_pCompoGainLbl->move( 147, 361 );

	m_pLayerPitchCoarseLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerPitchFineLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerPitchLbl = new ClickableLabel( m_pLayerProp, QSize( 45, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchLabel() );
	m_pLayerPitchLbl->move( 17, 412 );

	m_pLayerPitchCoarseRotary = new Rotary( m_pLayerProp, Rotary::Type::Center, tr( "Layer pitch (Coarse)" ), true, -24.0, 24.0 );
	connect( m_pLayerPitchCoarseRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerPitchCoarseLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchCoarseLabel() );
	m_pLayerPitchCoarseLbl->move( 61, 411 );

	m_pLayerPitchFineRotary = new Rotary( m_pLayerProp, Rotary::Type::Center, tr( "Layer pitch (Fine)" ), true, -50.0, 50.0 );
	connect( m_pLayerPitchFineRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );
	m_pLayerPitchFineLbl = new ClickableLabel( m_pLayerProp, QSize( 44, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getPitchFineLabel() );
	m_pLayerPitchFineLbl->move( 147, 411 );

	m_pLayerGainLCD->move( 54, 344 );
	m_pLayerGainRotary->move( 94, 341 );

	m_pCompoGainLCD->move( 151, 344 );
	m_pCompoGainRotary->move( 191, 341 );


	m_pLayerPitchCoarseLCD->move( 66, 394 );
	m_pLayerPitchCoarseRotary->move( 105, 391 );

	m_pLayerPitchFineLCD->move( 151, 394 );
	m_pLayerPitchFineRotary->move( 191, 391 );

	m_sampleSelectionAlg = new LCDCombo(m_pLayerProp, QSize( width() - 76 - 8, 16 ) );
	m_sampleSelectionAlg->move( 76, 433 );
	m_sampleSelectionAlg->setToolTip( tr( "Select selection algorithm" ) );
	m_sampleSelectionAlg->addItem( QString( "First in Velocity" ) );
	m_sampleSelectionAlg->addItem( QString( "Round Robin" ) );
	m_sampleSelectionAlg->addItem( QString( "Random" ) );
	connect( m_sampleSelectionAlg, SIGNAL( currentIndexChanged( int ) ), this, SLOT( sampleSelectionChanged( int ) ) );
	m_pSampleSelectionLbl = new ClickableLabel( m_pLayerProp, QSize( 70, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getSampleSelectionLabel() );
	m_pSampleSelectionLbl->move( 7, 437 );

	//~ Layer properties

	//component handling
	QStringList itemsCompo;
	popCompo = new QMenu( this );
	itemsCompo.clear();

	std::vector<DrumkitComponent*>* pComponentList = Hydrogen::get_instance()->getSong()->getComponents();
	for (std::vector<DrumkitComponent*>::iterator it = pComponentList->begin() ; it != pComponentList->end(); ++it) {
		DrumkitComponent* pComponent = *it;
		if( !itemsCompo.contains( pComponent->get_name() ) ) {
			itemsCompo.append( pComponent->get_name() );
		}
	}
	itemsCompo.append("--sep--");
	itemsCompo.append("add");
	itemsCompo.append("delete");
	itemsCompo.append("rename");

	m_nSelectedComponent = pComponentList->front()->get_id();

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
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	
	if ( pSong != nullptr ) {
		InstrumentList *pInstrList = pSong->getInstrumentList();
		int nInstr = pHydrogen->getSelectedInstrumentNumber();
		if ( nInstr >= pInstrList->size() ) {
			nInstr = -1;
		}

		if ( nInstr == -1 ) {
			m_pInstrument = nullptr;
		}
		else {
			m_pInstrument = pInstrList->get( nInstr );
			//INFOLOG( "new instr: " + m_pInstrument->m_sName );
		}
	}
	else {
		m_pInstrument = nullptr;
	}
	pHydrogen->getAudioEngine()->unlock();

	// update layer list
	if ( m_pInstrument ) {
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
		m_pFilterBypassBtn->setPressed( !m_pInstrument->is_filter_active() );
		m_pCutoffRotary->setValue( m_pInstrument->get_filter_cutoff() );
		m_pResonanceRotary->setValue( m_pInstrument->get_filter_resonance() );
		//~ filter

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
		m_sampleSelectionAlg->setCurrentIndex( m_pInstrument->sample_selection_alg() );

		itemsCompo.clear();
		std::vector<DrumkitComponent*>* compoList = pSong->getComponents();
		for (auto& it : *pSong->getComponents() ) {
			DrumkitComponent* pDrumkitComponent = it;
			if( !itemsCompo.contains( pDrumkitComponent->get_name() ) ) {
				itemsCompo.append( pDrumkitComponent->get_name() );
			}
		}
		itemsCompo.append("--sep--");
		itemsCompo.append("add");
		itemsCompo.append("delete");
		itemsCompo.append("rename");

		update();

		bool bFound = false;
		for (std::vector<DrumkitComponent*>::iterator it = compoList->begin() ; it != compoList->end(); ++it) {
			DrumkitComponent* pComponent = *it;
			if ( pComponent->get_id() == m_nSelectedComponent ) {
				bFound = true;
				break;
			}
		}
		if ( !bFound ){
			m_nSelectedComponent = compoList->front()->get_id();
		}

		DrumkitComponent* pTmpComponent = pSong->getComponent( m_nSelectedComponent );

		assert(pTmpComponent);

		m_pCompoNameLbl->setText( pTmpComponent->get_name() );

		if( m_nSelectedLayer >= 0 ){
			
			auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
			if( pComponent ) {

				char tmp[20];
				sprintf( tmp, "%#.2f", pComponent->get_gain());
				m_pCompoGainLCD->setText( tmp );

				m_pCompoGainRotary->setValue( pComponent->get_gain() );

				auto pLayer = pComponent->get_layer( m_nSelectedLayer );
				if(pLayer) {
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
		else{
			m_pWaveDisplay->updateDisplay( nullptr );
		}
	}
	else {
		m_pNameLbl->setText( QString( "NULL Instrument..." ) );
		m_pWaveDisplay->updateDisplay( nullptr );
		m_nSelectedLayer = 0;
	}
	
	selectLayer( m_nSelectedLayer );
}



void InstrumentEditor::rotaryChanged( WidgetWithInput *ref)
{
	Rotary* pRotary = dynamic_cast<Rotary*>( ref );
	
	float fVal = pRotary->getValue();

	if ( m_pInstrument ) {
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
			m_pInstrument->get_adsr()->set_attack( fVal * fVal * 100000 );
		}
		else if ( pRotary == m_pDecayRotary ) {
			m_pInstrument->get_adsr()->set_decay( fVal * fVal * 100000 );
		}
		else if ( pRotary == m_pSustainRotary ) {
			m_pInstrument->get_adsr()->set_sustain( fVal );
		}
		else if ( pRotary == m_pReleaseRotary ) {
			m_pInstrument->get_adsr()->set_release( 256.0 + fVal * fVal * 100000 );
		}
		else if ( pRotary == m_pLayerGainRotary ) {
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pLayerGainLCD->setText( tmp );

			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				auto pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
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
			if( pCompo ) {
				auto pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
					float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
					pLayer->set_pitch( fCoarse + fFine );
					INFOLOG( tr( "layer pitch: %1" ).arg( pLayer->get_pitch() ) );
				}
			}
		}
		else if ( pRotary == m_pLayerPitchFineRotary ) {
			m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal ) );
			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				auto pLayer = pCompo->get_layer( m_nSelectedLayer );
				if ( pLayer ) {
					float fCoarse = round( m_pLayerPitchCoarseRotary->getValue() );
					float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
					pLayer->set_pitch( fCoarse + fFine );
					INFOLOG( tr( "layer pitch: %1" ).arg( pLayer->get_pitch() ) );
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
	
	auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
	if( !pCompo ) {
		return;
	}
			
	auto pLayer = pCompo->get_layer( m_nSelectedLayer );
	if ( pLayer ) {
		auto pSample = pLayer->get_sample();
		
		if( pSample != nullptr ) {
			QString name = pSample->get_filepath();
			HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
		}
	}
	else {
		loadLayer();
	}
}

void InstrumentEditor::showLayers()
{
	m_pShowLayersBtn->setPressed( true );
	m_pShowInstrumentBtn->setPressed( false );
	m_pLayerProp->show();
	m_pInstrumentProp->hide();

	m_pShowLayersBtn->show();
	m_pShowInstrumentBtn->show();
}

void InstrumentEditor::showInstrument()
{
	m_pShowInstrumentBtn->setPressed( true );
	m_pShowLayersBtn->setPressed( false );
	m_pInstrumentProp->show();
	m_pLayerProp->hide();

	m_pShowLayersBtn->show();
	m_pShowInstrumentBtn->show();
}

void InstrumentEditor::showSampleEditor()
{
	if ( m_pInstrument ) {
		auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if( pCompo ) {
			auto pLayer = pCompo->get_layer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				auto pSample = pLayer->get_sample();
				if ( pSample == nullptr ) {
					return;
				}
				QString name = pSample->get_filepath();
				HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedComponent, m_nSelectedLayer );
			}
		}
	}
}
	
void InstrumentEditor::buttonClicked( Button* pButton )
{

	if ( pButton == m_pShowInstrumentBtn ) {
		showInstrument();
	}
	else if ( pButton == m_pShowLayersBtn ) {
		showLayers();
	}
	else if ( pButton == m_pLoadLayerBtn ) {
		loadLayer();
	}
	else if ( pButton == m_pRemoveLayerBtn ) {
		//Hydrogen *pHydrogen = Hydrogen::get_instance();
		Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

		if ( m_pInstrument ) {
			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if( pCompo ) {
				pCompo->set_layer( nullptr, m_nSelectedLayer );

				int nCount = 0;
				for( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
					auto pLayer = pCompo->get_layer( n );
					if( pLayer ){
						nCount++;
					}
				}

				if( nCount == 0 ){
					m_pInstrument->get_components()->erase( m_pInstrument->get_components()->begin() + m_nSelectedComponent );
				}
			}
		}
		Hydrogen::get_instance()->getAudioEngine()->unlock();
		selectedInstrumentChangedEvent();    // update all
		m_pLayerPreview->updateAll();
	}
	else if ( pButton == m_pSampleEditorBtn ){
		showSampleEditor();
	}
	else {
		ERRORLOG( "[buttonClicked] unhandled button" );
	}
}



void InstrumentEditor::loadLayer()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

	AudioFileBrowser *pFileBrowser = new AudioFileBrowser( nullptr, true, true);
	QStringList filename;
	filename << "false" << "false" << "";

	if (pFileBrowser->exec() == QDialog::Accepted) {
		filename = pFileBrowser->getSelectedFiles();
	}

	delete pFileBrowser;

	if ( filename[2].isEmpty() ) return;

	bool fnc = false;
	if ( filename[0] ==  "true" ){
		fnc = true;
	}

	int selectedLayer =  m_nSelectedLayer;
	int firstSelection = selectedLayer;

	// Ensure instrument pointer is current
	Song *pSong = pHydrogen->getSong();
	if ( pSong ) {
		InstrumentList *pInstrList = pSong->getInstrumentList();
		m_pInstrument = pInstrList->get( pHydrogen->getSelectedInstrumentNumber() );
	} else {
		m_pInstrument = nullptr;
	}

	if ( m_pInstrument == nullptr ) {
		return;
	}

	if (filename.size() > 2) {

		for(int i=2;i < filename.size();++i)
		{
			selectedLayer = m_nSelectedLayer + i - 2;
			if( ( i-2 >= InstrumentComponent::getMaxLayers() ) || ( selectedLayer + 1  > InstrumentComponent::getMaxLayers() ) ) break;

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

			if (pLayer != nullptr) {
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
	std::vector<int> layerInUse( InstrumentComponent::getMaxLayers(), 0 );
	int nLayers = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers() ; i++ ) {

		auto pLayer = pCompo->get_layer( i );
		if ( pLayer ) {
			nLayers++;
			layerInUse[i] = i;
		}
	}
	
	if( nLayers == 0){
		nLayers = 1;
	}

	float velocityrange = 1.0 / nLayers;

	for ( int i = 0; i < InstrumentComponent::getMaxLayers() ; i++ ) {
		if ( layerInUse[i] == i ){
			nLayers--;
			auto pLayer = pCompo->get_layer( i );
			if ( pLayer ) {
				pLayer->set_start_velocity( nLayers * velocityrange);
				pLayer->set_end_velocity( nLayers * velocityrange + velocityrange );
			}
		}
	}
}

void InstrumentEditor::labelCompoClicked( ClickableLabel* pRef )
{
	UNUSED( pRef );
	Song *pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	DrumkitComponent* pComponent = pSong->getComponent( m_nSelectedComponent );
	if ( pComponent != nullptr ) {
		return;
	}
	QString sOldName = pComponent->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New component name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );

	if ( bIsOkPressed  ) {
		pComponent->set_name( sNewName );

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
		QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
		if ( bIsOkPressed  ) {
			m_pInstrument->set_name( sNewName );
			selectedInstrumentChangedEvent();

#ifdef H2CORE_HAVE_JACK
			Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
			Hydrogen *engine = Hydrogen::get_instance();
			engine->renameJackPorts(engine->getSong());
			Hydrogen::get_instance()->getAudioEngine()->unlock();
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

	auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
	if(pComponent && nLayer >= 0 ){
		auto pLayer = pComponent->get_layer( nLayer );
		m_pWaveDisplay->updateDisplay( pLayer );
		if (pLayer) {
			char tmp[20];

			// Layer GAIN
			m_pLayerGainRotary->setValue( pLayer->get_gain() );
			sprintf( tmp, "%#.2f", pLayer->get_gain() );
			m_pLayerGainLCD->setText( tmp );

			//Component GAIN
			char tmp2[20];
			sprintf( tmp2, "%#.2f", pComponent->get_gain());
			m_pCompoGainRotary->setValue( pComponent->get_gain());
			m_pCompoGainLCD->setText( tmp2 );

			// Layer PITCH
			float fCoarsePitch = round( pLayer->get_pitch() );
			float fFinePitch = pLayer->get_pitch() - fCoarsePitch;
			//INFOLOG( "fine pitch: " + to_string( fFinePitch ) );
			m_pLayerPitchCoarseRotary->setValue( fCoarsePitch );
			m_pLayerPitchFineRotary->setValue( fFinePitch * 100 );

			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int) fCoarsePitch ) );
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
		m_pWaveDisplay->updateDisplay( nullptr );

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



void InstrumentEditor::muteGroupChanged( int nValue )
{
	assert( m_pInstrument );

	m_pInstrument->set_mute_group( nValue );
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

void InstrumentEditor::midiOutChannelChanged( int nValue ) {
	assert( m_pInstrument );

	if ( nValue != 0 ) {
		m_pInstrument->set_midi_out_channel( nValue );
	} else {
		if ( m_nPreviousMidiOutChannel == -1 ) {
			m_pMidiOutChannelLCD->setValue( 1 );
			return;
		} else {
			m_pMidiOutChannelLCD->setValue( -1 );
			return;
		}
	}

	m_nPreviousMidiOutChannel = nValue;
}

void InstrumentEditor::midiOutNoteChanged( int nValue ) {
	assert( m_pInstrument );

	m_pInstrument->set_midi_out_note( nValue );
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
	std::vector<DrumkitComponent*>* pDrumkitComponentList = Hydrogen::get_instance()->getSong()->getComponents();
	for (std::vector<DrumkitComponent*>::iterator it = pDrumkitComponentList->begin() ; it != pDrumkitComponentList->end(); ++it) {
		DrumkitComponent* pDrumkitComponent = *it;
		if( pDrumkitComponent->get_id() == startingPoint ) {
			bFoundFreeSlot = false;
			break;
		}
	}

	if(bFoundFreeSlot) {
		return startingPoint;
	} else {
		return findFreeDrumkitComponentId( startingPoint + 1 );
	}
}

void InstrumentEditor::compoChangeAddDelete(QAction* pAction)
{
	QString sSelectedAction = pAction->text();

	Hydrogen * pHydrogen = Hydrogen::get_instance();

	if( sSelectedAction.compare("add") == 0 ) {
		if ( m_pInstrument ) {
			bool bIsOkPressed;
			QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "Component name" ), QLineEdit::Normal, "New Component", &bIsOkPressed );
			if ( bIsOkPressed  ) {
				DrumkitComponent* pDrumkitComponent = new DrumkitComponent( findFreeDrumkitComponentId(), sNewName );
				pHydrogen->getSong()->getComponents()->push_back( pDrumkitComponent );

				//auto instrument_component = std::make_shared<InstrumentComponent>( dm_component->get_id() );
				//instrument_component->set_gain( 1.0f );
				//m_pInstrument->get_components()->push_back( instrument_component );

				m_nSelectedComponent = pDrumkitComponent->get_id();
				m_pLayerPreview->set_selected_component( pDrumkitComponent->get_id() );

				selectedInstrumentChangedEvent();

				// this will force an update...
				EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

#ifdef H2CORE_HAVE_JACK
				pHydrogen->renameJackPorts(pHydrogen->getSong());
#endif
			}
			else {
				// user entered nothing or pressed Cancel
			}
		}
	}
	else if( sSelectedAction.compare("delete") == 0 ) {
		std::vector<DrumkitComponent*>* pDrumkitComponents = pHydrogen->getSong()->getComponents();

		if(pDrumkitComponents->size() == 1){
			return;
		}

		DrumkitComponent* pDrumkitComponent = pHydrogen->getSong()->getComponent( m_nSelectedComponent );

		InstrumentList* pInstruments = pHydrogen->getSong()->getInstrumentList();
		for ( int n = ( int )pInstruments->size() - 1; n >= 0; n-- ) {
			auto pInstrument = pInstruments->get( n );
			for( int o = 0 ; o < pInstrument->get_components()->size() ; o++ ) {
				auto  pInstrumentComponent = pInstrument->get_components()->at( o );
				if( pInstrumentComponent->get_drumkit_componentID() == pDrumkitComponent->get_id() ) {
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
		labelCompoClicked( nullptr );
	}
	else {
		m_nSelectedComponent = -1;
		std::vector<DrumkitComponent*>* pDrumkitComponents = pHydrogen->getSong()->getComponents();
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

			auto pInstrComponent = std::make_shared<InstrumentComponent>( m_nSelectedComponent );
			pInstrComponent->set_gain( 1.0f );

			m_pInstrument->get_components()->push_back( pInstrComponent );



#ifdef H2CORE_HAVE_JACK
			pHydrogen->renameJackPorts(pHydrogen->getSong());
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *song = pHydrogen->getSong();
	assert(song);
	if(song){
		InstrumentList *pSongInstrList = song->getInstrumentList();
		assert(pSongInstrList);
		for ( unsigned nInstr = 0; nInstr < pSongInstrList->size(); ++nInstr ) {
			auto pInstr = pSongInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				auto pInstrumentComponent = pInstr->get_component(m_nSelectedComponent);
				if (!pInstrumentComponent) {
					continue; // regular case when you have a new component empty
				}
				
				for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
					auto pLayer = pInstrumentComponent->get_layer( nLayer );
					if ( pLayer ) {
						auto pSample = pLayer->get_sample();
						if ( pSample ) {
							if( pSample->get_rubberband().use ) {
								//INFOLOG( QString("Instrument %1 Layer %2" ).arg(nInstr).arg(nLayer));
								auto pNewSample = Sample::load(
														pSample->get_filepath(),
														pSample->get_loops(),
														pSample->get_rubberband(),
														*pSample->get_velocity_envelope(),
														*pSample->get_pan_envelope()
														);
								if( !pNewSample  ){
									continue;
								}
								
								// insert new sample from newInstrument
								Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
								pLayer->set_sample( pNewSample );
								Hydrogen::get_instance()->getAudioEngine()->unlock();

							}
						}
					}
				}
			}
		}
	}

}

void InstrumentEditor::sampleSelectionChanged( int selected )
{
	assert( m_pInstrument );

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

void InstrumentEditor::hihatGroupChanged( int nValue )
{
	assert( m_pInstrument );
	m_pInstrument->set_hihat_grp( nValue );
}

void InstrumentEditor::hihatMinRangeChanged( int nValue )
{
	assert( m_pInstrument );
	m_pInstrument->set_lower_cc( nValue );
	m_pHihatMaxRangeLCD->setMinimum( nValue );
}

void InstrumentEditor::hihatMaxRangeChanged( int nValue )
{
	assert( m_pInstrument );
	m_pInstrument->set_higher_cc( nValue );
	m_pHihatMinRangeLCD->setMaximum( nValue );
}
