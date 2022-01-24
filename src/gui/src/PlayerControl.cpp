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
 * along with this program. If not, see 
https://www.gnu.org/licenses
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
#include <core/Basics/Song.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/IO/JackAudioDriver.h>
#include <core/EventQueue.h>
using namespace H2Core;


//beatconter global
int bcDisplaystatus = 0;
//~ beatcounter

PlayerControl::PlayerControl(QWidget *parent)
 : QLabel(parent)
 , m_midiActivityTimeout( 125 )
{
	setObjectName( "PlayerControl" );
	HydrogenApp::get_instance()->addEventListener( this );

	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
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
	m_pTimeDisplay->setText( "00:00:00:000" );
	m_pTimeDisplay->setStyleSheet( "font-size: 19px;" );

	m_pTimeHoursLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ),
										  pCommonStrings->getTimeHoursLabel() );
	m_pTimeHoursLbl->move( 22, 30 );
	m_pTimeMinutesLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ),
											pCommonStrings->getTimeMinutesLabel() );
	m_pTimeMinutesLbl->move( 53, 30 );
	m_pTimeSecondsLbl = new ClickableLabel( pControlsPanel, QSize( 33, 9 ),
											pCommonStrings->getTimeSecondsLabel() );
	m_pTimeSecondsLbl->move( 83, 30 );
	m_pTimeMilliSecondsLbl = new ClickableLabel( pControlsPanel, QSize( 34, 9 ),
												 pCommonStrings->getTimeMilliSecondsLabel() );
	m_pTimeMilliSecondsLbl->move( 119, 30 );

	// Rewind button
	m_pRwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push,
							"rewind.svg", "", false, QSize( 13, 13 ), tr("Rewind") );
	m_pRwdBtn->move( 166, 15 );
	connect(m_pRwdBtn, SIGNAL( pressed() ), this, SLOT( rewindBtnClicked() ));
	std::shared_ptr<Action> pAction = std::make_shared<Action>("<<_PREVIOUS_BAR");
	m_pRwdBtn->setAction( pAction );

	// Record button
	m_pRecBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle,
							"record.svg", "", false, QSize( 11, 11 ), tr("Record"), true );
	m_pRecBtn->move( 193, 15 );
	m_pRecBtn->setChecked(false);
	m_pRecBtn->setHidden(false);
	connect(m_pRecBtn, SIGNAL( pressed() ), this, SLOT( recBtnClicked() ));
	pAction = std::make_shared<Action>("RECORD_READY");
	m_pRecBtn->setAction( pAction );

	// Play button
	m_pPlayBtn = new Button( pControlsPanel, QSize( 30, 21 ), Button::Type::Toggle,
							 "play_pause.svg", "", false, QSize( 30, 21 ), tr("Play/ Pause") );
	m_pPlayBtn->move( 220, 15 );
	m_pPlayBtn->setChecked(false);
	connect(m_pPlayBtn, SIGNAL( pressed() ), this, SLOT( playBtnClicked() ));
	pAction = std::make_shared<Action>("PLAY/PAUSE_TOGGLE");
	m_pPlayBtn->setAction( pAction );

	// Stop button
	m_pStopBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push,
							 "stop.svg", "", false, QSize( 11, 11 ), tr("Stop") );
	m_pStopBtn->move( 252, 15 );
	connect(m_pStopBtn, SIGNAL( pressed() ), this, SLOT( stopBtnClicked() ));
	pAction = std::make_shared<Action>("STOP");
	m_pStopBtn->setAction( pAction );

	// Fast forward button
	m_pFfwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push,
							 "fast_forward.svg", "", false, QSize( 13, 13 ), tr("Fast Forward") );
	m_pFfwdBtn->move( 279, 15 );
	connect(m_pFfwdBtn, SIGNAL( pressed() ), this, SLOT( fastForwardBtnClicked() ));
	pAction = std::make_shared<Action>(">>_NEXT_BAR");
	m_pFfwdBtn->setAction( pAction );

	// Loop song button button
	m_pSongLoopBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle,
								 "loop.svg", "", false, QSize( 17, 13 ), tr("Loop song") );
	m_pSongLoopBtn->move( 308, 15);
	connect( m_pSongLoopBtn, SIGNAL( pressed() ), this, SLOT( songLoopBtnClicked() ) );

	// Live mode button
	m_pPatternModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pPatternModeLED->move( 179, 4 );
	m_pPatternModeLED->setActivated( true );
	m_pPatternModeBtn = new Button( pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle,
									"", pCommonStrings->getPatternModeButton(), false, QSize(),
									tr("Pattern Mode") );
	m_pPatternModeBtn->move( 190, 3 );
	m_pPatternModeBtn->setChecked(true);
	connect(m_pPatternModeBtn, SIGNAL( pressed() ), this, SLOT( patternModeBtnClicked() ));

	// Song mode button
	m_pSongModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pSongModeLED->move( 252, 4 );
	m_pSongModeBtn = new Button( pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle,
								 "", pCommonStrings->getSongModeButton(), false, QSize(),
								 tr("Song Mode") );
	m_pSongModeBtn->move( 263, 3 );
	m_pSongModeBtn->setChecked(false);
	connect(m_pSongModeBtn, SIGNAL( pressed() ), this, SLOT( songModeBtnClicked() ));

