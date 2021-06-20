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
#include "CommonStrings.h"
#include "PlayerControl.h"
#include "InstrumentRack.h"
#include "HydrogenApp.h"

#include "Widgets/ClickableLabel.h"
#include "Widgets/LCDDisplay.h"
#include "Widgets/LCDSpinBox.h"
#include "Widgets/LED.h"
#include "Widgets/Button.h"
#include "Widgets/CpuLoadWidget.h"
#include "Widgets/PixmapWidget.h"

#include "Mixer/Mixer.h"
#include "SongEditor/SongEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"

#include <core/Hydrogen.h>
#include <core/AudioEngine.h>
#include <core/IO/JackAudioDriver.h>
#include <core/EventQueue.h>
using namespace H2Core;


//beatconter global
int bcDisplaystatus = 0;
//~ beatcounter

const char* PlayerControl::__class_name = "PlayerControl";

PlayerControl::PlayerControl(QWidget *parent)
 : QLabel(parent)
 , Object( __class_name )
 , m_midiActivityTimeout( 250 )
{
	setObjectName( "PlayerControl" );
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
	pControlsPanel->setObjectName( "ControlsPanel" );
	hbox->addWidget( pControlsPanel );

	m_pTimeDisplay = new LCDDisplay( pControlsPanel, QSize( 146, 22 ), true );
	m_pTimeDisplay->move( 13, 7 );
	m_pTimeDisplay->setAlignment( Qt::AlignRight );
	m_pTimeDisplay->setText( "00:00:00:0000" );
	m_pTimeDisplay->setStyleSheet( "font-size: 17px;" );

	m_pTimeHoursLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getTimeHoursLabel(), ClickableLabel::Color::LCD );
	m_pTimeHoursLbl->move( 24, 30 );
	m_pTimeMinutesLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getTimeMinutesLabel(), ClickableLabel::Color::LCD );
	m_pTimeMinutesLbl->move( 51, 30 );
	m_pTimeSecondsLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getTimeSecondsLabel(), ClickableLabel::Color::LCD );
	m_pTimeSecondsLbl->move( 79, 30 );
	m_pTimeMilliSecondsLbl = new ClickableLabel( pControlsPanel, QSize( 34, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getTimeMilliSecondsLabel(), ClickableLabel::Color::LCD );
	m_pTimeMilliSecondsLbl->move( 116, 30 );

	// Rewind button
	m_pRwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "rewind.svg", "", false, QSize( 13, 13 ), tr("Rewind") );
	m_pRwdBtn->move( 166, 15 );
	connect(m_pRwdBtn, SIGNAL( pressed() ), this, SLOT( rewindBtnClicked() ));
	Action* pAction = new Action("<<_PREVIOUS_BAR");
	m_pRwdBtn->setAction( pAction );

	// Record button
	m_pRecBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle, "record.svg", "", false, QSize( 11, 11 ), tr("Record"), true );
	m_pRecBtn->move( 193, 15 );
	m_pRecBtn->setChecked(false);
	m_pRecBtn->setHidden(false);
	connect(m_pRecBtn, SIGNAL( pressed() ), this, SLOT( recBtnClicked() ));
	pAction = new Action("RECORD_READY");
	m_pRecBtn->setAction( pAction );

	// Play button
	m_pPlayBtn = new Button( pControlsPanel, QSize( 30, 21 ), Button::Type::Toggle, "play_pause.svg", "", false, QSize( 30, 21 ), tr("Play/ Pause") );
	m_pPlayBtn->move( 220, 15 );
	m_pPlayBtn->setChecked(false);
	connect(m_pPlayBtn, SIGNAL( pressed() ), this, SLOT( playBtnClicked() ));
	pAction = new Action("PLAY/PAUSE_TOGGLE");
	m_pPlayBtn->setAction( pAction );

	// Stop button
	m_pStopBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "stop.svg", "", false, QSize( 11, 11 ), tr("Stop") );
	m_pStopBtn->move( 252, 15 );
	connect(m_pStopBtn, SIGNAL( pressed() ), this, SLOT( stopBtnClicked() ));
	pAction = new Action("STOP");
	m_pStopBtn->setAction( pAction );

	// Fast forward button
	m_pFfwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "fast_forward.svg", "", false, QSize( 13, 13 ), tr("Fast Forward") );
	m_pFfwdBtn->move( 279, 15 );
	connect(m_pFfwdBtn, SIGNAL( pressed() ), this, SLOT( fastForwardBtnClicked() ));
	pAction = new Action(">>_NEXT_BAR");
	m_pFfwdBtn->setAction( pAction );

	// Loop song button button
	m_pSongLoopBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle, "loop.svg", "", false, QSize( 19, 15 ), tr("Loop song") );
	m_pSongLoopBtn->move( 308, 15);
	connect( m_pSongLoopBtn, SIGNAL( pressed() ), this, SLOT( songLoopBtnClicked() ) );

	// Live mode button
	m_pPatternModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pPatternModeLED->move( 179, 4 );
	m_pPatternModeLED->setActivated( true );
	m_pPatternModeBtn = new Button( pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getPatternModeButton(), false, QSize(), tr("Pattern Mode") );
	m_pPatternModeBtn->move( 190, 3 );
	m_pPatternModeBtn->setChecked(true);
	connect(m_pPatternModeBtn, SIGNAL( pressed() ), this, SLOT( patternModeBtnClicked() ));

	// Song mode button
	m_pSongModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pSongModeLED->move( 252, 4 );
	m_pSongModeBtn = new Button( pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSongModeButton(), false, QSize(), tr("Song Mode") );
	m_pSongModeBtn->move( 263, 3 );
	m_pSongModeBtn->setChecked(false);
	connect(m_pSongModeBtn, SIGNAL( pressed() ), this, SLOT( songModeBtnClicked() ));

