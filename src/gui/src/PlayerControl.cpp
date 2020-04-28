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


#include "Skin.h"
#include "PlayerControl.h"
#include "InstrumentRack.h"
#include "HydrogenApp.h"

#include "Widgets/LCD.h"
#include "Widgets/Button.h"
#include "Widgets/CpuLoadWidget.h"
#include "Widgets/MidiActivityWidget.h"
#include "Widgets/PixmapWidget.h"

#include "Mixer/Mixer.h"
#include "SongEditor/SongEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/IO/jack_audio_driver.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;


//beatconter global
int bcDisplaystatus = 0;
//~ beatcounter

const char* PlayerControl::__class_name = "PlayerControl";

PlayerControl::PlayerControl(QWidget *parent)
 : QLabel(parent)
 , Object( __class_name )
{
	HydrogenApp::get_instance()->addEventListener( this );
	
	auto pPreferences = Preferences::get_instance();
	
	// Background image
	setPixmap( QPixmap( Skin::getImagePath() + "/playerControlPanel/background.png" ) );
	setScaledContents( true );

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setMargin( 0 );
	setLayout( hbox );



// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( nullptr );
	pControlsPanel->setFixedSize( 344, 43 );
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
	m_pRwdBtn->setToolTip( tr("Rewind") );
	connect(m_pRwdBtn, SIGNAL(clicked(Button*)), this, SLOT(RewindBtnClicked(Button*)));

	// Record button
	m_pRecBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_rec_on.png",
			"/playerControlPanel/btn_rec_off.png",
			"/playerControlPanel/btn_rec_over.png",
			QSize(21, 15)
	);
	m_pRecBtn->move(195, 17);
	m_pRecBtn->setPressed(false);
	m_pRecBtn->setHidden(false);
	m_pRecBtn->setToolTip( tr("Record") );
	connect(m_pRecBtn, SIGNAL(clicked(Button*)), this, SLOT(recBtnClicked(Button*)));
	connect(m_pRecBtn, SIGNAL(rightClicked(Button*)), this, SLOT(recBtnRightClicked(Button*)));

	Action* pAction = new Action("RECORD_READY");
	m_pRecBtn->setAction( pAction );


	// Record+delete button
	m_pRecDelBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_recdel_on.png",
			"/playerControlPanel/btn_recdel_off.png",
			"/playerControlPanel/btn_recdel_over.png",
			QSize(21, 15)
	);
	m_pRecDelBtn->move(195, 17);
	m_pRecDelBtn->setPressed(false);
	m_pRecDelBtn->setHidden(true);
	m_pRecDelBtn->setToolTip( tr("Destructive Record") );
	connect(m_pRecDelBtn, SIGNAL(clicked(Button*)), this, SLOT(recBtnClicked(Button*)));
	connect(m_pRecDelBtn, SIGNAL(rightClicked(Button*)), this, SLOT(recBtnRightClicked(Button*)));



	// Play button
	m_pPlayBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_play_on.png",
			"/playerControlPanel/btn_play_off.png",
			"/playerControlPanel/btn_play_over.png",
			QSize(26, 17)
	);
	m_pPlayBtn->move(222, 17);
	m_pPlayBtn->setPressed(false);
	m_pPlayBtn->setToolTip( tr("Play/ Pause") );
	connect(m_pPlayBtn, SIGNAL(clicked(Button*)), this, SLOT(playBtnClicked(Button*)));

	pAction = new Action("PLAY");
	m_pPlayBtn->setAction( pAction );


	// Stop button
	m_pStopBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_stop_on.png",
			"/playerControlPanel/btn_stop_off.png",
			"/playerControlPanel/btn_stop_over.png",
			QSize(21, 15)
	);
	m_pStopBtn->move(254, 17);
	m_pStopBtn->setToolTip( tr("Stop") );
	connect(m_pStopBtn, SIGNAL(clicked(Button*)), this, SLOT(stopBtnClicked(Button*)));
	pAction = new Action("STOP");
	m_pStopBtn->setAction( pAction );

	// Fast forward button
	m_pFfwdBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_ffwd_on.png",
			"/playerControlPanel/btn_ffwd_off.png",
			"/playerControlPanel/btn_ffwd_over.png",
			QSize(21, 15)
	);
	m_pFfwdBtn->move(281, 17);
	m_pFfwdBtn->setToolTip( tr("Fast Forward") );
	connect(m_pFfwdBtn, SIGNAL(clicked(Button*)), this, SLOT(FFWDBtnClicked(Button*)));

	// Loop song button button
	m_pSongLoopBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_loop_on.png",
			"/playerControlPanel/btn_loop_off.png",
			"/playerControlPanel/btn_loop_over.png",
			QSize(21, 15)
	);
	m_pSongLoopBtn->move(310, 17);
	m_pSongLoopBtn->setToolTip( tr("Loop song") );
	connect( m_pSongLoopBtn, SIGNAL( clicked(Button*) ), this, SLOT( songLoopBtnClicked(Button*) ) );




	// Live mode button
	m_pLiveModeBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/statusLED_on.png",
			"/playerControlPanel/statusLED_off.png",
			"/playerControlPanel/statusLED_off.png",
			QSize(68, 9)
	);
	m_pLiveModeBtn->move(180, 5);
	m_pLiveModeBtn->setPressed(true);
	m_pLiveModeBtn->setToolTip( tr("Pattern Mode") );
	connect(m_pLiveModeBtn, SIGNAL(clicked(Button*)), this, SLOT(liveModeBtnClicked(Button*)));

	// Song mode button
	m_pSongModeBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/statusLED_on.png",
			"/playerControlPanel/statusLED_off.png",
			"/playerControlPanel/statusLED_off.png",
			QSize(68, 9)
	);

	m_pSongModeBtn->move(253, 5);
	m_pSongModeBtn->setPressed(false);
	m_pSongModeBtn->setToolTip( tr("Song Mode") );
	connect(m_pSongModeBtn, SIGNAL(clicked(Button*)), this, SLOT(songModeBtnClicked(Button*)));

