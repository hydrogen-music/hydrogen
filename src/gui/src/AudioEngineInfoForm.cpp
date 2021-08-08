/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "AudioEngineInfoForm.h"

#include <QtGui>
#include <QtWidgets>


#include "HydrogenApp.h"

#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Preferences.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiInput.h>
#include <core/IO/AudioOutput.h>
#include <core/Sampler/Sampler.h>
#include <core/AudioEngine.h>
using namespace H2Core;

#include "Skin.h"


AudioEngineInfoForm::AudioEngineInfoForm(QWidget* parent)
 : QWidget( parent )
 , Object()
{
	setupUi( this );
	adjustSize();
	setFixedSize( width(), height() );	// not resizable

	setWindowTitle( tr( "Audio Engine Info" ) );

	updateInfo();

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateInfo()));

	HydrogenApp::get_instance()->addEventListener( this );
	updateAudioEngineState();
}



/**
 * Destructor
 */
AudioEngineInfoForm::~AudioEngineInfoForm()
{
}


/**
 * show event
 */
void AudioEngineInfoForm::showEvent ( QShowEvent* )
{
	updateInfo();
	m_pTimer->start(200);
}


/**
 * hide event
 */
void AudioEngineInfoForm::hideEvent ( QHideEvent* )
{
	m_pTimer->stop();
}


void AudioEngineInfoForm::updateInfo()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();;
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// Song position
	QString sSongPos = "N/A";
	if ( pHydrogen->getPatternPos() != -1 ) {
		sSongPos = QString::number( pHydrogen->getPatternPos() );
	}
	m_pSongPositionLbl->setText( sSongPos );


	// Audio engine Playing notes
	char tmp[100];

	// Process time
	int perc = 0;
	if ( pAudioEngine->getMaxProcessTime() != 0.0 ) {
		perc= (int)( pAudioEngine->getProcessTime() / ( pAudioEngine->getMaxProcessTime() / 100.0 ) );
	}
	sprintf(tmp, "%#.2f / %#.2f  (%d%%)", pAudioEngine->getProcessTime(), pAudioEngine->getMaxProcessTime(), perc );
	processTimeLbl->setText(tmp);

	// Song state
	if (pSong == nullptr) {
		songStateLbl->setText( "NULL song" );
	}
	else {
		if (pSong->getIsModified()) {
			songStateLbl->setText( "Modified" );
		}
		else {
			songStateLbl->setText( "Saved" );
		}
	}

	// tick number
	sprintf(tmp, "%03d", (int)pHydrogen->getTickPosition() );
	nTicksLbl->setText(tmp);



	// Audio driver info
	AudioOutput *driver = pHydrogen->getAudioOutput();
	if (driver) {
		QString audioDriverName = driver->class_name();
		driverLbl->setText(audioDriverName);

		// Audio driver buffer size
		sprintf(tmp, "%d", driver->getBufferSize());
		bufferSizeLbl->setText(QString(tmp));

		// Audio driver sampleRate
		sprintf(tmp, "%d", driver->getSampleRate());
		sampleRateLbl->setText(QString(tmp));

		// Number of frames
		sprintf(tmp, "%d", (int)driver->m_transport.m_nFrames );
		nFramesLbl->setText(tmp);
	}
	else {
		driverLbl->setText( "NULL driver" );
		bufferSizeLbl->setText( "N/A" );
		sampleRateLbl->setText( "N/A" );
		nFramesLbl->setText( "N/A" );
	}
	nRealtimeFramesLbl->setText( QString( "%1" ).arg( pHydrogen->getRealtimeFrames() ) );


	// Midi driver info
	MidiInput *pMidiDriver = pHydrogen->getMidiInput();
	if (pMidiDriver) {
		midiDriverName->setText( pMidiDriver->class_name() );
	}
	else {
		midiDriverName->setText("No MIDI driver support");
	}

	m_pMidiDeviceName->setText( Preferences::get_instance()->m_sMidiPortName );


	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if (nSelectedPatternNumber == -1) {
		selectedPatLbl->setText( "N/A");
	}
	else {
		selectedPatLbl->setText( QString("%1").arg(nSelectedPatternNumber) );
	}

	int nSelectedInstrumentNumber = pHydrogen->getSelectedInstrumentNumber();
	if (nSelectedInstrumentNumber == -1) {
		m_pSelectedInstrLbl->setText( "N/A" );
	}
	else {
		m_pSelectedInstrLbl->setText( QString("%1").arg(nSelectedInstrumentNumber) );
	}

	PatternList *pPatternList = Hydrogen::get_instance()->getCurrentPatternList();
	if (pPatternList) {
		currentPatternLbl->setText( QString::number(pPatternList->size()) );
	}
	else {
		currentPatternLbl->setText( "N/A" );
	}

	// SAMPLER
	Sampler *pSampler = pAudioEngine->getSampler();
	sampler_playingNotesLbl->setText(QString( "%1 / %2" ).arg(pSampler->getPlayingNotesNumber()).arg(Preferences::get_instance()->m_nMaxNotes));

	// Synth
	Synth *pSynth = pAudioEngine->getSynth();
	synth_playingNotesLbl->setText( QString( "%1" ).arg( pSynth->getPlayingNotesNumber() ) );
}






/**
 * Update engineStateLbl with the current audio engine state
 */
void AudioEngineInfoForm::updateAudioEngineState() {
	// Audio Engine state
	QString stateTxt;
	int state = Hydrogen::get_instance()->getState();
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


void AudioEngineInfoForm::stateChangedEvent( int )
{
	updateAudioEngineState();
}


void AudioEngineInfoForm::patternChangedEvent()
{
	updateAudioEngineState();
}


