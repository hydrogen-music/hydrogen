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
 * $Id: LadspaFXProperties.cpp,v 1.12 2005/07/05 15:18:33 comix Exp $
 *
 */

#include "config.h"

#include "LadspaFXProperties.h"
#include "HydrogenApp.h"
#include "LadspaFXSelector.h"
#include "Skin.h"

#include "qpixmap.h"
#include "qtimer.h"

#include "lib/Hydrogen.h"
#include "lib/Song.h"
#include "lib/fx/LadspaFX.h"
#include "lib/drivers/GenericDriver.h"

LadspaFXProperties::LadspaFXProperties(QWidget* parent, uint nLadspaFX)
 : QWidget( parent )
 , Object( "LadspaFXProp" )
{
//	infoLog( "INIT" );

	m_nLadspaFX = nLadspaFX;

	resize( 500, 200 );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	// Background image
	QPixmap background;
	string sBackground_path = Skin::getImagePath() + string( "/mixerPanel/mixer_background.png" );
	bool ok = background.load( sBackground_path.c_str() );
	if( !ok ){
		errorLog( "Error loading pixmap " + sBackground_path );
	}


	m_pScrollView = new QScrollView( this );
	m_pScrollView->move( 0, 0 );
	m_pScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pScrollView->setHScrollBarMode( QScrollView::AlwaysOn );
	m_pScrollView->resize( width(), height() );

	m_pFrame = new QFrame( m_pScrollView->viewport() );
	m_pFrame->resize( width(), height() );
	m_pFrame->setPaletteBackgroundPixmap( background );

	m_pScrollView->addChild( m_pFrame );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->setPaletteForegroundColor( QColor( 230, 230, 230 ) );
	m_pNameLbl->move( 10, 10 );
	m_pNameLbl->resize( 270, 24 );
	m_pNameLbl->setPaletteBackgroundPixmap( background );

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );

	m_pSelectFXBtn = new QPushButton( trUtf8("Select FX"), this);
	m_pSelectFXBtn->move( 280, 10 );
	m_pSelectFXBtn->resize( 100, 24 );
	connect( m_pSelectFXBtn, SIGNAL(clicked()), this, SLOT(selectFXBtnClicked()) );


	m_pActivateBtn = new QPushButton( trUtf8("Activate"), this);
	m_pActivateBtn->move( 390, 10 );
	m_pActivateBtn->resize( 100, 24 );
	connect( m_pActivateBtn, SIGNAL(clicked()), this, SLOT(activateBtnClicked()) );


	m_pTimer = new QTimer( this );
	connect(m_pTimer, SIGNAL( timeout() ), this, SLOT( updateOutputControls() ) );
}


LadspaFXProperties::~LadspaFXProperties()
{
//	infoLog( "DESTROY" );
}



void LadspaFXProperties::showEvent ( QShowEvent *ev )
{
//	infoLog( "[showEvent]" );
	updateControls();
}

void LadspaFXProperties::hideEvent ( QShowEvent *ev )
{
//	infoLog( "[hideEvent]" );
}



void LadspaFXProperties::closeEvent( QCloseEvent *ev )
{
//	infoLog( "[closeEvent]" );
	ev->accept();
}


void LadspaFXProperties::faderChanged(Fader * ref)
{
	ref->setPeak_L( ref->getValue() / 100.0 );
	ref->setPeak_R( ref->getValue() / 100.0 );

	Song *pSong = (Hydrogen::getInstance() )->getSong();

#ifdef LADSPA_SUPPORT
	LadspaFX *pFX = pSong->getLadspaFX( m_nLadspaFX );

	for (uint i = 0; i < m_pInputControlFaders.size(); i++) {
		if (ref == m_pInputControlFaders[ i ] ) {
			LadspaControlPort *pControl = pFX->inputControlPorts[ i ];

			float fValue = ref->getValue() / 100.0;
			float fInterval = pControl->fUpperBound - pControl->fLowerBound;

			pControl->fControlValue = pControl->fLowerBound + fValue * fInterval;

			QString sValue;
			if (pControl->fControlValue < 1.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 2);
			}
			else if ( pControl->fControlValue < 100.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 1);
			}
			else {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 0);
			}
			m_pInputControlLabel[ i ]->setText( sValue.ascii() );
		}
	}
	pSong->m_bIsModified = true;