//~ CONTROLS

// BC on off
	PixmapWidget *pControlsBBTBConoffPanel = new PixmapWidget( nullptr );
	pControlsBBTBConoffPanel->setFixedSize( 15, 43 );
	pControlsBBTBConoffPanel->setPixmap( "/playerControlPanel/onoff.png" );
	hbox->addWidget( pControlsBBTBConoffPanel );

	m_pBConoffBtn = new ToggleButton(
			pControlsBBTBConoffPanel,
			"/playerControlPanel/bc_on.png",
			"/playerControlPanel/bc_off.png",
			"/playerControlPanel/bc_off.png",
			QSize(10, 40)
	);
	m_pBConoffBtn->move(1, 1);
	m_pBConoffBtn->setPressed(false);
	m_pBConoffBtn->setToolTip( tr("BeatCounter Panel on") );
	connect(m_pBConoffBtn, SIGNAL(clicked(Button*)), this, SLOT(bconoffBtnClicked(Button*)));
//~  BC on off

//beatcounter
	m_pControlsBCPanel = new PixmapWidget( nullptr );
	m_pControlsBCPanel->setFixedSize( 86, 43 );
	m_pControlsBCPanel->setPixmap( "/playerControlPanel/beatConter_BG.png" );
	hbox->addWidget( m_pControlsBCPanel );


	m_pBCDisplayZ = new LCDDisplay( m_pControlsBCPanel, LCDDigit::LARGE_GRAY, 2 );
	m_pBCDisplayZ->move( 36, 8 );
	m_pBCDisplayZ->setText( "--" );


	m_pBCDisplayT = new LCDDisplay( m_pControlsBCPanel, LCDDigit::SMALL_GRAY, 1 );
	m_pBCDisplayT->move( 23, 26 );
	m_pBCDisplayT->setText( "4" );

	m_pBCDisplayB = new LCDDisplay( m_pControlsBCPanel, LCDDigit::SMALL_GRAY, 2 );
	m_pBCDisplayB->move( 39, 26 );
