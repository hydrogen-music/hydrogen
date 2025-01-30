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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Song.h>
#include <core/FX/Effects.h>
#include <core/Preferences/Preferences.h>
#include <core/IO/AudioOutput.h>


#include "LadspaFXProperties.h"
#include "HydrogenApp.h"
#include "LadspaFXSelector.h"
#include "Skin.h"
#include "Widgets/Fader.h"
#include "Widgets/LCDDisplay.h"

#include "Mixer/Mixer.h"
#include "Mixer/MixerLine.h"

using namespace H2Core;

LadspaFXProperties::LadspaFXProperties(QWidget* parent, uint nLadspaFX)
 : QWidget( parent )
 , Object()
{
//	

	m_nLadspaFX = nLadspaFX;

	resize( 500, 200 );
	setMinimumSize( width(), height() );
	setFixedHeight( height() );

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );
	setLayout( hbox );


	// Background image
	QPixmap background;
	bool ok = background.load( Skin::getImagePath() + "/mixerPanel/mixer_background.png" );
	if( !ok ){
		ERRORLOG( "Error loading pixmap" );
	}


	m_pScrollArea = new QScrollArea( nullptr );
	hbox->addWidget( m_pScrollArea );

	m_pScrollArea->move( 0, 0 );
	m_pScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pScrollArea->resize( width(), height() );

	m_pFrame = new QFrame( this );
	m_pFrame->resize( width(), height() );

	m_pScrollArea->setWidget( m_pFrame );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->move( 10, 10 );
	m_pNameLbl->resize( 270, 24 );

	QFont boldFont;
	boldFont.setBold(true);
	m_pNameLbl->setFont( boldFont );

	m_pSelectFXBtn = new QPushButton( tr("Select FX"), this);
	m_pSelectFXBtn->move( 170, 10 );
	m_pSelectFXBtn->resize( 100, 24 );
	connect( m_pSelectFXBtn, SIGNAL(clicked()), this, SLOT(selectFXBtnClicked()) );

	m_pRemoveFXBtn = new QPushButton( tr("Remove FX"), this);
	m_pRemoveFXBtn->move( 280, 10 );
	m_pRemoveFXBtn->resize( 100, 24 );
	connect( m_pRemoveFXBtn, SIGNAL(clicked()), this, SLOT(removeFXBtnClicked()) );


	m_pActivateBtn = new QPushButton( tr("Activate"), this);
	m_pActivateBtn->move( 390, 10 );
	m_pActivateBtn->resize( 100, 24 );
	connect( m_pActivateBtn, SIGNAL(clicked()), this, SLOT(activateBtnClicked()) );


	m_pTimer = new QTimer( this );
	connect(m_pTimer, SIGNAL( timeout() ), this, SLOT( updateOutputControls() ) );
}


LadspaFXProperties::~LadspaFXProperties()
{
//	INFOLOG( "DESTROY" );
}



void LadspaFXProperties::showEvent ( QShowEvent* )
{
	updateControls();
}



void LadspaFXProperties::closeEvent( QCloseEvent *ev )
{
	ev->accept();
}


void LadspaFXProperties::faderChanged( WidgetWithInput * pRef )
{
	Fader* pFader = dynamic_cast<Fader*>( pRef );
	pFader->setPeak_L( pFader->getValue() );
	pFader->setPeak_R( pFader->getValue() );

#ifdef H2CORE_HAVE_LADSPA
	LadspaFX *pFX = Effects::get_instance()->getLadspaFX( m_nLadspaFX );

	for ( uint i = 0; i < m_pInputControlFaders.size(); i++ ) {
		if (pFader == m_pInputControlFaders[ i ] ) {
			LadspaControlPort *pControl = pFX->inputControlPorts[ i ];

			pControl->fControlValue = pFader->getValue();
			//float fInterval = pControl->fUpperBound - pControl->fLowerBound;
			//pControl->fControlValue = pControl->fLowerBound + fValue * fInterval;

			QString sValue;
			if (pControl->fControlValue < 1.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 2 );
			}
			else if ( pControl->fControlValue < 100.0 ) {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 1 );
			}
			else {
				sValue = QString("%1").arg( pControl->fControlValue, 0, 'f', 0 );
			}
			m_pInputControlLabel[ i ]->setText( sValue );
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
#endif
}



