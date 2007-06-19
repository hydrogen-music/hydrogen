/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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


#include <QPixmap>
#include <QInputDialog>
#include <QFileDialog>

#include <math.h>
#include <assert.h>

#include <hydrogen/Song.h>
#include <hydrogen/Hydrogen.h>
#include <hydrogen/Globals.h>
#include <hydrogen/adsr.h>
#include <hydrogen/Sample.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/EventQueue.h>
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


InstrumentEditor::InstrumentEditor( QWidget* pParent )
 : QWidget( pParent )
 , Object( "InstrumentEditor" )
 , m_pInstrument( NULL )
 , m_nSelectedLayer( 0 )
{
	setFixedWidth( 248 );


	m_pShowInstrumentBtn = new ToggleButton(
			this,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 96, 17 ),
			true
	);
	m_pShowInstrumentBtn->setText(trUtf8("General"));
	m_pShowInstrumentBtn->setToolTip( trUtf8( "Show instrument properties" ) );
	connect( m_pShowInstrumentBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowInstrumentBtn->move( 23, 0 );
	m_pShowInstrumentBtn->setPressed( true );


	m_pShowLayersBtn = new ToggleButton(
			this,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 96, 17 ),
			true
	);
	m_pShowLayersBtn->setText( trUtf8("Layers") );
	m_pShowLayersBtn->setToolTip( trUtf8( "Show layers properties" ) );
	connect( m_pShowLayersBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowLayersBtn->move( 119, 0 );




// Instrument properties
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move(0, 20);
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );


	m_pNameLbl = new ClickableLabel( m_pInstrumentProp );
	m_pNameLbl->setGeometry( 8, 36, 232, 25 );

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );
	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelClicked(ClickableLabel*) ) );

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Random pitch factor" ), false, true );
	m_pRandomPitchRotary->move( 96, 223 );
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

	m_pFilterBypassBtn->move( 49, 183 );
	m_pCutoffRotary->move( 96, 177 );
	m_pResonanceRotary->move( 149, 177 );
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
	m_pAttackRotary->move( 32, 83 );
	m_pDecayRotary->move( 84, 83 );
	m_pSustainRotary->move( 136, 83 );
	m_pReleaseRotary->move( 188, 83 );
	//~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Instrument gain" ), false, false );
	connect( m_pInstrumentGain, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	m_pInstrumentGainLCD->move( 46, 136 );
	m_pInstrumentGain->move( 96, 131 );


	m_pMuteGroupLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pMuteGroupLCD->move( 139, 136 );

	m_pAddMuteGroupBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize( 16, 8 )
	);

	m_pAddMuteGroupBtn->move( 181, 135 );
	connect( m_pAddMuteGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteGroupBtnClicked(Button*) ) );


	m_pDelMuteGroupBtn = new Button(
			m_pInstrumentProp,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16,8)
	);
	m_pDelMuteGroupBtn->move( 181, 144 );
	connect( m_pDelMuteGroupBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteGroupBtnClicked(Button*) ) );

//~ Instrument properties





// LAYER properties
	m_pLayerProp = new PixmapWidget( this );
	m_pLayerProp->move( 0, 20 );
	m_pLayerProp->hide();
	m_pLayerProp->setPixmap( "/instrumentEditor/layerTab.png" );

	// Layer preview
	m_pLayerPreview = new LayerPreview( m_pLayerProp );
	m_pLayerPreview->move( 7, 35 );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( m_pLayerProp );
	m_pWaveDisplay->updateDisplay( NULL );
	m_pWaveDisplay->move( 6, 232 );

	m_pLoadLayerBtn = new Button(
			m_pLayerProp,
			"/instrumentEditor/loadLayer_on.png",
			"/instrumentEditor/loadLayer_off.png",
			"/instrumentEditor/loadLayer_over.png",
			QSize( 94, 13 )
	);

	m_pRemoveLayerBtn = new Button(
			m_pLayerProp,
			"/instrumentEditor/deleteLayer_on.png",
			"/instrumentEditor/deleteLayer_off.png",
			"/instrumentEditor/deleteLayer_over.png",
			QSize( 94, 13 )
	);

	m_pLoadLayerBtn->move( 28, 298 );
	m_pRemoveLayerBtn->move( 125, 298 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );

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

	m_pLayerGainLCD->move( 34, 332 + 3 );
	m_pLayerGainRotary->move( 82, 332 );

	m_pLayerPitchCoarseLCD->move( 34, 391 + 3 );
	m_pLayerPitchCoarseRotary->move( 82, 391 );

	m_pLayerPitchFineLCD->move(  131, 391 + 3 );
	m_pLayerPitchFineRotary->move( 179, 391 );
//~ Layer properties








	selectLayer( m_nSelectedLayer );

	HydrogenApp::getInstance()->addEventListener(this);

	selectedInstrumentChangedEvent(); 	// force an update
}