// set display from 4 to 04. fix against qt4 transparent problem
//	m_pBCDisplayB->setText( "4" );
	m_pBCDisplayB->setText( "04" );

	m_pBCTUpBtn = new Button(
			m_pControlsBCPanel,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize(16, 8)
	);
	m_pBCTUpBtn->move( 4, 6 );
	connect( m_pBCTUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bctButtonClicked( Button* ) ) );

	m_pBCTDownBtn = new Button(
			m_pControlsBCPanel,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16, 8)
	);
	m_pBCTDownBtn->move( 4, 16 );
	connect( m_pBCTDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bctButtonClicked( Button* ) ) );

	m_pBCBUpBtn = new Button(
			m_pControlsBCPanel,
			"/lcd/LCDSpinBox_up_on.png",
			"/lcd/LCDSpinBox_up_off.png",
			"/lcd/LCDSpinBox_up_over.png",
			QSize(16, 8)
	);
	m_pBCBUpBtn->move( 65, 6 );
	connect( m_pBCBUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bcbButtonClicked( Button* ) ) );

	m_pBCBDownBtn = new Button(
			m_pControlsBCPanel,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16, 8)
	);
	m_pBCBDownBtn->move( 65, 16 );
	connect( m_pBCBDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bcbButtonClicked( Button* ) ) );

	m_pBCSetPlayBtn = new ToggleButton(
			m_pControlsBCPanel,
			"/playerControlPanel/btn_set_play_on.png",
			"/playerControlPanel/btn_set_play_off.png",
			"/playerControlPanel/btn_set_play_off.png",
			QSize(15, 13)
	);
	m_pBCSetPlayBtn->move(67, 27);
	m_pBCSetPlayBtn->setPressed(false);
	m_pBCSetPlayBtn->setToolTip( tr("Set BPM / Set BPM and play") );
	connect(m_pBCSetPlayBtn, SIGNAL(clicked(Button*)), this, SLOT(bcSetPlayBtnClicked(Button*)));
//~ beatcounter


// BPM
	PixmapWidget *pBPMPanel = new PixmapWidget( nullptr );
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
			QSize(16, 8),
			false,
			true
	);
	m_pBPMUpBtn->move( 12, 5 );
	connect( m_pBPMUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );

	m_pBPMDownBtn = new Button(
			pBPMPanel,
			"/lcd/LCDSpinBox_down_on.png",
			"/lcd/LCDSpinBox_down_off.png",
			"/lcd/LCDSpinBox_down_over.png",
			QSize(16, 8),
			false,
			true
	);
	m_pBPMDownBtn->move( 12, 14 );
	connect( m_pBPMDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT(bpmButtonClicked( Button* ) ) );

	m_pRubberBPMChange = new ToggleButton(
			pBPMPanel,
			"/playerControlPanel/rubber_on.png",
			"/playerControlPanel/rubber_off.png",
			"/playerControlPanel/rubber_off.png",
			QSize(9, 37)
	);

	m_pRubberBPMChange->move( 133, 3 );
	m_pRubberBPMChange->setToolTip( tr("Recalculate Rubberband modified samples if bpm will change") );
	m_pRubberBPMChange->setPressed( pPreferences->getRubberBandBatchMode());

	connect( m_pRubberBPMChange, SIGNAL( clicked( Button* ) ), this, SLOT(rubberbandButtonToggle( Button* ) ) );
	QString program = pPreferences->m_rubberBandCLIexecutable;
	//test the path. if test fails, no button
	if ( QFile( program ).exists() == false) {
		m_pRubberBPMChange->hide();
	}

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
	m_pMetronomeBtn->setToolTip( tr("Switch metronome on/off") );
	connect( m_pMetronomeBtn, SIGNAL( clicked( Button* ) ), this, SLOT(metronomeButtonClicked( Button* ) ) );
		pAction = new Action("TOGGLE_METRONOME");
		m_pMetronomeBtn->setAction( pAction );

//~ BPM


