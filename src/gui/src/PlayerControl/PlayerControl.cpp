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
#include "BpmSpinBox.h"
#include "MetronomeButton.h"
#include "MidiControlButton.h"
#include "../Compatibility/MouseEvent.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Mixer/Mixer.h"
#include "../Skin.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/PanelGroupBox.h"
#include "../Widgets/PanelSeparator.h"

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
	pMainLayout->setContentsMargins(
		PlayerControl::nMargin, PlayerControl::nMargin, PlayerControl::nMargin,
		PlayerControl::nMargin );
	pMainLayout->setSpacing( PlayerControl::nSpacing );
	pMainLayout->setAlignment( Qt::AlignLeft );
	pMainToolbar->setLayout( pMainLayout );

	const int nButtonHeight = PlayerControl::nWidgetHeight - 2;
	const auto buttonSize = QSize(
		static_cast<int>(std::round( nButtonHeight *
									 Skin::fButtonWidthHeightRatio ) ),
		nButtonHeight );
	const auto iconSize = QSize( buttonSize.width() - 4, buttonSize.height() - 4 );

	const int nButtonHeightGroup = PlayerControl::nWidgetHeight -
		PanelGroupBox::nBorder * 2 - PanelGroupBox::nMarginVertical * 2;
	const auto buttonSizeGroup = QSize(
		static_cast<int>(std::round( nButtonHeightGroup *
									 Skin::fButtonWidthHeightRatio ) ),
		nButtonHeightGroup );
	const auto iconSizeGroup = QSize( buttonSizeGroup.width() - 4,
									  buttonSizeGroup.height() - 4 );

	////////////////////////////////////////////////////////////////////////////
	m_pTimeDisplay = new LCDDisplay(
		pMainToolbar, QSize( 146, PlayerControl::nWidgetHeight ), true, false );
	m_pTimeDisplay->setAlignment( Qt::AlignRight );
	m_pTimeDisplay->setText( "00:00:00:000" );
	m_pTimeDisplay->setStyleSheet(
		m_pTimeDisplay->styleSheet().append(
			QString( " QLineEdit { font-size: %1px; }" )
			.arg( PlayerControl::nFontSize ) ) );
	pMainLayout->addWidget( m_pTimeDisplay );

	////////////////////////////////////////////////////////////////////////////
	// Invisible wrapper group for snapshots.
	m_pTransportGroup = new QWidget( this );
	m_pTransportGroup->setObjectName( "PlayerControlTransport" );
	pMainLayout->addWidget( m_pTransportGroup );
	auto pTransportGroupLayout = new QHBoxLayout( m_pTransportGroup );
	pTransportGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pTransportGroupLayout->setSpacing( PlayerControl::nSpacing );

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
		m_pTransportGroup, buttonSize, Button::Type::Toggle,
		"play_pause.svg", "", iconSize + QSize( 2, 2 ), tr( "Play/ Pause" ) );
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
	pMainLayout->addWidget( m_pSongLoopBtn );

	m_pSeparatorTransport = new PanelSeparator( pMainToolbar );
	m_pSeparatorTransport->setFixedHeight( buttonSize.height() );
	pMainLayout->addWidget( m_pSeparatorTransport );

	////////////////////////////////////////////////////////////////////////////
	m_pEditorGroup = new PanelGroupBox( this );
	m_pEditorGroup->setFixedHeight( nWidgetHeight );
	pMainLayout->addWidget( m_pEditorGroup );

	m_pPatternModeBtn = new Button(
		m_pEditorGroup, buttonSizeGroup, Button::Type::Toggle,
		"pattern-editor.svg", "", iconSizeGroup, tr( "Pattern Mode" ),
		false, true );
	m_pPatternModeBtn->setObjectName( "PlayerControlPatternModeButton" );
	connect( m_pPatternModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( false ); } );
	m_pEditorGroup->addWidget( m_pPatternModeBtn );

	m_pSongModeBtn = new Button(
		m_pEditorGroup, buttonSizeGroup, Button::Type::Toggle,
		"song-editor.svg", "", iconSizeGroup, tr( "Song Mode" ),
		false, true );
	m_pSongModeBtn->setObjectName( "PlayerControlSongModeButton" );
	connect( m_pSongModeBtn, &QPushButton::clicked,
			[=]() { activateSongMode( true ); } );
	m_pEditorGroup->addWidget( m_pSongModeBtn );

	m_pSeparatorEditor = new PanelSeparator( pMainToolbar );
	m_pSeparatorEditor->setFixedHeight( buttonSize.height() );
	pMainLayout->addWidget( m_pSeparatorEditor );

	////////////////////////////////////////////////////////////////////////////
	// Invisible wrapper group for snapshots.
	m_pTempoGroup = new QWidget( this );
	m_pTempoGroup->setObjectName( "BPM" );
	pMainLayout->addWidget( m_pTempoGroup );
	auto pTempoGroupLayout = new QHBoxLayout( m_pTempoGroup );
	pTempoGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pTempoGroupLayout->setSpacing( PlayerControl::nSpacing );

	m_pMetronomeBtn = new MetronomeButton( m_pTempoGroup, buttonSize );
	m_pMetronomeBtn->setObjectName( "MetronomeButton" );
	m_pMetronomeBtn->setChecked( pPref->m_bUseMetronome );
	connect( m_pMetronomeBtn, SIGNAL( clicked() ),
			 this, SLOT( metronomeButtonClicked() ) );
	m_pMetronomeBtn->setAction( std::make_shared<Action>("TOGGLE_METRONOME") );
	pTempoGroupLayout->addWidget( m_pMetronomeBtn );

	m_sLCDBPMSpinboxToolTip =
		tr("Alter the Playback Speed");
	m_sLCDBPMSpinboxTimelineToolTip =
		tr( "While the Timeline is active this widget is in read-only mode and just displays the tempo set using the current Timeline position" );
	m_sLCDBPMSpinboxJackTimebaseToolTip =
		tr( "In the presence of an external JACK Timebase controller this widget just displays the tempo broadcasted by JACK" );

	m_pBpmSpinBox = new BpmSpinBox(
		pMainToolbar, QSize( 95, PlayerControl::nWidgetHeight ) );
	m_pBpmSpinBox->setStyleSheet(
		m_pBpmSpinBox->styleSheet().append(
			QString( " QAbstractSpinBox {font-size: %1px;}" )
			.arg( PlayerControl::nFontSize ) ) );
	connect( m_pBpmSpinBox, SIGNAL( valueChanged( double ) ),
			 this, SLOT( bpmChanged( double ) ) );
	pTempoGroupLayout->addWidget( m_pBpmSpinBox );

	m_pSeparatorTempo = new PanelSeparator( pMainToolbar );
	m_pSeparatorTempo->setFixedHeight( buttonSize.height() );
	pMainLayout->addWidget( m_pSeparatorTempo );

	////////////////////////////////////////////////////////////////////////////
	// BeatCounter
	//
	// Additional wrapper object to associate separator with beat counter while
	// not including it into the outlined box.
	m_pBeatCounterWrapper = new QWidget( this );
	pMainLayout->addWidget( m_pBeatCounterWrapper );
	auto pBeatCounterWrapperLayout = new QHBoxLayout( m_pBeatCounterWrapper );
	pBeatCounterWrapperLayout->setContentsMargins( 0, 0, 0, 0 );
	pBeatCounterWrapperLayout->setSpacing( PlayerControl::nSpacing );
	m_pBeatCounterWrapper->setLayout( pBeatCounterWrapperLayout );

	m_pBeatCounterGroup = new QWidget( m_pBeatCounterWrapper );
	m_pBeatCounterGroup->setObjectName( "PlayerControlBeatCounter" );
	pBeatCounterWrapperLayout->addWidget( m_pBeatCounterGroup );
	auto pBeatCounterGroupLayout = new QHBoxLayout( m_pBeatCounterGroup );
	pBeatCounterGroupLayout->setContentsMargins( 2, 2, 2, 2 );
	m_pBeatCounterGroup->setLayout( pBeatCounterGroupLayout );

	m_sBCOnOffBtnTimelineToolTip =
		tr( "Please deactivate the Timeline first in order to use the BeatCounter" );
	m_sBCOnOffBtnJackTimebaseToolTip =
		tr( "In the presence of an external JACK Timebase controller the BeatCounter can not be used" );

	m_pBeatCounter = new BeatCounter( this );
	pBeatCounterGroupLayout->addWidget( m_pBeatCounter );

	m_pSeparatorBeatCounter = new PanelSeparator( m_pBeatCounterWrapper );
	m_pSeparatorBeatCounter->setFixedHeight( PlayerControl::nWidgetHeight );
	pBeatCounterWrapperLayout->addWidget( m_pSeparatorBeatCounter );

	////////////////////////////////////////////////////////////////////////////
	m_pRubberBandGroup = new QWidget( pMainToolbar );
	m_pRubberBandGroup->setObjectName( "PlayerControlRubberBand" );
	pMainLayout->addWidget( m_pRubberBandGroup );
	auto pRubberBandGroupLayout = new QHBoxLayout( m_pRubberBandGroup );
	pRubberBandGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pRubberBandGroupLayout->setSpacing( PlayerControl::nSpacing );

	m_pRubberBandBtn = new Button(
		m_pRubberBandGroup, buttonSize, Button::Type::Toggle, "rubberband.svg",
		"", iconSize,
		tr( "Recalculate Rubberband modified samples if bpm will change" ),
		false, true );
	m_pRubberBandBtn->setObjectName( "PlayerControlRubberbandButton" );
	m_pRubberBandBtn->setChecked( pPref->getRubberBandBatchMode() );
	connect( m_pRubberBandBtn, SIGNAL( clicked() ),
			 this, SLOT( rubberbandButtonToggle() ) );
	pRubberBandGroupLayout->addWidget( m_pRubberBandBtn );
	// test the path. if test fails, no button
	if ( QFile( pPref->m_sRubberBandCLIexecutable ).exists() == false) {
		m_pRubberBandBtn->hide();
	}

	m_pSeparatorRubberBand = new PanelSeparator( m_pRubberBandGroup );
	m_pSeparatorRubberBand->setFixedHeight( buttonSize.height() );
	pRubberBandGroupLayout->addWidget( m_pSeparatorRubberBand );

	////////////////////////////////////////////////////////////////////////////
	// Invisible wrapper group for snapshots.
	m_pJackGroup = new QWidget( this );
	m_pJackGroup->setObjectName( "JackPanel" );
	pMainLayout->addWidget( m_pJackGroup );
	auto pJackGroupLayout = new QHBoxLayout( m_pJackGroup );
	pJackGroupLayout->setContentsMargins( 0, 0, 0, 0 );
	pJackGroupLayout->setSpacing( PlayerControl::nSpacing );

	m_pJackTransportBtn = new Button(
		m_pJackGroup, buttonSize, Button::Type::Toggle, "jack.svg", "", iconSize,
		tr( "JACK transport on/off" ), false, true );
	m_pJackTransportBtn->setObjectName( "PlayerControlJackTransportButton" );
	connect( m_pJackTransportBtn, SIGNAL( clicked() ),
			 this, SLOT( jackTransportBtnClicked() ));
	pJackGroupLayout->addWidget( m_pJackTransportBtn );

	m_pJackTimebaseBtn = new Button(
		m_pJackGroup, buttonSize, Button::Type::Toggle, "jack-timebase.svg",
		"", iconSize, pCommonStrings->getJackTimebaseTooltip(), false, true );
	m_pJackTimebaseBtn->setObjectName( "PlayerControlJackTimebaseButton" );
	connect( m_pJackTimebaseBtn, SIGNAL( clicked() ), this,
			SLOT( jackTimebaseBtnClicked() ) );
	pJackGroupLayout->addWidget( m_pJackTimebaseBtn );

	m_pSeparatorJack = new PanelSeparator( m_pJackGroup );
	m_pSeparatorJack->setFixedHeight( buttonSize.height() );
	pJackGroupLayout->addWidget( m_pSeparatorJack );

	////////////////////////////////////////////////////////////////////////////
	m_pMidiControlButton = new MidiControlButton( pMainToolbar );
	m_pMidiControlButton->setFixedSize(
		buttonSize.width() * 3, buttonSize.height() );
	pMainLayout->addWidget( m_pMidiControlButton );

	m_pSeparatorMidi = new PanelSeparator( pMainToolbar );
	m_pSeparatorMidi->setFixedHeight( buttonSize.height() );
	pMainLayout->addWidget( m_pSeparatorMidi );

	////////////////////////////////////////////////////////////////////////////
	// Invisible wrapper group for snapshots.
	m_pVisibilityGroup = new QWidget( this );
	m_pVisibilityGroup->setObjectName( "PlayerControlVisibility" );
	pMainLayout->addWidget( m_pVisibilityGroup );
	auto pVisibilityLayout = new QHBoxLayout( m_pVisibilityGroup );
	pVisibilityLayout->setContentsMargins( 0, 0, 0, 0 );
	pVisibilityLayout->setSpacing( PlayerControl::nSpacing );

	m_pShowMixerBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "mixer.svg",
		"", iconSize, tr( "Show mixer" ) );
	m_pShowMixerBtn->setChecked( pPref->getMixerProperties().visible );
	connect( m_pShowMixerBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showMixer( m_pShowMixerBtn->isChecked() ); });
	pVisibilityLayout->addWidget( m_pShowMixerBtn );

	m_pShowInstrumentRackBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "component-editor.svg",
		"", iconSize + QSize( 2, 2 ), tr( "Show Instrument Rack" ) );
	m_pShowInstrumentRackBtn->setChecked(
		pPref->getInstrumentRackProperties().visible );
	connect( m_pShowInstrumentRackBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showInstrumentRack(
			m_pShowInstrumentRackBtn->isChecked() ); });
	pVisibilityLayout->addWidget( m_pShowInstrumentRackBtn );

	m_pShowDirectorBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "director.svg",
		"", iconSize + QSize( 1, 1 ), tr( "Show Director" ) );
	m_pShowDirectorBtn->setChecked( false );
	connect( m_pShowDirectorBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showDirector();
	});
	pVisibilityLayout->addWidget( m_pShowDirectorBtn );

	m_pShowPlaylistEditorBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "playlist.svg",
		"", iconSize + QSize( 2, 2 ), tr( "Show Playlist Editor" ) );
	m_pShowPlaylistEditorBtn->setChecked( false );
	connect( m_pShowPlaylistEditorBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showPlaylistEditor();
	});
	pVisibilityLayout->addWidget( m_pShowPlaylistEditorBtn );

	m_pShowAutomationBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "automation.svg",
		"", iconSize + QSize( 1, 1 ), tr( "Show Automation" ) );
	m_pShowAutomationBtn->setChecked( false );
	connect( m_pShowAutomationBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->getSongEditorPanel()->
			toggleAutomationAreaVisibility();
	});
	pVisibilityLayout->addWidget( m_pShowAutomationBtn );

	m_pShowPlaybackTrackBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle,
		"playback-track.svg", "", iconSize + QSize( 4, 4 ),
		tr( "Show Playback Track" ) );
	m_pShowPlaybackTrackBtn->setChecked( false );
	connect( m_pShowPlaybackTrackBtn, &Button::clicked, [=]() {
		if ( m_pShowPlaybackTrackBtn->isChecked() ) {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				showTimeline();
		} else {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				showPlaybackTrack();
		}
	});
	pVisibilityLayout->addWidget( m_pShowPlaybackTrackBtn );

	m_pShowPreferencesBtn = new Button(
		m_pVisibilityGroup, buttonSize, Button::Type::Toggle, "cog.svg", "",
		iconSize - QSize( 6, 6 ), tr( "Show Preferences" ) );
	m_pShowPreferencesBtn->setChecked( false );
	connect( m_pShowPreferencesBtn, &Button::clicked, [&]() {
		HydrogenApp::get_instance()->showPreferencesDialog();
	});
	pVisibilityLayout->addWidget( m_pShowPreferencesBtn );

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
	if ( m_pRubberBandBtn->isChecked() != pPref->getRubberBandBatchMode() ) {
		m_pRubberBandBtn->setChecked( pPref->getRubberBandBatchMode());
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
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() !=
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
	auto pHydrogen = Hydrogen::get_instance();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning( this, "Hydrogen",
							   QString( "%1\n%2" )
							  .arg( pCommonStrings->getAudioDriverNotPresent() )
							  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
		return;
	}
	
	if ( m_pPlayBtn->isChecked() ) {
		pHydrogen->sequencerPlay();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Playing.") );
	}
	else {
		pHydrogen->sequencerStop();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Pause.") );
	}
}

