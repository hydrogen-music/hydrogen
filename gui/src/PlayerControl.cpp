/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/audio_engine.h>
#include <hydrogen/IO/JackOutput.h>
#include <hydrogen/Preferences.h>
using namespace H2Core;

#include <QTimer>
#include <QPixmap>
#include <QPaintEvent>
#include <QInputDialog>
#include <QPainter>
#include <string>

#include "Skin.h"
#include "PlayerControl.h"
#include "InstrumentRack.h"

#include "Mixer/Mixer.h"
#include "SongEditor/SongEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"

using namespace H2Core;

PlayerControl::PlayerControl(QWidget *parent)
 : QLabel(parent)
 , Object( "PlayerControl" )
{
	// Background image
	setPixmap( QPixmap( Skin::getImagePath() + "/playerControlPanel/background.png" ) );
	setScaledContents( true );

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );
	setLayout( hbox );



// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( NULL );
	pControlsPanel->setFixedSize( 317, 43 );
	pControlsPanel->setPixmap( "/playerControlPanel/background_Control.png" );
	hbox->addWidget( pControlsPanel );

	m_pTimeDisplayH = new LCDDisplay( pControlsPanel, LCDDigit::LARGE_GRAY, 2 );
	m_pTimeDisplayH->move( 27, 12 );
	m_pTimeDisplayH->setText( "00" );

	m_pTimeDisplayM = new LCDDisplay( pControlsPanel, LCDDigit::LARGE_GRAY, 2 );
	m_pTimeDisplayM->move( 61, 12 );
	m_pTimeDisplayM->setText( "00" );

	m_pTimeDisplayS = new LCDDisplay( pControlsPanel, LCDDigit::LARGE_GRAY, 2 );
	m_pTimeDisplayS->move( 95, 12 );
	m_pTimeDisplayS->setText( "00" );

	m_pTimeDisplayMS = new LCDDisplay( pControlsPanel, LCDDigit::SMALL_GRAY, 3 );
	m_pTimeDisplayMS->move( 122, 16 );
	m_pTimeDisplayMS->setText( "000" );

	// Rewind button
	m_pRwdBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_rwd_on.png",
			"/playerControlPanel/btn_rwd_off.png",
			"/playerControlPanel/btn_rwd_over.png",
			QSize(21, 15)
	);
	m_pRwdBtn->move(168, 17);
	m_pRwdBtn->setToolTip( trUtf8("Rewind") );
	connect(m_pRwdBtn, SIGNAL(clicked(Button*)), this, SLOT(RewindBtnClicked(Button*)));

	// Play button
	m_pPlayBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_play_on.png",
			"/playerControlPanel/btn_play_off.png",
			"/playerControlPanel/btn_play_over.png",
			QSize(26, 17)
	);
	m_pPlayBtn->move(195, 17);
	m_pPlayBtn->setPressed(false);
	m_pPlayBtn->setToolTip( trUtf8("Play/ Pause") );
	connect(m_pPlayBtn, SIGNAL(clicked(Button*)), this, SLOT(playBtnClicked(Button*)));

	// Stop button
	m_pStopBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_stop_on.png",
			"/playerControlPanel/btn_stop_off.png",
			"/playerControlPanel/btn_stop_over.png",
			QSize(21, 15)
	);
	m_pStopBtn->move(227, 17);
	m_pStopBtn->setToolTip( trUtf8("Stop") );
	connect(m_pStopBtn, SIGNAL(clicked(Button*)), this, SLOT(stopBtnClicked(Button*)));

	// Fast forward button
	m_pFfwdBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_ffwd_on.png",
			"/playerControlPanel/btn_ffwd_off.png",
			"/playerControlPanel/btn_ffwd_over.png",
			QSize(21, 15)
	);
	m_pFfwdBtn->move(254, 17);
	m_pFfwdBtn->setToolTip( trUtf8("Fast Forward") );
	connect(m_pFfwdBtn, SIGNAL(clicked(Button*)), this, SLOT(FFWDBtnClicked(Button*)));

	// Loop song button button
	m_pSongLoopBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_loop_on.png",
			"/playerControlPanel/btn_loop_off.png",
			"/playerControlPanel/btn_loop_over.png",
			QSize(21, 15)
	);
	m_pSongLoopBtn->move(283, 17);
	m_pSongLoopBtn->setToolTip( trUtf8("Loop song") );
	connect( m_pSongLoopBtn, SIGNAL( clicked(Button*) ), this, SLOT( songLoopBtnClicked(Button*) ) );