#endif
}



void LadspaFXProperties::updateControls()
{
#ifdef LADSPA_SUPPORT
	infoLog( "[updateControls]" );
	m_pTimer->stop();

	Song *pSong = (Hydrogen::getInstance() )->getSong();
	LadspaFX *pFX = pSong->getLadspaFX(m_nLadspaFX);

	// svuoto i vettori..
	if ( m_pInputControlNames.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlNames.size(); i++) {
			delete m_pInputControlNames[ i ];
		}
		m_pInputControlNames.clear();
	}
	if ( m_pInputControlLabel.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlLabel.size(); i++) {
			delete m_pInputControlLabel[ i ];
		}
		m_pInputControlLabel.clear();
	}
	if ( m_pInputControlFaders.size() != 0 ) {
		for (uint i = 0; i < m_pInputControlFaders.size(); i++) {
			delete m_pInputControlFaders[ i ];
		}
		m_pInputControlFaders.clear();
	}

	if ( m_pOutputControlFaders.size() != 0 ) {
		for (uint i = 0; i < m_pOutputControlFaders.size(); i++) {
			delete m_pOutputControlFaders[ i ];
		}
		m_pOutputControlFaders.clear();
	}
	if ( m_pOutputControlNames.size() != 0 ) {
		for (uint i = 0; i < m_pOutputControlNames.size(); i++) {
			delete m_pOutputControlNames[ i ];
		}
		m_pOutputControlNames.clear();
	}

	if (pFX) {
		QString sPluginName = pFX->getPluginLabel().c_str();
		setCaption( trUtf8( "[%1] LADSPA FX Properties" ).arg( sPluginName ) );

		int nControlsFrameWidth = 10 + 45 * (pFX->inputControlPorts.size() + pFX->outputControlPorts.size()) + 10 + 45;
		if ( nControlsFrameWidth < width() ) {
			nControlsFrameWidth = width();
		}
		m_pFrame->resize( nControlsFrameWidth, height() );

		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( trUtf8("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( trUtf8("Activate") );
		}

		string mixerline_text_path = Skin::getImagePath() + string( "/mixerPanel/mixer_background.png");
		QPixmap textBackground;
		if( textBackground.load( mixerline_text_path.c_str() ) == false ){
			errorLog( string("Error loading pixmap ") + mixerline_text_path );
		}

		// input controls
		uint nInputControl_X = 0;
		for (uint i = 0; i < pFX->inputControlPorts.size(); i++) {
			LadspaControlPort *pControlPort = pFX->inputControlPorts[ i ];

			nInputControl_X = 10 + 45 * i;

			if (pControlPort->isToggle){	// toggle button
				warningLog( "[updateControls] LADSPA toggle controls not implemented yet");
			}

			// peak volume label
			QString sValue;
			if (pControlPort->fControlValue < 1.0 ) {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 2);
			}
			else if ( pControlPort->fControlValue < 100.0 ) {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 1);
			}
			else {
				sValue = QString("%1").arg( pControlPort->fControlValue, 0, 'f', 0);
			}

			LCDDisplay *pLCD = new LCDDisplay( m_pFrame, LCDDigit::SMALL_BLUE, 4 );
			pLCD->move( nInputControl_X, 40 );
			pLCD->setText( sValue.ascii() );
			pLCD->show();
			pLCD->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

			m_pInputControlLabel.push_back( pLCD );

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( nInputControl_X, 60 );
			pName->show();
			pName->setText( QString( pControlPort->sName.c_str() ) );
			m_pInputControlNames.push_back( pName );
			QToolTip::add( pName, pName->text() );


			// fader
			Fader *pFader = new Fader( m_pFrame );
			connect( pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );
			m_pInputControlFaders.push_back( pFader );
			pFader->move( nInputControl_X + 20, 60 );
			pFader->show();

			float fInterval = pControlPort->fUpperBound - pControlPort->fLowerBound;
			float fValue = ( pControlPort->fControlValue - pControlPort->fLowerBound ) / fInterval;
			pFader->setValue( fValue * 100.0 );
			pFader->setPeak_L( fValue );
			pFader->setPeak_R( fValue );

			faderChanged( pFader );

			m_pNameLbl->setText( QString(pFX->getPluginName().c_str()) );
		}

		nInputControl_X += 45;
		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[ i ];

			uint xPos = nInputControl_X + 10 + 45 * i;

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( xPos, 60 );
			pName->show();
			pName->setText( QString( pControl->sName.c_str() ) );
			m_pInputControlNames.push_back( pName );
			QToolTip::add( pName, pName->text() );

			// fader
			Fader *pFader = new Fader( m_pFrame, true );	// without knob!
			pFader->move( xPos + 20, 60 );
			float fInterval = pControl->fUpperBound - pControl->fLowerBound;
			float fValue = pControl->fControlValue / fInterval;
			pFader->setValue( (int)( fValue * 100.0 ) );
			pFader->setPeak_L( fValue );
			pFader->setPeak_R( fValue );
			pFader->show();

			m_pOutputControlFaders.push_back( pFader );
		}
	}
	else {
		setCaption( trUtf8( "LADSPA FX %1 Properties" ).arg( m_nLadspaFX) );
		m_pNameLbl->setText( trUtf8("No plugin") );
		m_pActivateBtn->setEnabled(false);
	}

	m_pTimer->start(100);
