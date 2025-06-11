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
 * along with this program. If not, see 
https://www.gnu.org/licenses
 *
 */

#include "PlayerControl.h"

#include "MidiControlButton.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Mixer/Mixer.h"
#include "../Skin.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/LED.h"
#include "../Widgets/Button.h"
#include "../Widgets/CpuLoadWidget.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/StatusMessageDisplay.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/IO/JackAudioDriver.h>
#include <core/Hydrogen.h>

using namespace H2Core;

//beatconter global
int bcDisplaystatus = 0;
// ~ beatcounter

PlayerControl::PlayerControl( QWidget* pParent) : QLabel( pParent ) {

	m_pHydrogen = Hydrogen::get_instance();
	
	setObjectName( "PlayerControl" );
	HydrogenApp::get_instance()->addEventListener( this );

	const auto pPref = H2Core::Preferences::get_instance();
	auto pSong = m_pHydrogen->getSong();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	// Background image
	setPixmap( QPixmap( Skin::getImagePath() + "/playerControlPanel/background.png" ) );
	setScaledContents( true );

	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->setSpacing( 0 );
	hbox->setContentsMargins( 0, 0, 0, 0 );
	setLayout( hbox );

// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( nullptr );
	pControlsPanel->setFixedSize( 344, m_nMinimumHeight );
	pControlsPanel->setPixmap( "/playerControlPanel/background_Control.png" );
	pControlsPanel->setObjectName( "ControlsPanel" );
	hbox->addWidget( pControlsPanel );

	m_pTimeDisplay = new LCDDisplay( pControlsPanel, QSize( 146, 22 ), true, false );
	m_pTimeDisplay->move( 13, 7 );
	m_pTimeDisplay->setAlignment( Qt::AlignRight );
	m_pTimeDisplay->setText( "00:00:00:000" );
	m_pTimeDisplay->setStyleSheet( m_pTimeDisplay->styleSheet().
								   append(" QLineEdit { font-size: 19px; }" ) );

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
	m_pRwdBtn = new Button(
		pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "rewind.svg", "",
		QSize( 13, 13 ), tr( "Rewind" ) );
	m_pRwdBtn->setObjectName( "PlayerControlRewindButton" );
	m_pRwdBtn->move( 166, 15 );
	connect( m_pRwdBtn, SIGNAL( clicked() ), this, SLOT( rewindBtnClicked() ));
	std::shared_ptr<Action> pAction = std::make_shared<Action>("<<_PREVIOUS_BAR");
	m_pRwdBtn->setAction( pAction );

	// Record button
	m_pRecBtn = new Button(
		pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle, "record.svg", "",
		QSize( 11, 11 ), tr( "Record" ), true );
	m_pRecBtn->setObjectName( "PlayerControlRecordButton" );
	m_pRecBtn->move( 193, 15 );
	m_pRecBtn->setChecked(false);
	m_pRecBtn->setHidden(false);
	connect( m_pRecBtn, SIGNAL( clicked() ), this, SLOT( recBtnClicked() ));
	pAction = std::make_shared<Action>("RECORD_READY");
	m_pRecBtn->setAction( pAction );

	// Play button
	m_pPlayBtn = new Button(
		pControlsPanel, QSize( 30, 21 ), Button::Type::Toggle, "play_pause.svg",
		"", QSize( 30, 21 ), tr( "Play/ Pause" ) );
	m_pPlayBtn->setObjectName( "PlayerControlPlayButton" );
	m_pPlayBtn->move( 220, 15 );
	m_pPlayBtn->setChecked(false);
	connect( m_pPlayBtn, SIGNAL( clicked() ), this, SLOT( playBtnClicked() ));
	pAction = std::make_shared<Action>("PLAY/PAUSE_TOGGLE");
	m_pPlayBtn->setAction( pAction );

	// Stop button
	m_pStopBtn = new Button(
		pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "stop.svg", "",
		QSize( 11, 11 ), tr( "Stop" ) );
	m_pStopBtn->setObjectName( "PlayerControlStopButton" );
	m_pStopBtn->move( 252, 15 );
	connect(m_pStopBtn, SIGNAL( clicked() ), this, SLOT( stopBtnClicked() ));
	pAction = std::make_shared<Action>("STOP");
	m_pStopBtn->setAction( pAction );

	// Fast forward button
	m_pFfwdBtn = new Button(
		pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "fast_forward.svg",
		"", QSize( 13, 13 ), tr( "Fast Forward" ) );
	m_pFfwdBtn->setObjectName( "PlayerControlForwardButton" );
	m_pFfwdBtn->move( 279, 15 );
	connect(m_pFfwdBtn, SIGNAL( clicked() ), this, SLOT( fastForwardBtnClicked() ));
	pAction = std::make_shared<Action>(">>_NEXT_BAR");
	m_pFfwdBtn->setAction( pAction );

	// Loop song button button
	m_pSongLoopBtn = new Button(
		pControlsPanel, QSize( 25, 19 ), Button::Type::Toggle, "loop.svg", "",
		QSize( 17, 13 ), tr( "Loop song" ), false, true );
	m_pSongLoopBtn->setObjectName( "PlayerControlLoopButton" );
	m_pSongLoopBtn->move( 308, 15);
	connect( m_pSongLoopBtn, &QPushButton::clicked,
			 [=]( bool bChecked ) {
				 auto pHydrogenApp = HydrogenApp::get_instance();
				 CoreActionController::activateLoopMode( bChecked );
				 if ( bChecked ) {
					 pHydrogenApp->showStatusBarMessage( tr("Loop song = On") );
				 } else {
					 pHydrogenApp->showStatusBarMessage( tr("Loop song = Off") );
				 }

			 });
	updateLoopMode();

	m_pPatternModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pPatternModeLED->move( 179, 4 );
	m_pPatternModeLED->setActivated( true );
	m_pPatternModeBtn = new Button(
		pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle, "",
		pCommonStrings->getPatternModeButton(), QSize(), tr( "Pattern Mode" ),
		false, true );
	m_pPatternModeBtn->setObjectName( "PlayerControlPatternModeButton" );
	m_pPatternModeBtn->move( 190, 3 );
	m_pPatternModeBtn->setChecked(true);
	connect( m_pPatternModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( false ); } );

	// Song mode button
	m_pSongModeLED = new LED( pControlsPanel, QSize( 11, 9 ) );
	m_pSongModeLED->move( 252, 4 );
	m_pSongModeBtn = new Button(
		pControlsPanel, QSize( 59, 11 ), Button::Type::Toggle, "",
		pCommonStrings->getSongModeButton(), QSize(), tr( "Song Mode" ),
		false, true );
	m_pSongModeBtn->setObjectName( "PlayerControlSongModeButton" );
	m_pSongModeBtn->move( 263, 3 );
	connect( m_pSongModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( true ); } );
	updateSongMode();