//~ CONTROLS


// MODE
	PixmapWidget *pModePanel = new PixmapWidget( NULL );
	pModePanel->setFixedSize( 90, 43 );
	pModePanel->setPixmap( "/playerControlPanel/background_Mode.png" );
	hbox->addWidget( pModePanel );

	// Live mode button
	m_pLiveModeBtn = new ToggleButton(
			pModePanel,
			"/playerControlPanel/statusLED_on.png",
			"/playerControlPanel/statusLED_off.png",
			"/playerControlPanel/statusLED_off.png",
			QSize(11, 9)
	);
	m_pLiveModeBtn->move(10, 4);
	m_pLiveModeBtn->setPressed(true);
	m_pLiveModeBtn->setToolTip( trUtf8("Pattern Mode") );
	connect(m_pLiveModeBtn, SIGNAL(clicked(Button*)), this, SLOT(liveModeBtnClicked(Button*)));

	// Song mode button
	m_pSongModeBtn = new ToggleButton(
			pModePanel,
			"/playerControlPanel/statusLED_on.png",
			"/playerControlPanel/statusLED_off.png",
			"/playerControlPanel/statusLED_off.png",
			QSize(11, 9)
	);
	m_pSongModeBtn->move(10, 15);
	m_pSongModeBtn->setPressed(false);
	m_pSongModeBtn->setToolTip( trUtf8("Song Mode") );
	connect(m_pSongModeBtn, SIGNAL(clicked(Button*)), this, SLOT(songModeBtnClicked(Button*)));

	// Switch mode button
	m_pSwitchModeBtn = new Button(
			pModePanel,
			"/playerControlPanel/btn_mode_on.png",
			"/playerControlPanel/btn_mode_off.png",
			"/playerControlPanel/btn_mode_over.png",
			QSize(69, 13)
	);
	m_pSwitchModeBtn->move(10, 26);
	m_pSwitchModeBtn->setToolTip( trUtf8("Switch Song/ Pattern Mode") );
	connect(m_pSwitchModeBtn, SIGNAL(clicked(Button*)), this, SLOT(switchModeBtnClicked(Button*)));
//~ MODE


// BPM
	PixmapWidget *pBPMPanel = new PixmapWidget( NULL );
	pBPMPanel->setFixedSize( 145, 43 );
	pBPMPanel->setPixmap( "/playerControlPanel/background_BPM.png" );
	hbox->addWidget( pBPMPanel );

	// LCD BPM SpinBox
	m_pLCDBPMSpinbox = new LCDSpinBox( pBPMPanel, 6, LCDSpinBox::FLOAT, 30, 400 );
	m_pLCDBPMSpinbox->move( 43, 6 );
	connect( m_pLCDBPMSpinbox, SIGNAL(changed(LCDSpinBox*)), this, SLOT(bpmChanged()));
	connect( m_pLCDBPMSpinbox, SIGNAL(spinboxClicked()), this, SLOT(bpmClicked()));

	m_pBPMUpBtn = new Button(
			pBPMPanel,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize(16, 8)
	);
	m_pBPMUpBtn->move( 12, 5 );
	connect( m_pBPMUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );
	connect( m_pBPMUpBtn, SIGNAL( mousePress( Button* ) ), this, SLOT(bpmButtonPressed( Button* ) ) );

	m_pBPMDownBtn = new Button(
			pBPMPanel,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16, 8)
	);
	m_pBPMDownBtn->move( 12, 14 );
	connect( m_pBPMDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );
	connect( m_pBPMDownBtn, SIGNAL( mousePress( Button* ) ), this, SLOT(bpmButtonPressed( Button* ) ) );


	m_pMetronomeWidget = new MetronomeWidget( pBPMPanel );
	m_pMetronomeWidget->resize( 85, 5 );
	m_pMetronomeWidget->move( 42, 25 );

	m_pMetronomeBtn = new ToggleButton(
			pBPMPanel,
			"/playerControlPanel/btn_metronome_on.png",
			"/playerControlPanel/btn_metronome_off.png",
			"/playerControlPanel/btn_metronome_over.png",
			QSize( 20, 13 )
	);
	m_pMetronomeBtn->move( 10, 26 );
	connect( m_pMetronomeBtn, SIGNAL( clicked( Button* ) ), this, SLOT(metronomeButtonClicked( Button* ) ) );