//~ CONTROLS

// BC on off
	QWidget *pControlsBBTBConoffPanel = new QWidget( nullptr );
	pControlsBBTBConoffPanel->setFixedSize( 15, 43 );
	pControlsBBTBConoffPanel->setObjectName( "BeatCounterOnOff" );
	hbox->addWidget( pControlsBBTBConoffPanel );

	m_sBCOnOffBtnToolTip = tr("BeatCounter Panel on");
	m_pBCOnOffBtn = new Button( pControlsBBTBConoffPanel, QSize( 12, 40 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getBeatCounterButton(), false, QSize(), m_sBCOnOffBtnToolTip );
	m_pBCOnOffBtn->move(1, 1);
	m_pBCOnOffBtn->setChecked(false);
	connect(m_pBCOnOffBtn, SIGNAL( pressed() ), this, SLOT( bcOnOffBtnClicked() ));
	pAction = new Action("BEATCOUNTER");
	m_pBCOnOffBtn->setAction( pAction );
//~  BC on off

//beatcounter
	m_pControlsBCPanel = new PixmapWidget( nullptr );
	m_pControlsBCPanel->setFixedSize( 86, 43 );
	m_pControlsBCPanel->setPixmap( "/playerControlPanel/beatConter_BG.png" );
	m_pControlsBCPanel->setObjectName( "BeatCounter" );
	hbox->addWidget( m_pControlsBCPanel );


	m_pBCDisplayZ = new QLabel( m_pControlsBCPanel );
	m_pBCDisplayZ->resize( QSize( 23, 13 ) );
	m_pBCDisplayZ->move( 45, 8 );
	m_pBCDisplayZ->setText( "--" );

	QLabel* pLabelBC1 = new QLabel( m_pControlsBCPanel );
	pLabelBC1->resize( QSize( 9, 11 ) );
	pLabelBC1->move( 25, 9 );
	pLabelBC1->setText( "1" );
	QLabel* pLabelBC2 = new QLabel( m_pControlsBCPanel );
	pLabelBC2->resize( QSize( 9, 3 ) );
	pLabelBC2->move( 25, 20 );
	pLabelBC2->setText( "—" );
	
	m_pBCDisplayT = new QLabel( m_pControlsBCPanel );
	m_pBCDisplayT->resize( QSize( 9, 11 ) );
	m_pBCDisplayT->move( 25, 25 );
	m_pBCDisplayT->setText( "4" );

	m_pBCDisplayB = new QLabel( m_pControlsBCPanel );
	m_pBCDisplayB->resize( QSize( 23, 11 ) );
	m_pBCDisplayB->move( 45, 25 );
	m_pBCDisplayB->setText( "04" );

	m_pBCTUpBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "plus.svg", "", false, QSize( 10, 10 ) );
	m_pBCTUpBtn->move( 2, 3 );
	connect( m_pBCTUpBtn, SIGNAL( pressed() ), this, SLOT( bctUpButtonClicked() ) );

	m_pBCTDownBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "minus.svg", "", false, QSize( 10, 10 ) );
	m_pBCTDownBtn->move( 2, 14 );
	connect( m_pBCTDownBtn, SIGNAL( pressed() ), this, SLOT( bctDownButtonClicked() ) );

	m_pBCBUpBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "plus.svg", "", false, QSize( 10, 10 ) );
	m_pBCBUpBtn->move( 64, 3 );
	connect( m_pBCBUpBtn, SIGNAL( pressed() ), this, SLOT( bcbUpButtonClicked() ) );

	m_pBCBDownBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "minus.svg", "", false, QSize( 10, 10 ) );
	m_pBCBDownBtn->move( 64, 14 );
	connect( m_pBCBDownBtn, SIGNAL( pressed() ), this, SLOT( bcbDownButtonClicked() ) );

	m_pBCSetPlayBtn = new Button( m_pControlsBCPanel, QSize( 19, 15 ), Button::Type::Push, "", HydrogenApp::get_instance()->getCommonStrings()->getBeatCounterSetPlayButtonOff(), false, QSize(), tr("Set BPM / Set BPM and play") );
	m_pBCSetPlayBtn->move( 64, 25 );
	connect(m_pBCSetPlayBtn, SIGNAL( pressed() ), this, SLOT( bcSetPlayBtnClicked() ));