//~ CONTROLS

// BC on off
	QWidget *pControlsBBTBConoffPanel = new QWidget( nullptr );
	pControlsBBTBConoffPanel->setFixedSize( 15, 43 );
	pControlsBBTBConoffPanel->setObjectName( "BeatCounterOnOff" );
	hbox->addWidget( pControlsBBTBConoffPanel );

	m_sBCOnOffBtnToolTip = tr("Toggle the BeatCounter Panel");
	m_sBCOnOffBtnTimelineToolTip = tr( "Please deactivate the Timeline first in order to use the BeatCounter" );
	m_sBCOnOffBtnJackTimebaseToolTip = tr( "In the presence of an external JACK Timebase master the BeatCounter can not be used" );
	m_pBCOnOffBtn = new Button( pControlsBBTBConoffPanel, QSize( 13, 42 ), Button::Type::Toggle,
								"", pCommonStrings->getBeatCounterButton(), false, QSize(),
								m_sBCOnOffBtnToolTip );
	m_pBCOnOffBtn->move(0, 0);
	if ( pPref->m_bbc == Preferences::BC_ON ) {
		m_bLastBCOnOffBtnState = true;
	} else {
		m_bLastBCOnOffBtnState = false;
	}
	if ( pHydrogen->getTempoSource() != H2Core::Hydrogen::Tempo::Song ) {
		m_pBCOnOffBtn->setChecked( false );
		m_pBCOnOffBtn->setIsActive( false );
	} else {
		m_pBCOnOffBtn->setChecked( m_bLastBCOnOffBtnState );
	}
	updateBeatCounterToolTip();
		
	connect(m_pBCOnOffBtn, SIGNAL( pressed() ), this, SLOT( bcOnOffBtnClicked() ));
	pAction = std::make_shared<Action>("BEATCOUNTER");
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

	m_pBCTUpBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push,
							  "plus.svg", "", false, QSize( 8, 8 ) );
	m_pBCTUpBtn->move( 2, 3 );
	connect( m_pBCTUpBtn, SIGNAL( pressed() ), this, SLOT( bctUpButtonClicked() ) );

	m_pBCTDownBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push,
								"minus.svg", "", false, QSize( 8, 8 ) );
	m_pBCTDownBtn->move( 2, 14 );
	connect( m_pBCTDownBtn, SIGNAL( pressed() ), this, SLOT( bctDownButtonClicked() ) );

	m_pBCBUpBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push,
							  "plus.svg", "", false, QSize( 8, 8 ) );
	m_pBCBUpBtn->move( 64, 3 );
	connect( m_pBCBUpBtn, SIGNAL( pressed() ), this, SLOT( bcbUpButtonClicked() ) );

	m_pBCBDownBtn = new Button( m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push,
								"minus.svg", "", false, QSize( 8, 8 ) );
	m_pBCBDownBtn->move( 64, 14 );
	connect( m_pBCBDownBtn, SIGNAL( pressed() ), this, SLOT( bcbDownButtonClicked() ) );

	m_pBCSetPlayBtn = new Button( m_pControlsBCPanel, QSize( 19, 15 ), Button::Type::Push,
								  "", pCommonStrings->getBeatCounterSetPlayButtonOff(), false,
								  QSize(), tr("Set BPM / Set BPM and play") );
	m_pBCSetPlayBtn->move( 64, 25 );
	connect(m_pBCSetPlayBtn, SIGNAL( pressed() ), this, SLOT( bcSetPlayBtnClicked() ));
//~ beatcounter