//~ BPM


// JACK
	PixmapWidget *pJackPanel = new PixmapWidget( NULL );
	pJackPanel->setFixedSize( 113, 43 );
	pJackPanel->setPixmap( "/playerControlPanel/background_Jack.png" );
	hbox->addWidget( pJackPanel );

	// Jack transport mode button
	m_pJackTransportBtn = new ToggleButton(
			pJackPanel,
			"/playerControlPanel/jackTransportBtn_on.png",
			"/playerControlPanel/jackTransportBtn_off.png",
			"/playerControlPanel/jackTransportBtn_over.png",
			QSize(92, 13)
	);
	m_pJackTransportBtn->hide();
	m_pJackTransportBtn->setPressed(true);
	m_pJackTransportBtn->setToolTip( trUtf8("Jack-transport on/off") );
	connect(m_pJackTransportBtn, SIGNAL(clicked(Button*)), this, SLOT(jackTransportBtnClicked(Button*)));
	m_pJackTransportBtn->move(10, 26);

	m_pEngine = Hydrogen::get_instance();

	// CPU load widget
	m_pCpuLoadWidget = new CpuLoadWidget( pJackPanel );

	// Midi Activity widget
	m_pMidiActivityWidget = new MidiActivityWidget( pJackPanel );

	m_pMidiActivityWidget->move( 10, 14 );
	m_pCpuLoadWidget->move( 10, 4 );
//~ JACK


	QVBoxLayout *vbox = new QVBoxLayout();
	hbox->addLayout( vbox );

	QHBoxLayout *pButtonsHBox = new QHBoxLayout();
	vbox->addLayout( pButtonsHBox );

	m_pShowMixerBtn = new ToggleButton(
			NULL,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 80, 17 ),
			true
	);
	m_pShowMixerBtn->setToolTip( trUtf8( "Show mixer" ) );
	m_pShowMixerBtn->setText( trUtf8( "Mixer" ) );
	connect(m_pShowMixerBtn, SIGNAL(clicked(Button*)), this, SLOT(showButtonClicked(Button*)));
	pButtonsHBox->addWidget( m_pShowMixerBtn );

	m_pShowInstrumentRackBtn = new ToggleButton(
			NULL,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 130, 17 ),
			true
	);
	m_pShowInstrumentRackBtn->setToolTip( trUtf8( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->setText( trUtf8( "Instrument rack" ) );
	connect( m_pShowInstrumentRackBtn, SIGNAL( clicked(Button*) ), this, SLOT( showButtonClicked( Button*)) );
	pButtonsHBox->addWidget( m_pShowInstrumentRackBtn );

	// STATUS LCD
	m_pStatusLabel = new LCDDisplay( NULL, LCDDigit::SMALL_BLUE, 30, true );
	vbox->addWidget( m_pStatusLabel );


	hbox->addStretch( 1000 );	// this must be the last widget in the HBOX!!




	QTimer *timer = new QTimer( this );
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayerControl()));
	timer->start(100);	// update player control at 10 fps

	m_pBPMTimer = new QTimer( this );
	connect(m_pBPMTimer, SIGNAL(timeout()), this, SLOT(onBpmTimerEvent()));

	m_pStatusTimer = new QTimer( this );
	connect( m_pStatusTimer, SIGNAL( timeout() ), this, SLOT( onStatusTimerEvent() ) );
}