//~ beatcounter


// BPM
	PixmapWidget *pBPMPanel = new PixmapWidget( nullptr );
	pBPMPanel->setFixedSize( 145, 43 );
	pBPMPanel->setPixmap( "/playerControlPanel/background_BPM.png" );
	pBPMPanel->setObjectName( "BPM" );
	hbox->addWidget( pBPMPanel );
	m_pBPMLbl = new ClickableLabel( pBPMPanel, QSize( 26, 10 ), HydrogenApp::get_instance()->getCommonStrings()->getBPMLabel(), ClickableLabel::Color::Dark );
	m_pBPMLbl->move( 36, 31 );

	// LCD BPM SpinBox
	m_pLCDBPMSpinbox = new LCDSpinBox( pBPMPanel, QSize( 95, 30), LCDSpinBox::Type::Double, static_cast<double>( MIN_BPM ), static_cast<double>( MAX_BPM ) );
	m_pLCDBPMSpinbox->move( 36, 1 );
	m_pLCDBPMSpinbox->setStyleSheet( "font-size: 16px;" );
	connect( m_pLCDBPMSpinbox, SIGNAL( valueChanged( double ) ), this, SLOT( bpmChanged( double ) ) );

	m_pRubberBPMChange = new Button( pBPMPanel, QSize( 12, 40 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getRubberbandButton(), false, QSize(), tr("Recalculate Rubberband modified samples if bpm will change") );

	m_pRubberBPMChange->move( 131, 1 );
	m_pRubberBPMChange->setChecked( pPreferences->getRubberBandBatchMode());
	connect( m_pRubberBPMChange, SIGNAL( pressed() ), this, SLOT( rubberbandButtonToggle() ) );
	QString program = pPreferences->m_rubberBandCLIexecutable;
	//test the path. if test fails, no button
	if ( QFile( program ).exists() == false) {
		m_pRubberBPMChange->hide();
	}

	m_pMetronomeLED = new MetronomeLED( pBPMPanel, QSize( 22, 7 ) );
	m_pMetronomeLED->move( 7, 32 );

	m_pMetronomeBtn = new Button( pBPMPanel, QSize( 24, 28 ), Button::Type::Toggle, "metronome.svg", "", false, QSize( 24, 24 ), tr("Switch metronome on/off") );
	m_pMetronomeBtn->move( 7, 2 );
	connect( m_pMetronomeBtn, SIGNAL( pressed() ), this, SLOT( metronomeButtonClicked() ) );
		pAction = new Action("TOGGLE_METRONOME");
		m_pMetronomeBtn->setAction( pAction );

//~ BPM


// JACK
	PixmapWidget *pJackPanel = new PixmapWidget( nullptr );
	pJackPanel->setFixedSize( 113, 43 );
	pJackPanel->setPixmap( "/playerControlPanel/background_Jack.png" );
	pJackPanel->setObjectName( "JackPanel" );
	hbox->addWidget( pJackPanel );

	// Jack transport mode button
	
	m_pJackTransportBtn = new Button( pJackPanel, QSize( 53, 16 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getJackTransportButton(), false, QSize(), tr("JACK transport on/off") );
	m_pJackTransportBtn->hide();
	if ( pPreferences->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT ) {
		m_pJackTransportBtn->setChecked( true );
	} else {
		m_pJackTransportBtn->setChecked( false );
	}
	connect(m_pJackTransportBtn, SIGNAL( pressed() ), this, SLOT( jackTransportBtnClicked() ));
	m_pJackTransportBtn->move( 3, 24 );

	//jack time master
	m_sJackMasterModeToolTip = tr("JACK Timebase master on/off");
	m_pJackMasterBtn = new Button( pJackPanel, QSize( 53, 16 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getJackMasterButton(), false, QSize(), m_sJackMasterModeToolTip );
	m_pJackMasterBtn->hide();
	if ( m_pJackTransportBtn->isChecked() &&
		 pPreferences->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER &&
		 pPreferences->m_bJackTimebaseEnabled ) {
		m_pJackMasterBtn->setChecked( true );
	} else {
		m_pJackMasterBtn->setChecked( false );
	}
	connect(m_pJackMasterBtn, SIGNAL( pressed() ), this, SLOT( jackMasterBtnClicked() ));
	m_pJackMasterBtn->move( 56, 24 );
	//~ jack time master

	m_pHydrogen = Hydrogen::get_instance();

	// CPU load widget
	m_pCpuLoadWidget = new CpuLoadWidget( pJackPanel );
	m_pCpuLoadWidget->setObjectName( "CpuLoadWidget" );

	// Midi Activity widget
	m_pMidiActivityLED = new LED( pJackPanel, QSize( 11, 9 ) );
	m_pMidiActivityLED->setObjectName( "MidiActivityLED" );
	m_pMidiActivityTimer = new QTimer( this );
	connect( m_pMidiActivityTimer, SIGNAL( timeout() ), this, SLOT( deactivateMidiActivityLED() ) );
	m_pMidiInLbl = new ClickableLabel( pJackPanel, QSize( 45, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getMidiInLabel() );
	m_pMidiInLbl->move( 22, 14 );
	m_pCpuLbl = new ClickableLabel( pJackPanel, QSize( 30, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getCpuLabel() );
	m_pCpuLbl->move( 71, 14 );

	m_pMidiActivityLED->move( 11, 14 );
	m_pCpuLoadWidget->move( 10, 4 );
//~ JACK


	QWidget *pLcdBackGround = new QWidget( nullptr );
	pLcdBackGround->setFixedSize( 256, 43 );
	pLcdBackGround->setObjectName( "LcdBackground" );
	hbox->addWidget( pLcdBackGround );

	m_pShowMixerBtn = new Button( pLcdBackGround, QSize( 88, 23 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getMixerButton(), false, QSize(), tr( "Show mixer" ) );
	m_pShowMixerBtn->move( 0, 0 );
	connect(m_pShowMixerBtn, SIGNAL( pressed() ), this, SLOT( showMixerButtonClicked() ));

	m_pShowInstrumentRackBtn = new Button( pLcdBackGround, QSize( 168, 23 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getInstrumentRackButton(), false, QSize(), tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->move( 88, 0 );
	connect( m_pShowInstrumentRackBtn, SIGNAL( pressed() ), this, SLOT( showInstrumentRackButtonClicked() ) );

	m_pStatusLabel = new LCDDisplay(pLcdBackGround, QSize( 255, 18 ) );
	m_pStatusLabel->setMaxLength( 37 );
	m_pStatusLabel->move( 0, 24 );

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

	if ( ! m_pShowMixerBtn->isDown() ) {
		m_pShowMixerBtn->setChecked( pH2App->getMixer()->isVisible() );
	}
	if ( ! m_pShowInstrumentRackBtn->isDown() ) {
		m_pShowInstrumentRackBtn->setChecked( pH2App->getInstrumentRack()->isVisible() );
	}

	int state = m_pHydrogen->getState();
	if ( ! m_pPlayBtn->isDown() ) {
		if (state == STATE_PLAYING ) {
			m_pPlayBtn->setChecked(true);
		}
		else {
			m_pPlayBtn->setChecked(false);
		}
	}

	if ( ! m_pRecBtn->isDown() ) {
		if (pPref->getRecordEvents()) {
			m_pRecBtn->setChecked(true);
		}
		else {
			m_pRecBtn->setChecked(false);
		}
	}

	Song *song = m_pHydrogen->getSong();

	if ( ! m_pSongLoopBtn->isDown() ) {
		m_pSongLoopBtn->setChecked( song->getIsLoopEnabled() );
	}

	m_pLCDBPMSpinbox->setValue( song->getBpm() );

	if ( song->getMode() == Song::PATTERN_MODE ) {
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked( true );
		}
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked( false );
		}
		m_pPatternModeLED->setActivated( true );
		m_pSongModeLED->setActivated( false );
	}
	else {
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked( false );
		}
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked( true );
		}
		m_pPatternModeLED->setActivated( false );
		m_pSongModeLED->setActivated( true );
	}

	//beatcounter
	if ( pPref->m_bbc == Preferences::BC_OFF ) {
		m_pControlsBCPanel->hide();
		if ( ! m_pBCOnOffBtn->isDown() ) {
			m_pBCOnOffBtn->setChecked(false);
		}
	}else
	{
		m_pControlsBCPanel->show();
		if ( ! m_pBCOnOffBtn->isDown() ) {
			m_pBCOnOffBtn->setChecked(true);
		}
	}

	if ( ! m_pBCSetPlayBtn->isDown() ) {
		if ( pPref->m_mmcsetplay ==  Preferences::SET_PLAY_OFF) {
			m_pBCSetPlayBtn->setChecked(false);
		}else
			{
				m_pBCSetPlayBtn->setChecked(true);
			}
	}
	//~ beatcounter




	if ( m_pHydrogen->haveJackAudioDriver() ) {
		m_pJackTransportBtn->show();
		m_pJackMasterBtn->show();
		
		switch ( pPref->m_bJackTransportMode ) {
			case Preferences::NO_JACK_TRANSPORT:
				if ( ! m_pJackTransportBtn->isDown() ) {
					m_pJackTransportBtn->setChecked(false);
				}
				if ( ! m_pJackMasterBtn->isDown() ) {
					m_pJackMasterBtn->setChecked(false);
				}
				break;

			case Preferences::USE_JACK_TRANSPORT:
				if ( ! m_pJackTransportBtn->isDown() ) {
					m_pJackTransportBtn->setChecked(true);
				}

				if ( ! m_pJackMasterBtn->isDown() ) {
					if ( m_pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Master ) {
						m_pJackMasterBtn->setChecked( true );
					} else {
						m_pJackMasterBtn->setChecked( false );
					}
				}

				if ( pPref->m_bJackTimebaseEnabled ) {
					m_pJackMasterBtn->setDisabled( false );
					m_pJackMasterBtn->setBaseToolTip( m_sJackMasterModeToolTip );
				} else {
					m_pJackMasterBtn->setDisabled( true );
					m_pJackMasterBtn->setBaseToolTip( tr( "JACK timebase support is disabled in the Preferences" ) );
				}

				if ( m_pHydrogen->getJackTimebaseState() == JackAudioDriver::Timebase::Slave ) {
					QString sTBMToolTip( tr( "In the presence of an external JACK Timebase master the tempo can not be altered from within Hydrogen" ) );
					if ( ! m_pBCOnOffBtn->isDown() ) {
						m_pBCOnOffBtn->setChecked( false );
					}
					m_pBCOnOffBtn->setDisabled( true );
					m_pBCOnOffBtn->setBaseToolTip( sTBMToolTip );
					m_pControlsBCPanel->hide();
					pPref->m_bbc = Preferences::BC_OFF;
					m_pLCDBPMSpinbox->setDisabled( true );
					m_pLCDBPMSpinbox->setToolTip( sTBMToolTip );
					
				} else {
					m_pBCOnOffBtn->setDisabled( false );
					m_pBCOnOffBtn->setBaseToolTip( m_sBCOnOffBtnToolTip );
					m_pLCDBPMSpinbox->setDisabled( false );
					m_pLCDBPMSpinbox->setToolTip( "" );
				}
				
				break;
		}

	}
	else {
		m_pJackTransportBtn->hide();
		m_pJackMasterBtn->hide();
	}

	// time
	float fSeconds = m_pHydrogen->getAudioEngine()->getElapsedTime();
	
	int nMSec = (int)( (fSeconds - (int)fSeconds) * 1000.0 );
	int nSeconds = ( (int)fSeconds ) % 60;
	int nMins = (int)( fSeconds / 60.0 ) % 60;
	int nHours = (int)( fSeconds / 3600.0 );

	char tmp[100];
	QString sTime;

	if ( nHours  < 10 ) {
		sTime.append( "0" );
	}
	sTime.append( QString::number( nHours ) + ":");
	if ( nMins  < 10 ) {
		sTime.append( "0" );
	}
	sTime.append( QString::number( nMins ) + ":" );
	if ( nSeconds  < 10 ) {
		sTime.append( "0" );
	}
	sTime.append( QString::number( nSeconds ) + ":" );
	if ( nMSec  < 10 ) {
		sTime.append( "000" );
	} else if ( nMSec < 100 ) {
		sTime.append( "00" );
	} else if ( nMSec < 1000 ) {
		sTime.append( "0" );
	}
	sTime.append( QString::number( nMSec ) );

	m_pTimeDisplay->setText( sTime );

	if ( ! m_pMetronomeBtn->isDown() ) {
		m_pMetronomeBtn->setChecked(pPref->m_bUseMetronome);
	}


	//beatcounter get BC message
	char bcstatus[4];
	int beatstocountondisplay = 1;
	beatstocountondisplay = m_pHydrogen->getBcStatus();

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
void PlayerControl::recBtnClicked() {
	if ( m_pHydrogen->getState() != STATE_PLAYING ) {
		if ( ! m_pRecBtn->isChecked() ) {
			Preferences::get_instance()->setRecordEvents(true);
			(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Record midi events = On" ), 2000 );
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
			(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Record midi events = Off" ), 2000 );
		}
	}
}

/// Start audio engine
void PlayerControl::playBtnClicked() {
	if ( ! m_pPlayBtn->isChecked() ) {
		m_pHydrogen->sequencer_play();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Playing."), 5000);
	}
	else {
		m_pHydrogen->sequencer_stop();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Pause."), 5000);
	}
}

/// Stop audio engine
void PlayerControl::stopBtnClicked()
{
	auto pHydrogen = Hydrogen::get_instance();
	if ( ! m_pPlayBtn->isDown() ) {
		m_pPlayBtn->setChecked(false);
	}
	pHydrogen->sequencer_stop();
	pHydrogen->getCoreActionController()->relocate( 0 );
	(HydrogenApp::get_instance())->setStatusBarMessage(tr("Stopped."), 5000);
}

void PlayerControl::midiActivityEvent() {
	m_pMidiActivityTimer->stop();
	m_pMidiActivityLED->setActivated( true );
	m_pMidiActivityTimer->start( m_midiActivityTimeout );
}

void PlayerControl::deactivateMidiActivityLED() {
	m_pMidiActivityTimer->stop();
	m_pMidiActivityLED->setActivated( false );
}


/// Set Song mode
void PlayerControl::songModeBtnClicked()
{
	Hydrogen::get_instance()->getCoreActionController()->activateSongMode( true, false );
	songModeActivationEvent( 1 );
}


///Set Live mode
void PlayerControl::patternModeBtnClicked()
{
	Hydrogen::get_instance()->getCoreActionController()->activateSongMode( false, false );
	songModeActivationEvent( 0 );
}

void PlayerControl::songModeActivationEvent( int nValue )
{
	if ( nValue != 0 ) {
		m_pPatternModeLED->setActivated( false );
		m_pSongModeLED->setActivated( true );
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked( true );
		}
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked(false);
		}
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Song mode selected."), 5000);
	} else {
		m_pPatternModeLED->setActivated( true );
		m_pSongModeLED->setActivated( false );
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked(false);
		}
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked(true);
		}
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Pattern mode selected."), 5000);
	}
}

void PlayerControl::bpmChanged( double fNewBpmValue ) {
	m_pHydrogen->getSong()->setIsModified( true );

	m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
	m_pHydrogen->setBPM( static_cast<float>( fNewBpmValue ) );
	m_pHydrogen->getAudioEngine()->unlock();
}



//beatcounter
void PlayerControl::bcOnOffBtnClicked()
{
	Preferences *pPref = Preferences::get_instance();
	if ( ! m_pBCOnOffBtn->isChecked() ) {
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

void PlayerControl::bcSetPlayBtnClicked()
{
	Preferences *pPref = Preferences::get_instance();
	if ( m_pBCSetPlayBtn->text() == HydrogenApp::get_instance()->getCommonStrings()->getBeatCounterSetPlayButtonOff() ) {
		pPref->m_mmcsetplay = Preferences::SET_PLAY_ON;
		m_pBCSetPlayBtn->setText( HydrogenApp::get_instance()->getCommonStrings()->getBeatCounterSetPlayButtonOn() );
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Count BPM and start PLAY"), 5000);
	}
	else {
		pPref->m_mmcsetplay = Preferences::SET_PLAY_OFF;
		m_pBCSetPlayBtn->setText( HydrogenApp::get_instance()->getCommonStrings()->getBeatCounterSetPlayButtonOff() );
		(HydrogenApp::get_instance())->setStatusBarMessage(tr(" Count and set BPM"), 5000);
	}
}

void PlayerControl::rubberbandButtonToggle()
{
	Preferences *pPref = Preferences::get_instance();
	if ( ! m_pRubberBPMChange->isChecked() ) {
		EventQueue::get_instance()->push_event( EVENT_RECALCULATERUBBERBAND, -1);
		pPref->setRubberBandBatchMode(true);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Recalculate all samples using Rubberband ON"), 2000);
	}
	else {
		pPref->setRubberBandBatchMode(false);
		(HydrogenApp::get_instance())->setScrollStatusBarMessage(tr("Recalculate all samples using Rubberband OFF"), 2000);
	}
}


void PlayerControl::bcbUpButtonClicked()
{
	int tmp = m_pHydrogen->getbeatsToCount();
	char tmpb[3];

	tmp ++;
	if (tmp > 16) {
		tmp = 2;
	}
	
	sprintf(tmpb, "%02d", tmp );
	m_pBCDisplayB->setText( QString( tmpb ) );
	m_pHydrogen->setbeatsToCount( tmp );
}

void PlayerControl::bcbDownButtonClicked()
{
	int tmp = m_pHydrogen->getbeatsToCount();
	char tmpb[3];
	tmp --;
	if (tmp < 2 ) {
		tmp = 16;
	}
	sprintf(tmpb, "%02d", tmp );
	m_pBCDisplayB->setText( QString( tmpb ) );
	m_pHydrogen->setbeatsToCount( tmp );
}

void PlayerControl::bctUpButtonClicked()
{
	float tmp = m_pHydrogen->getNoteLength() * 4;

	tmp = tmp / 2 ;
	if (tmp < 1) {
		tmp = 8;
	}

	m_pBCDisplayT->setText( QString::number( tmp ) );
	m_pHydrogen->setNoteLength( (tmp) / 4 );
}

void PlayerControl::bctDownButtonClicked()
{
	float tmp = m_pHydrogen->getNoteLength() * 4;

	tmp = tmp * 2;
	if (tmp > 8 ) {
		tmp = 1;
	}
	m_pBCDisplayT->setText( QString::number(tmp) );
	m_pHydrogen->setNoteLength( (tmp) / 4 );
}
//~ beatcounter



void PlayerControl::jackTransportBtnClicked()
{
	Preferences *pPref = Preferences::get_instance();

	if ( !m_pHydrogen->haveJackAudioDriver() ) {
		QMessageBox::warning( this, "Hydrogen", tr( "JACK-transport will work only with JACK driver." ) );
		return;
	}

	if ( ! m_pJackTransportBtn->isChecked() ) {
		m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pPref->m_bJackTransportMode = Preferences::USE_JACK_TRANSPORT;
		m_pHydrogen->getAudioEngine()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Jack-transport mode = On"), 5000);
		m_pJackMasterBtn->setDisabled( false );
	}
	else {
		m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pPref->m_bJackTransportMode = Preferences::NO_JACK_TRANSPORT;
		m_pHydrogen->getAudioEngine()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("Jack-transport mode = Off"), 5000);
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( false );
		}
		m_pJackMasterBtn->setDisabled( true );
	}
}


//jack time master
void PlayerControl::jackMasterBtnClicked()
{
#ifdef H2CORE_HAVE_JACK
	Preferences *pPref = Preferences::get_instance();

	if ( !m_pHydrogen->haveJackTransport() ) {
		QMessageBox::warning( this, "Hydrogen", tr( "JACK transport will work only with JACK driver." ) );
		return;
	}

	if ( ! m_pJackMasterBtn->isChecked() ) {
		m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pPref->m_bJackMasterMode = Preferences::USE_JACK_TIME_MASTER;
		m_pHydrogen->getAudioEngine()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK Timebase master mode = On"), 5000);
		Hydrogen::get_instance()->onJackMaster();

	} else {
		m_pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pPref->m_bJackMasterMode = Preferences::NO_JACK_TIME_MASTER;
		m_pHydrogen->getAudioEngine()->unlock();
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK Timebase master mode = Off"), 5000);
		Hydrogen::get_instance()->offJackMaster();
	}
	HydrogenApp::get_instance()->getSongEditorPanel()->updateTimelineUsage();
#endif
}
//~ jack time master

void PlayerControl::fastForwardBtnClicked()
{
	DEBUGLOG( "relocate via button press" );

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() + 1 );
}



void PlayerControl::rewindBtnClicked()
{
	DEBUGLOG( "relocate via button press" );
	
	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() - 1 );
}