// JACK
	PixmapWidget *pJackPanel = new PixmapWidget( nullptr );
	pJackPanel->setFixedSize( 113, 43 );
	pJackPanel->setPixmap( "/playerControlPanel/background_Jack.png" );
	hbox->addWidget( pJackPanel );

	// Jack transport mode button
	m_pJackTransportBtn = new ToggleButton(
			pJackPanel,
			"/playerControlPanel/jackTransportBtn_on.png",
			"/playerControlPanel/jackTransportBtn_off.png",
			"/playerControlPanel/jackTransportBtn_over.png",
			QSize(45, 13)
	);
	m_pJackTransportBtn->hide();
	if ( pPreferences->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT ) {
		m_pJackTransportBtn->setPressed( true );
	} else {
		m_pJackTransportBtn->setPressed( false );
	}
	m_pJackTransportBtn->setToolTip( tr("Jack-transport on/off") );
	connect(m_pJackTransportBtn, SIGNAL(clicked(Button*)), this, SLOT(jackTransportBtnClicked(Button*)));
	m_pJackTransportBtn->move(10, 26);

	//jack time master
	m_pJackMasterBtn = new ToggleButton(
			pJackPanel,
			"/playerControlPanel/jackMasterBtn_on.png",
			"/playerControlPanel/jackMasterBtn_off.png",
			"/playerControlPanel/jackMasterBtn_over.png",
			QSize(45, 13)
	);
	m_pJackMasterBtn->hide();
	if ( m_pJackTransportBtn->isPressed() &&
		 pPreferences->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER ) {
		m_pJackMasterBtn->setPressed( true );
	} else {
		m_pJackMasterBtn->setPressed( false );
	}
	m_pJackMasterBtn->setToolTip( tr("Jack-Time-Master on/off") );
	connect(m_pJackMasterBtn, SIGNAL(clicked(Button*)), this, SLOT(jackMasterBtnClicked(Button*)));
	m_pJackMasterBtn->move(56, 26);
	//~ jack time master

	m_pEngine = Hydrogen::get_instance();

	// CPU load widget
	m_pCpuLoadWidget = new CpuLoadWidget( pJackPanel );

	// Midi Activity widget
	m_pMidiActivityWidget = new MidiActivityWidget( pJackPanel );

	m_pMidiActivityWidget->move( 10, 14 );
	m_pCpuLoadWidget->move( 10, 4 );
//~ JACK


	PixmapWidget *pLcdBackGround = new PixmapWidget( nullptr );
	pLcdBackGround->setFixedSize( 256, 43 );
	pLcdBackGround->setPixmap( "/playerControlPanel/lcd_background.png" );
	hbox->addWidget( pLcdBackGround );

	m_pShowMixerBtn = new ToggleButton(
			pLcdBackGround,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 80, 17 ),
			true
	);
	m_pShowMixerBtn->move( 7, 6 );
	m_pShowMixerBtn->setToolTip( tr( "Show mixer" ) );
	m_pShowMixerBtn->setText( tr( "Mixer" ) );
	connect(m_pShowMixerBtn, SIGNAL(clicked(Button*)), this, SLOT(showButtonClicked(Button*)));

	m_pShowInstrumentRackBtn = new ToggleButton(
			pLcdBackGround,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize( 160, 17 ),
			true
	);
	m_pShowInstrumentRackBtn->move( 88, 6 );
	m_pShowInstrumentRackBtn->setToolTip( tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->setText( tr( "Instrument rack" ) );
	connect( m_pShowInstrumentRackBtn, SIGNAL( clicked(Button*) ), this, SLOT( showButtonClicked( Button*)) );

	m_pStatusLabel = new LCDDisplay(pLcdBackGround , LCDDigit::SMALL_BLUE, 30, true );
	m_pStatusLabel->move( 7, 25 );


	hbox->addStretch( 1000 );	// this must be the last widget in the HBOX!!




	QTimer *timer = new QTimer( this );
	connect(timer, SIGNAL(timeout()), this, SLOT(updatePlayerControl()));
	timer->start(100);	// update player control at 10 fps

	m_pStatusTimer = new QTimer( this );
	connect( m_pStatusTimer, SIGNAL( timeout() ), this, SLOT( onStatusTimerEvent() ) );

	m_pScrollTimer = new QTimer( this );
	connect( m_pScrollTimer, SIGNAL( timeout() ), this, SLOT( onScrollTimerEvent() ) );
	m_pScrollMessage = "";
}




PlayerControl::~PlayerControl() {
}





