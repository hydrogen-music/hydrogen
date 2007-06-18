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
 * $Id: PlayerControl.cpp,v 1.38 2005/05/27 17:48:33 wsippel Exp $
 *
 */

 #include <qinputdialog.h>
 #include <qtimer.h>

#include "Skin.h"
#include "PlayerControl.h"

#include "lib/drivers/JackDriver.h"
#include "lib/Preferences.h"

PlayerControl::PlayerControl(QWidget *parent)
 : QWidget(parent)
 , Object("PlayerControl")
{
	static const uint w = 1700;	// FIXME... ;)
	static const uint h = 50;

	setMinimumSize( w, h );
	setMaximumSize( width(), height() );
	resize( width(), height() );

	// Background image
	bool ok = m_background.load( QString( Skin::getImagePath().append("/playerControlPanel/background.png").c_str() ) );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}
	setPaletteBackgroundPixmap( m_background );


	// CONTROLS
	QWidget *pControlsPanel = new QWidget( this );
	pControlsPanel->resize( 317, 43 );
	pControlsPanel->move( 5, 5 );
	pControlsPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/playerControlPanel/background_Control.png" ).c_str() ) ) );

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
	string rwd_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_rwd_on.png" );
	string rwd_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_rwd_off.png" );
	string rwd_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_rwd_over.png" );
	m_pRwdBtn = new Button( pControlsPanel, QSize(21, 15), rwd_on_path, rwd_off_path, rwd_over_path);
	m_pRwdBtn->move(168, 17);
	QToolTip::add( m_pRwdBtn, trUtf8("Rewind") );
	connect(m_pRwdBtn, SIGNAL(clicked(Button*)), this, SLOT(RewindBtnClicked(Button*)));

	// Play button
	string play_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_play_on.png" );
	string play_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_play_off.png" );
	string play_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_play_over.png" );
	m_pPlayBtn = new ToggleButton( pControlsPanel, QSize(26, 17), play_on_path, play_off_path, play_over_path);
	m_pPlayBtn->move(195, 17);
	m_pPlayBtn->setPressed(false);
	QToolTip::add( m_pPlayBtn, trUtf8("Play/ Pause") );
	connect(m_pPlayBtn, SIGNAL(clicked(Button*)), this, SLOT(playBtnClicked(Button*)));

	// Stop button
	string stop_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_stop_on.png" );
	string stop_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_stop_off.png" );
	string stop_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_stop_over.png" );
	m_pStopBtn = new Button( pControlsPanel, QSize(21, 15), stop_on_path, stop_off_path, stop_over_path);
	m_pStopBtn->move(227, 17);
	QToolTip::add( m_pStopBtn, trUtf8("Stop") );
	connect(m_pStopBtn, SIGNAL(clicked(Button*)), this, SLOT(stopBtnClicked(Button*)));

	// Fast forward button
	string ffwd_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_ffwd_on.png" );
	string ffwd_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_ffwd_off.png" );
	string ffwd_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_ffwd_over.png" );
	m_pFfwdBtn = new Button( pControlsPanel, QSize(21, 15), ffwd_on_path, ffwd_off_path, ffwd_over_path);
	m_pFfwdBtn->move(254, 17);
	QToolTip::add( m_pFfwdBtn, trUtf8("Fast Forward") );
	connect(m_pFfwdBtn, SIGNAL(clicked(Button*)), this, SLOT(FFWDBtnClicked(Button*)));

	// Loop song button button
	string loop_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_loop_on.png" );
	string loop_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_loop_off.png" );
	string loop_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_loop_over.png" );
	m_pSongLoopBtn = new ToggleButton( pControlsPanel, QSize(21, 15), loop_on_path, loop_off_path, loop_over_path);
	m_pSongLoopBtn->move(283, 17);
	QToolTip::add( m_pSongLoopBtn, trUtf8("Loop song") );
	connect( m_pSongLoopBtn, SIGNAL( clicked(Button*) ), this, SLOT( songLoopBtnClicked(Button*) ) );
	//~ CONTROLS


	// MODE
	QWidget *pModePanel = new QWidget( this );
	pModePanel->resize( 90, 43 );
	pModePanel->move( 322, 5 );
	pModePanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/playerControlPanel/background_Mode.png" ).c_str() ) ) );

	// Live mode button
	string liveMode_on_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_on.png" );
	string liveMode_off_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_off.png" );
	string liveMode_over_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_off.png" );
	m_pLiveModeBtn = new ToggleButton( pModePanel, QSize(11, 9), liveMode_on_path, liveMode_off_path, liveMode_over_path);
	m_pLiveModeBtn->move(10, 4);
	m_pLiveModeBtn->setPressed(true);
	QToolTip::add( m_pLiveModeBtn, trUtf8("Pattern Mode") );
	connect(m_pLiveModeBtn, SIGNAL(clicked(Button*)), this, SLOT(liveModeBtnClicked(Button*)));

	// Song mode button
	string songMode_on_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_on.png" );
	string songMode_off_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_off.png" );
	string songMode_over_path = Skin::getImagePath() + string( "/playerControlPanel/statusLED_off.png" );
	m_pSongModeBtn = new ToggleButton( pModePanel, QSize(11, 9), songMode_on_path, songMode_off_path, songMode_over_path);
	m_pSongModeBtn->move(10, 15);
	m_pSongModeBtn->setPressed(false);
	QToolTip::add( m_pSongModeBtn, trUtf8("Song Mode") );
	connect(m_pSongModeBtn, SIGNAL(clicked(Button*)), this, SLOT(songModeBtnClicked(Button*)));

	// Switch mode button
	string switchMode_on_path = Skin::getImagePath() + string( "/playerControlPanel/btn_mode_on.png" );
	string switchMode_off_path = Skin::getImagePath() + string( "/playerControlPanel/btn_mode_off.png" );
	string switchMode_over_path = Skin::getImagePath() + string( "/playerControlPanel/btn_mode_over.png" );
	m_pSwitchModeBtn = new Button( pModePanel, QSize(69, 13), switchMode_on_path, switchMode_off_path, switchMode_over_path);
	m_pSwitchModeBtn->move(10, 26);
	QToolTip::add( m_pSwitchModeBtn, trUtf8("Switch Song/ Pattern Mode") );
	connect(m_pSwitchModeBtn, SIGNAL(clicked(Button*)), this, SLOT(switchModeBtnClicked(Button*)));
	//~ MODE


	// BPM
	QWidget *pBPMPanel = new QWidget( this );
	pBPMPanel->resize( 145, 43 );
	pBPMPanel->move( 412, 5 );
	pBPMPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/playerControlPanel/background_BPM.png" ).c_str() ) ) );

	// LCD BPM SpinBox
	m_pLCDBPMSpinbox = new LCDSpinBox( pBPMPanel, 6, LCDSpinBox::FLOAT, 30, 400 );
	m_pLCDBPMSpinbox->move( 43, 6 );
	connect( m_pLCDBPMSpinbox, SIGNAL(changed(LCDSpinBox*)), this, SLOT(bpmChanged()));
	connect( m_pLCDBPMSpinbox, SIGNAL(spinboxClicked()), this, SLOT(bpmClicked()));

	string bpmUP_on_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_on.png" );
	string bpmUP_off_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_off.png" );
	string bpmUP_over_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_over.png" );
	m_pBPMUpBtn = new Button( pBPMPanel, QSize(16, 8), bpmUP_on_path, bpmUP_off_path, bpmUP_over_path );
	m_pBPMUpBtn->move( 12, 5 );
	connect( m_pBPMUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );
	connect( m_pBPMUpBtn, SIGNAL( mousePress( Button* ) ), this, SLOT(bpmButtonPressed( Button* ) ) );

	string bpmDown_on_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_on.png" );
	string bpmDown_off_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_off.png" );
	string bpmDown_over_path = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_over.png" );
	m_pBPMDownBtn = new Button( pBPMPanel, QSize(16, 8), bpmDown_on_path, bpmDown_off_path, bpmDown_over_path );
	m_pBPMDownBtn->move( 12, 14 );
	connect( m_pBPMDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );
	connect( m_pBPMDownBtn, SIGNAL( mousePress( Button* ) ), this, SLOT(bpmButtonPressed( Button* ) ) );


	m_pMetronomeWidget = new MetronomeWidget( pBPMPanel );
	m_pMetronomeWidget->resize( 85, 5 );
	m_pMetronomeWidget->move( 42, 25 );

	string sMetro_on = Skin::getImagePath() + string( "/playerControlPanel/btn_metronome_on.png" );
	string sMetro_off = Skin::getImagePath() + string( "/playerControlPanel/btn_metronome_off.png" );
	string sMetro_over = Skin::getImagePath() + string( "/playerControlPanel/btn_metronome_over.png" );
	m_pMetronomeBtn = new ToggleButton( pBPMPanel, QSize( 20, 13 ), sMetro_on, sMetro_off, sMetro_over );
	m_pMetronomeBtn->move( 10, 26 );
	connect( m_pMetronomeBtn, SIGNAL( clicked( Button* ) ), this, SLOT(metronomeButtonClicked( Button* ) ) );
	//~ BPM


	// JACK
	QWidget *pJackPanel = new QWidget( this );
	pJackPanel->resize( 113, 43 );
	pJackPanel->move( 557, 5 );
	pJackPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/playerControlPanel/background_Jack.png" ).c_str() ) ) );

	// Jack transport mode button
	string jackTransportMode_on_path = Skin::getImagePath() + string( "/playerControlPanel/jackTransportBtn_on.png" );
	string jackTransportMode_off_path = Skin::getImagePath() + string( "/playerControlPanel/jackTransportBtn_off.png" );
	string jackTransportMode_over_path = Skin::getImagePath() + string( "/playerControlPanel/jackTransportBtn_over.png" );
	m_pJackTransportBtn = new ToggleButton(pJackPanel, QSize(92, 13), jackTransportMode_on_path, jackTransportMode_off_path, jackTransportMode_over_path);
	m_pJackTransportBtn->setPressed(true);
	QToolTip::add( m_pJackTransportBtn, trUtf8("Jack-transport on/off") );
	connect(m_pJackTransportBtn, SIGNAL(clicked(Button*)), this, SLOT(jackTransportBtnClicked(Button*)));
	m_pJackTransportBtn->move(10, 26);

	m_pEngine = Hydrogen::getInstance();

	// CPU load widget
	m_pCpuLoadWidget = new CpuLoadWidget( pJackPanel );

	// Midi Activity widget
	m_pMidiActivityWidget = new MidiActivityWidget( pJackPanel );

	m_pMidiActivityWidget->move( 10, 14 );
	m_pCpuLoadWidget->move( 10, 4 );
	//~ JACK


	QTimer *timer = new QTimer( this );
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayerControl()));
	timer->start(100);	// update player control at 10 fps

	m_pBPMTimer = new QTimer( this );
	connect(m_pBPMTimer, SIGNAL(timeout()), this, SLOT(onBpmTimerEvent()));
}