PlayerControl::~PlayerControl() {
}





void PlayerControl::updatePlayerControl()
{
	HydrogenApp *pH2App = HydrogenApp::getInstance();
	m_pShowMixerBtn->setPressed( pH2App->getMixer()->isVisible() );
	m_pShowInstrumentRackBtn->setPressed( pH2App->getInstrumentRack()->isVisible() );

	int state = m_pEngine->getState();
	if (state == STATE_PLAYING ) {
		m_pPlayBtn->setPressed(true);
	}
	else {
		m_pPlayBtn->setPressed(false);
	}

	Song *song = m_pEngine->getSong();

	m_pSongLoopBtn->setPressed( song->isLoopEnabled() );

	m_pLCDBPMSpinbox->setValue( song->m_fBPM );

	if ( song->getMode() == Song::PATTERN_MODE ) {
		m_pLiveModeBtn->setPressed( true );
		m_pSongModeBtn->setPressed( false );
	}
	else {
		m_pLiveModeBtn->setPressed( false );
		m_pSongModeBtn->setPressed( true );
	}

	Preferences *pPref = Preferences::getInstance();


	if ( pPref->m_sAudioDriver == "Jack" ) {
		m_pJackTransportBtn->show();
		switch ( pPref->m_bJackTransportMode ) {
			case Preferences::NO_JACK_TRANSPORT:
				m_pJackTransportBtn->setPressed(false);
				break;

			case Preferences::USE_JACK_TRANSPORT:
				m_pJackTransportBtn->setPressed(true);
				break;
		}
	}
	else {
		m_pJackTransportBtn->hide();
	}


	// time
	float fFrames = m_pEngine->getAudioOutput()->m_transport.m_nFrames;
	float fSampleRate = m_pEngine->getAudioOutput()->getSampleRate();
	if ( fSampleRate != 0 ) {
		float fSeconds = fFrames / fSampleRate;

		int nMSec = (int)( (fSeconds - (int)fSeconds) * 1000.0 );
		int nSeconds = ( (int)fSeconds ) % 60;
		int nMins = (int)( fSeconds / 60.0 ) % 60;
		int nHours = (int)( fSeconds / 3600.0 );

		char tmp[100];
		sprintf(tmp, "%02d", nHours );
		m_pTimeDisplayH->setText( QString( tmp ) );

		sprintf(tmp, "%02d", nMins );
		m_pTimeDisplayM->setText( QString( tmp ) );

		sprintf(tmp, "%02d", nSeconds );
		m_pTimeDisplayS->setText( QString( tmp ) );

		sprintf(tmp, "%03d", nMSec );
		m_pTimeDisplayMS->setText( QString( tmp ) );
	}

	m_pMetronomeBtn->setPressed(pPref->m_bUseMetronome);
}




/// Start audio engine
void PlayerControl::playBtnClicked(Button* ref) {
	if (ref->isPressed()) {
		m_pEngine->sequencer_play();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Playing."), 5000);
	}
	else {
//		m_pPlayBtn->setPressed(true);
		m_pEngine->sequencer_stop();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Pause."), 5000);
	}
}




/// Stop audio engine
void PlayerControl::stopBtnClicked(Button* ref)
{
	UNUSED( ref );
	m_pPlayBtn->setPressed(false);
	m_pEngine->sequencer_stop();
	m_pEngine->setPatternPos( 0 );
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Stopped."), 5000);
}