void PlayerControl::updatePlayerControl()
{
	Preferences *pPref = Preferences::get_instance();
	HydrogenApp *pH2App = HydrogenApp::get_instance();

	m_pShowMixerBtn->setPressed( pH2App->getMixer()->isVisible() );
	m_pShowInstrumentRackBtn->setPressed( pH2App->getInstrumentRack()->isVisible() );

	int state = m_pEngine->getState();
	if (state == STATE_PLAYING ) {
		m_pPlayBtn->setPressed(true);
	}
	else {
		m_pPlayBtn->setPressed(false);
	}

	if (pPref->getRecordEvents()) {
		m_pRecBtn->setPressed(true);
		m_pRecDelBtn->setPressed(true);
	}
	else {
		m_pRecBtn->setPressed(false);
		m_pRecDelBtn->setPressed(false);
	}

	if (pPref->getDestructiveRecord()) {
		if (  m_pRecDelBtn->isHidden() ) {
			m_pRecBtn->setHidden(true);
			m_pRecDelBtn->setHidden(false);
		}
	}
	else {
		if (  m_pRecBtn->isHidden() ) {
			m_pRecBtn->setHidden(false);
			m_pRecDelBtn->setHidden(true);
		}
	}

	Song *song = m_pEngine->getSong();

	m_pSongLoopBtn->setPressed( song->is_loop_enabled() );

	m_pLCDBPMSpinbox->setValue( song->__bpm );

	if ( song->get_mode() == Song::PATTERN_MODE ) {
		m_pLiveModeBtn->setPressed( true );
		m_pSongModeBtn->setPressed( false );
	}
	else {
		m_pLiveModeBtn->setPressed( false );
		m_pSongModeBtn->setPressed( true );
	}

	//beatcounter
	if ( pPref->m_bbc == Preferences::BC_OFF ) {
		m_pControlsBCPanel->hide();
		m_pBConoffBtn->setPressed(false);
	}else
	{
		m_pControlsBCPanel->show();
		m_pBConoffBtn->setPressed(true);
	}

	if ( pPref->m_mmcsetplay ==  Preferences::SET_PLAY_OFF) {
		m_pBCSetPlayBtn->setPressed(false);
	}else
	{
		m_pBCSetPlayBtn->setPressed(true);
	}
	//~ beatcounter


#ifdef H2CORE_HAVE_JACK
	AudioOutput *p_Driver = m_pEngine->getAudioOutput();

	if ( m_pEngine->haveJackAudioDriver() ) {
		m_pJackTransportBtn->show();
		m_pJackMasterBtn->show();
		
		switch ( pPref->m_bJackTransportMode ) {
			case Preferences::NO_JACK_TRANSPORT:
				m_pJackTransportBtn->setPressed(false);
				m_pJackMasterBtn->setPressed(false);
				break;

			case Preferences::USE_JACK_TRANSPORT:
				m_pJackTransportBtn->setPressed(true);
				
				if ( static_cast<JackAudioDriver*>(p_Driver)->getIsTimebaseMaster() > 0 ) {
					m_pJackMasterBtn->setPressed( true );
				} else {
					m_pJackMasterBtn->setPressed( false );
				}
				
				break;
		}

	}
	else {
		m_pJackTransportBtn->hide();
		m_pJackMasterBtn->hide();
	}
#endif

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


	//beatcounter get BC message
	char bcstatus[4];
	int beatstocountondisplay = 1;
	beatstocountondisplay = m_pEngine->getBcStatus();

	switch (beatstocountondisplay){
		case 1 :
			if (bcDisplaystatus == 1){
				Preferences::get_instance()->m_bbc = Preferences::BC_OFF;
				bcDisplaystatus = 0;
			}
			sprintf(bcstatus, "R");
				m_pBCDisplayZ->setText( QString (bcstatus) );

			break;
		default:
			if (Preferences::get_instance()->m_bbc == Preferences::BC_OFF){
				Preferences::get_instance()->m_bbc = Preferences::BC_ON;
				bcDisplaystatus = 1;
			}
			sprintf(bcstatus, "%02d ", beatstocountondisplay -1);
			m_pBCDisplayZ->setText( QString (bcstatus) );

	}
	//~ beatcounter
}