// BC on off
	QWidget *pControlsBBTBConoffPanel = new QWidget( nullptr );
	pControlsBBTBConoffPanel->setFixedSize( 15, 43 );
	pControlsBBTBConoffPanel->setObjectName( "BeatCounterOnOff" );
	hbox->addWidget( pControlsBBTBConoffPanel );

	m_sBCOnOffBtnToolTip = tr("Toggle the BeatCounter Panel");
	m_sBCOnOffBtnTimelineToolTip = tr( "Please deactivate the Timeline first in order to use the BeatCounter" );
	m_sBCOnOffBtnJackTimebaseToolTip = tr( "In the presence of an external JACK Timebase controller the BeatCounter can not be used" );
	m_pBCOnOffBtn = new Button(
		pControlsBBTBConoffPanel, QSize( 13, 42 ), Button::Type::Toggle, "",
		pCommonStrings->getBeatCounterButton(), QSize(), m_sBCOnOffBtnToolTip,
		false, true );
	m_pBCOnOffBtn->move(0, 0);
	connect( m_pBCOnOffBtn, SIGNAL( clicked() ),
			 this, SLOT( activateBeatCounter() ) );
	pAction = std::make_shared<Action>("BEATCOUNTER");
	m_pBCOnOffBtn->setAction( pAction );

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
	
	m_pBeatCounterBeatLengthDisplay = new QLabel( m_pControlsBCPanel );
	m_pBeatCounterBeatLengthDisplay->resize( QSize( 9, 11 ) );
	m_pBeatCounterBeatLengthDisplay->move( 25, 25 );
	m_pBeatCounterBeatLengthDisplay->setText( "4" );

	m_pBeatCounterTotalBeatsDisplay = new QLabel( m_pControlsBCPanel );
	m_pBeatCounterTotalBeatsDisplay->resize( QSize( 23, 11 ) );
	m_pBeatCounterTotalBeatsDisplay->move( 45, 25 );
	m_pBeatCounterTotalBeatsDisplay->setText( "04" );

	m_pBeatCounterBeatLengthUpBtn = new Button(
		m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "plus.svg", "",
		QSize( 8, 8 ), "", false, true );
	m_pBeatCounterBeatLengthUpBtn->move( 2, 3 );
	connect( m_pBeatCounterBeatLengthUpBtn, SIGNAL( clicked() ),
			 this, SLOT( beatCounterBeatLengthUpBtnClicked() ) );

	m_pBeatCounterBeatLengthDownBtn = new Button(
		m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "minus.svg", "",
		QSize( 8, 8 ), "", false, true );
	m_pBeatCounterBeatLengthDownBtn->move( 2, 14 );
	connect( m_pBeatCounterBeatLengthDownBtn, SIGNAL( clicked() ),
			 this, SLOT( beatCounterBeatLengthDownBtnClicked() ) );

	m_pBeatCounterTotalBeatsUpBtn = new Button(
		m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "plus.svg", "",
		QSize( 8, 8 ), "", false, true );
	m_pBeatCounterTotalBeatsUpBtn->move( 64, 3 );
	connect( m_pBeatCounterTotalBeatsUpBtn, SIGNAL( clicked() ),
			 this, SLOT( beatCounterTotalBeatsUpBtnClicked() ) );

	m_pBeatCounterTotalBeatsDownBtn = new Button(
		m_pControlsBCPanel, QSize( 19, 12 ), Button::Type::Push, "minus.svg", "",
		QSize( 8, 8 ), "", false, true );
	m_pBeatCounterTotalBeatsDownBtn->move( 64, 14 );
	connect( m_pBeatCounterTotalBeatsDownBtn, SIGNAL( clicked() ),
			 this, SLOT( beatCounterTotalBeatsDownBtnClicked() ) );

	m_pBeatCounterSetPlayBtn = new Button(
		m_pControlsBCPanel, QSize( 19, 15 ), Button::Type::Push, "",
		pCommonStrings->getBeatCounterSetPlayButtonOff(), QSize(),
		tr("Set BPM / Set BPM and play"), false, true );
	m_pBeatCounterSetPlayBtn->setObjectName( "BeatCounterSetPlayButton" );
	m_pBeatCounterSetPlayBtn->move( 64, 25 );
	connect(m_pBeatCounterSetPlayBtn, SIGNAL( clicked() ),
			this, SLOT( beatCounterSetPlayBtnClicked() ));
	updateBeatCounter();


