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

#include <math.h>
#include <assert.h>

#include <hydrogen/Song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/adsr.h>
#include <hydrogen/sample.h>
#include <hydrogen/instrument.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;

#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../widgets/Rotary.h"
#include "../widgets/ClickableLabel.h"
#include "../widgets/Button.h"
#include "../widgets/LCD.h"
#include "../widgets/Fader.h"
#include "InstrumentEditor.h"
#include "WaveDisplay.h"
#include "LayerPreview.h"
#include "AudioFileBrowser/AudioFileBrowser.h"

InstrumentEditor::InstrumentEditor( QWidget* pParent )
 : QWidget( pParent )
 , Object( "InstrumentEditor" )
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
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp );
	m_pNameLbl->setGeometry( 8, 5, 275, 28 );
	
	/////////////
	//Midi Out
	
	m_pMidiOutChannelLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMidiOutChannelLCD->move( 67, 243 );

	m_pAddMidiOutChannelBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize( 16, 8 )
	);

	m_pAddMidiOutChannelBtn->move( 109, 243 );
	connect( m_pAddMidiOutChannelBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutChannelBtnClicked(Button*) ) );


	m_pDelMidiOutChannelBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16,8)
	);
	m_pDelMidiOutChannelBtn->move( 109, 251 );
	connect( m_pDelMidiOutChannelBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutChannelBtnClicked(Button*) ) );
	
	///
	m_pMidiOutNoteLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMidiOutNoteLCD->move( 160, 243 );

	m_pAddMidiOutNoteBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize( 16, 8 )
	);

	m_pAddMidiOutNoteBtn->move( 202, 243 );
	connect( m_pAddMidiOutNoteBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutNoteBtnClicked(Button*) ) );


	m_pDelMidiOutNoteBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16,8)
	);
	m_pDelMidiOutNoteBtn->move( 202, 251 );
	connect( m_pDelMidiOutNoteBtn, SIGNAL( clicked(Button*) ), this, SLOT( midiOutNoteBtnClicked(Button*) ) );
	
	/////////////

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );
	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelClicked(ClickableLabel*) ) );

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Random pitch factor" ), false, true );
	m_pRandomPitchRotary->move( 117, 192 );
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
	connect( m_pCutoffRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Filter resonance" ), false, true );
	connect( m_pResonanceRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pFilterBypassBtn->move( 70, 152 );
	m_pCutoffRotary->move( 117, 146 );
	m_pResonanceRotary->move( 170, 146 );
	//~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Attack" ), false, true );
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Decay" ), false, true );
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Sustain" ), false, true );
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Release" ), false, true );
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

        m_pIsStopNoteCheckBox = new QCheckBox ( trUtf8( "Auto-Stop-Note" ), m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 15, 300 );
	m_pIsStopNoteCheckBox->setToolTip( trUtf8( "Stop the current playing instrument-note before trigger the next note sample." ) );
	connect( m_pIsStopNoteCheckBox, SIGNAL( toggled( bool ) ), this, SLOT( onIsStopNoteCheckBoxClicked( bool ) ) );

//~ Instrument properties





// LAYER properties
	m_pLayerProp = new PixmapWidget( this );
	m_pLayerProp->move( 0, 31 );
	m_pLayerProp->hide();
	m_pLayerProp->setPixmap( "/instrumentEditor/layerTabnew.png" );



	// Layer preview
	m_pLayerPreview = new LayerPreview( NULL );
 
	m_pLayerScrollArea = new QScrollArea( m_pLayerProp);
	m_pLayerScrollArea->setFrameShape( QFrame::NoFrame );
	m_pLayerScrollArea->move( 6, 4 );
	m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	if ( MAX_LAYERS > 16)
		m_pLayerScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_pLayerScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pLayerScrollArea->setMaximumHeight( 182 );
	m_pLayerScrollArea->setWidget( m_pLayerPreview  );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( m_pLayerProp );
	m_pWaveDisplay->updateDisplay( NULL );
	m_pWaveDisplay->move( 5, 201 );

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

	m_pSamleEditorBtn = new Button(
			m_pLayerProp,
			"/instrumentEditor/editLayer_on.png",
			"/instrumentEditor/editLayer_off.png",
			"/instrumentEditor/editLayer_over.png",
			QSize( 90, 13 )
	);
	m_pLoadLayerBtn->move( 48, 267 );
	m_pRemoveLayerBtn->move( 145, 267 );



	m_pLoadLayerBtn->move( 6, 266 );
	m_pRemoveLayerBtn->move( 99, 266 );
	m_pSamleEditorBtn->move( 191, 266 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pSamleEditorBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerGainRotary = new Rotary( m_pLayerProp,  Rotary::TYPE_NORMAL, trUtf8( "Layer gain" ), false, false );
	connect( m_pLayerGainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerPitchCoarseLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerPitchFineLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );

	m_pLayerPitchCoarseRotary = new Rotary( m_pLayerProp, Rotary::TYPE_CENTER, trUtf8( "Layer pitch (Coarse)" ), true, false );
	m_pLayerPitchCoarseRotary->setMin( -24.0 );
	m_pLayerPitchCoarseRotary->setMax( 24.0 );
	connect( m_pLayerPitchCoarseRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerPitchFineRotary = new Rotary( m_pLayerProp, Rotary::TYPE_CENTER, trUtf8( "Layer pitch (Fine)" ), false, false );
	m_pLayerPitchFineRotary->setMin( -50.0 );
	m_pLayerPitchFineRotary->setMax( 50.0 );
	connect( m_pLayerPitchFineRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerGainLCD->move( 54, 301 + 3 );
	m_pLayerGainRotary->move( 102, 301 );

	m_pLayerPitchCoarseLCD->move( 54, 360 + 3 );
	m_pLayerPitchCoarseRotary->move( 102, 360 );

	m_pLayerPitchFineLCD->move(  151, 360 + 3 );
	m_pLayerPitchFineRotary->move( 199, 360 );
//~ Layer properties








	selectLayer( m_nSelectedLayer );

	HydrogenApp::get_instance()->addEventListener(this);

	selectedInstrumentChangedEvent(); 	// force an update
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
		if ( nInstr >= (int)pInstrList->get_size() ) {
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
		m_pAttackRotary->setValue( sqrtf(m_pInstrument->get_adsr()->__attack / 100000.0) );
		m_pDecayRotary->setValue( sqrtf(m_pInstrument->get_adsr()->__decay / 100000.0) );
		m_pSustainRotary->setValue( m_pInstrument->get_adsr()->__sustain );
		float fTmp = m_pInstrument->get_adsr()->__release - 256.0;
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
		
		// midi out
		QString sMidiOutChannel = QString("%1").arg( m_pInstrument->get_midi_out_channel()+1);
		if (m_pInstrument->get_midi_out_channel() == -1 ) {
			sMidiOutChannel = "Off";
		}
		m_pMidiOutChannelLCD->setText( sMidiOutChannel );
		
		//Convert note id into notation
		{
			int note = m_pInstrument->get_midi_out_note();
			int octave = (note / 12) - 2;
			const char *noteStrs[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
			QString sMidiOutNote = QString(noteStrs[note % 12]) + QString::number(octave);
			m_pMidiOutNoteLCD->setText( sMidiOutNote );			
		}

		// select the last valid layer
		for (int i = MAX_LAYERS - 1; i >= 0; i-- ) {
			if ( m_pInstrument->get_layer( i ) ) {
				m_nSelectedLayer = i;
				break;
			}
		}
		m_pWaveDisplay->updateDisplay( m_pInstrument->get_layer( m_nSelectedLayer ) );
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
			m_pInstrument->get_adsr()->__attack = fVal * fVal * 100000;
		}
		else if ( ref == m_pDecayRotary ) {
			m_pInstrument->get_adsr()->__decay = fVal * fVal * 100000;
		}
		else if ( ref == m_pSustainRotary ) {
			m_pInstrument->get_adsr()->__sustain = fVal;
		}
		else if ( ref == m_pReleaseRotary ) {
			m_pInstrument->get_adsr()->__release = 256.0 + fVal * fVal * 100000;
		}
		else if ( ref == m_pLayerGainRotary ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pLayerGainLCD->setText( tmp );

			H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				pLayer->set_gain( fVal );
				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
		else if ( ref == m_pLayerPitchCoarseRotary ) {
			//fVal = fVal * 24.0 - 12.0;
			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int)fVal ) );
			H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->set_pitch( nCoarse + fFine );
				INFOLOG( QString("pitch: %1").arg( pLayer->get_pitch() ) );
			}
		}
		else if ( ref == m_pLayerPitchFineRotary ) {
			m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal ) );
			H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->set_pitch( nCoarse + fFine );
				INFOLOG( QString("pitch: %1").arg( pLayer->get_pitch()) );
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
			H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				m_pInstrument->set_layer( NULL, m_nSelectedLayer );
				delete pLayer;
			}
		}
		AudioEngine::get_instance()->unlock();
		selectedInstrumentChangedEvent();    // update all
		m_pLayerPreview->updateAll();
	}
	else if ( pButton == m_pSamleEditorBtn ){
		if ( m_pInstrument ) {
			H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				Sample* pSample = pLayer->get_sample();
				if( pSample == NULL) return;
				QString name = pSample->get_filename();
				HydrogenApp::get_instance()->showSampleEditor( name, m_nSelectedLayer );
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
	int firstselection = selectedLayer;
	
	

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
			
			H2Core::InstrumentLayer *pLayer = pInstr->get_layer( selectedLayer );
			if (pLayer != NULL) {
				// delete old sample
				Sample *oldSample = pLayer->get_sample();
				delete oldSample;
	
				// insert new sample from newInstrument
				pLayer->set_sample( newSample );
			}
			else {
				pLayer = new H2Core::InstrumentLayer(newSample);
				pInstr->set_layer( pLayer, selectedLayer );
			}
	
			if ( fnc ){
				QString newfilename = filename[i].section( '/', -1 );
				newfilename.replace( "." + newfilename.section( '.', -1 ), "");
				m_pInstrument->set_name( newfilename );
			}
	
			//set automatic velocity
			if ( filename[1] ==  "true" ){
				setAutoVelocity();
			}
	
			pInstr->set_drumkit_name( "" );   // external sample, no drumkit info
	
			AudioEngine::get_instance()->unlock();

		}
	}

	selectedInstrumentChangedEvent();    // update all
	selectLayer( firstselection );
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::setAutoVelocity()
{
	int layerinuse[ MAX_LAYERS ] = {0};
	int layers = 0;
	for ( int i = 0; i < MAX_LAYERS ; i++ ) {
		InstrumentLayer *pLayers = m_pInstrument->get_layer( i );
		if ( pLayers ) {
			layers++;
			layerinuse[i] = i;
		}
	}

	float velocityrange = 1.0 / layers;

	for ( int i = 0; i < MAX_LAYERS ; i++ ) {
		if ( layerinuse[i] == i ){
			layers--;
			InstrumentLayer *pLayer = m_pInstrument->get_layer( i );
			if ( pLayer ) {
				pLayer->set_start_velocity( layers * velocityrange);
				pLayer->set_end_velocity( layers * velocityrange + velocityrange );
			}
		}
	}
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
			
			#ifdef JACK_SUPPORT
                        AudioEngine::get_instance()->lock( RIGHT_HERE );
			Hydrogen *engine = Hydrogen::get_instance();
			engine->renameJackPorts();
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

	H2Core::InstrumentLayer *pLayer = m_pInstrument->get_layer( nLayer );
	m_pWaveDisplay->updateDisplay( pLayer );
	if (pLayer) {
		char tmp[20];

		// Layer GAIN
		m_pLayerGainRotary->setValue( pLayer->get_gain() / 5.0 );
		sprintf( tmp, "%#.2f", pLayer->get_gain() );
		m_pLayerGainLCD->setText( tmp );

		// Layer PITCH
		//int nCoarsePitch = pLayer->m_fPitch / 24 + 0.5;
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

	if (pRef == m_pAddMuteGroupBtn ) {
		m_pInstrument->set_mute_group( m_pInstrument->get_mute_group() + 1);
	}
	else if (pRef == m_pDelMuteGroupBtn ) {
		m_pInstrument->set_mute_group( m_pInstrument->get_mute_group() - 1);
	}

	selectedInstrumentChangedEvent();	// force an update
}

void InstrumentEditor::onIsStopNoteCheckBoxClicked( bool on )
{
	m_pInstrument->set_stop_note( on );
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


 void InstrumentEditor::rubberbandbpmchangeEvent()
{
	if( !Preferences::get_instance()->m_useTheRubberbandBpmChangeEvent /*&& Preferences::get_instance()->__usetimeline */){
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
		for ( unsigned nInstr = 0; nInstr < songInstrList->get_size(); ++nInstr ) {
			Instrument *pInstr = songInstrList->get( nInstr );
			assert( pInstr );
			if ( pInstr ){
				for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
					InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
					if ( pLayer ) {
						Sample *pSample = pLayer->get_sample();
						if ( pSample ) {
							if(pSample->get_use_rubber()){
								//INFOLOG( QString("Instrument %1 Layer %2" ).arg(nInstr).arg(nLayer));
	
								QString filename = pSample->get_filename();
								unsigned startframe = pSample->get_start_frame();
								unsigned loopframe = pSample->get_loop_frame();
								unsigned endframe = pSample->get_end_frame();
								int loops = pSample->get_repeats();
								QString	mode = pSample->get_sample_mode();
								bool userubber = pSample->get_use_rubber();
								float rd = pSample->get_rubber_divider();
								int csettings = pSample->get_rubber_C_settings();
								float rpitch = pSample->get_rubber_pitch();

								Hydrogen::HVeloVector velovector;
								pEngine->m_volumen.clear();
								for( int i = 0 ; i < static_cast<int>(pSample->__velo_pan.m_Samplevolumen.size()); i++){
									velovector.m_hxframe = pSample->__velo_pan.m_Samplevolumen[i].m_SampleVeloframe;
									velovector.m_hyvalue = pSample->__velo_pan.m_Samplevolumen[i].m_SampleVelovalue;
									pEngine->m_volumen.push_back( velovector );	
								}

								Hydrogen::HPanVector panvector;
								pEngine->m_pan.clear();
								for( int i = 0 ; i < static_cast<int>(pSample->__velo_pan.m_SamplePan.size()); i++){
									panvector.m_hxframe = pSample->__velo_pan.m_SamplePan[i].m_SamplePanframe;
									panvector.m_hyvalue = pSample->__velo_pan.m_SamplePan[i].m_SamplePanvalue;
									pEngine->m_pan.push_back( panvector );
								}
			
                                                                Sample *newSample = Sample::load_edit_sndfile( filename,
                                                                                                               startframe,
                                                                                                               loopframe,
                                                                                                               endframe,
                                                                                                               loops,
                                                                                                               mode,
                                                                                                               userubber,
                                                                                                               rd,
                                                                                                               csettings,
                                                                                                               rpitch);
	
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