void PlayerControl::songLoopBtnClicked()
{
	Hydrogen::get_instance()->getCoreActionController()->activateLoopMode( ! m_pSongLoopBtn->isChecked(), false );

	if ( ! m_pSongLoopBtn->isChecked() ){
		loopModeActivationEvent( 1 );
	} else {
		loopModeActivationEvent( 0 );
	}
}

void PlayerControl::loopModeActivationEvent( int nValue ) {

	if ( nValue == 0 ) {
		if ( ! m_pSongLoopBtn->isDown() ) {
			m_pSongLoopBtn->setChecked( false );
		}
		HydrogenApp::get_instance()->setStatusBarMessage(tr("Loop song = Off"), 5000);
	}
	else {
		if ( ! m_pSongLoopBtn->isDown() ) {
			m_pSongLoopBtn->setChecked( true );
		}
		HydrogenApp::get_instance()->setStatusBarMessage(tr("Loop song = On"), 5000);
	}
}

void PlayerControl::metronomeButtonClicked()
{
	Hydrogen*	pHydrogen = Hydrogen::get_instance();
	CoreActionController* pController = pHydrogen->getCoreActionController();
	
	pController->setMetronomeIsActive( ! m_pMetronomeBtn->isChecked() );
}

void PlayerControl::showMixerButtonClicked()
{
	HydrogenApp *pH2App = HydrogenApp::get_instance();
	bool isVisible = pH2App->getMixer()->isVisible();
	pH2App->showMixer( !isVisible );
}