// BPM
	m_sLCDBPMSpinboxToolTip = tr("Alter the Playback Speed");
	m_sLCDBPMSpinboxTimelineToolTip = tr( "While the Timeline is active this widget is in read-only mode and just displays the tempo set using the current Timeline position" );
	m_sLCDBPMSpinboxJackTimebaseToolTip = tr( "In the presence of an external JACK Timebase controller this widget just displays the tempo broadcasted by JACK" );

	PixmapWidget *pBPMPanel = new PixmapWidget( nullptr );
	pBPMPanel->setFixedSize( 145, 43 );
	pBPMPanel->setPixmap( "/playerControlPanel/background_BPM.png" );
	pBPMPanel->setObjectName( "BPM" );
	hbox->addWidget( pBPMPanel );
	m_pBPMLbl = new ClickableLabel( pBPMPanel, QSize( 26, 10 ),
									pCommonStrings->getBPMLabel(), ClickableLabel::Color::Dark );
	m_pBPMLbl->move( 36, 31 );

	// LCD BPM SpinBox
	m_pBpmSpinBox = new LCDSpinBox(
		pBPMPanel, QSize( 95, 30), LCDSpinBox::Type::Double,
		static_cast<double>( MIN_BPM ), static_cast<double>( MAX_BPM ), true );
	m_pBpmSpinBox->move( 36, 1 );
	m_pBpmSpinBox->setStyleSheet( m_pBpmSpinBox->styleSheet().
								  append( " QAbstractSpinBox {font-size: 16px;}" ) );
	updateBpmSpinBox();
	connect( m_pBpmSpinBox, SIGNAL( valueChanged( double ) ),
			 this, SLOT( bpmChanged( double ) ) );

	m_pRubberBPMChange = new Button(
		pBPMPanel, QSize( 13, 42 ), Button::Type::Toggle, "",
		pCommonStrings->getRubberbandButton(), QSize(),
		tr( "Recalculate Rubberband modified samples if bpm will change" ),
		false, true );
	m_pRubberBPMChange->setObjectName( "PlayerControlRubberbandButton" );
	m_pRubberBPMChange->move( 131, 0 );
	m_pRubberBPMChange->setChecked( pPref->getRubberBandBatchMode());
	QString program = pPref->m_sRubberBandCLIexecutable;
	//test the path. if test fails, no button
	if ( QFile( program ).exists() == false) {
		m_pRubberBPMChange->hide();
	}
	connect( m_pRubberBPMChange, SIGNAL( clicked() ),
			 this, SLOT( rubberbandButtonToggle() ) );

	m_pMetronomeLED = new MetronomeLED( pBPMPanel, QSize( 22, 7 ) );
	m_pMetronomeLED->move( 7, 32 );

	m_pMetronomeBtn = new Button(
		pBPMPanel, QSize( 24, 28 ), Button::Type::Toggle, "metronome.svg", "",
		QSize( 20, 20 ), tr( "Switch metronome on/off" ), false, true );
	m_pMetronomeBtn->setObjectName( "MetronomeButton" );
	m_pMetronomeBtn->setChecked( pPref->m_bUseMetronome );
	m_pMetronomeBtn->move( 6, 2 );
	connect( m_pMetronomeBtn, SIGNAL( clicked() ),
			 this, SLOT( metronomeButtonClicked() ) );
	pAction = std::make_shared<Action>("TOGGLE_METRONOME");
	m_pMetronomeBtn->setAction( pAction );