/// Switch mode
void PlayerControl::switchModeBtnClicked(Button* ref)
{
	UNUSED( ref );

	Song *song = m_pEngine->getSong();

	m_pEngine->sequencer_stop();
	m_pEngine->setPatternPos( 0 );	// from start
	if( song->getMode() == Song::PATTERN_MODE ) {
		m_pEngine->getSong()->setMode( Song::SONG_MODE );
		m_pSongModeBtn->setPressed(true);
		m_pLiveModeBtn->setPressed(false);
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Song mode selected."), 5000);
	}
	else {
		m_pEngine->getSong()->setMode( Song::PATTERN_MODE );
		m_pSongModeBtn->setPressed(false);
		m_pLiveModeBtn->setPressed(true);
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Pattern mode selected."), 5000);
	}
}




/// Set Song mode
void PlayerControl::songModeBtnClicked(Button* ref)
{
	UNUSED( ref );

	m_pEngine->sequencer_stop();
	m_pEngine->setPatternPos( 0 );	// from start
	m_pEngine->getSong()->setMode( Song::SONG_MODE );
	m_pSongModeBtn->setPressed(true);
	m_pLiveModeBtn->setPressed(false);
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Song mode selected."), 5000);
}




///Set Live mode
void PlayerControl::liveModeBtnClicked(Button* ref)
{
	UNUSED( ref );

	m_pEngine->sequencer_stop();
	m_pEngine->getSong()->setMode( Song::PATTERN_MODE );
	//m_pEngine->sequencer_setNextPattern( m_pEngine->getSelectedPatternNumber() );	// imposto il pattern correntemente selezionato come il prossimo da suonare
	m_pSongModeBtn->setPressed(false);
	m_pLiveModeBtn->setPressed(true);
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Pattern mode selected."), 5000);
}



void PlayerControl::bpmChanged() {
	float fNewBpmValue = m_pLCDBPMSpinbox->getValue();
	if (fNewBpmValue < 30) {
		fNewBpmValue = 30;
	}
	else if (fNewBpmValue > 400 ) {
		fNewBpmValue = 400;
	}

	m_pEngine->getSong()->m_bIsModified = true;

	AudioEngine::get_instance()->lock("PlayerControl::bpmChanged");
	m_pEngine->setBPM( fNewBpmValue );
	AudioEngine::get_instance()->unlock();
}



void PlayerControl::jackTransportBtnClicked( Button* )
{
	Preferences *pPref = Preferences::getInstance();

	if (m_pJackTransportBtn->isPressed()) {
		AudioEngine::get_instance()->lock( "PlayerControl::jackTransportBtnClicked" );
		pPref->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Jack-transport mode = On"), 5000);
	}
	else {
		AudioEngine::get_instance()->lock( "PlayerControl::jackTransportBtnClicked" );
		pPref->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Jack-transport mode = Off"), 5000);
	}

	if (pPref->m_sAudioDriver != "Jack") {
		QMessageBox::warning( this, "Hydrogen", trUtf8( "JACK-transport will work only with JACK driver." ) );
	}
}


void PlayerControl::bpmClicked()
{
	bool bIsOkPressed;
	double fNewVal= QInputDialog::getDouble( this, "Hydrogen", trUtf8( "New BPM value" ),  m_pLCDBPMSpinbox->getValue(), 10, 400, 2, &bIsOkPressed );
	if ( bIsOkPressed  ) {
		if ( fNewVal < 30 ) {
			return;
		}

		m_pEngine->getSong()->m_bIsModified  = true;

		AudioEngine::get_instance()->lock( "PlayerControl::bpmChanged");
		m_pEngine->setBPM( fNewVal );
		AudioEngine::get_instance()->unlock();
	}
	else {
		// user entered nothing or pressed Cancel
	}
}


void PlayerControl::bpmButtonPressed( Button* pBtn)
{
	if ( pBtn == m_pBPMUpBtn ) {
		m_pLCDBPMSpinbox->upBtnClicked();
		m_nBPMIncrement = 1;
	}
	else {
		m_pLCDBPMSpinbox->downBtnClicked();
		m_nBPMIncrement = -1;
	}
	m_pBPMTimer->start( 100 );
}