void PlayerControl::showInstrumentRackButtonClicked()
{
	HydrogenApp *pH2App = HydrogenApp::get_instance();
	bool isVisible = pH2App->getInstrumentRack()->isVisible();
	pH2App->showInstrumentPanel( isVisible );
}

void PlayerControl::showMessage( const QString& msg, int msec )
{
	if ( m_pScrollTimer->isActive ()) {
		m_pScrollTimer->stop(); 
	}
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
	int lwl = 34;
	int msgLength = m_pScrollMessage.length();
	if ( msgLength > lwl) {
		m_pScrollMessage = m_pScrollMessage.right( msgLength - 1 ); 
	}
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
	
	m_pLCDBPMSpinbox->setValue( m_pHydrogen->getSong()->getBpm() );
}

void PlayerControl::jackTransportActivationEvent( int nValue ) {

	if ( nValue == 0 && m_pJackTransportBtn->isChecked() ){
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK transport mode = Off"), 5000);
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( false );
		}
		m_pJackMasterBtn->setDisabled( true );
	} else if ( nValue != 0 && !m_pJackTransportBtn->isChecked() ) {
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK transport mode = On"), 5000);
		m_pJackMasterBtn->setDisabled( false );
	}
}

void PlayerControl::jackTimebaseActivationEvent( int nValue ) {
	if ( nValue == 0 && m_pJackMasterBtn->isChecked() ){
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK Timebase master mode = Off"), 5000);
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( false );
		}
		
	} else if ( nValue != 0 && !m_pJackMasterBtn->isChecked() ) {
		(HydrogenApp::get_instance())->setStatusBarMessage(tr("JACK Timebase master mode = On"), 5000);
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( true );
		}
	}
	
	HydrogenApp::get_instance()->getSongEditorPanel()->updateTimelineUsage();
}