// ~ BPM


// JACK
	PixmapWidget *pJackPanel = new PixmapWidget( nullptr );
	pJackPanel->setFixedSize( 124, 43 );
	pJackPanel->setPixmap( "/playerControlPanel/background_Jack.png" );
	pJackPanel->setObjectName( "JackPanel" );
	hbox->addWidget( pJackPanel );

	// Jack transport mode button
	m_pJackTransportBtn = new Button(
		pJackPanel, QSize( 53, 16 ), Button::Type::Toggle, "",
		pCommonStrings->getJackTransportButton(), QSize(),
		tr( "JACK transport on/off" ), false, true );
	m_pJackTransportBtn->setObjectName( "PlayerControlJackTransportButton" );
	updateJackTransport();
	connect(m_pJackTransportBtn, SIGNAL( clicked() ), this, SLOT( jackTransportBtnClicked() ));
	m_pJackTransportBtn->move( 3, 24 );

	m_pJackTimebaseBtn = new Button(
		pJackPanel, QSize( 64, 16 ), Button::Type::Toggle, "",
		pCommonStrings->getJackTimebaseButton(), QSize(),
		pCommonStrings->getJackTimebaseTooltip(), false, true );
	m_pJackTimebaseBtn->setObjectName( "PlayerControlJackTimebaseButton" );
	updateJackTimebase();
	connect( m_pJackTimebaseBtn, SIGNAL( clicked() ), this,
			SLOT( jackTimebaseBtnClicked() ) );
	m_pJackTimebaseBtn->move( 56, 24 );
	// ~ JACK

	// CPU load widget
	m_pCpuLoadWidget = new CpuLoadWidget( pJackPanel );
	m_pCpuLoadWidget->setObjectName( "CpuLoadWidget" );
	m_pCpuLoadWidget->move( 10, 3 );
	m_pCpuLbl = new ClickableLabel( pJackPanel, QSize( 30, 9 ),
									pCommonStrings->getCpuLabel() );
	m_pCpuLbl->move( 71, 14 );

	// Midi Activity widget
	m_pMidiControlButton = new MidiControlButton(pJackPanel );
	m_pMidiControlButton->move( 11, 14 );
	m_pMidiControlButton->setFixedSize( 64, 16 );

	QWidget *pLcdBackGround = new QWidget( nullptr );
	pLcdBackGround->setFixedSize( 256, 43 );
	pLcdBackGround->setObjectName( "LcdBackground" );
	hbox->addWidget( pLcdBackGround );

	m_pShowMixerBtn = new Button(
		pLcdBackGround, QSize( 88, 23 ), Button::Type::Toggle, "",
		pCommonStrings->getMixerButton(), QSize(), tr( "Show mixer" ) );
	m_pShowMixerBtn->setChecked( pPref->getMixerProperties().visible );
	m_pShowMixerBtn->move( 0, 0 );
	connect( m_pShowMixerBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showMixer( m_pShowMixerBtn->isChecked() ); });

	m_pShowInstrumentRackBtn = new Button(
		pLcdBackGround, QSize( 168, 23 ), Button::Type::Toggle, "",
		pCommonStrings->getInstrumentRackButton(), QSize(),
		tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->setChecked(
		pPref->getInstrumentRackProperties().visible );
	m_pShowInstrumentRackBtn->move( 88, 0 );
	connect( m_pShowInstrumentRackBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showInstrumentRack(
			m_pShowInstrumentRackBtn->isChecked() ); });

	m_pStatusLabel = new StatusMessageDisplay( pLcdBackGround, QSize( 255, 18 ) );
	m_pStatusLabel->move( 0, 24 );

	hbox->addStretch( 1000 );	// this must be the last widget in the HBOX!!

	QTimer *timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
	timer->start( 100 );	// update at 10 fps
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PlayerControl::onPreferencesChanged );
}