/// Toggle record mode
void PlayerControl::recBtnClicked(Button* ref) {
	if ( m_pEngine->getState() != STATE_PLAYING ) {
		if (ref->isPressed()) {
			Preferences::get_instance()->setRecordEvents(true);
			(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Record midi events = On" ), 2000 );
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
			(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Record midi events = Off" ), 2000 );
		}
	}
}


/// Toggle destructive/nondestructive move
void PlayerControl::recBtnRightClicked(Button* ref) {
	UNUSED( ref );
	if ( Preferences::get_instance()->getDestructiveRecord() ) {
		Preferences::get_instance()->setDestructiveRecord(false);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Destructive mode = Off" ), 2000 );
	}
	else {
		Preferences::get_instance()->setDestructiveRecord(true);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Destructive mode = On" ), 2000 );
	}
	HydrogenApp::get_instance()->enableDestructiveRecMode();
}


/// Start audio engine
void PlayerControl::playBtnClicked(Button* ref) {
	if (ref->isPressed()) {
		m_pEngine->sequencer_play();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Playing."), 5000);
	}
	else {
		m_pEngine->sequencer_stop();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Pause."), 5000);
	}
}




/// Stop audio engine
void PlayerControl::stopBtnClicked(Button* ref)
{
	UNUSED( ref );
	m_pPlayBtn->setPressed(false);
	m_pEngine->sequencer_stop();
	m_pEngine->setPatternPos( 0 );
	(HydrogenApp::get_instance())->setStatusBarMessage(tr("Stopped."), 5000);
	Hydrogen::get_instance()->setTimelineBpm();
}




/// Set Song mode
void PlayerControl::songModeBtnClicked(Button* ref)
{
	UNUSED( ref );

	m_pEngine->sequencer_stop();
	m_pEngine->setPatternPos( 0 );	// from start
	m_pEngine->getSong()->set_mode( Song::SONG_MODE );
	m_pSongModeBtn->setPressed(true);
	m_pLiveModeBtn->setPressed(false);
	(HydrogenApp::get_instance())->setStatusBarMessage(tr("Song mode selected."), 5000);
}




///Set Live mode
void PlayerControl::liveModeBtnClicked(Button* ref)
{
	UNUSED( ref );

	m_pEngine->sequencer_stop();
	m_pEngine->getSong()->set_mode( Song::PATTERN_MODE );
	m_pSongModeBtn->setPressed(false);
	m_pLiveModeBtn->setPressed(true);
	(HydrogenApp::get_instance())->setStatusBarMessage(tr("Pattern mode selected."), 5000);
}



void PlayerControl::bpmChanged() {
	float fNewBpmValue = m_pLCDBPMSpinbox->getValue();
	if (fNewBpmValue < 30) {
		fNewBpmValue = 30;
	}
	else if (fNewBpmValue > 400 ) {
		fNewBpmValue = 400;
	}

	m_pEngine->getSong()->set_is_modified( true );

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	m_pEngine->setBPM( fNewBpmValue );
	AudioEngine::get_instance()->unlock();
}



//beatcounter
void PlayerControl::bconoffBtnClicked( Button* )
{
	Preferences *pPref = Preferences::get_instance();
	if (m_pBConoffBtn->isPressed()) {
		pPref->m_bbc = Preferences::BC_ON;
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" BC Panel on"), 5000);
		m_pControlsBCPanel->show();

	}
	else {
		pPref->m_bbc = Preferences::BC_OFF;
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" BC Panel off"), 5000);
		m_pControlsBCPanel->hide();
	}

}

void PlayerControl::bcSetPlayBtnClicked( Button* )
{
	Preferences *pPref = Preferences::get_instance();
	if (m_pBCSetPlayBtn->isPressed()) {
		pPref->m_mmcsetplay = Preferences::SET_PLAY_ON;
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Count BPM and start PLAY"), 5000);
	}
	else {
		pPref->m_mmcsetplay = Preferences::SET_PLAY_OFF;
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Count and set BPM"), 5000);
	}
}


void PlayerControl::rubberbandButtonToggle(Button* )
{
	Preferences *pPref = Preferences::get_instance();
	if (m_pRubberBPMChange->isPressed()) {
		EventQueue::get_instance()->push_event( EVENT_RECALCULATERUBBERBAND, -1);
				pPref->setRubberBandBatchMode(true);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Recalculate all samples using Rubberband ON"), 2000);
	}
	else {
		pPref->setRubberBandBatchMode(false);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Recalculate all samples using Rubberband OFF"), 2000);
	}
}