// BPM
	m_sLCDBPMSpinboxToolTip = tr("Alter the Playback Speed");
	m_sLCDBPMSpinboxTimelineToolTip = tr( "While the Timeline is active this widget is in read-only mode and just displays the tempo set using the current Timeline position" );
	m_sLCDBPMSpinboxJackTimebaseToolTip = tr( "In the presence of an external JACK Timebase master this widget just displays the tempo broadcasted by JACK" );

	PixmapWidget *pBPMPanel = new PixmapWidget( nullptr );
	pBPMPanel->setFixedSize( 145, 43 );
	pBPMPanel->setPixmap( "/playerControlPanel/background_BPM.png" );
	pBPMPanel->setObjectName( "BPM" );
	hbox->addWidget( pBPMPanel );
	m_pBPMLbl = new ClickableLabel( pBPMPanel, QSize( 26, 10 ),
									pCommonStrings->getBPMLabel(), ClickableLabel::Color::Dark );
	m_pBPMLbl->move( 36, 31 );

	// LCD BPM SpinBox
	m_pLCDBPMSpinbox = new LCDSpinBox( pBPMPanel, QSize( 95, 30), LCDSpinBox::Type::Double,
									   static_cast<double>( MIN_BPM ),
									   static_cast<double>( MAX_BPM ) );
	m_pLCDBPMSpinbox->move( 36, 1 );
	m_pLCDBPMSpinbox->setStyleSheet( "font-size: 16px;" );
	connect( m_pLCDBPMSpinbox, SIGNAL( valueChanged( double ) ),
			 this, SLOT( bpmChanged( double ) ) );
	// initialize BPM widget
	m_pLCDBPMSpinbox->setIsActive( pHydrogen->getTempoSource() ==
								   H2Core::Hydrogen::Tempo::Song );
	updateBPMSpinboxToolTip();

	m_pRubberBPMChange = new Button( pBPMPanel, QSize( 13, 42 ), Button::Type::Toggle,
									 "", pCommonStrings->getRubberbandButton(), false, QSize(),
									 tr("Recalculate Rubberband modified samples if bpm will change") );

	m_pRubberBPMChange->move( 131, 0 );
	m_pRubberBPMChange->setChecked( pPref->getRubberBandBatchMode());
	connect( m_pRubberBPMChange, SIGNAL( pressed() ), this, SLOT( rubberbandButtonToggle() ) );
	QString program = pPref->m_rubberBandCLIexecutable;
	//test the path. if test fails, no button
	if ( QFile( program ).exists() == false) {
		m_pRubberBPMChange->hide();
	}

	m_pMetronomeLED = new MetronomeLED( pBPMPanel, QSize( 22, 7 ) );
	m_pMetronomeLED->move( 7, 32 );

	m_pMetronomeBtn = new Button( pBPMPanel, QSize( 24, 28 ), Button::Type::Toggle,
								  "metronome.svg", "", false, QSize( 20, 20 ),
								  tr("Switch metronome on/off") );
	m_pMetronomeBtn->move( 6, 2 );
	connect( m_pMetronomeBtn, SIGNAL( pressed() ), this, SLOT( metronomeButtonClicked() ) );
	pAction = std::make_shared<Action>("TOGGLE_METRONOME");
	m_pMetronomeBtn->setAction( pAction );

//~ BPM


