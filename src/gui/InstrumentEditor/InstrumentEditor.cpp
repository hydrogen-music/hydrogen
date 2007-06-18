/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: InstrumentEditor.cpp,v 1.25 2005/06/04 21:29:05 comix Exp $
 *
 */

#include <qlineedit.h>
#include <qlistbox.h>
#include <qfiledialog.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qinputdialog.h>
#include <qgroupbox.h>
#include <math.h>

#include "config.h"
#include "lib/Song.h"
#include "lib/Hydrogen.h"
#include "lib/Globals.h"
#include "lib/ADSR.h"
#include "lib/Sample.h"
#include "gui/PatternEditor/PatternEditorPanel.h"
#include "gui/FilePreview.h"
#include "gui/HydrogenApp.h"
#include "gui/Skin.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/ClickableLabel.h"
#include "gui/widgets/Button.h"
#include "gui/widgets/LCD.h"
#include "gui/widgets/Fader.h"
#include "InstrumentEditor.h"
#include "WaveDisplay.h"
#include "LayerPreview.h"

// workaround for gcc 2.96
#if __GNUC__ < 3
inline int round(double x) { return x > 0 ? (int) (x+0.5) : -(int)(-x+0.5); }
#endif


InstrumentEditor::InstrumentEditor( QWidget* pParent )
 : QWidget( pParent, "InstrumentEditor", Qt::WStyle_DialogBorder )
 , Object( "InstrumentEditor" )
 , m_pInstrument( NULL )
 , m_nSelectedLayer( 0 )
{
	//infoLog( "INIT" );
	resize( 260, 390 );
	setMinimumSize( width(), height() );	// not resizable
	setMaximumSize( width(), height() );	// not resizable
	setCaption( trUtf8( "Instrument editor" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	setPaletteBackgroundColor( QColor( 58, 62, 72 ) );


// Instrument properties
	m_pInstrumentProp = new QWidget( this );
	m_pInstrumentProp->resize( width(), height() );
	m_pInstrumentProp->move( 0, 0 );

	string sInstrumentTabPath = Skin::getImagePath() + string( "/instrumentEditor/instrumentTab.png" );
	QPixmap instrumentTabPixmap;
	if( !instrumentTabPixmap.load( sInstrumentTabPath.c_str() ) ){
		errorLog( "Error loading pixmap " + sInstrumentTabPath );
	}
	m_pInstrumentProp->setPaletteBackgroundPixmap( instrumentTabPixmap );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp );
	m_pNameLbl->setGeometry( 14, 42, 232, 25 );
	m_pNameLbl->setPaletteForegroundColor( QColor( 230, 230, 230 ) );
	m_pNameLbl->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );
	m_pNameLbl->setAlignment( Qt::AlignCenter );
	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );
	connect( m_pNameLbl, SIGNAL( labelClicked(ClickableLabel*) ), this, SLOT( labelClicked(ClickableLabel*) ) );

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Random pitch factor" ) );
	m_pRandomPitchRotary->move( 102, 229 );
	connect( m_pRandomPitchRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	// Filter
	string sFilterOn = Skin::getImagePath() + string( "/instrumentEditor/bypass_on.png" );
	string sFilterOff = Skin::getImagePath() + string( "/instrumentEditor/bypass_off.png" );
	string sFilterOver = Skin::getImagePath() + string( "/instrumentEditor/bypass_over.png" );
	m_pFilterBypassBtn = new ToggleButton( m_pInstrumentProp, QSize( 30, 13 ), sFilterOn, sFilterOff, sFilterOver );
	connect( m_pFilterBypassBtn, SIGNAL( clicked(Button*) ), this, SLOT( filterActiveBtnClicked(Button*) ) );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Filter Cutoff" ) );
	connect( m_pCutoffRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Filter resonance" ) );
	connect( m_pResonanceRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pFilterBypassBtn->move( 55, 189 );
	m_pCutoffRotary->move( 102, 183 );
	m_pResonanceRotary->move( 155, 183 );
	//~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Attack" ) );
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Decay" ) );
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Sustain" ) );
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Release" ) );
	connect( m_pAttackRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pDecayRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pSustainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	connect( m_pReleaseRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	m_pAttackRotary->move( 38, 89 );
	m_pDecayRotary->move( 90, 89 );
	m_pSustainRotary->move( 142, 89 );
	m_pReleaseRotary->move( 194, 89 );
	//~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, LCDDigit::SMALL_BLUE, 4 );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::TYPE_NORMAL, trUtf8( "Instrument gain" ), false );
	connect( m_pInstrumentGain, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );
	m_pInstrumentGainLCD->move( 52, 142 );
	m_pInstrumentGain->move( 102, 137 );

//~ Instrument properties





// LAYER properties
	m_pLayerProp = new QWidget( this );
	m_pLayerProp->resize( width(), height() );
	m_pLayerProp->move( 0, 0 );
	m_pLayerProp->hide();

	string sLayerTabPath = Skin::getImagePath() + string( "/instrumentEditor/layerTab.png" );
	QPixmap layerTabPixmap;
	if( !layerTabPixmap.load( sLayerTabPath.c_str() ) ){
		errorLog( "Error loading pixmap " + sLayerTabPath );
	}
	m_pLayerProp->setPaletteBackgroundPixmap( layerTabPixmap );

	// Layer preview
	m_pLayerPreview = new LayerPreview( m_pLayerProp );
	m_pLayerPreview->move( 13, 41 );


	// Waveform display
	m_pWaveDisplay = new WaveDisplay( m_pLayerProp );
	m_pWaveDisplay->updateDisplay( NULL );
	m_pWaveDisplay->move( 12, 238 );

	string sLoadLayer_on = Skin::getImagePath() + string( "/instrumentEditor/loadLayer_on.png" );
	string sLoadLayer_off = Skin::getImagePath() + string( "/instrumentEditor/loadLayer_off.png" );
	string sLoadLayer_over = Skin::getImagePath() + string( "/instrumentEditor/loadLayer_over.png" );
	m_pLoadLayerBtn = new Button( m_pLayerProp, QSize( 94, 13 ), sLoadLayer_on, sLoadLayer_off, sLoadLayer_over );

	string sRemoveLayer_on = Skin::getImagePath() + string( "/instrumentEditor/deleteLayer_on.png" );
	string sRemoveLayer_off = Skin::getImagePath() + string( "/instrumentEditor/deleteLayer_off.png" );
	string sRemoveLayer_over = Skin::getImagePath() + string( "/instrumentEditor/deleteLayer_over.png" );
	m_pRemoveLayerBtn = new Button( m_pLayerProp, QSize( 94, 13 ), sRemoveLayer_on, sRemoveLayer_off, sRemoveLayer_over );

	m_pLoadLayerBtn->move( 34, 304 );
	m_pRemoveLayerBtn->move( 131, 304 );

	connect( m_pLoadLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	connect( m_pRemoveLayerBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );

	// Layer gain
	m_pLayerGainLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerGainRotary = new Rotary( m_pLayerProp,  Rotary::TYPE_NORMAL, trUtf8( "Layer gain" ), false );
	connect( m_pLayerGainRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerPitchLCD = new LCDDisplay( m_pLayerProp, LCDDigit::SMALL_BLUE, 4 );
	m_pLayerPitchRotary = new Rotary( m_pLayerProp, Rotary::TYPE_CENTER, trUtf8( "Layer pitch" ), false );
	connect( m_pLayerPitchRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pLayerGainLCD->move( 40, 338 + 3 );
	m_pLayerGainRotary->move( 88, 338 );

	m_pLayerPitchLCD->move( 137, 338 + 3 );
	m_pLayerPitchRotary->move( 185, 338 );
//~ Layer properties


	string sInstrumentBtn_on = Skin::getImagePath() + string( "/instrumentEditor/instrument_on.png" );
	string sInstrumentBtn_off = Skin::getImagePath() + string( "/instrumentEditor/instrument_off.png" );
	string sInstrumentBtn_over = Skin::getImagePath() + string( "/instrumentEditor/instrument_off.png" );
	m_pShowInstrumentBtn = new ToggleButton( this, QSize( 96, 17 ), sInstrumentBtn_on, sInstrumentBtn_off, sInstrumentBtn_over );
	QToolTip::add( m_pShowInstrumentBtn, trUtf8( "Show instrument properties" ) );
	connect( m_pShowInstrumentBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowInstrumentBtn->move( 29, 13 );
	m_pShowInstrumentBtn->setPressed( true );

	string sLayerBtn_on = Skin::getImagePath() + string( "/instrumentEditor/layers_on.png" );
	string sLayerBtn_off = Skin::getImagePath() + string( "/instrumentEditor/layers_off.png" );
	string sLayerBtn_over = Skin::getImagePath() + string( "/instrumentEditor/layers_off.png" );
	m_pShowLayersBtn = new ToggleButton( this, QSize( 96, 17 ), sLayerBtn_on, sLayerBtn_off, sLayerBtn_over );
	QToolTip::add( m_pShowLayersBtn, trUtf8( "Show layers properties" ) );
	connect( m_pShowLayersBtn, SIGNAL( clicked(Button*) ), this, SLOT( buttonClicked(Button*) ) );
	m_pShowLayersBtn->move( 125, 13 );

	selectLayer( m_nSelectedLayer );

	HydrogenApp::getInstance()->addEventListener(this);

	selectedInstrumentChangedEvent(); 	// force an update
}



InstrumentEditor::~InstrumentEditor()
{
	//infoLog( "DESTROY" );
}



void InstrumentEditor::selectedInstrumentChangedEvent()
{
	Hydrogen::getInstance()->lockEngine("InstrumentEditor::selectedInstrumentChanged" );

	Song *pSong = Hydrogen::getInstance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->getInstrumentList();
		int nInstr = Hydrogen::getInstance()->getSelectedInstrumentNumber();
		if (nInstr == -1) {
			m_pInstrument = NULL;
		}
		else {
			m_pInstrument = pInstrList->get( nInstr );
		}
	}
	else {
		m_pInstrument = NULL;
	}
	Hydrogen::getInstance()->unlockEngine();

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

		// select the last valid layer
		for (int i = MAX_LAYERS - 1; i >= 0; i-- ) {
			if ( m_pInstrument->getLayer( i ) ) {
				m_nSelectedLayer = i;
				break;
			}
		}
	}
	else {
		m_nSelectedLayer = 0;
	}
	m_pWaveDisplay->updateDisplay( m_pInstrument->getLayer( m_nSelectedLayer ) );
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

			InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				pLayer->m_fGain = fVal;
				m_pWaveDisplay->updateDisplay( pLayer );
			}
		}
		else if ( ref == m_pLayerPitchRotary ) {
			fVal = fVal * 24.0 - 12.0;
			char tmp[20];
			sprintf( tmp, "%#.2f", fVal );
			m_pLayerPitchLCD->setText( tmp );
			InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				pLayer->m_fPitch = fVal;
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
			errorLog( "[rotaryChanged] unhandled rotary" );
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
	}
	else if ( pButton == m_pShowLayersBtn ) {
		m_pShowLayersBtn->setPressed( true );
		m_pShowInstrumentBtn->setPressed( false );
		m_pLayerProp->show();
		m_pInstrumentProp->hide();
	}
	else if ( pButton == m_pLoadLayerBtn ) {
		loadLayer();
	}
	else if ( pButton == m_pRemoveLayerBtn ) {
		Hydrogen *pEngine = Hydrogen::getInstance();
		pEngine->lockEngine( "InstrumentPropertiesDialog::deleteBtnClicked" );

		if ( m_pInstrument ) {
			InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				m_pInstrument->setLayer( NULL, m_nSelectedLayer );
				delete pLayer;
			}
		}
		pEngine->unlockEngine();
		selectedInstrumentChangedEvent();    // update all
		m_pLayerPreview->updateAll();
	}
	else {
		errorLog( "[buttonClicked] unhandled button" );
	}
}



void InstrumentEditor::loadLayer()
{
	static QString lastUsedDir = "";

	Hydrogen *engine = Hydrogen::getInstance();
	MainForm *pMainForm = HydrogenApp::getInstance()->getMainForm();

	QFileDialog *fd = new QFileDialog( (QWidget*)pMainForm, "File Dialog", TRUE);
	fd->setMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Audio files (*.wav *.WAV *.au *.AU *.aiff *.AIFF *.flac *.FLAC)") );
	fd->setCaption( trUtf8("Hydrogen - Load instrument") );
	fd->setDir( lastUsedDir );

	FilePreview *pPreview = new FilePreview();
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( pPreview, pPreview );
	fd->setPreviewMode( QFileDialog::Contents );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}

	if (filename != "") {
		lastUsedDir = fd->dirPath();
		Sample *newSample = Sample::load( filename.latin1() );

		Instrument *pInstr = NULL;

		engine->lockEngine("InstrumentPropertiesDialog::browseBtnClicked");
		Song *song = engine->getSong();
		InstrumentList *instrList = song->getInstrumentList();
		pInstr = instrList->get( engine->getSelectedInstrumentNumber() );

		InstrumentLayer *pLayer = pInstr->getLayer( m_nSelectedLayer );
		if (pLayer != NULL) {
			// delete old sample
			Sample *oldSample = pLayer->m_pSample;
			delete oldSample;

			// insert new sample from newInstrument
			pLayer->m_pSample = newSample;
		}
		else {
			pLayer = new InstrumentLayer(newSample);
			pInstr->setLayer( pLayer, m_nSelectedLayer );
		}

		pInstr->m_sDrumkitName = "";   // external sample, no drumkit info

		engine->unlockEngine();
	}

	selectedInstrumentChangedEvent();    // update all
	m_pLayerPreview->updateAll();
}


void InstrumentEditor::labelClicked( ClickableLabel* pRef )
{
	if (m_pInstrument) {
		QString sOldName = QString( m_pInstrument->m_sName.c_str() );
		bool bIsOkPressed;
		QString sNewName = QInputDialog::getText( "Hydrogen", trUtf8( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed, this );
		if ( bIsOkPressed  ) {
			m_pInstrument->m_sName = string( sNewName.latin1() );
			selectedInstrumentChangedEvent();
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

	InstrumentLayer *pLayer = m_pInstrument->getLayer( nLayer );
	m_pWaveDisplay->updateDisplay( pLayer );
	if (pLayer) {
		char tmp[20];

		// Layer GAIN
		m_pLayerGainRotary->setValue( pLayer->m_fGain / 5.0 );
		sprintf( tmp, "%#.2f", pLayer->m_fGain );
		m_pLayerGainLCD->setText( tmp );

		// Layer PITCH
		m_pLayerPitchRotary->setValue( pLayer->m_fPitch / 24 + 0.5 );
		sprintf( tmp, "%#.2f", pLayer->m_fPitch );
		m_pLayerPitchLCD->setText( tmp );
	}
	else {
		// Layer GAIN
		m_pLayerGainRotary->setValue( 1.0 );
		m_pLayerGainLCD->setText( "" );

		// Layer PITCH
		m_pLayerPitchRotary->setValue( 0.5 );
		m_pLayerPitchLCD->setText( "" );
	}
}