PlayerControl::~PlayerControl() {
}





void PlayerControl::updatePlayerControl()
{
	const auto pPref = Preferences::get_instance();
	HydrogenApp *pH2App = HydrogenApp::get_instance();
	const auto pHydrogen = Hydrogen::get_instance();

	m_pShowMixerBtn->setChecked( pH2App->getMixer()->isVisible() );
	m_pShowInstrumentRackBtn->setChecked(
		pH2App->getInstrumentRack()->isVisible() );

	m_pMetronomeBtn->setChecked( pPref->m_bUseMetronome );

	// Rubberband
	if ( m_pRubberBPMChange->isChecked() != pPref->getRubberBandBatchMode() ) {
		m_pRubberBPMChange->setChecked( pPref->getRubberBandBatchMode());
	}
}

void PlayerControl::driverChangedEvent() {
	updateJackTransport();
	updateJackTimebase();
	updateTransportControl();
}

void PlayerControl::jackTransportActivationEvent() {
	updateJackTransport();
	updateJackTimebase();
}

void PlayerControl::jackTimebaseStateChangedEvent( int nState )
{
	updateJackTransport();
	updateJackTimebase();

	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Since this event can be caused by an external application we handle the
	// corresponding statue message differently and allow it to be triggered by
	// the event itself.
	QString sMessage = tr("JACK Timebase mode" ) + QString( " = " );

	switch( Hydrogen::get_instance()->getJackTimebaseState() ) {
	case JackAudioDriver::Timebase::Controller:
		sMessage.append( "Controller" );
		break;

	case JackAudioDriver::Timebase::Listener:
		sMessage.append( "Listener" );
		break;

	default:
		sMessage.append( pCommonStrings->getStatusOff() );
	}
	HydrogenApp::get_instance()->showStatusBarMessage( sMessage );

	updateBeatCounter();
	updateBpmSpinBox();
}

void PlayerControl::loopModeActivationEvent() {
	updateLoopMode();
}

void PlayerControl::metronomeEvent( int nValue ) {
	updatePlayerControl();
}

void PlayerControl::songModeActivationEvent() {
	updateSongMode();
	updateBpmSpinBox();
	updateBeatCounter();
	updateTransportControl();
}

void PlayerControl::stateChangedEvent( const H2Core::AudioEngine::State& ) {
	updateTransportControl();
}

void PlayerControl::tempoChangedEvent( int nValue )
{
	updateBpmSpinBox();
	if ( nValue == -1 ) {
		// Value was changed via API commands and not by the
		// AudioEngine.
		auto pHydrogen = H2Core::Hydrogen::get_instance();
		if ( pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Timeline ) {
			QMessageBox::warning( this, "Hydrogen",
								  tr("A tempo change via MIDI, OSC, BeatCounter, or TapTempo was detected. It will only be used after deactivating the Timeline and left of the first Tempo Marker when activating it again.") );
		}
		else if ( pHydrogen->getTempoSource() ==
					H2Core::Hydrogen::Tempo::Jack ) {
			QMessageBox::warning( this, "Hydrogen",
								  tr("A tempo change via MIDI, OSC, BeatCounter, or TapTempo was detected. It will only take effect when deactivating JACK Timebase support or making Hydrogen take Timebase control.") );
		}
	}
}

void PlayerControl::timelineActivationEvent() {
	updateBpmSpinBox();
	updateBeatCounter();
}

void PlayerControl::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateSongMode();
		updateBpmSpinBox();
		updateBeatCounter();
		updateLoopMode();
		updateJackTransport();
		updateJackTimebase();
		updatePlayerControl();
		updateTransportControl();
	}
}

/// Toggle record mode
void PlayerControl::recBtnClicked() {
	if ( m_pHydrogen->getAudioEngine()->getState() !=
		 H2Core::AudioEngine::State::Playing ) {
		if ( m_pRecBtn->isChecked() ) {
			Preferences::get_instance()->setRecordEvents(true);
			(HydrogenApp::get_instance())->showStatusBarMessage(
				tr("Record midi events = On" ) );
		}
		else {
			Preferences::get_instance()->setRecordEvents(false);
			(HydrogenApp::get_instance())->showStatusBarMessage(
				tr("Record midi events = Off" ) );
		}
	}
}