PlayerControl::~PlayerControl() {
}





/// Start audio engine
void PlayerControl::playBtnClicked(Button* ref) {
	if (ref->isPressed()) {
		m_pEngine->start();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Playing."), 5000);
	}
	else {
//		m_pPlayBtn->setPressed(true);
		m_pEngine->stop();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Pause."), 5000);
	}
}




/// Stop audio engine
void PlayerControl::stopBtnClicked(Button* ref) {
	m_pPlayBtn->setPressed(false);
	m_pEngine->stop();
	m_pEngine->setPatternPos( 0 );
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Stopped."), 5000);
}




/// Switch mode
void PlayerControl::switchModeBtnClicked(Button* ref) {

	Song *song = m_pEngine->getSong();

	m_pEngine->stop();
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
void PlayerControl::songModeBtnClicked(Button* ref) {
	m_pEngine->stop();
	m_pEngine->setPatternPos( 0 );	// from start
	m_pEngine->getSong()->setMode( Song::SONG_MODE );
	m_pSongModeBtn->setPressed(true);
	m_pLiveModeBtn->setPressed(false);
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Song mode selected."), 5000);
}




///Set Live mode
void PlayerControl::liveModeBtnClicked(Button* ref) {
	m_pEngine->stop();
	m_pEngine->getSong()->setMode( Song::PATTERN_MODE );
	m_pEngine->setNextPattern( m_pEngine->getSelectedPatternNumber() );	// imposto il pattern correntemente selezionato come il prossimo da suonare
	m_pSongModeBtn->setPressed(false);
	m_pLiveModeBtn->setPressed(true);
	(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Pattern mode selected."), 5000);
}




void PlayerControl::updatePlayerControl() {
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

	switch ( pPref->m_bJackTransportMode ) {
		case Preferences::NO_JACK_TRANSPORT:
			m_pJackTransportBtn->setPressed(false);
			break;

		case Preferences::USE_JACK_TRANSPORT:
			m_pJackTransportBtn->setPressed(true);
			break;
	}


	// time
	float fFrames = m_pEngine->getAudioDriver()->m_transport.m_nFrames;
	float fSampleRate = m_pEngine->getAudioDriver()->getSampleRate();
	if ( fSampleRate != 0 ) {
		float fSeconds = fFrames / fSampleRate;

		int nMSec = (fSeconds - (int)fSeconds) * 1000.0;
		int nSeconds = ( (int)fSeconds ) % 60;
		int nMins = (int)( fSeconds / 60.0 ) % 60;
		int nHours = fSeconds / 3600.0;

		char tmp[100];
		sprintf(tmp, "%02d", nHours );
		m_pTimeDisplayH->setText( string( tmp ) );

		sprintf(tmp, "%02d", nMins );
		m_pTimeDisplayM->setText( string( tmp ) );

		sprintf(tmp, "%02d", nSeconds );
		m_pTimeDisplayS->setText( string( tmp ) );

		sprintf(tmp, "%03d", nMSec );
		m_pTimeDisplayMS->setText( string( tmp ) );
	}

	m_pMetronomeBtn->setPressed(pPref->m_bUseMetronome);
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

	m_pEngine->lockEngine("PlayerControl::bpmChanged");
	m_pEngine->setBPM( fNewBpmValue );
	m_pEngine->unlockEngine();
}



void PlayerControl::jackTransportBtnClicked(Button* ref)
{
	Preferences *pPref = Preferences::getInstance();

	if (m_pJackTransportBtn->isPressed()) {
		m_pEngine->lockEngine( "PlayerControl::jackTransportBtnClicked" );
		pPref->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
		m_pEngine->unlockEngine();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Jack-transport mode = On"), 5000);
	}
	else {
		m_pEngine->lockEngine( "PlayerControl::jackTransportBtnClicked" );
		pPref->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
		m_pEngine->unlockEngine();
		(HydrogenApp::getInstance())->setStatusBarMessage(trUtf8("Jack-transport mode = Off"), 5000);
	}

	if (pPref->m_sAudioDriver != "Jack") {
		QMessageBox::warning( this, "Hydrogen", trUtf8( "JACK-transport will work only with JACK driver." ) );
	}
}


void PlayerControl::bpmClicked()
{
	bool bIsOkPressed;
	double fNewVal= QInputDialog::getDouble( "Hydrogen", trUtf8( "New BPM value" ),  m_pLCDBPMSpinbox->getValue(), 10, 400, 2, &bIsOkPressed, this );
	if ( bIsOkPressed  ) {
		if ( fNewVal < 30 ) {
			return;
		}

		m_pEngine->getSong()->m_bIsModified  = true;

		m_pEngine->lockEngine("PlayerControl::bpmChanged");
		m_pEngine->setBPM( fNewVal );
		m_pEngine->unlockEngine();
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
	m_pBPMTimer->start( 100, false );
}


void PlayerControl::bpmButtonClicked( Button *pBtn )
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


void PlayerControl::FFWDBtnClicked(Button *pRef)
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->setPatternPos( pEngine->getPatternPos() + 1 );
}



void PlayerControl::RewindBtnClicked(Button *pRef)
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->setPatternPos( pEngine->getPatternPos() - 1 );
}



void PlayerControl::songLoopBtnClicked( Button* ref )
{
	Hydrogen *pEngine = Hydrogen::getInstance();
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




//::::::::::::::::::::::::::::::::::::::::::::::::



MetronomeWidget::MetronomeWidget(QWidget *pParent)
 : QWidget( pParent )
 , Object("MetronomeWidget")
 , m_nValue( 0 )
 , m_state( METRO_OFF )
{
//	infoLog( "INIT" );
	HydrogenApp::getInstance()->addEventListener( this );

	m_metro_off.load( QString( Skin::getImagePath().append("/playerControlPanel/metronome_off.png").c_str() ) );
	m_metro_on_firstbeat.load( QString( Skin::getImagePath().append("/playerControlPanel/metronome_up.png").c_str() ) );
	m_metro_on.load( QString( Skin::getImagePath().append("/playerControlPanel/metronome_down.png").c_str() ) );

	QTimer *timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateWidget() ) );
	timer->start(50);	// update player control at 20 fps
}


MetronomeWidget::~MetronomeWidget()
{
//	infoLog( "DESTROY" );
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


void MetronomeWidget::paintEvent( QPaintEvent*)
{
	if ( !isVisible() ) {
		return;
	}

	switch( m_state ) {
		case METRO_FIRST:
			bitBlt( this, 0, 0, &m_metro_on_firstbeat, 0, 0, width(), height(), CopyROP );
			break;

		case METRO_ON:
			bitBlt( this, 0, 0, &m_metro_on, 0, 0, width(), height(), CopyROP );
			break;

		case METRO_OFF:
			bitBlt( this, 0, 0, &m_metro_off, 0, 0, width(), height(), CopyROP );
			break;
	}

}