void PlayerControl::bcbButtonClicked( Button* bBtn)
{
	int tmp = m_pEngine->getbeatsToCount();
	char tmpb[3];       // m_pBCBUpBtn
		if ( bBtn == m_pBCBUpBtn ) {
			tmp ++;
			if (tmp > 16)
				tmp = 2;
//small fix against qt4 png transparent problem
//think this will be solved in next time
//			if (tmp < 10 ){
//				sprintf(tmpb, "%01d", tmp );
//			}else
//			{
				sprintf(tmpb, "%02d", tmp );
//			}
			m_pBCDisplayB->setText( QString( tmpb ) );
			m_pEngine->setbeatsToCount( tmp );
	}
	else {
			tmp --;
			if (tmp < 2 )
				 tmp = 16;
//small fix against qt4 png transparent problem
//think this will be solved in next time
//			if (tmp < 10 ){
//				sprintf(tmpb, "%01d", tmp );
//			}else
//			{
				sprintf(tmpb, "%02d", tmp );
//			}
			m_pBCDisplayB->setText( QString( tmpb ) );
			m_pEngine->setbeatsToCount( tmp );
	}
}



void PlayerControl::bctButtonClicked( Button* tBtn)
{
	float tmp = m_pEngine->getNoteLength() * 4;

	if ( tBtn == m_pBCTUpBtn) {
			tmp = tmp / 2 ;
			if (tmp < 1)
				tmp = 8;

			m_pBCDisplayT->setText( QString::number( tmp ) );
			m_pEngine->setNoteLength( (tmp) / 4 );
	} else {
			tmp = tmp * 2;
			if (tmp > 8 )
				 tmp = 1;
			m_pBCDisplayT->setText( QString::number(tmp) );
			m_pEngine->setNoteLength( (tmp) / 4 );
	}
}
//~ beatcounter



void PlayerControl::jackTransportBtnClicked( Button* )
{
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *p_Driver = m_pEngine->getAudioOutput();

	if ( !m_pEngine->haveJackAudioDriver() ) {
		QMessageBox::warning( this, "Hydrogen", tr( "JACK-transport will work only with JACK driver." ) );
		return;
	}

	if (m_pJackTransportBtn->isPressed()) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		pPref->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Jack-transport mode = On"), 5000);
		m_pJackMasterBtn->setDisabled( false );
	}
	else {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		pPref->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Jack-transport mode = Off"), 5000);
		m_pJackMasterBtn->setPressed( false );
		m_pJackMasterBtn->setDisabled( true );
	}
}


//jack time master
void PlayerControl::jackMasterBtnClicked( Button* )
{
#ifdef H2CORE_HAVE_JACK
	Preferences *pPref = Preferences::get_instance();
	AudioOutput *p_Driver = m_pEngine->getAudioOutput();

	if ( !m_pEngine->haveJackTransport() ) {
		QMessageBox::warning( this, "Hydrogen", tr( "JACK-transport will work only with JACK driver." ) );
		return;
	}

	if (m_pJackMasterBtn->isPressed()) {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		pPref->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Jack-Time-Master mode = On"), 5000);
		Hydrogen::get_instance()->onJackMaster();

	} else {
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		pPref->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		AudioEngine::get_instance()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Jack-Time-Master mode = Off"), 5000);
		Hydrogen::get_instance()->offJackMaster();
	}
	HydrogenApp::get_instance()->getSongEditorPanel()->updateTimelineUsage();
#endif
}
//~ jack time master

void PlayerControl::bpmClicked()
{
	bool bIsOkPressed;
	double fNewVal= QInputDialog::getDouble( this, "Hydrogen", tr( "New BPM value" ),  m_pLCDBPMSpinbox->getValue(), 10, 400, 2, &bIsOkPressed );
	if ( bIsOkPressed  ) {
		if ( fNewVal < 30 ) {
			return;
		}

		m_pEngine->getSong()->set_is_modified( true );

		AudioEngine::get_instance()->lock( RIGHT_HERE );

		m_pEngine->setBPM( fNewVal );
		AudioEngine::get_instance()->unlock();
	}
	else {
		// user entered nothing or pressed Cancel
	}
}