/// Start audio engine
void PlayerControl::playBtnClicked() {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( m_pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(m_pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning( this, "Hydrogen",
							   QString( "%1\n%2" )
							  .arg( pCommonStrings->getAudioDriverNotPresent() )
							  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
		return;
	}
	
	if ( m_pPlayBtn->isChecked() ) {
		m_pHydrogen->sequencerPlay();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Playing.") );
	}
	else {
		m_pHydrogen->sequencerStop();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Pause.") );
	}
}

/// Stop audio engine
void PlayerControl::stopBtnClicked()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( m_pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(m_pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning( this, "Hydrogen",
							   QString( "%1\n%2" )
							  .arg( pCommonStrings->getAudioDriverNotPresent() )
							  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
		return;
	}
	
	m_pHydrogen->sequencerStop();
	CoreActionController::locateToColumn( 0 );
	(HydrogenApp::get_instance())->showStatusBarMessage( tr("Stopped.") );
}

void PlayerControl::updateTime() {
	const float fSeconds =
		Hydrogen::get_instance()->getAudioEngine()->getElapsedTime();

	const int nMSec = (int)( (fSeconds - (int)fSeconds) * 1000.0 );
	const int nSeconds = ( (int)fSeconds ) % 60;
	const int nMins = (int)( fSeconds / 60.0 ) % 60;
	const int nHours = (int)( fSeconds / 3600.0 );

	QString const sTime = QString( "%1:%2:%3.%4" )
		.arg( nHours, 2, 10, QLatin1Char( '0' ) )
		.arg( nMins, 2, 10, QLatin1Char( '0' ) )
		.arg( nSeconds, 2, 10, QLatin1Char( '0' ) )
		.arg( nMSec, 3, 10, QLatin1Char( '0' ) );

	if ( m_pTimeDisplay->text() != sTime ) {
		m_pTimeDisplay->setText( sTime );
	}
}

void PlayerControl::activateSongMode( bool bActivate ) {
	auto pHydrogenApp = HydrogenApp::get_instance();

	CoreActionController::activateSongMode( bActivate );
	if ( bActivate ) {
		pHydrogenApp->showStatusBarMessage( tr("Song mode selected.") );
	}
	else {
		pHydrogenApp->showStatusBarMessage( tr("Pattern mode selected.") );
	}
}

void PlayerControl::bpmChanged( double fNewBpmValue ) {
	if ( m_pBpmSpinBox->getIsActive() ) {
		CoreActionController::setBpm( static_cast<float>( fNewBpmValue ) );
	}
}



//beatcounter
void PlayerControl::activateBeatCounter()
{
	auto pPref = Preferences::get_instance();
	if ( m_pBCOnOffBtn->isChecked() ) {
		pPref->m_bBeatCounterOn = Preferences::BEAT_COUNTER_ON;
		HydrogenApp::get_instance()->showStatusBarMessage( tr(" BC Panel on") );
	}
	else {
		pPref->m_bBeatCounterOn = Preferences::BEAT_COUNTER_OFF;
		HydrogenApp::get_instance()->showStatusBarMessage( tr(" BC Panel off") );
	}
	Hydrogen::get_instance()->updateBeatCounterSettings();
}

void PlayerControl::beatCounterSetPlayBtnClicked()
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pPref = Preferences::get_instance();
	if ( m_pBeatCounterSetPlayBtn->text() ==
		 pCommonStrings->getBeatCounterSetPlayButtonOff() ) {
		pPref->m_bBeatCounterSetPlay = Preferences::BEAT_COUNTER_SET_PLAY_ON;
		pHydrogenApp->showStatusBarMessage( tr(" Count BPM and start PLAY") );
	}
	else {
		pPref->m_bBeatCounterSetPlay = Preferences::BEAT_COUNTER_SET_PLAY_OFF;
		pHydrogenApp->showStatusBarMessage( tr(" Count and set BPM") );
	}

	Hydrogen::get_instance()->updateBeatCounterSettings();
}

void PlayerControl::rubberbandButtonToggle()
{
	auto pPref = Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( m_pRubberBPMChange->isChecked() ) {
		auto pSong = pHydrogen->getSong();

		if ( pSong != nullptr ) {
			auto pDrumkit = pSong->getDrumkit();
			if ( pDrumkit != nullptr ) {
				// Recalculate all samples ones just to be safe since the
				// recalculation is just triggered if there is a tempo change
				// in the audio engine.
				pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
				pDrumkit->recalculateRubberband(
					pHydrogen->getAudioEngine()->getTransportPosition()->getBpm() );
				pHydrogen->getAudioEngine()->unlock();
			}
		}
		pPref->setRubberBandBatchMode(true);
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Recalculate all samples using Rubberband ON") );
	}
	else {
		pPref->setRubberBandBatchMode(false);
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Recalculate all samples using Rubberband OFF") );
	}

	updatePlayerControl();
}


void PlayerControl::beatCounterTotalBeatsUpBtnClicked()
{
	int nBeatsToCount = m_pHydrogen->getBeatCounterTotalBeats();
	nBeatsToCount++;
	if ( nBeatsToCount > 16 ) {
		nBeatsToCount = 2;
	}

	m_pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
}

void PlayerControl::beatCounterTotalBeatsDownBtnClicked()
{
	int nBeatsToCount = m_pHydrogen->getBeatCounterTotalBeats();
	nBeatsToCount--;
	if ( nBeatsToCount < 2 ) {
		nBeatsToCount = 16;
	}
	m_pHydrogen->setBeatCounterTotalBeats( nBeatsToCount );
}

void PlayerControl::beatCounterBeatLengthUpBtnClicked()
{
	float tmp = m_pHydrogen->getBeatCounterBeatLength() * 4;

	tmp = tmp / 2 ;
	if ( tmp < 1 ) {
		tmp = 8;
	}

	m_pHydrogen->setBeatCounterBeatLength( (tmp) / 4 );
}

void PlayerControl::beatCounterBeatLengthDownBtnClicked()
{
	float tmp = m_pHydrogen->getBeatCounterBeatLength() * 4;

	tmp = tmp * 2;
	if ( tmp > 8 ) {
		tmp = 1;
	}
	m_pHydrogen->setBeatCounterBeatLength( (tmp) / 4 );
}

void PlayerControl::beatCounterEvent() {
	updateBeatCounter();
}

void PlayerControl::jackTransportBtnClicked()
{
	if ( !m_pHydrogen->hasJackAudioDriver() ) {
		QMessageBox::warning(
			this, "Hydrogen", tr( "JACK-transport will work only with JACK driver." ) );
		return;
	}

	const auto pPref = Preferences::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	if ( pPref->m_nJackTransportMode == Preferences::USE_JACK_TRANSPORT ) {
		CoreActionController::activateJackTransport( false );
		pHydrogenApp->showStatusBarMessage( tr( "JACK transport mode = Off" ) );
	}
	else {
		CoreActionController::activateJackTransport( true );
		pHydrogenApp->showStatusBarMessage( tr( "JACK transport mode = On" ) );
	}
}


void PlayerControl::jackTimebaseBtnClicked()
{
#ifdef H2CORE_HAVE_JACK
	if ( !m_pHydrogen->hasJackTransport() ) {
		QMessageBox::warning( this, "Hydrogen", tr( "JACK transport will work only with JACK driver." ) );
		return;
	}

	auto pPref = Preferences::get_instance();

	if ( pPref->m_bJackTimebaseMode == Preferences::USE_JACK_TIMEBASE_CONTROL ) {
		CoreActionController::activateJackTimebaseControl( false );
	}
	else {
		CoreActionController::activateJackTimebaseControl( true );
	}
#endif
}

void PlayerControl::fastForwardBtnClicked() {
	auto pHydrogen = Hydrogen::get_instance();
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1 );
}



void PlayerControl::rewindBtnClicked() {
	auto pHydrogen = Hydrogen::get_instance();
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() - 1 );
}