InstrumentEditor::~InstrumentEditor()
{
	//INFOLOG( "DESTROY" );
}



void InstrumentEditor::selectedInstrumentChangedEvent()
{
	AudioEngine::getInstance()->lock( "InstrumentEditor::selectedInstrumentChanged" );

	Song *pSong = Hydrogen::getInstance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->getInstrumentList();
		int nInstr = Hydrogen::getInstance()->getSelectedInstrumentNumber();
		if ( nInstr >= (int)pInstrList->getSize() ) {
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
	AudioEngine::getInstance()->unlock();

	// update layer list
	if (m_pInstrument) {
		m_pNameLbl->setText( QString( m_pInstrument->m_sName.c_str() ) );

		// ADSR
		m_pAttackRotary->setValue( m_pInstrument->m_pADSR->m_fAttack / 10000.0 );
		m_pDecayRotary->setValue( m_pInstrument->m_pADSR->m_fDecay / 10000.0 );
		m_pSustainRotary->setValue( m_pInstrument->m_pADSR->m_fSustain );
		m_pReleaseRotary->setValue( m_pInstrument->m_pADSR->m_fRelease / 10000.0 );
		//~ ADSR

		// filter
		m_pFilterBypassBtn->setPressed( !m_pInstrument->m_bFilterActive );
		m_pCutoffRotary->setValue( m_pInstrument->m_fCutoff );
		m_pResonanceRotary->setValue( m_pInstrument->m_fResonance );
		//~ filter

		// random pitch
		m_pRandomPitchRotary->setValue( m_pInstrument->m_fRandomPitchFactor );

		// instr gain
		char tmp[20];
		sprintf( tmp, "%#.2f", m_pInstrument->m_fGain);
		m_pInstrumentGainLCD->setText( tmp );
		m_pInstrumentGain->setValue( m_pInstrument->m_fGain / 5.0 );

		// instr mute group
		string sMuteGroup = toString( m_pInstrument->m_nMuteGroup );
		if (m_pInstrument->m_nMuteGroup == -1 ) {
			sMuteGroup = "Off";
		}
		m_pMuteGroupLCD->setText( QString( sMuteGroup.c_str() ) );

		// select the last valid layer
		for (int i = MAX_LAYERS - 1; i >= 0; i-- ) {
			if ( m_pInstrument->getLayer( i ) ) {
				m_nSelectedLayer = i;
				break;
			}
		}
		m_pWaveDisplay->updateDisplay( m_pInstrument->getLayer( m_nSelectedLayer ) );
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
			m_pInstrument->m_fRandomPitchFactor = fVal;
		}
		else if ( ref == m_pCutoffRotary ) {
			m_pInstrument->m_fCutoff = fVal;
		}
		else if ( ref == m_pResonanceRotary ) {
			if ( fVal > 0.95f ) {
				fVal = 0.95f;
			}
			m_pInstrument->m_fResonance = fVal;
		}
		else if ( ref == m_pAttackRotary ) {
			m_pInstrument->m_pADSR->m_fAttack = fVal * 10000;
		}
		else if ( ref == m_pDecayRotary ) {
			m_pInstrument->m_pADSR->m_fDecay = fVal * 10000;
		}
		else if ( ref == m_pSustainRotary ) {
			m_pInstrument->m_pADSR->m_fSustain = fVal;
		}
		else if ( ref == m_pReleaseRotary ) {
			m_pInstrument->m_pADSR->m_fRelease = fVal * 10000;
		}
		else if ( ref == m_pLayerGainRotary ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pLayerGainLCD->setText( tmp );

			H2Core::InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				pLayer->m_fGain = fVal;
				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
		else if ( ref == m_pLayerPitchCoarseRotary ) {
			//fVal = fVal * 24.0 - 12.0;
			m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( (int)fVal ) );
			H2Core::InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->m_fPitch = nCoarse + fFine;
				INFOLOG( "pitch: " + toString( pLayer->m_fPitch ) );
			}
		}
		else if ( ref == m_pLayerPitchFineRotary ) {
			m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fVal ) );
			H2Core::InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				int nCoarse = (int)m_pLayerPitchCoarseRotary->getValue();
				float fFine = m_pLayerPitchFineRotary->getValue() / 100.0;
				pLayer->m_fPitch = nCoarse + fFine;
				INFOLOG( "pitch: " + toString( pLayer->m_fPitch ) );
			}

		}
		else if ( ref == m_pInstrumentGain ) {
			fVal = fVal * 5.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pInstrumentGainLCD->setText( tmp );

			m_pInstrument->m_fGain = fVal;
		}
		else {
			ERRORLOG( "[rotaryChanged] unhandled rotary" );
		}
	}
}


