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
 * $Id: AudioEngineInfoForm.cpp,v 1.14 2005/05/09 18:10:52 comix Exp $
 *
 */


#include "AudioEngineInfoForm.h"

#include "config.h"
#include "qwidget.h"

#include "HydrogenApp.h"

#include "lib/Preferences.h"
#include "lib/Hydrogen.h"
#include "lib/drivers/MidiDriver.h"
#include "lib/drivers/GenericDriver.h"
#include "Skin.h"

AudioEngineInfoForm::AudioEngineInfoForm(QWidget* parent)
 : AudioEngineInfoForm_UI( parent )
 , Object( "AudioEngineInfoForm" )
{
	setMinimumSize( width(), height() );	// not resizable
	setMaximumSize( width(), height() );	// not resizable

	setCaption( trUtf8( "Audio Engine Info" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	updateInfo();
	//currentPatternLbl->setText("NULL pattern");

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateInfo()));

	HydrogenApp::getInstance()->addEventListener( this );
	updateAudioEngineState();
}



/**
 * Destructor
 */
AudioEngineInfoForm::~AudioEngineInfoForm() {
}




/**
 * show event
 */
void AudioEngineInfoForm::showEvent ( QShowEvent *ev ) {
	updateInfo();
	timer->start(200);
}




/**
 * hide event
 */
void AudioEngineInfoForm::hideEvent ( QHideEvent *ev ) {
	timer->stop();
}




void AudioEngineInfoForm::updateInfo() {
	Hydrogen *pEngine = Hydrogen::getInstance();
	Song *song = pEngine->getSong();

	// Song position
	QString sSongPos = "N/A";
	if ( pEngine->getPatternPos() != -1 ) {
		sSongPos = QString::number( pEngine->getPatternPos() );
	}
	m_pSongPositionLbl->setText( sSongPos );


	// Audio engine Playing notes
	char tmp[100];
	sprintf(tmp, "%03d / %03d", pEngine->getPlayingNotes(), Preferences::getInstance()->m_nMaxNotes );
	playingNotesLbl->setText( QString(tmp) );

	// Process time
	int perc = 0;
	if ( pEngine->getMaxProcessTime() != 0.0 ) {
		perc= (int)( pEngine->getProcessTime() / ( pEngine->getMaxProcessTime() / 100.0 ) );
	}
	sprintf(tmp, "%#.2f / %#.2f  (%d%%)", pEngine->getProcessTime(), pEngine->getMaxProcessTime(), perc );
	processTimeLbl->setText(tmp);

	// Song state
	if (song == NULL) {
		songStateLbl->setText( "NULL song" );
	}
	else {
		if (song->m_bIsModified) {
			songStateLbl->setText( "Modified" );
		}
		else {
			songStateLbl->setText( "Saved" );
		}
	}

	// Number of frames
	sprintf(tmp, "%d", (int)pEngine->getTotalFrames() );
	nFramesLbl->setText(tmp);

	// tick number
	sprintf(tmp, "%03d", (int)pEngine->getTickPosition() );
	nTicksLbl->setText(tmp);



	// Audio driver info
	GenericDriver *driver = pEngine->getAudioDriver();
	if (driver) {
		QString audioDriverName  = (driver->getClassName()).c_str();
		driverLbl->setText(audioDriverName);

		// Audio driver buffer size
		sprintf(tmp, "%d", driver->getBufferSize());
		bufferSizeLbl->setText(QString(tmp));

		// Audio driver sampleRate
		sprintf(tmp, "%d", driver->getSampleRate());
		sampleRateLbl->setText(QString(tmp));
	}
	else {
		driverLbl->setText( "NULL driver" );
		bufferSizeLbl->setText( "N/A" );
		sampleRateLbl->setText( "N/A" );
	}


	// Midi driver info
	MidiDriver *pMidiDriver = pEngine->getMidiDriver();
	if (pMidiDriver) {
		QString midiName( pMidiDriver->getClassName().c_str() );
		midiDriverName->setText(midiName);
	}
	else {
		midiDriverName->setText("No MIDI driver support");
	}

	m_pMidiDeviceName->setText( Preferences::getInstance()->m_sMidiPortName.c_str() );


	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if (nSelectedPatternNumber == -1) {
		selectedPatLbl->setText( "N/A");
	}
	else {
		selectedPatLbl->setText( QString("%1").arg(nSelectedPatternNumber) );
	}

	int nSelectedInstrumentNumber = pEngine->getSelectedInstrumentNumber();
	if (nSelectedInstrumentNumber == -1) {
		m_pSelectedInstrLbl->setText( "N/A" );
	}
	else {
		m_pSelectedInstrLbl->setText( QString("%1").arg(nSelectedInstrumentNumber) );
	}


	string currentPatternName;
	PatternList *pPatternList = (Hydrogen::getInstance())->getCurrentPatternList();
	if (pPatternList) {
		currentPatternLbl->setText( QString::number(pPatternList->getSize()) );
	}
	else {
		currentPatternLbl->setText( "N/A" );
	}

}






/**
 * Update engineStateLbl with the current audio engine state
 */
void AudioEngineInfoForm::updateAudioEngineState() {
	// Audio Engine state
	QString stateTxt;
	int state = (Hydrogen::getInstance())->getState();
	switch (state) {
	case STATE_UNINITIALIZED:
		stateTxt = "Uninitialized";
		break;

	case STATE_INITIALIZED:
		stateTxt = "Initialized";
		break;

	case STATE_PREPARED:
		stateTxt = "Prepared";
		break;

	case STATE_READY:
		stateTxt = "Ready";
		break;

	case STATE_PLAYING:
		stateTxt = "Playing";
		break;

	default:
		stateTxt = "Unknown!?";
		break;
	}
	engineStateLbl->setText(stateTxt);
}


void AudioEngineInfoForm::stateChangedEvent( int nState )
{
	updateAudioEngineState();
}


void AudioEngineInfoForm::patternChangedEvent()
{
	updateAudioEngineState();
}

