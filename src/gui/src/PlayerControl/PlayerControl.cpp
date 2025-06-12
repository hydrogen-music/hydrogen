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

#include "BeatCounter.h"
#include "CpuLoadWidget.h"
#include "MidiControlButton.h"
#include "../Compatibility/MouseEvent.h"
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

PlayerControl::PlayerControl( QWidget* pParent) : QWidget( pParent ) {

	const auto pPref = Preferences::get_instance();
	const auto pSong = Hydrogen::get_instance()->getSong();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pHydrogen = Hydrogen::get_instance();

	setFixedHeight( PlayerControl::nHeight );
	setFocusPolicy( Qt::ClickFocus );
	setObjectName( "PlayerControl" );

	auto pOverallLayout = new QHBoxLayout( this );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->setSpacing( 0 );
	setLayout( pOverallLayout );

	auto pMainToolbar = new QWidget( this );
	pMainToolbar->setObjectName( "MainToolbar" );
	pOverallLayout->addWidget( pMainToolbar );

	auto pMainLayout = new QHBoxLayout( pMainToolbar );
	pMainLayout->setContentsMargins( 2, 2, 2, 2 );
	pMainLayout->setSpacing( 2 );
	pMainLayout->setAlignment( Qt::AlignLeft );
	pMainToolbar->setLayout( pMainLayout );

	////////////////////////////////////////////////////////////////////////////
	m_pTimeGroup = new QWidget( this );
	m_pTimeGroup->setFixedWidth( 150 );
	m_pTimeGroup->setObjectName( "GroupBox" );
	pMainLayout->addWidget( m_pTimeGroup );
	auto pTimeGroupVBoxLayout = new QVBoxLayout( m_pTimeGroup );
	pTimeGroupVBoxLayout->setContentsMargins( 2, 2, 2, 1 );
	pTimeGroupVBoxLayout->setSpacing( 0 );
	m_pTimeDisplay = new LCDDisplay(
		m_pTimeGroup, QSize( 146, PlayerControl::nHeight -
							 PlayerControl::nLabelHeight - 9 ), true, false );
	m_pTimeDisplay->setAlignment( Qt::AlignRight );
	m_pTimeDisplay->setText( "00:00:00:000" );
	m_pTimeDisplay->setStyleSheet(
		m_pTimeDisplay->styleSheet().append(" QLineEdit { font-size: 20px; }" ) );
	pTimeGroupVBoxLayout->addWidget( m_pTimeDisplay );

	auto pTimeGroupLabels = new QWidget( m_pTimeGroup );
	pTimeGroupLabels->setFixedHeight( PlayerControl::nLabelHeight );
	auto pTimeGroupLabelsHBoxLayout = new QHBoxLayout( pTimeGroupLabels );
	pTimeGroupLabelsHBoxLayout->setContentsMargins( 2, 1, 2, 1 );
	pTimeGroupLabelsHBoxLayout->setSpacing( 0 );

	m_pTimeHoursLbl = new ClickableLabel(
		m_pTimeGroup, QSize( 31, PlayerControl::nLabelHeight ),
		pCommonStrings->getTimeHoursLabel() );
	m_pTimeHoursLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pTimeGroupLabelsHBoxLayout->addWidget( m_pTimeHoursLbl );
	m_pTimeMinutesLbl = new ClickableLabel(
		m_pTimeGroup, QSize( 29, PlayerControl::nLabelHeight ),
		pCommonStrings->getTimeMinutesLabel() );
	m_pTimeMinutesLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pTimeGroupLabelsHBoxLayout->addWidget( m_pTimeMinutesLbl );
	m_pTimeSecondsLbl = new ClickableLabel(
		m_pTimeGroup, QSize( 30, PlayerControl::nLabelHeight ),
		pCommonStrings->getTimeSecondsLabel() );
	m_pTimeSecondsLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pTimeGroupLabelsHBoxLayout->addWidget( m_pTimeSecondsLbl );
	m_pTimeMilliSecondsLbl = new ClickableLabel(
		m_pTimeGroup, QSize( 35, PlayerControl::nLabelHeight ),
		pCommonStrings->getTimeMilliSecondsLabel() );
	m_pTimeMilliSecondsLbl->setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::Fixed );
	pTimeGroupLabelsHBoxLayout->addWidget( m_pTimeMilliSecondsLbl );
	pTimeGroupVBoxLayout->addWidget( pTimeGroupLabels );

	////////////////////////////////////////////////////////////////////////////
	m_pSongModeGroup = new QWidget( this );
	m_pSongModeGroup->setFixedWidth( 61 );
	m_pSongModeGroup->setObjectName( "GroupBox" );
	pMainLayout->addWidget( m_pSongModeGroup );
	auto pSongModeGroupLayout = new QVBoxLayout( m_pSongModeGroup );
	pSongModeGroupLayout->setContentsMargins( 2, 2, 2, 1 );
	pSongModeGroupLayout->setSpacing( 1 );

	const auto modeButtonSize = QSize(
		m_pSongModeGroup->width() - 4, PlayerControl::nHeight / 2 - 4 );

	m_pPatternModeBtn = new Button(
		m_pSongModeGroup, modeButtonSize, Button::Type::Toggle, "",
		pCommonStrings->getPatternModeButton(), QSize(), tr( "Pattern Mode" ),
		false, true );
	m_pPatternModeBtn->setObjectName( "PlayerControlPatternModeButton" );
	connect( m_pPatternModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( false ); } );
	pSongModeGroupLayout->addWidget( m_pPatternModeBtn );

	m_pSongModeBtn = new Button(
		m_pSongModeGroup, modeButtonSize, Button::Type::Toggle, "",
		pCommonStrings->getSongModeButton(), QSize(), tr( "Song Mode" ),
		false, true );
	m_pSongModeBtn->setObjectName( "PlayerControlSongModeButton" );
	connect( m_pSongModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( true ); } );
	pSongModeGroupLayout->addWidget( m_pSongModeBtn );

	////////////////////////////////////////////////////////////////////////////
	m_pTransportGroup = new QWidget( this );
	m_pTransportGroup->setObjectName( "GroupBox" );
	pMainLayout->addWidget( m_pTransportGroup );
	auto pTransportGroupLayout = new QHBoxLayout( m_pTransportGroup );
	pTransportGroupLayout->setContentsMargins( 2, 1, 2, 1 );
	pTransportGroupLayout->setSpacing( 3 );

	const auto buttonSize = QSize(
		PlayerControl::nHeight - 7, PlayerControl::nHeight - 12 );
	const auto iconSize = QSize(
		PlayerControl::nHeight - 9, PlayerControl::nHeight - 14 );

	// Rewind button
	m_pRwdBtn = new Button(
		m_pTransportGroup, buttonSize, Button::Type::Push, "rewind.svg", "",
		iconSize - QSize( 4, 4 ), tr( "Rewind" ) );
	m_pRwdBtn->setObjectName( "PlayerControlRewindButton" );
	connect( m_pRwdBtn, SIGNAL( clicked() ), this, SLOT( rewindBtnClicked() ));
	m_pRwdBtn->setAction( std::make_shared<Action>("<<_PREVIOUS_BAR") );
	pTransportGroupLayout->addWidget( m_pRwdBtn );

	// Record button
	m_pRecBtn = new Button(
		m_pTransportGroup, buttonSize, Button::Type::Toggle, "record.svg", "",
		iconSize - QSize( 8, 8 ), tr( "Record" ), true );
	m_pRecBtn->setObjectName( "PlayerControlRecordButton" );
	m_pRecBtn->setChecked( false );
	connect( m_pRecBtn, SIGNAL( clicked() ), this, SLOT( recBtnClicked() ));
	m_pRecBtn->setAction( std::make_shared<Action>("RECORD_READY") );
	pTransportGroupLayout->addWidget( m_pRecBtn );

	// Play button
	m_pPlayBtn = new Button(
		m_pTransportGroup, buttonSize + QSize( 4, 4 ), Button::Type::Toggle,
		"play_pause.svg", "", iconSize + QSize( 6, 6 ), tr( "Play/ Pause" ) );
	m_pPlayBtn->setObjectName( "PlayerControlPlayButton" );
	m_pPlayBtn->setChecked( false );
	connect( m_pPlayBtn, SIGNAL( clicked() ), this, SLOT( playBtnClicked() ));
	m_pPlayBtn->setAction( std::make_shared<Action>("PLAY/PAUSE_TOGGLE") );
	pTransportGroupLayout->addWidget( m_pPlayBtn );

	// Stop button
	m_pStopBtn = new Button(
		m_pTransportGroup, buttonSize, Button::Type::Push, "stop.svg", "",
		iconSize - QSize( 8, 8 ), tr( "Stop" ) );
	m_pStopBtn->setObjectName( "PlayerControlStopButton" );
	connect(m_pStopBtn, SIGNAL( clicked() ), this, SLOT( stopBtnClicked() ));
	m_pStopBtn->setAction( std::make_shared<Action>("STOP") );
	pTransportGroupLayout->addWidget( m_pStopBtn );

	// Fast forward button
	m_pFfwdBtn = new Button(
		m_pTransportGroup, buttonSize, Button::Type::Push, "fast_forward.svg",
		"", iconSize - QSize( 4, 4 ), tr( "Fast Forward" ) );
	m_pFfwdBtn->setObjectName( "PlayerControlForwardButton" );
	connect(m_pFfwdBtn, SIGNAL( clicked() ), this, SLOT( fastForwardBtnClicked() ));
	m_pFfwdBtn->setAction( std::make_shared<Action>(">>_NEXT_BAR") );
	pTransportGroupLayout->addWidget( m_pFfwdBtn );

	// Loop song button button
	m_pSongLoopBtn = new Button(
		m_pTransportGroup, buttonSize, Button::Type::Toggle, "loop.svg", "",
		iconSize - QSize( 8, 8 ), tr( "Loop song" ), false, true );
	m_pSongLoopBtn->setObjectName( "PlayerControlLoopButton" );
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
	pTransportGroupLayout->addWidget( m_pSongLoopBtn );

	////////////////////////////////////////////////////////////////////////////
	m_pBeatCounterGroup = new QWidget( this );
	m_pBeatCounterGroup->setFixedWidth( BeatCounter::nWidth + 2 );
	m_pBeatCounterGroup->setObjectName( "GroupBox" );
	pMainLayout->addWidget( m_pBeatCounterGroup );
	auto pBeatCounterGroupLayout = new QHBoxLayout( m_pBeatCounterGroup );
	pBeatCounterGroupLayout->setContentsMargins( 1, 1, 1, 1 );
	m_pBeatCounterGroup->setLayout( pBeatCounterGroupLayout );

	m_sBCOnOffBtnTimelineToolTip =
		tr( "Please deactivate the Timeline first in order to use the BeatCounter" );
	m_sBCOnOffBtnJackTimebaseToolTip =
		tr( "In the presence of an external JACK Timebase controller the BeatCounter can not be used" );

	m_pBeatCounter = new BeatCounter( this );
	pBeatCounterGroupLayout->addWidget( m_pBeatCounter );

	////////////////////////////////////////////////////////////////////////////
	m_pTempoGroup = new QWidget( this );
	m_pTempoGroup->setFixedWidth( 125 );
	m_pTempoGroup->setObjectName( "BPM" );
	pMainLayout->addWidget( m_pTempoGroup );
	auto pTempoGroupGridLayout = new QGridLayout( m_pTempoGroup );
	pTempoGroupGridLayout->setContentsMargins( 2, 1, 2, 1 );
	pTempoGroupGridLayout->setSpacing( 2 );

	m_pMetronomeBtn = new Button(
		m_pTempoGroup, QSize( 24, 30 ), Button::Type::Toggle, "metronome.svg", "",
		QSize( 20, 20 ), tr( "Switch metronome on/off" ), false, true );
	m_pMetronomeBtn->setObjectName( "MetronomeButton" );
	m_pMetronomeBtn->setChecked( pPref->m_bUseMetronome );
	connect( m_pMetronomeBtn, SIGNAL( clicked() ),
			 this, SLOT( metronomeButtonClicked() ) );
	m_pMetronomeBtn->setAction( std::make_shared<Action>("TOGGLE_METRONOME") );
	pTempoGroupGridLayout->addWidget( m_pMetronomeBtn, 0, 0 );

	m_pMetronomeLED = new MetronomeLED(
		m_pTempoGroup, QSize( 24, PlayerControl::nLabelHeight - 2 ) );
	pTempoGroupGridLayout->addWidget( m_pMetronomeLED, 1, 0 );

	m_sLCDBPMSpinboxToolTip =
		tr("Alter the Playback Speed");
	m_sLCDBPMSpinboxTimelineToolTip =
		tr( "While the Timeline is active this widget is in read-only mode and just displays the tempo set using the current Timeline position" );
	m_sLCDBPMSpinboxJackTimebaseToolTip =
		tr( "In the presence of an external JACK Timebase controller this widget just displays the tempo broadcasted by JACK" );

	m_pBpmSpinBox = new LCDSpinBox(
		m_pTempoGroup, QSize( 95, 30 ), LCDSpinBox::Type::Double,
		static_cast<double>( MIN_BPM ), static_cast<double>( MAX_BPM ), true );
	m_pBpmSpinBox->setStyleSheet(
		m_pBpmSpinBox->styleSheet().append(
			" QAbstractSpinBox {font-size: 16px;}" ) );
	connect( m_pBpmSpinBox, SIGNAL( valueChanged( double ) ),
			 this, SLOT( bpmChanged( double ) ) );
	pTempoGroupGridLayout->addWidget( m_pBpmSpinBox, 0, 1 );

	m_pBPMLbl = new ClickableLabel(
		m_pTempoGroup, QSize( 95, PlayerControl::nLabelHeight ),
		pCommonStrings->getBPMLabel() );
	m_pBPMLbl->setAlignment( Qt::AlignLeft );
	m_pBPMLbl->setIndent( 10 );
	pTempoGroupGridLayout->addWidget( m_pBPMLbl, 1, 1 );

	////////////////////////////////////////////////////////////////////////////
	m_pRubberBandGroup = new QWidget( this );
	m_pRubberBandGroup->setFixedWidth( 13 );
	m_pRubberBandGroup->setObjectName( "GroupBox" );
	pMainLayout->addWidget( m_pRubberBandGroup );
	auto pRubberBandGroupLayout = new QHBoxLayout( m_pRubberBandGroup );
	pRubberBandGroupLayout->setContentsMargins( 2, 1, 2, 1 );

	m_pRubberBPMChange = new Button(
		m_pRubberBandGroup, QSize( 13, 42 ), Button::Type::Toggle, "",
		pCommonStrings->getRubberbandButton(), QSize(),
		tr( "Recalculate Rubberband modified samples if bpm will change" ),
		false, true );
	m_pRubberBPMChange->setObjectName( "PlayerControlRubberbandButton" );
	m_pRubberBPMChange->setChecked( pPref->getRubberBandBatchMode() );
	connect( m_pRubberBPMChange, SIGNAL( clicked() ),
			 this, SLOT( rubberbandButtonToggle() ) );
	pRubberBandGroupLayout->addWidget( m_pRubberBPMChange );
	// test the path. if test fails, no button
	if ( QFile( pPref->m_sRubberBandCLIexecutable ).exists() == false) {
		m_pRubberBandGroup->hide();
	}

	////////////////////////////////////////////////////////////////////////////
	m_pJackGroup = new QWidget( this );
	m_pJackGroup->setObjectName( "JackPanel" );
	m_pJackGroup->setFixedWidth( 61 );
	pMainLayout->addWidget( m_pJackGroup );
	auto pJackGroupLayout = new QVBoxLayout( m_pJackGroup );
	pJackGroupLayout->setContentsMargins( 2, 2, 2, 1 );
	pJackGroupLayout->setSpacing( 1 );

	m_pJackTransportBtn = new Button(
		m_pJackGroup, modeButtonSize, Button::Type::Toggle, "",
		pCommonStrings->getJackTransportButton(), QSize(),
		tr( "JACK transport on/off" ), false, true );
	m_pJackTransportBtn->setObjectName( "PlayerControlJackTransportButton" );
	connect( m_pJackTransportBtn, SIGNAL( clicked() ),
			 this, SLOT( jackTransportBtnClicked() ));
	pJackGroupLayout->addWidget( m_pJackTransportBtn );

	m_pJackTimebaseBtn = new Button(
		m_pJackGroup, modeButtonSize, Button::Type::Toggle, "",
		pCommonStrings->getJackTimebaseButton(), QSize(),
		pCommonStrings->getJackTimebaseTooltip(), false, true );
	m_pJackTimebaseBtn->setObjectName( "PlayerControlJackTimebaseButton" );
	connect( m_pJackTimebaseBtn, SIGNAL( clicked() ), this,
			SLOT( jackTimebaseBtnClicked() ) );
	pJackGroupLayout->addWidget( m_pJackTimebaseBtn );

	////////////////////////////////////////////////////////////////////////////
	m_pSystemGroup = new QWidget( this );
	m_pSystemGroup->setObjectName( "GroupBox" );
	m_pSystemGroup->setFixedWidth( 100 );
	pMainLayout->addWidget( m_pSystemGroup );
	auto pSystemGroupLayout = new QVBoxLayout( m_pSystemGroup );
	pSystemGroupLayout->setContentsMargins( 2, 2, 2, 2 );
	pSystemGroupLayout->setSpacing( 0 );

	auto pSystemCpuGroup = new QWidget( m_pSystemGroup );
	pSystemGroupLayout->addWidget( pSystemCpuGroup );
	auto pSystemCpuGroupLayout = new QVBoxLayout( pSystemCpuGroup );
	pSystemCpuGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pSystemCpuGroupLayout->setSpacing( 2 );

	m_pCpuLoadWidget = new CpuLoadWidget( pSystemCpuGroup );
	m_pCpuLoadWidget->setObjectName( "CpuLoadWidget" );
	pSystemCpuGroupLayout->addWidget( m_pCpuLoadWidget );
	m_pCpuLbl = new ClickableLabel(
		pSystemCpuGroup, QSize( 96, PlayerControl::nLabelHeight ),
		pCommonStrings->getCpuLabel() );
	m_pCpuLbl->setAlignment( Qt::AlignLeft );
	m_pCpuLbl->setIndent( 10 );
	pSystemCpuGroupLayout->addWidget( m_pCpuLbl );

	m_pMidiControlButton = new MidiControlButton( m_pSystemGroup );
	m_pMidiControlButton->setFixedSize(
		96, PlayerControl::nHeight / 2 - 4 );
	pSystemGroupLayout->addWidget( m_pMidiControlButton );

	////////////////////////////////////////////////////////////////////////////
	auto pOthersGroup = new QWidget( this );
	pOthersGroup->setFixedWidth( 262 );
	pMainLayout->addWidget( pOthersGroup );
	auto pOthersGroupLayout = new QVBoxLayout( pOthersGroup );
	pOthersGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pOthersGroupLayout->setSpacing( 1 );

	auto pOthersButtonsGroup = new QWidget( pOthersGroup );
	pOthersGroupLayout->addWidget( pOthersButtonsGroup );
	auto pOthersButtonsGroupLayout = new QHBoxLayout( pOthersButtonsGroup );
	pOthersButtonsGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pOthersButtonsGroupLayout->setSpacing( 2 );

	m_pShowMixerBtn = new Button(
		pOthersButtonsGroup, QSize( 88, modeButtonSize.height() + 2 ),
		Button::Type::Toggle, "", pCommonStrings->getMixerButton(), QSize(),
		tr( "Show mixer" ) );
	m_pShowMixerBtn->setChecked( pPref->getMixerProperties().visible );
	connect( m_pShowMixerBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showMixer( m_pShowMixerBtn->isChecked() ); });
	pOthersButtonsGroupLayout->addWidget( m_pShowMixerBtn );

	m_pShowInstrumentRackBtn = new Button(
		pOthersButtonsGroup, QSize( 168, modeButtonSize.height() + 2 ),
		Button::Type::Toggle, "", pCommonStrings->getInstrumentRackButton(),
		QSize(), tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->setChecked(
		pPref->getInstrumentRackProperties().visible );
	connect( m_pShowInstrumentRackBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showInstrumentRack(
			m_pShowInstrumentRackBtn->isChecked() ); });
	pOthersButtonsGroupLayout->addWidget( m_pShowInstrumentRackBtn );

	m_pStatusLabel = new StatusMessageDisplay(
		pOthersGroup, QSize( 260, modeButtonSize.height() + 2 ) );
	pOthersGroupLayout->addWidget( m_pStatusLabel );

	////////////////////////////////////////////////////////////////////////////
	pMainLayout->addStretch();

	QTimer *timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
	timer->start( 100 );	// update at 10 fps
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PlayerControl::onPreferencesChanged );

	////////////////////////////////////////////////////////////////////////////

	m_pPopupMenu = new QMenu( this );
	auto showBeatCounterAction = m_pPopupMenu->addAction(
		tr( "Show BeatCounter" ) );
	showBeatCounterAction->setCheckable( true );
	showBeatCounterAction->setChecked(
	 	pPref->m_bBeatCounterOn == Preferences::BEAT_COUNTER_ON );
	connect( showBeatCounterAction, &QAction::triggered, this, [=](){
		if ( showBeatCounterAction->isChecked() ) {
			Preferences::get_instance()->m_bBeatCounterOn =
				Preferences::BEAT_COUNTER_ON;
		} else {
			Preferences::get_instance()->m_bBeatCounterOn =
				Preferences::BEAT_COUNTER_OFF;
		}
		updateBeatCounter();
	} );

	////////////////////////////////////////////////////////////////////////////

	updateBeatCounter();
	updateBpmSpinBox();
	updateJackTimebase();
	updateJackTransport();
	updateLoopMode();
	updateSongMode();
	updateStyleSheet();

	HydrogenApp::get_instance()->addEventListener( this );
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