void InstrumentEditor::filterActiveBtnClicked(Button *ref)
{
	if ( m_pInstrument ) {
		m_pInstrument->m_bFilterActive = !ref->isPressed();
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
		//Hydrogen *pEngine = Hydrogen::getInstance();
		AudioEngine::getInstance()->lock( "InstrumentPropertiesDialog::deleteBtnClicked" );

		if ( m_pInstrument ) {
			H2Core::InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				m_pInstrument->setLayer( NULL, m_nSelectedLayer );
				delete pLayer;
			}
		}
		AudioEngine::getInstance()->unlock();
		selectedInstrumentChangedEvent();    // update all
		m_pLayerPreview->updateAll();
	}
	else {
		ERRORLOG( "[buttonClicked] unhandled button" );
	}
}



void InstrumentEditor::loadLayer()
{
	static QString lastUsedDir = QDir::homePath();

	Hydrogen *engine = Hydrogen::getInstance();
	MainForm *pMainForm = HydrogenApp::getInstance()->getMainForm();

	QFileDialog *fd = new QFileDialog( (QWidget*)pMainForm );
	fd->setFileMode( QFileDialog::ExistingFile );
	fd->setFilter( trUtf8("Audio files (*.wav *.WAV *.au *.AU *.aiff *.AIFF *.flac *.FLAC)") );
	fd->setWindowTitle( trUtf8("Hydrogen - Load instrument") );
	fd->setDirectory( lastUsedDir );

//	FilePreview *pPreview = new FilePreview();
//	fd->setContentsPreviewEnabled( TRUE );
//	fd->setContentsPreview( pPreview, pPreview );
//	fd->setPreviewMode( QFileDialog::Contents );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().front();
	}

	if (filename != "") {
		lastUsedDir = fd->directory().absolutePath();
		Sample *newSample = Sample::load( filename.toStdString() );

		H2Core::Instrument *pInstr = NULL;

		AudioEngine::getInstance()->lock( "InstrumentPropertiesDialog::browseBtnClicked" );
		Song *song = engine->getSong();
		InstrumentList *instrList = song->getInstrumentList();
		pInstr = instrList->get( engine->getSelectedInstrumentNumber() );

		H2Core::InstrumentLayer *pLayer = pInstr->getLayer( m_nSelectedLayer );
		if (pLayer != NULL) {
			// delete old sample
			Sample *oldSample = pLayer->m_pSample;
			delete oldSample;

			// insert new sample from newInstrument
			pLayer->m_pSample = newSample;
		}
		else {
			pLayer = new H2Core::InstrumentLayer(newSample);
			pInstr->setLayer( pLayer, m_nSelectedLayer );
		}

		pInstr->m_sDrumkitName = "";   // external sample, no drumkit info

		AudioEngine::getInstance()->unlock();
	}

	selectedInstrumentChangedEvent();    // update all
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::labelClicked( ClickableLabel* pRef )
{
	UNUSED( pRef );

	if (m_pInstrument) {
		QString sOldName = QString( m_pInstrument->m_sName.c_str() );
		bool bIsOkPressed;
		QString sNewName = QInputDialog::getText( this, "Hydrogen", trUtf8( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
		if ( bIsOkPressed  ) {
			m_pInstrument->m_sName = sNewName.toStdString();
			selectedInstrumentChangedEvent();

			// this will force an update...
			EventQueue::getInstance()->pushEvent( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

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

	H2Core::InstrumentLayer *pLayer = m_pInstrument->getLayer( nLayer );
	m_pWaveDisplay->updateDisplay( pLayer );
	if (pLayer) {
		char tmp[20];

		// Layer GAIN
		m_pLayerGainRotary->setValue( pLayer->m_fGain / 5.0 );
		sprintf( tmp, "%#.2f", pLayer->m_fGain );
		m_pLayerGainLCD->setText( tmp );

		// Layer PITCH
		//int nCoarsePitch = pLayer->m_fPitch / 24 + 0.5;
		int nCoarsePitch = (int)pLayer->m_fPitch;
		float fFinePitch = pLayer->m_fPitch - nCoarsePitch;
		//INFOLOG( "fine pitch: " + toString( fFinePitch ) );
		m_pLayerPitchCoarseRotary->setValue( nCoarsePitch );
		m_pLayerPitchFineRotary->setValue( fFinePitch );

		m_pLayerPitchCoarseLCD->setText( QString( "%1" ).arg( nCoarsePitch ) );
		m_pLayerPitchFineLCD->setText( QString( "%1" ).arg( fFinePitch ) );
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
		m_pInstrument->m_nMuteGroup++;
	}
	else if (pRef == m_pDelMuteGroupBtn ) {
		m_pInstrument->m_nMuteGroup--;
	}

	selectedInstrumentChangedEvent();	// force an update
}