void LadspaFXProperties::updateControls()
{
#ifdef H2CORE_HAVE_LADSPA
	INFOLOG( "*** [updateControls] ***" );
	m_pTimer->stop();

	LadspaFX *pFX = Effects::get_instance()->getLadspaFX( m_nLadspaFX );

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
		QString sPluginName = pFX->getPluginLabel();
		setWindowTitle( tr( "[%1] LADSPA FX Properties" ).arg( sPluginName ) );

		int nControlsFrameWidth = 10 + 45 * (pFX->inputControlPorts.size() + pFX->outputControlPorts.size()) + 10 + 45;
		if ( nControlsFrameWidth < width() ) {
			nControlsFrameWidth = width();
		}
		m_pFrame->resize( nControlsFrameWidth, height() );

		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( tr("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( tr("Activate") );
		}

		QString mixerline_text_path = Skin::getImagePath() + "/mixerPanel/mixer_background.png";
		QPixmap textBackground;
		if( textBackground.load( mixerline_text_path ) == false ){
			ERRORLOG( "Error loading pixmap"  );
		}

		// input controls
		uint nInputControl_X = 0;
		for (uint i = 0; i < pFX->inputControlPorts.size(); i++) {
			LadspaControlPort *pControlPort = pFX->inputControlPorts[ i ];

			nInputControl_X = 10 + 45 * i;

			if (pControlPort->isToggle){	// toggle button
				WARNINGLOG( "[updateControls] LADSPA toggle controls not implemented yet");
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

			LCDDisplay *pLCD = new LCDDisplay( m_pFrame, QSize( 38, 18 ), false, false );
			pLCD->move( nInputControl_X, 40 );
			pLCD->setText( sValue );
			pLCD->show();
			QPalette lcdPalette;
			lcdPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
			pLCD->setPalette( lcdPalette );

			m_pInputControlLabel.push_back( pLCD );

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( nInputControl_X, 60 );
			pName->show();
			pName->setText( pControlPort->sName );
			m_pInputControlNames.push_back( pName );
			pName->setToolTip( pName->text() );


			// fader
			Fader *pFader = new Fader( m_pFrame, Fader::Type::Normal, tr( "Input control param. value" ), pControlPort->m_bIsInteger, false, pControlPort->fLowerBound, pControlPort->fUpperBound );
			connect( pFader, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( faderChanged( WidgetWithInput* ) ) );
			m_pInputControlFaders.push_back( pFader );
			pFader->move( nInputControl_X + 20, 60 );
			pFader->show();
			pFader->setMaxPeak( pControlPort->fUpperBound );
			pFader->setMinPeak( pControlPort->fLowerBound );
			pFader->setValue( pControlPort->fControlValue );
			pFader->setPeak_L( pControlPort->fControlValue );
			pFader->setPeak_R( pControlPort->fControlValue );
			pFader->setDefaultValue( pControlPort->fDefaultValue );

			faderChanged( pFader );

			m_pNameLbl->setText( pFX->getPluginName() );
		}

		nInputControl_X += 45;
		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[ i ];

			uint xPos = nInputControl_X + 10 + 45 * i;

			InstrumentNameWidget *pName = new InstrumentNameWidget( m_pFrame );
			pName->move( xPos, 60 );
			pName->show();
			pName->setText( pControl->sName );
			m_pInputControlNames.push_back( pName );
			pName->setToolTip( pName->text() );

			// fader
			Fader *pFader = new Fader( m_pFrame, Fader::Type::Normal, tr( "Output control param. value" ), true, true, pControl->fLowerBound, pControl->fUpperBound );
			pFader->move( xPos + 20, 60 );
			//float fInterval = pControl->fUpperBound - pControl->fLowerBound;
			//float fValue = pControl->fControlValue / fInterval;
			pFader->show();
			pFader->setMaxPeak( pControl->fUpperBound );
			pFader->setMinPeak( pControl->fLowerBound );
			pFader->setValue( pControl->fControlValue );
			pFader->setPeak_L( pControl->fControlValue );
			pFader->setPeak_R( pControl->fControlValue );
			pFader->setDefaultValue( pControl->fDefaultValue );

			m_pOutputControlFaders.push_back( pFader );
		}
	}
	else {
		INFOLOG( "NULL PLUGIN" );
		setWindowTitle( tr( "LADSPA FX %1 Properties" ).arg( m_nLadspaFX) );
		m_pNameLbl->setText( tr("No plugin") );
		m_pActivateBtn->setEnabled(false);
	}

	m_pTimer->start(100);
#endif
}



