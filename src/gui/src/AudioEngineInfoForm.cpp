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

#include "AudioEngineInfoForm.h"

#include <QtGui>
#include <QtWidgets>


#include "HydrogenApp.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Hydrogen.h>
#include <core/IO/AudioOutput.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>

using namespace H2Core;

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
	const auto pPref = Preferences::get_instance();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();;
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	// Song position
	QString sColumn = "N/A";
	if ( pAudioEngine->getTransportPosition()->getColumn() != -1 ) {
		sColumn = QString::number( pAudioEngine->getTransportPosition()->getColumn() );
	}
	m_pSongPositionLbl->setText( sColumn );


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
	sprintf(tmp, "%03d", (int)pAudioEngine->getTransportPosition()->getPatternTickPosition() );
	nTicksLbl->setText(tmp);



	// Audio driver info
	auto pAudioDriver = pHydrogen->getAudioOutput();
	if ( pAudioDriver != nullptr ) {
		QString audioDriverName = pAudioDriver->class_name();
		driverLbl->setText( audioDriverName );

		// Audio pAudioDriver buffer size
		sprintf( tmp, "%d", pAudioDriver->getBufferSize() );
		bufferSizeLbl->setText( QString( tmp ) );

		// Audio latency estimate
		latencyLbl->setText(
			QString( "%1 frames" ).arg( pAudioDriver->getLatency() )
		);

		// Audio pAudioDriver sampleRate
		sprintf( tmp, "%d", pAudioDriver->getSampleRate() );
		sampleRateLbl->setText( QString( tmp ) );

		// Number of frames
		sprintf(
			tmp, "%d",
			static_cast<int>( pAudioEngine->getTransportPosition()->getFrame() )
		);
		nFramesLbl->setText( tmp );
	}
	else {
		driverLbl->setText( "NULL pAudioDriver" );
		bufferSizeLbl->setText( "N/A" );
		latencyLbl->setText( "N/A" );
		sampleRateLbl->setText( "N/A" );
		nFramesLbl->setText( "N/A" );
	}
	nRealtimeFramesLbl->setText(
		QString( "%1" ).arg( pAudioEngine->getRealtimeFrame() )
	);

	// Midi driver info
	midiDriverName->setText(
		Preferences::midiDriverToQString( pPref->m_midiDriver ) );
	m_pMidiDeviceName->setText( pPref->m_sMidiPortName );

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

	auto pPatternList = pAudioEngine->getPlayingPatterns();
	if ( pPatternList != nullptr ) {
		currentPatternLbl->setText( QString::number( pPatternList->size() ) );
	} else {
		currentPatternLbl->setText( "N/A" );
	}

	// SAMPLER
	Sampler *pSampler = pAudioEngine->getSampler();
	sampler_playingNotesLbl->setText( QString( "%1 / %2" )
									  .arg( pSampler->getPlayingNotesNumber() )
									  .arg( pPref->m_nMaxNotes ) );
}






/**
 * Update engineStateLbl with the current audio engine state
 */
void AudioEngineInfoForm::updateAudioEngineState() {
	engineStateLbl->setText(
		AudioEngine::StateToQString(
			H2Core::Hydrogen::get_instance()->getAudioEngine()->getState() ) );
}


void AudioEngineInfoForm::stateChangedEvent( const H2Core::AudioEngine::State& state ) {
	updateAudioEngineState();
}


void AudioEngineInfoForm::playingPatternsChangedEvent()
{
	updateAudioEngineState();
}

void AudioEngineInfoForm::updateSongEvent( int nValue )
{
	// A new song got loaded
	if ( nValue == 0 ) {
		updateAudioEngineState();
	}
}