void PlayerControl::bpmButtonClicked( Button* )
{
	m_pBPMTimer->stop();
}


void PlayerControl::onBpmTimerEvent()
{
	if (m_nBPMIncrement == 1) {
		m_pLCDBPMSpinbox->upBtnClicked();
	}
	else {
		m_pLCDBPMSpinbox->downBtnClicked();
	}
}


void PlayerControl::FFWDBtnClicked( Button* )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() + 1 );
}



void PlayerControl::RewindBtnClicked( Button* )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() - 1 );
}



void PlayerControl::songLoopBtnClicked( Button* )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	song->setLoopEnabled( ! song->isLoopEnabled() );
	song->m_bIsModified = true;

	if ( song->isLoopEnabled() ) {
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Loop song = On"), 5000);
	}
	else {
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Loop song = Off"), 5000);
	}
}

void PlayerControl::metronomeButtonClicked(Button* ref)
{
	Preferences::getInstance()->m_bUseMetronome = ref->isPressed();
}



void PlayerControl::showButtonClicked( Button* pRef )
{
	//INFOLOG( "[showButtonClicked]" );
	HydrogenApp *pH2App = HydrogenApp::getInstance();

	if ( pRef == m_pShowMixerBtn ) {
		bool isVisible = pH2App->getMixer()->isVisible();
		pH2App->showMixer( !isVisible );
	}
	else if ( pRef == m_pShowInstrumentRackBtn ) {
		bool isVisible = pH2App->getInstrumentRack()->isVisible();
		pH2App->getInstrumentRack()->setHidden( isVisible );
	}
}



void PlayerControl::showMessage( const QString& msg, int msec )
{
	m_pStatusLabel->setText( msg );
	m_pStatusTimer->start( msec );


}


void PlayerControl::onStatusTimerEvent()
{
	m_pStatusTimer->stop();
	m_pStatusLabel->setText( "" );
}



//::::::::::::::::::::::::::::::::::::::::::::::::



MetronomeWidget::MetronomeWidget(QWidget *pParent)
 : QWidget( pParent )
 , Object( "MetronomeWidget" )
 , m_nValue( 0 )
 , m_state( METRO_OFF )
{
//	INFOLOG( "INIT" );
	HydrogenApp::getInstance()->addEventListener( this );

	m_metro_off.load( Skin::getImagePath() + "/playerControlPanel/metronome_off.png" );
	m_metro_on_firstbeat.load( Skin::getImagePath() + "/playerControlPanel/metronome_up.png" );
	m_metro_on.load( Skin::getImagePath() + "/playerControlPanel/metronome_down.png" );

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateWidget() ) );
	timer->start(50);	// update player control at 20 fps
}


MetronomeWidget::~MetronomeWidget()
{
//	INFOLOG( "DESTROY" );
}


void MetronomeWidget::metronomeEvent( int nValue )
{
	if (nValue == 1) {
		m_state = METRO_FIRST;
		m_nValue = 5;
	}
	else {
		m_state = METRO_ON;
		m_nValue = 5;
	}
	updateWidget();
}


void MetronomeWidget::updateWidget()
{
	if ( m_nValue > 0 ) {
		m_nValue -= 1;
		if (m_nValue == 0 ) {
			m_nValue = 0;
			m_state = METRO_OFF;
		}
		update();
	}
}


void MetronomeWidget::paintEvent( QPaintEvent* ev)
{
	QPainter painter(this);
	switch( m_state ) {
		case METRO_FIRST:
			painter.drawPixmap( ev->rect(), m_metro_on_firstbeat, ev->rect() );
			break;

		case METRO_ON:
			painter.drawPixmap( ev->rect(), m_metro_on, ev->rect() );
			break;

		case METRO_OFF:
			painter.drawPixmap( ev->rect(), m_metro_off, ev->rect() );
			break;
	}
}