// JACK
	PixmapWidget *pJackPanel = new PixmapWidget( nullptr );
	pJackPanel->setFixedSize( 113, 43 );
	pJackPanel->setPixmap( "/playerControlPanel/background_Jack.png" );
	pJackPanel->setObjectName( "JackPanel" );
	hbox->addWidget( pJackPanel );

	// Jack transport mode button

	/*: Using the JACK the audio/midi input and output ports of any
	  number of application can be connected.*/
	m_pJackTransportBtn = new Button( pJackPanel, QSize( 53, 16 ), Button::Type::Toggle,
									  "", pCommonStrings->getJackTransportButton(), false, QSize(),
									  tr("JACK transport on/off") );
	m_pJackTransportBtn->hide();
	if ( pPref->m_bJackTransportMode == Preferences::USE_JACK_TRANSPORT ) {
		m_pJackTransportBtn->setChecked( true );
	} else {
		m_pJackTransportBtn->setChecked( false );
	}
	connect(m_pJackTransportBtn, SIGNAL( pressed() ), this, SLOT( jackTransportBtnClicked() ));
	m_pJackTransportBtn->move( 3, 24 );

	//jack time master
	/*: Using the JACK Timebase Master functionality one of the
connected programs can broadcast both speed and measure information to
all other connected applications in order to have a more fine-grained
transport control.*/
	m_sJackMasterModeToolTip = tr("JACK Timebase master on/off");
	m_pJackMasterBtn = new Button( pJackPanel, QSize( 53, 16 ), Button::Type::Toggle,
								   "", pCommonStrings->getJackMasterButton(), false, QSize(),
								   m_sJackMasterModeToolTip );
	m_pJackMasterBtn->hide();
	if ( m_pJackTransportBtn->isChecked() &&
		 pPref->m_bJackMasterMode == Preferences::USE_JACK_TIME_MASTER &&
		 pPref->m_bJackTimebaseEnabled ) {
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
	connect( m_pMidiActivityTimer, SIGNAL( timeout() ),
			 this, SLOT( deactivateMidiActivityLED() ) );
	m_pMidiInLbl = new ClickableLabel( pJackPanel, QSize( 45, 9 ),
									   pCommonStrings->getMidiInLabel() );
	m_pMidiInLbl->move( 22, 14 );
	m_pCpuLbl = new ClickableLabel( pJackPanel, QSize( 30, 9 ),
									pCommonStrings->getCpuLabel() );
	m_pCpuLbl->move( 71, 14 );

	m_pMidiActivityLED->move( 11, 14 );
	m_pCpuLoadWidget->move( 10, 3 );
//~ JACK


	QWidget *pLcdBackGround = new QWidget( nullptr );
	pLcdBackGround->setFixedSize( 256, 43 );
	pLcdBackGround->setObjectName( "LcdBackground" );
	hbox->addWidget( pLcdBackGround );

	m_pShowMixerBtn = new Button( pLcdBackGround, QSize( 88, 23 ), Button::Type::Toggle,
								  "", pCommonStrings->getMixerButton(), false, QSize(),
								  tr( "Show mixer" ) );
	m_pShowMixerBtn->move( 0, 0 );
	connect(m_pShowMixerBtn, SIGNAL( pressed() ), this, SLOT( showMixerButtonClicked() ));

	m_pShowInstrumentRackBtn = new Button( pLcdBackGround, QSize( 168, 23 ), Button::Type::Toggle,
										   "", pCommonStrings->getInstrumentRackButton(), false, QSize(),
										   tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->move( 88, 0 );
	connect( m_pShowInstrumentRackBtn, SIGNAL( pressed() ),
			 this, SLOT( showInstrumentRackButtonClicked() ) );

	m_pStatusLabel = new LCDDisplay(pLcdBackGround, QSize( 255, 18 ) );
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

	updateStatusLabel();
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PlayerControl::onPreferencesChanged );
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

	auto state = m_pHydrogen->getAudioEngine()->getState();
	if ( ! m_pPlayBtn->isDown() && ! m_pStopBtn->isDown() &&
		 ! m_pFfwdBtn->isDown() && ! m_pRwdBtn->isDown() ) {
		if ( state == H2Core::AudioEngine::State::Playing ) {
			m_pPlayBtn->setChecked(true);
		} else {
			m_pPlayBtn->setChecked(false);
		}
	}

	if ( ! m_pRecBtn->isDown() ) {
		if (pPref->getRecordEvents()) {
			m_pRecBtn->setChecked(true);
		} else {
			m_pRecBtn->setChecked(false);
		}
	}

	std::shared_ptr<Song> song = m_pHydrogen->getSong();

	if ( ! m_pSongLoopBtn->isDown() ) {
		m_pSongLoopBtn->setChecked( song->getIsLoopEnabled() );
	}

	if ( ! m_pLCDBPMSpinbox->hasFocus() ) {
		m_pLCDBPMSpinbox->setValue( m_pHydrogen->getAudioEngine()->getBpm() );
	}

	if ( m_pHydrogen->getMode() == Song::Mode::Pattern ) {
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
	} else {
		m_pControlsBCPanel->show();
		if ( ! m_pBCOnOffBtn->isDown() ) {
			m_pBCOnOffBtn->setChecked(true);
		}
	}

	if ( ! m_pBCSetPlayBtn->isDown() ) {
		if ( pPref->m_mmcsetplay ==  Preferences::SET_PLAY_OFF) {
			m_pBCSetPlayBtn->setChecked(false);
		} else {
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
					m_pJackMasterBtn->setIsActive( true );
					m_pJackMasterBtn->setBaseToolTip( m_sJackMasterModeToolTip );
				} else {
					m_pJackMasterBtn->setIsActive( false );
					m_pJackMasterBtn->setBaseToolTip( tr( "JACK timebase support is disabled in the Preferences" ) );
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

	QString sTime = QString( "%1:%2:%3.%4" )
		.arg( nHours, 2, 10, QLatin1Char( '0' ) )
		.arg( nMins, 2, 10, QLatin1Char( '0' ) )
		.arg( nSeconds, 2, 10, QLatin1Char( '0' ) )
		.arg( nMSec, 3, 10, QLatin1Char( '0' ) );

	if ( m_pTimeDisplay->text() != sTime ) {
		m_pTimeDisplay->setText( sTime );
	}

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
	if ( m_pHydrogen->getAudioEngine()->getState() != H2Core::AudioEngine::State::Playing ) {
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
	pHydrogen->getCoreActionController()->locateToColumn( 0 );
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
	Hydrogen::get_instance()->getCoreActionController()->activateSongMode( true );
}


///Set Live mode
void PlayerControl::patternModeBtnClicked()
{
	Hydrogen::get_instance()->getCoreActionController()->activateSongMode( false );
}

void PlayerControl::songModeActivationEvent( int nValue )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
	
	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		// Song mode
		m_pPatternModeLED->setActivated( false );
		m_pSongModeLED->setActivated( true );
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked( true );
		}
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked(false);
		}
		pHydrogenApp->setStatusBarMessage(tr("Song mode selected."), 5000);
	} else {
		// Pattern mode
		m_pPatternModeLED->setActivated( true );
		m_pSongModeLED->setActivated( false );
		if ( ! m_pSongModeBtn->isDown() ) {
			m_pSongModeBtn->setChecked(false);
		}
		if ( ! m_pPatternModeBtn->isDown() ) {
			m_pPatternModeBtn->setChecked(true);
		}
		
		pHydrogenApp->setStatusBarMessage(tr("Pattern mode selected."), 5000);
	}

	updateBPMSpinbox();
	updateBeatCounter();
}