#endif
}



void LadspaFXProperties::selectFXBtnClicked()
{
#ifdef LADSPA_SUPPORT
	LadspaFXSelector fxSelector(m_nLadspaFX);
	if (fxSelector.exec() == QDialog::Accepted) {
		string sSelectedFX = fxSelector.getSelectedFX();
		if (sSelectedFX != "") {
			LadspaFX *pFX = NULL;

			vector<LadspaFXInfo*> pluginList = (HydrogenApp::getInstance())->getPluginList();
			for (uint i = 0; i < pluginList.size(); i++) {
				LadspaFXInfo *pFXInfo = pluginList[i];
				if (pFXInfo->sName == sSelectedFX ) {
					int nSampleRate = (Hydrogen::getInstance())->getAudioDriver()->getSampleRate();
					pFX = LadspaFX::load( pFXInfo->sFilename, pFXInfo->sLabel, nSampleRate );
					pFX->setEnabled( true );
					break;
				}
			}
			( Hydrogen::getInstance() )->lockEngine("LadspaFXProperties::selectFXBtnClicked");
			Song *pSong = (Hydrogen::getInstance() )->getSong();
			delete pSong->getLadspaFX(m_nLadspaFX);
			pSong->setLadspaFX( m_nLadspaFX, pFX );
			( Hydrogen::getInstance() )->unlockEngine();
			( Hydrogen::getInstance() )->restartLadspaFX();
			pSong->m_bIsModified = true;
			updateControls();
		}
		else {	// no plugin selected
//			infoLog( "no plugin selected" );
		}
	}
#endif
}



void LadspaFXProperties::updateOutputControls()
{
#ifdef LADSPA_SUPPORT

//	infoLog( "[updateOutputControls]" );
	Song *pSong = (Hydrogen::getInstance() )->getSong();
	LadspaFX *pFX = pSong->getLadspaFX(m_nLadspaFX);

	if (pFX) {
		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( trUtf8("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( trUtf8("Activate") );
		}

		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[i];

			vector<Fader*>::iterator it = m_pOutputControlFaders.begin() + i;
			if (it != m_pOutputControlFaders.end() ) {
				Fader *pFader = *it;
				if (pFader == NULL) {
					errorLog( "[updateOutputControls] pFader = NULL" );
					continue;
				}

				float fValue = pControl->fControlValue;
				float fInterval = pControl->fUpperBound - pControl->fLowerBound;
				fValue = pControl->fControlValue / fInterval;

				if (fValue < 0) fValue = -fValue;

				pFader->setPeak_L( fValue );
				pFader->setPeak_R( fValue );
				pFader->updateFader();
			}
		}
	}
	else {
		m_pActivateBtn->setEnabled(false);
	}
#endif
}




void LadspaFXProperties::activateBtnClicked()
{
#ifdef LADSPA_SUPPORT
	Song *pSong = (Hydrogen::getInstance() )->getSong();
	LadspaFX *pFX = pSong->getLadspaFX(m_nLadspaFX);
	if (pFX) {
		(Hydrogen::getInstance())->lockEngine("LadspaFXProperties::activateBtnClicked");
		pFX->setEnabled( !pFX->isEnabled() );
		(Hydrogen::getInstance())->unlockEngine();
	}
#endif
}