void PlayerControl::metronomeButtonClicked() {
	CoreActionController::setMetronomeIsActive( m_pMetronomeBtn->isChecked() );
}

void PlayerControl::updateBeatCounter() {
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pPref->m_bBeatCounterOn == Preferences::BEAT_COUNTER_ON &&
		 pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Song ) {
		m_pControlsBCPanel->show();
		m_pBCOnOffBtn->setChecked( true );
	}
	else {
		m_pControlsBCPanel->hide();
		m_pBCOnOffBtn->setChecked( false );
	}
	m_pBCOnOffBtn->setIsActive(
		pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Song );

	if ( pPref->m_bBeatCounterSetPlay == Preferences::BEAT_COUNTER_SET_PLAY_ON ) {
		m_pBeatCounterSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOn() );
	}
	else {
		m_pBeatCounterSetPlayBtn->setText(
			pCommonStrings->getBeatCounterSetPlayButtonOff() );
	}

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

	m_pBeatCounterTotalBeatsDisplay->setText(
		QString( "%1" ).arg( pHydrogen->getBeatCounterTotalBeats(), 2, 10,
							 QChar( 0x0030 ) ) );
	m_pBeatCounterBeatLengthDisplay->setText(
		QString::number( pHydrogen->getBeatCounterBeatLength() * 4 ) );

	QString sBcStatus;
	const int nEventCount = pHydrogen->getBeatCounterEventCount();
	if ( nEventCount == 1 ) {
		sBcStatus = "R";
	}
	else {
		sBcStatus = QString( "%1" ).arg( nEventCount - 1, 2, 10, QChar( 0x0030 ) );
	}
	m_pBCDisplayZ->setText( sBcStatus );
}