void LadspaFXProperties::selectFXBtnClicked()
{
#ifdef H2CORE_HAVE_LADSPA
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioOutput();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}

	LadspaFXSelector fxSelector(m_nLadspaFX);
	if (fxSelector.exec() == QDialog::Accepted) {
		QString sSelectedFX = fxSelector.getSelectedFX();
		if ( !sSelectedFX.isEmpty() ) {
			LadspaFX *pFX = nullptr;

			std::vector<H2Core::LadspaFXInfo*> pluginList = Effects::get_instance()->getPluginList();
			for (uint i = 0; i < pluginList.size(); i++) {
				H2Core::LadspaFXInfo *pFXInfo = pluginList[i];
				if (pFXInfo->m_sName == sSelectedFX ) {
					int nSampleRate = pAudioDriver->getSampleRate();
					pFX = LadspaFX::load( pFXInfo->m_sFilename, pFXInfo->m_sLabel, nSampleRate );
					pFX->setEnabled( true );
					break;
				}
			}
			Effects::get_instance()->setLadspaFX( pFX, m_nLadspaFX );

			pHydrogen->restartLadspaFX();
			updateControls();
		}
		else {	// no plugin selected
			INFOLOG( "no plugin selected" );
		}
	}
#endif
}


void LadspaFXProperties::removeFXBtnClicked()
{
#ifdef H2CORE_HAVE_LADSPA
	Hydrogen::get_instance()->setIsModified( true );
	Effects::get_instance()->setLadspaFX( nullptr, m_nLadspaFX );
	Hydrogen::get_instance()->restartLadspaFX();
	updateControls();	
#endif
}


void LadspaFXProperties::updateOutputControls()
{
#ifdef H2CORE_HAVE_LADSPA

	LadspaFX *pFX = Effects::get_instance()->getLadspaFX(m_nLadspaFX);

	if (pFX) {
		m_pActivateBtn->setEnabled(true);
		if (pFX->isEnabled()) {
			m_pActivateBtn->setText( tr("Deactivate") );
		}
		else {
			m_pActivateBtn->setText( tr("Activate") );
		}

		for (uint i = 0; i < pFX->outputControlPorts.size(); i++) {
			LadspaControlPort *pControl = pFX->outputControlPorts[i];

			std::vector<Fader*>::iterator it = m_pOutputControlFaders.begin() + i;
			if (it != m_pOutputControlFaders.end() ) {
				Fader *pFader = *it;
				if (pFader == nullptr) {
					ERRORLOG( "[updateOutputControls] pFader = NULL" );
					continue;
				}

				float fInterval = pControl->fUpperBound - pControl->fLowerBound;
				float fValue = pControl->fControlValue / fInterval;

				if (fValue < 0) fValue = -fValue;

				pFader->setPeak_L( fValue );
				pFader->setPeak_R( fValue );
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
#ifdef H2CORE_HAVE_LADSPA
	LadspaFX *pFX = Effects::get_instance()->getLadspaFX(m_nLadspaFX);
	if (pFX) {
		Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
		pFX->setEnabled( !pFX->isEnabled() );
		Hydrogen::get_instance()->getAudioEngine()->unlock();
	}
#endif
}