void PlayerControl::mousePressEvent( QMouseEvent* pEvent ) {
	auto pEv = static_cast<MouseEvent*>( pEvent );
	if ( pEvent->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( pEv->globalPosition().toPoint() );
	}
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
	const auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	if ( pPref->m_bBeatCounterOn == Preferences::BEAT_COUNTER_ON ) {
		m_pBeatCounterGroup->show();
	}
	else {
		m_pBeatCounterGroup->hide();
		return;
	}

	m_pBeatCounter->updateBeatCounter();

	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pBeatCounterGroup->setToolTip( m_sBCOnOffBtnJackTimebaseToolTip );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pBeatCounterGroup->setToolTip( m_sBCOnOffBtnTimelineToolTip );
		break;
	default:
		m_pBeatCounterGroup->setToolTip( "" );
	}
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
		m_pJackGroup->hide();
		return;
	}
	else {
		m_pJackGroup->show();
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
		m_pJackGroup->hide();
		return;
	}
	else {
		m_pJackGroup->show();
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
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
}

void PlayerControl::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorText = colorTheme.m_windowTextColor;
	const QColor colorToolbar =
		colorTheme.m_windowColor.lighter( 134 );
	const QColor colorToolbarLighter = colorToolbar.lighter( 130 );

	setStyleSheet( QString( "\
QWidget#MainToolbar {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
}")
				   .arg( colorToolbar.name() ).arg( colorText.name() ) );

	const QString sGroupStyleSheet = QString( "\
QWidget#GroupBox, QWidget#BPM, QWidget#JackPanel {\
    background-color: %1;\
    color: %2;\
    border: 1px solid #000;\
    border-radius: 2px;\
}" )
		.arg( colorToolbarLighter.name() ).arg( colorText.name() );
	m_pTimeGroup->setStyleSheet( sGroupStyleSheet );
	m_pTransportGroup->setStyleSheet( sGroupStyleSheet );
	m_pSongModeGroup->setStyleSheet( sGroupStyleSheet );
	m_pBeatCounterGroup->setStyleSheet( sGroupStyleSheet );
	m_pTempoGroup->setStyleSheet( sGroupStyleSheet );
	m_pRubberBandGroup->setStyleSheet( sGroupStyleSheet );
	m_pJackGroup->setStyleSheet( sGroupStyleSheet );
	m_pSystemGroup->setStyleSheet( sGroupStyleSheet );
}