void PlayerControl::updateBpmSpinBox() {
	auto pHydrogen = Hydrogen::get_instance();

	m_pBpmSpinBox->setIsActive(
		pHydrogen->getTempoSource() == H2Core::Hydrogen::Tempo::Song );
	m_pBpmSpinBox->setValue(
		pHydrogen->getAudioEngine()->getTransportPosition()->getBpm(),
		H2Core::Event::Trigger::Suppress );

	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pBpmSpinBox->setToolTip( m_sLCDBPMSpinboxJackTimebaseToolTip );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pBpmSpinBox->setToolTip( m_sLCDBPMSpinboxTimelineToolTip );
		break;
	default:
		m_pBpmSpinBox->setToolTip( m_sLCDBPMSpinboxToolTip );
	}
}

void PlayerControl::updateJackTransport() {
	auto pHydrogen = Hydrogen::get_instance();
	if ( ! pHydrogen->hasJackAudioDriver() ) {
		m_pJackTransportBtn->hide();
	} else {
		m_pJackTransportBtn->show();
	}

	if ( pHydrogen->hasJackTransport() ) {
		m_pJackTransportBtn->setChecked( true );
	} else {
		m_pJackTransportBtn->setChecked( false );
	}
}

void PlayerControl::updateJackTimebase()
{
	const auto theme = Preferences::get_instance()->getTheme();
	auto pHydrogen = Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( ! pHydrogen->hasJackAudioDriver() ) {
		m_pJackTimebaseBtn->hide();
	} else {
		m_pJackTimebaseBtn->show();
	}

	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		m_pJackTimebaseBtn->setChecked( false );
		m_pJackTimebaseBtn->setCheckedBackgroundColor(
			theme.m_color.m_accentColor );
		m_pJackTimebaseBtn->setCheckedBackgroundTextColor(
			theme.m_color.m_accentTextColor );
		m_pJackTimebaseBtn->setIsActive( false );
		m_pJackTimebaseBtn->setBaseToolTip(
			pCommonStrings->getJackTimebaseDisabledTooltip() );
		return;
	}
	else {
		m_pJackTimebaseBtn->setIsActive( true );
		m_pJackTimebaseBtn->setChecked( false );
		m_pJackTimebaseBtn->setCheckedBackgroundColor(
			theme.m_color.m_accentColor );
		m_pJackTimebaseBtn->setCheckedBackgroundTextColor(
			theme.m_color.m_accentTextColor );
		m_pJackTimebaseBtn->setToolTip(
			pCommonStrings->getJackTimebaseTooltip() );
	}

	if ( pHydrogen->hasJackTransport() ) {
		switch ( pHydrogen->getJackTimebaseState() ) {
		case JackAudioDriver::Timebase::Controller:
			m_pJackTimebaseBtn->setChecked( true );
			break;

		case JackAudioDriver::Timebase::Listener:
			m_pJackTimebaseBtn->setChecked( true );
		m_pJackTimebaseBtn->setCheckedBackgroundColor(
			theme.m_color.m_buttonRedColor );
		m_pJackTimebaseBtn->setCheckedBackgroundTextColor(
			theme.m_color.m_buttonRedTextColor );
			m_pJackTimebaseBtn->setToolTip(
				pCommonStrings->getJackTimebaseListenerTooltip() );
			break;
		}
	}
}

void PlayerControl::updateLoopMode() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
		m_pSongLoopBtn->setChecked( true );
	} else {
		m_pSongLoopBtn->setChecked( false );
	}
}

void PlayerControl::updateSongMode() {
	auto pHydrogen = Hydrogen::get_instance();

	const bool bSongMode = pHydrogen->getMode() == Song::Mode::Song;
	m_pPatternModeLED->setActivated( ! bSongMode );
	m_pSongModeLED->setActivated( bSongMode );
	m_pSongModeBtn->setChecked( bSongMode );
	m_pPatternModeBtn->setChecked( ! bSongMode );
	m_pSongLoopBtn->setIsActive( bSongMode );
}

void PlayerControl::updateTransportControl() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	m_pPlayBtn->setChecked(
		pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing );
	m_pRecBtn->setChecked( pPref->getRecordEvents() );
}

void PlayerControl::showStatusBarMessage( const QString& sMessage,
										  const QString& sCaller ) {
	if ( H2Core::Hydrogen::get_instance()->getGUIState() ==
		 H2Core::Hydrogen::GUIState::ready ) {
		m_pStatusLabel->showMessage( sMessage, sCaller );
	}
}

void PlayerControl::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::AudioTab ) {
		updateJackTransport();
		updateJackTimebase();
	}
}