void PlayerControl::bpmChanged( double fNewBpmValue ) {
	if ( m_pLCDBPMSpinbox->getIsActive() ) {
		// Store it's value in the .h2song file.
		m_pHydrogen->getSong()->setBpm( static_cast<float>( fNewBpmValue ) );
		// Use tempo in the next process cycle of the audio engine.
		m_pHydrogen->getAudioEngine()->setNextBpm( static_cast<float>( fNewBpmValue ) );
	}
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
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( ! m_pRubberBPMChange->isChecked() ) {
		// Recalculate all samples ones just to be safe since the
		// recalculation is just triggered if there is a tempo change
		// in the audio engine.
		pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		pHydrogen->recalculateRubberband( pHydrogen->getAudioEngine()->getBpm() );
		pHydrogen->getAudioEngine()->unlock();
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

	Hydrogen::get_instance()->getCoreActionController()->activateJackTimebaseMaster( ! m_pJackMasterBtn->isChecked() );
#endif
}
//~ jack time master

void PlayerControl::fastForwardBtnClicked()
{
	DEBUGLOG( "relocate via button press" );

	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getColumn() + 1 );
}



void PlayerControl::rewindBtnClicked()
{
	DEBUGLOG( "relocate via button press" );
	
	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getColumn() - 1 );
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

	if ( fontMetrics().size( Qt::TextSingleLine, text() ).width() > width() ) {
		// Text is too large to fit in the display. Use scrolled
		// message instead.
		showScrollMessage( msg, 150, false );
		return;
	}
	
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

void PlayerControl::timelineActivationEvent( int ) {
	updateBPMSpinbox();
	updateBeatCounter();
}

void PlayerControl::updateBPMSpinbox() {
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	m_pLCDBPMSpinbox->setIsActive( pHydrogen->getTempoSource() ==
								   H2Core::Hydrogen::Tempo::Song );
	updateBPMSpinboxToolTip();
}

void PlayerControl::updateBPMSpinboxToolTip() {
	auto pHydrogen = Hydrogen::get_instance();
	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pLCDBPMSpinbox->setToolTip( m_sLCDBPMSpinboxJackTimebaseToolTip );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pLCDBPMSpinbox->setToolTip( m_sLCDBPMSpinboxTimelineToolTip );
		break;
	default:
		m_pLCDBPMSpinbox->setToolTip( m_sLCDBPMSpinboxToolTip );
	}
}