void PlayerControl::bpmButtonClicked( Button* pBtn )
{
	if ( pBtn == m_pBPMUpBtn )
		m_pLCDBPMSpinbox->upBtnClicked();
	else
		m_pLCDBPMSpinbox->downBtnClicked();
}


void PlayerControl::FFWDBtnClicked( Button* )
{
	WARNINGLOG( "relocate via button press" );
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() + 1 );
	Hydrogen::get_instance()->setTimelineBpm();
}



void PlayerControl::RewindBtnClicked( Button* )
{
	WARNINGLOG( "relocate via button press" );
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() - 1 );
	Hydrogen::get_instance()->setTimelineBpm();
}


void PlayerControl::songLoopBtnClicked( Button* )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	song->set_loop_enabled( ! song->is_loop_enabled() );
	song->set_is_modified( true );

	if ( song->is_loop_enabled() ) {
		HydrogenApp::get_instance()->setStatusBarMessage(tr("Loop song = On"), 5000);
	}
	else {
		HydrogenApp::get_instance()->setStatusBarMessage(tr("Loop song = Off"), 5000);
	}
}

void PlayerControl::metronomeButtonClicked(Button* ref)
{
	Hydrogen*	pEngine = Hydrogen::get_instance();
	CoreActionController* pController = pEngine->getCoreActionController();
	
	pController->setMetronomeIsActive( ref->isPressed() );
}


void PlayerControl::showButtonClicked( Button* pRef )
{
	//INFOLOG( "[showButtonClicked]" );
	HydrogenApp *pH2App = HydrogenApp::get_instance();

	if ( pRef == m_pShowMixerBtn ) {
		bool isVisible = pH2App->getMixer()->isVisible();
		pH2App->showMixer( !isVisible );
	}
	else if ( pRef == m_pShowInstrumentRackBtn ) {
		bool isVisible = pH2App->getInstrumentRack()->isVisible();
		pH2App->showInstrumentPanel( isVisible );
	}
}



void PlayerControl::showMessage( const QString& msg, int msec )
{
	if ( m_pScrollTimer->isActive ())
		m_pScrollTimer->stop();
	m_pStatusLabel->setText( msg );
	m_pStatusTimer->start( msec );


}



void PlayerControl::showScrollMessage( const QString& msg, int msec, bool test )
{

	if ( test == false ){
		m_pStatusLabel->setText( msg );
		m_pScrollTimer->start( msec );
	}else
	{
		m_pScrollMessage = msg;
		m_pStatusLabel->setText( msg );
		m_pStatusTimer->start( msec );
		m_pScrollTimer->start( msec );
	}
}

void PlayerControl::onScrollTimerEvent()
{
	int lwl = 25;
	int msgLength = m_pScrollMessage.length();
	if ( msgLength > lwl)
		m_pScrollMessage = m_pScrollMessage.right( msgLength - 1 );
	m_pScrollTimer->stop();

	if ( msgLength > lwl){
		showScrollMessage( m_pScrollMessage, 150, false );
	}else
	{
		showMessage( m_pScrollMessage, 2000 );
	}
}

void PlayerControl::onStatusTimerEvent()
{
	resetStatusLabel();
}

void PlayerControl::resetStatusLabel()
{
	m_pStatusTimer->stop();
	m_pStatusLabel->setText( "" );
}

void PlayerControl::tempoChangedEvent( int nValue )
{
	/*
	 * This is an external tempo change, triggered
	 * via a midi or osc message.
	 *
	 * Just update the GUI using the current tempo
	 * of the song.
	 */
	
	m_pLCDBPMSpinbox->setValue( m_pEngine->getSong()->__bpm );
}

//::::::::::::::::::::::::::::::::::::::::::::::::

const char* MetronomeWidget::__class_name = "MetronomeWidget";

MetronomeWidget::MetronomeWidget(QWidget *pParent)
 : QWidget( pParent )
 , Object( __class_name )
 , m_nValue( 0 )
 , m_state( METRO_OFF )
{
//	INFOLOG( "INIT" );
	HydrogenApp::get_instance()->addEventListener( this );

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
	if (nValue == 2) // 2 = set pattern position is not needed here
		return;

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