/// Stop audio engine
void PlayerControl::stopBtnClicked()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning( this, "Hydrogen",
							   QString( "%1\n%2" )
							  .arg( pCommonStrings->getAudioDriverNotPresent() )
							  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
		return;
	}
	
	pHydrogen->sequencerStop();
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
	// Immediate update and prevent buttons from being inchecked when clicked
	// twice.
	updateSongMode();
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
	if ( m_pRubberBandBtn->isChecked() ) {
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
	if ( ! Hydrogen::get_instance()->hasJackAudioDriver() ) {
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
	if ( ! Hydrogen::get_instance()->hasJackTransport() ) {
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
		m_pBeatCounterWrapper->show();
	}
	else {
		m_pBeatCounterWrapper->hide();
		return;
	}

	m_pBeatCounter->updateBeatCounter();

	switch ( pHydrogen->getTempoSource() ) {
	case H2Core::Hydrogen::Tempo::Jack:
		m_pBeatCounterWrapper->setToolTip( m_sBCOnOffBtnJackTimebaseToolTip );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pBeatCounterWrapper->setToolTip( m_sBCOnOffBtnTimelineToolTip );
		break;
	default:
		m_pBeatCounterWrapper->setToolTip( "" );
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
	const QColor colorToolbar = colorTheme.m_baseColor;
	const QColor colorToolbarLighter = colorToolbar.lighter( 130 );

	QColor colorGroupBoxBorder, colorGroupBoxBackground;
	if ( Skin::moreBlackThanWhite( colorToolbar ) ) {
		colorGroupBoxBorder = colorToolbar.darker(
			Skin::nPanelGroupBoxBorderScaling );
		colorGroupBoxBackground = colorToolbar.darker(
			Skin::nPanelGroupBoxBackgroundScaling );
	}
	else {
		colorGroupBoxBorder = colorToolbar.lighter(
			Skin::nPanelGroupBoxBorderScaling );
		colorGroupBoxBackground = colorToolbar.lighter(
			Skin::nPanelGroupBoxBackgroundScaling );
	}

	setStyleSheet( QString( "\
QWidget#MainToolbar {\
     background-color: %1; \
     color: %2; \
     border: %3px solid #000;\
}")
				   .arg( colorToolbar.name() ).arg( colorText.name() )
				   .arg( PlayerControl::nBorder ) );

	m_pEditorGroup->setBackgroundColor( colorGroupBoxBackground );
	m_pEditorGroup->setBorderColor( colorGroupBoxBorder );
	m_pEditorGroup->updateStyleSheet();

	m_pBeatCounterGroup->setStyleSheet( QString( "\
#PlayerControlBeatCounter {\
    background-color: %1;\
    color: %2;\
    border: %3px solid %4;\
    border-radius: 2px;\
}" )
		.arg( colorGroupBoxBackground.name() ).arg( colorText.name() )
		.arg( PlayerControl::nBorder ).arg( colorGroupBoxBorder.name() ) );

	m_pSeparatorEditor->setColor( colorGroupBoxBorder );
	m_pSeparatorTransport->setColor( colorGroupBoxBorder );
	m_pSeparatorBeatCounter->setColor( colorGroupBoxBorder );
	m_pSeparatorTempo->setColor( colorGroupBoxBorder );
	m_pSeparatorRubberBand->setColor( colorGroupBoxBorder );
	m_pSeparatorJack->setColor( colorGroupBoxBorder );
	m_pSeparatorMidi->setColor( colorGroupBoxBorder );

	m_pBeatCounter->setBackgroundColor( colorGroupBoxBackground );
	m_pBeatCounter->setBorderColor( colorGroupBoxBorder );
	m_pBeatCounter->updateStyleSheet();
	m_pMetronomeBtn->updateStyleSheet();
}