void PlayerControl::updateBeatCounter() {
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Song &&
		 ! m_pBCOnOffBtn->getIsActive() ) {
		m_pBCOnOffBtn->setIsActive( true );
		if ( m_bLastBCOnOffBtnState ) {
			m_pBCOnOffBtn->setChecked( true );
			pPref->m_bbc = Preferences::BC_ON;
			m_pControlsBCPanel->show();
		}
		
	} else if ( pHydrogen->getTempoSource() !=
				H2Core::Hydrogen::Tempo::Song &&
				m_pBCOnOffBtn->getIsActive() ) {
		pPref->m_bbc = Preferences::BC_OFF;
		m_pControlsBCPanel->hide();
		m_bLastBCOnOffBtnState = m_pBCOnOffBtn->isChecked();
		m_pBCOnOffBtn->setChecked( false );
		m_pBCOnOffBtn->setIsActive( false );
	}

	updateBeatCounterToolTip();
}

void PlayerControl::updateBeatCounterToolTip() {
	auto pHydrogen = Hydrogen::get_instance();
	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pBCOnOffBtn->setToolTip( m_sBCOnOffBtnJackTimebaseToolTip );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pBCOnOffBtn->setToolTip( m_sBCOnOffBtnTimelineToolTip );
		break;
	default:
		m_pBCOnOffBtn->setToolTip( m_sBCOnOffBtnToolTip );
	}
}

void PlayerControl::tempoChangedEvent( int nValue )
{
	// Also update value if the BPM widget is disabled
	bool bIsReadOnly = m_pLCDBPMSpinbox->isReadOnly();

	if ( ! bIsReadOnly ) {
		m_pLCDBPMSpinbox->setReadOnly( true );
	}
	/*
	 * This is an external tempo change, triggered
	 * via a midi or osc message.
	 *
	 * Just update the GUI using the current tempo
	 * of the song.
	 */
	m_pLCDBPMSpinbox->setValue( m_pHydrogen->getAudioEngine()->getBpm() );
	
	if ( ! bIsReadOnly ) {
		m_pLCDBPMSpinbox->setReadOnly( false );
	}

	if ( nValue == -1 ) {
		// Value was changed via API commands and not by the
		// AudioEngine.
		auto pHydrogen = H2Core::Hydrogen::get_instance();
		if ( pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Timeline ) {
			QMessageBox::warning( this, "Hydrogen", tr("A tempo change via MIDI, OSC, BeatCounter, or TapTempo was detected. It will only be used left of the first Tempo Marker and takes full effect when deactivating the Timeline.") );
		} else if ( pHydrogen->getTempoSource() ==
					H2Core::Hydrogen::Tempo::Jack ) {
			QMessageBox::warning( this, "Hydrogen", tr("A tempo change via MIDI, OSC, BeatCounter, or TapTempo was detected. It will only take effect when deactivating JACK BBT transport or making Hydrogen the Timebase master.") );
		}
	}
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

void PlayerControl::jackTimebaseStateChangedEvent( int ) {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	
	bool bIsMaster = pHydrogen->getJackTimebaseState() ==
		JackAudioDriver::Timebase::Master;
	
	if ( !bIsMaster && m_pJackMasterBtn->isChecked() ){
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( false );
		}
		
	} else if ( bIsMaster && !m_pJackMasterBtn->isChecked() ) {
		if ( ! m_pJackMasterBtn->isDown() ) {
			m_pJackMasterBtn->setChecked( true );
		}
	}

	updateBeatCounter();
	updateBPMSpinbox();

	QString sMessage = tr("JACK Timebase master mode" ) +
		QString( " = %1" )
		.arg( bIsMaster ? pCommonStrings->getStatusOn() :
			  pCommonStrings->getStatusOff() );
	HydrogenApp::get_instance()->setStatusBarMessage( sMessage, 5000 );
}

void PlayerControl::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & H2Core::Preferences::Changes::Font ) {
		updateStatusLabel();
	}
}

void PlayerControl::updateStatusLabel() {

	QString sLongString( "ThisIsALongOneThatShouldNotFitInTheLCDDisplayEvenWithVeryNarrowFonts" );
	m_pStatusLabel->setMaxLength( 120 );
	
	while ( m_pStatusLabel->fontMetrics().size( Qt::TextSingleLine, sLongString ).width() >
			m_pStatusLabel->width() && ! sLongString.isEmpty() ) {
		sLongString.chop( 1 );
	}

	m_pStatusLabel->setMaxLength( sLongString.length() );
}
