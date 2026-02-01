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

#include "MainToolBar.h"

#include "BpmSpinBox.h"
#include "BpmTap.h"
#include "MidiControlButton.h"
#include "MidiControlDialog.h"
#include "../Compatibility/MouseEvent.h"
#include "../CommonStrings.h"
#include "../Director.h"
#include "../HydrogenApp.h"
#include "../Rack/Rack.h"
#include "../Mixer/Mixer.h"
#include "../PlaylistEditor/PlaylistEditor.h"
#include "../Skin.h"
#include "../SongEditor/PlaybackTrackWaveDisplay.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../Widgets/AutomationPathView.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/MidiLearnableToolButton.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/IO/JackDriver.h>
#include <core/Midi/MidiAction.h>
#include <core/Hydrogen.h>

using namespace H2Core;

MainToolBar::MainToolBar( QWidget* pParent) : QToolBar( pParent )
											, m_input( Editor::Input::Select )
{
	const auto pPref = Preferences::get_instance();
	const auto pSong = Hydrogen::get_instance()->getSong();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setFixedHeight( MainToolBar::nHeight );
	setMinimumWidth( HydrogenApp::nMinimumWidth );
	setFocusPolicy( Qt::ClickFocus );
	setObjectName( "MainToolBar" );

	const int nButtonHeight = MainToolBar::nWidgetHeight - 2;
	const auto buttonSize = QSize(
		static_cast<int>(std::round( nButtonHeight *
									 Skin::fButtonWidthHeightRatio ) ),
		nButtonHeight );

	auto createAction = [&]( const QString& sText ) {
		auto pAction = new QAction( this );
		pAction->setCheckable( true );
		pAction->setIconText( sText );
		pAction->setToolTip( sText );

		return pAction;
	};

	auto createButton = [&]( const QString& sText, bool bCheckable ) {
		auto pButton = new MidiLearnableToolButton( this, sText );
		pButton->setCheckable( bCheckable );

		return pButton;
	};
	////////////////////////////////////////////////////////////////////////////

	auto pInputModeGroup = new QActionGroup( this );
	pInputModeGroup->setExclusive( true );

	m_pSelectAction = createAction( pCommonStrings->getSelectModeButton() );
	m_pSelectAction->setObjectName( "PatternEditorSelectModeBtn" );
	connect( m_pSelectAction, &QAction::triggered, [&](){
		if ( m_input != Editor::Input::Select ) {
			m_input = Editor::Input::Select;
			updateInput();
		}
	} );
	addAction( m_pSelectAction );
	pInputModeGroup->addAction( m_pSelectAction );

	m_pDrawAction = createAction( pCommonStrings->getDrawModeButton() );
	m_pDrawAction->setObjectName( "PatternEditorDrawModeBtn" );
	connect( m_pDrawAction, &QAction::triggered, [&](){
		if ( m_input != Editor::Input::Draw ) {
			m_input = Editor::Input::Draw;
			updateInput();
		}
	} );
	addAction( m_pDrawAction );
	pInputModeGroup->addAction( m_pDrawAction );

	addSeparator();

	////////////////////////////////////////////////////////////////////////////
	auto pEditorGroup = new QActionGroup( this );
	pEditorGroup->setExclusive( true );

	m_pPatternModeAction = createAction( tr( "Pattern Mode" ) );
	m_pPatternModeAction->setObjectName( "MainToolBarPatternModeButton" );
	connect( m_pPatternModeAction, &QAction::triggered, [=]() {
		activateSongMode( false ); } );
	addAction( m_pPatternModeAction );
	pEditorGroup->addAction( m_pPatternModeAction );

	m_pSongModeAction = createAction( tr( "Song Mode" ) );
	m_pSongModeAction->setObjectName( "MainToolBarSongModeButton" );
	connect( m_pSongModeAction, &QAction::triggered, [=]() {
		activateSongMode( true ); } );
	addAction( m_pSongModeAction );
	pEditorGroup->addAction( m_pSongModeAction );

	addSeparator();

	////////////////////////////////////////////////////////////////////////////
	m_pTimeDisplay = new LCDDisplay(
		nullptr, QSize( 146, MainToolBar::nWidgetHeight ), true, false );
	m_pTimeDisplay->setAlignment( Qt::AlignRight );
	m_pTimeDisplay->setText( "00:00:00:000" );
	m_pTimeDisplay->setStyleSheet(
		m_pTimeDisplay->styleSheet().append(
			QString( " QLineEdit { font-size: %1px; }" )
			.arg( MainToolBar::nFontSize ) ) );
	addWidget( m_pTimeDisplay );

	////////////////////////////////////////////////////////////////////////////
	// Rewind button
	m_pRwdButton = createButton( tr( "Rewind" ), false );
	m_pRwdButton->setObjectName( "MainToolBarRewindButton" );
	connect( m_pRwdButton, &QToolButton::clicked, [&]() {
		rewindBtnClicked();
	});
	m_pRwdButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::PreviousBar ) );
	addWidget( m_pRwdButton );

	// Record button
	m_pRecButton = createButton( tr( "Record" ), true );
	m_pRecButton->setObjectName( "MainToolBarRecordButton" );
	connect( m_pRecButton, &QToolButton::clicked, [&]() {
		recBtnClicked();
	});
	m_pRecButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::RecordReady ) );
	addWidget( m_pRecButton );

	// Play button
	m_pPlayButton = createButton( tr( "Play/ Pause" ), true );
	m_pPlayButton->setObjectName( "MainToolBarPlayButton" );
	connect( m_pPlayButton, &QToolButton::clicked, [&]() {
		playBtnClicked();
	} );

	m_pPlayMidiAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlayPauseToggle );
	m_pPlayAction = new QAction( this );
	m_pPlayAction->setCheckable( true );
	m_pPlayAction->setText( tr( "Play/ Pause" ) );
	connect( m_pPlayAction, &QAction::triggered, [=]() {
		auto pPref = Preferences::get_instance();
		if ( pPref->getCountIn() ) {
			pPref->setCountIn( false );
		}
		if ( m_pPlayButton->defaultAction() != m_pPlayAction ) {
			m_pPlayButton->setDefaultAction( m_pPlayAction );
		}
		m_pPlayButton->setMidiAction( m_pPlayMidiAction );
		updateTransportControl();
	} );
	m_pCountInMidiAction = std::make_shared<MidiAction>(
		MidiAction::Type::PlayPauseToggle );
	m_pCountInAction = new QAction( this );
	m_pCountInAction->setCheckable( true );
	m_pCountInAction->setText( tr( "Count in and play/ Pause" ) );
	connect( m_pCountInAction, &QAction::triggered, [=]() {
		auto pPref = Preferences::get_instance();
		if ( ! pPref->getCountIn() ) {
			pPref->setCountIn( true );
		}
		if ( m_pPlayButton->defaultAction() != m_pCountInAction ) {
			m_pPlayButton->setDefaultAction( m_pCountInAction );
		}
		m_pPlayButton->setMidiAction( m_pCountInMidiAction );
		updateTransportControl();
	} );

	m_pPlayButton->addAction( m_pPlayAction );
	m_pPlayButton->addAction( m_pCountInAction );
	m_pPlayButton->setDefaultAction(
		pPref->getCountIn() ? m_pCountInAction : m_pPlayAction );
	m_pPlayButton->setMidiAction(
		pPref->getCountIn() ? m_pCountInMidiAction : m_pPlayMidiAction );
	addWidget( m_pPlayButton );

	// Stop button
	m_pStopButton = createButton( tr( "Stop" ), false );
	m_pStopButton->setObjectName( "MainToolBarStopButton" );
	connect( m_pStopButton, &QToolButton::clicked, [&](){
		stopBtnClicked();
	});
	m_pStopButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::Stop ) );
	addWidget( m_pStopButton );

	// Fast forward button
	m_pFfwdButton = createButton( tr( "Fast Forward" ), false );
	m_pFfwdButton->setObjectName( "MainToolBarForwardButton" );
	connect( m_pFfwdButton, &QToolButton::clicked, [&](){
		fastForwardBtnClicked();
	});
	m_pFfwdButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::NextBar ) );
	addWidget( m_pFfwdButton );

	// Loop song button button
	m_pSongLoopAction = createAction( tr( "Loop song" ) );
	m_pSongLoopAction->setObjectName( "MainToolBarLoopButton" );
	connect( m_pSongLoopAction, &QAction::triggered,
			 [=]( bool bChecked ) {
				 auto pHydrogenApp = HydrogenApp::get_instance();
				 CoreActionController::activateLoopMode(
					 m_pSongLoopAction->isChecked() );
				 if ( m_pSongLoopAction->isChecked() ) {
					 pHydrogenApp->showStatusBarMessage( tr("Loop song = On") );
				 } else {
					 pHydrogenApp->showStatusBarMessage( tr("Loop song = Off") );
				 }

			 });
	addAction( m_pSongLoopAction );

	addSeparator();

	////////////////////////////////////////////////////////////////////////////
	m_pMetronomeButton = createButton( tr( "Switch metronome on/off" ), true );
	m_pMetronomeButton->setObjectName( "MetronomeButton" );
	connect( m_pMetronomeButton, &QToolButton::clicked, []( bool bChecked ) {
		CoreActionController::setMetronomeIsActive( bChecked );
	} );
	m_pMetronomeButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::ToggleMetronome ) );
	addWidget( m_pMetronomeButton );

	m_sLCDBPMSpinboxToolTip =
		tr("Alter the Playback Speed");
	m_sLCDBPMSpinboxTimelineToolTip =
		tr( "While the Timeline is active this widget is in read-only mode and just displays the tempo set using the current Timeline position" );
	m_sLCDBPMSpinboxJackTimebaseToolTip =
		tr( "In the presence of an external JACK Timebase controller this widget just displays the tempo broadcasted by JACK" );

	m_pBpmSpinBox = new BpmSpinBox(
		this, QSize( 95, MainToolBar::nWidgetHeight ) );
	m_pBpmSpinBox->setStyleSheet(
		m_pBpmSpinBox->styleSheet().append(
			QString( " QAbstractSpinBox {font-size: %1px;}" )
			.arg( MainToolBar::nFontSize ) ) );
	connect( m_pBpmSpinBox, SIGNAL( valueChanged( double ) ),
			 this, SLOT( bpmChanged( double ) ) );
	addWidget( m_pBpmSpinBox );

	m_pBpmTap = new BpmTap( this );
	addWidget( m_pBpmTap );

	addSeparator();

	////////////////////////////////////////////////////////////////////////////

	m_pMidiControlDialog = new MidiControlDialog( this );

	m_pMidiControlButton = new MidiControlButton( this );
	m_pMidiControlButton->setFixedHeight( buttonSize.height() );
	connect( m_pMidiControlButton, &QToolButton::clicked, [&]() {
		m_pMidiControlDialog->setVisible( m_pMidiControlButton->isChecked() );
	});
	addWidget( m_pMidiControlButton );

	addSeparator();

	////////////////////////////////////////////////////////////////////////////
	m_pJackTransportAction = createAction( tr( "JACK transport on/off" ) );
	m_pJackTransportAction->setObjectName( "MainToolBarJackTransportButton" );
	connect( m_pJackTransportAction, &QAction::triggered, [&]() {
		jackTransportBtnClicked();
	});
	addAction( m_pJackTransportAction );

	m_pJackTimebaseButton = createButton(
		pCommonStrings->getJackTimebaseToolTip(), true );
	m_pJackTimebaseButton->setObjectName( "JackTimebaseButton" );
	connect( m_pJackTimebaseButton, &QToolButton::clicked, [&]() {
		jackTimebaseBtnClicked();
	} );
	m_pJackTimebaseAction = addWidget( m_pJackTimebaseButton );

	m_pJackSeparator = addSeparator();

	////////////////////////////////////////////////////////////////////////////
	m_pRubberBandAction = createAction(
		tr( "Recalculate Rubberband modified samples if bpm will change" ) );
	m_pRubberBandAction->setObjectName( "MainToolBarRubberbandButton" );
	m_pRubberBandAction->setChecked( pPref->getRubberBandBatchMode() );
	connect( m_pRubberBandAction, &QAction::triggered, [&]() {
			 rubberbandButtonToggle();
	});
	addAction( m_pRubberBandAction );

	m_pRubberBandSeparator = addSeparator();

	// test the path. if test fails, no button
	if ( QFile( pPref->m_sRubberBandCLIexecutable ).exists() == false) {
		m_pRubberBandAction->setVisible( false );
		m_pRubberBandSeparator->setVisible( false );
	}

	////////////////////////////////////////////////////////////////////////////
	m_pShowPlaylistEditorAction = createAction( tr( "Show Playlist Editor" ) );
	connect( m_pShowPlaylistEditorAction, &QAction::triggered, []() {
		HydrogenApp::get_instance()->showPlaylistEditor();
	});
	addAction( m_pShowPlaylistEditorAction );

	m_pShowDirectorAction = createAction( tr( "Show Director" ) );
	connect( m_pShowDirectorAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->showDirector();
	});
	addAction( m_pShowDirectorAction );

	m_pShowMixerAction = createAction( tr( "Show mixer" ) );
	connect( m_pShowMixerAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->showMixer( m_pShowMixerAction->isChecked() ); });
	addAction( m_pShowMixerAction );

	m_pShowRackAction = createAction( tr( "Show Rack" ) );
	connect( m_pShowRackAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->showRack(
			m_pShowRackAction->isChecked() ); });
	addAction( m_pShowRackAction );

	m_pShowAutomationAction = createAction( tr( "Show Automation" ) );
	connect( m_pShowAutomationAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->getSongEditorPanel()->
			toggleAutomationAreaVisibility();
	});
	addAction( m_pShowAutomationAction );

	m_pShowPlaybackTrackAction = createAction( tr( "Show Playback Track" ) );
	connect( m_pShowPlaybackTrackAction, &QAction::triggered, [=]() {
		if ( m_pShowPlaybackTrackAction->isChecked() ) {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				showPlaybackTrack();
		} else {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				showTimeline();
		}
	});
	addAction( m_pShowPlaybackTrackAction );

	m_pShowPreferencesAction = createAction( tr( "Show Preferences" ) );
	connect( m_pShowPreferencesAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->showPreferencesDialog();
	});
	addAction( m_pShowPreferencesAction );

	////////////////////////////////////////////////////////////////////////////

	QTimer *timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( updateTime() ) );
	timer->start( 100 );	// update at 10 fps
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &MainToolBar::onPreferencesChanged );

	////////////////////////////////////////////////////////////////////////////
	m_pTimer = new QTimer();
	connect( m_pTimer, &QTimer::timeout, [=]() {
		m_pMetronomeButton->setStyleSheet( "" );
		m_pTimer->stop();
	} );

	updateBpmSpinBox();
	updateInput();
	updateJackTimebase();
	updateJackTransport();
	updateLoopMode();
	updateRecordMode();
	updateSongMode();
	updateIcons();
	updateStyleSheet();
	m_pBpmTap->updateBpmTap();

	HydrogenApp::get_instance()->addEventListener( this );
}

MainToolBar::~MainToolBar() {
}

void MainToolBar::setMidiControlDialogVisible( bool bVisible ) {
	m_pMidiControlDialog->setVisible( bVisible );
	updateActions();
}

void MainToolBar::setPreferencesVisibilityState( bool bChecked ) {
	m_pShowPreferencesAction->setChecked( bChecked );
}

void MainToolBar::updateActions() {
	const auto pPref = Preferences::get_instance();
	const auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogenApp == nullptr ) {
		return;
	}

	m_pMidiControlButton->setChecked( m_pMidiControlDialog->isVisible() );

	if ( pHydrogenApp->getPlaylistEditor() != nullptr ) {
		m_pShowPlaylistEditorAction->setChecked(
			pHydrogenApp->getPlaylistEditor()->isVisible() );
	}
	if ( pHydrogenApp->getDirector() != nullptr ) {
		m_pShowDirectorAction->setChecked(
			pHydrogenApp->getDirector()->isVisible() );
	}
	if ( pHydrogenApp->getMixer() != nullptr ) {
		m_pShowMixerAction->setChecked( pHydrogenApp->getMixer()->isVisible() );
	}
	if ( pHydrogenApp->getRack() != nullptr ) {
		m_pShowRackAction->setChecked(
			pHydrogenApp->getRack()->isVisible() );
	}
	if ( pHydrogenApp->getSongEditorPanel() != nullptr ) {
		m_pShowAutomationAction->setChecked(
			pHydrogenApp->getSongEditorPanel()->getAutomationPathView()
			->isVisible() );
		m_pShowPlaybackTrackAction->setChecked(
			pHydrogenApp->getSongEditorPanel()->getPlaybackTrackWaveDisplay()
			->isVisible() );
	}

	m_pMetronomeButton->setChecked( pPref->m_bUseMetronome );

	// Rubberband
	if ( m_pRubberBandAction->isChecked() != pPref->getRubberBandBatchMode() ) {
		m_pRubberBandAction->setChecked( pPref->getRubberBandBatchMode());
	}
}

void MainToolBar::updateInput() {
	m_pSelectAction->setChecked( m_input == Editor::Input::Select );
	m_pDrawAction->setChecked( m_input == Editor::Input::Draw );
}

void MainToolBar::setInput( Editor::Input input ) {
	if ( m_input != input ) {
		m_input = input;
		updateInput();
	}
}

void MainToolBar::audioDriverChangedEvent() {
	updateJackTransport();
	updateJackTimebase();
	updateTransportControl();
}

void MainToolBar::jackTransportActivationEvent() {
	updateJackTransport();
	updateJackTimebase();
}

void MainToolBar::jackTimebaseStateChangedEvent( int nState )
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
	case JackDriver::Timebase::Controller:
		sMessage.append( "Controller" );
		break;

	case JackDriver::Timebase::Listener:
		sMessage.append( "Listener" );
		break;

	default:
		sMessage.append( pCommonStrings->getStatusOff() );
	}
	HydrogenApp::get_instance()->showStatusBarMessage( sMessage );

	m_pBpmTap->updateBpmTap();
	updateBpmSpinBox();
}

void MainToolBar::loopModeActivationEvent() {
	updateLoopMode();
}

void MainToolBar::midiClockActivationEvent() {
	updateBpmSpinBox();
	m_pBpmTap->updateBpmTap();
}

void MainToolBar::metronomeEvent( int nValue ) {
	const auto pPref = H2Core::Preferences::get_instance();

	// Value 2 corresponds to the metronome being turned on or off.
	if ( nValue == 2 ) {
		updateActions();
		return;
	}

	if ( ! pPref->m_bUseMetronome &&
		 Hydrogen::get_instance()->getAudioEngine()->getState() !=
		 AudioEngine::State::CountIn ) {
		return;
	}

	if ( nValue == 0 ) {
		m_pMetronomeButton->setStyleSheet( QString( "\
#MetronomeButton {				 \
   background-color: %1;\
}" ).arg( pPref->getColorTheme()->m_highlightColor.name() ) );
	}
	else {
		m_pMetronomeButton->setStyleSheet( QString( "\
#MetronomeButton {\
   background-color: %1;\
}" ).arg( pPref->getColorTheme()->m_accentColor.name() ) );
	}

	// Percentage [0,1] the button stays highlighted over the duration of a
	// beat. We make this one tempo-dependent so it works for both small and
	// high tempi as well.
	const float fPercentage = 0.5;
	const auto fBpm = H2Core::Hydrogen::get_instance()->getAudioEngine()->
		getTransportPosition()->getBpm();
	const std::chrono::milliseconds duration{ static_cast<int>(
		std::round( 60 * 1000 / fBpm * fPercentage))};
	m_pTimer->start( duration );
}

void MainToolBar::recordingModeChangedEvent() {
	updateRecordMode();
}

void MainToolBar::songModeActivationEvent() {
	updateSongMode();
	updateBpmSpinBox();
	m_pBpmTap->updateBpmTap();
	updateTransportControl();
}

void MainToolBar::stateChangedEvent( const H2Core::AudioEngine::State& ) {
	updateTransportControl();
}

void MainToolBar::tempoChangedEvent( int nValue )
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

		// Discard all pending tempo change events in order to not create a
		// flood of popups. Only after this method returns, the next event will
		// be handled.
		EventQueue::get_instance()->dropEvents( Event::Type::TempoChanged );
	}
}

void MainToolBar::timelineActivationEvent() {
	updateBpmSpinBox();
	m_pBpmTap->updateBpmTap();
}

void MainToolBar::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateSongMode();
		updateBpmSpinBox();
		m_pBpmTap->updateBpmTap();
		updateLoopMode();
		updateJackTransport();
		updateJackTimebase();
		updateActions();
		updateTransportControl();
	}
}

/// Toggle record mode
void MainToolBar::recBtnClicked() {
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() !=
		 H2Core::AudioEngine::State::Playing ) {
		if ( m_pRecButton->isChecked() ) {
			CoreActionController::activateRecordMode( true );
			(HydrogenApp::get_instance())->showStatusBarMessage(
				tr("Record midi events = On" ) );
		}
		else {
			CoreActionController::activateRecordMode( false );
			(HydrogenApp::get_instance())->showStatusBarMessage(
				tr("Record midi events = Off" ) );
		}
	}
}

/// Start audio engine
void MainToolBar::playBtnClicked() {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	const auto pPref = Preferences::get_instance();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 std::dynamic_pointer_cast<NullDriver>( pHydrogen->getAudioOutput() ) !=
			 nullptr ) {
		QMessageBox::warning(
			this, "Hydrogen",
			QString( "%1\n%2" )
				.arg( pCommonStrings->getAudioDriverNotPresent() )
				.arg( pCommonStrings->getAudioDriverErrorHint() )
		);
		return;
	}

	if ( pHydrogen->getAudioEngine()->getState() != AudioEngine::State::Playing ) {
		if ( pPref->getCountIn() ) {
			CoreActionController::startCountIn();
			pHydrogenApp->showStatusBarMessage( tr( "Counting in" ) );
		}
		else {
			pHydrogen->sequencerPlay();
			pHydrogenApp->showStatusBarMessage( tr( "Playing." ) );
		}
	}
	else {
		pHydrogen->sequencerStop();
		pHydrogenApp->showStatusBarMessage( tr( "Pause." ) );
	}
}

/// Stop audio engine
void MainToolBar::stopBtnClicked()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();

	// Hint that something is wrong in case there is no proper audio
	// driver set.
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 std::dynamic_pointer_cast<NullDriver>( pHydrogen->getAudioOutput() ) !=
			 nullptr ) {
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

void MainToolBar::updateTime() {
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

void MainToolBar::activateSongMode( bool bActivate ) {
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

void MainToolBar::bpmChanged( double fNewBpmValue ) {
	if ( m_pBpmSpinBox->getIsActive() ) {
		CoreActionController::setBpm( static_cast<float>( fNewBpmValue ) );
	}
}

void MainToolBar::rubberbandButtonToggle()
{
	auto pPref = Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( m_pRubberBandAction->isChecked() ) {
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

	updateActions();
}

void MainToolBar::beatCounterEvent() {
	m_pBpmTap->updateBpmTap();
}

void MainToolBar::jackTransportBtnClicked()
{
	if ( ! Hydrogen::get_instance()->hasJackDriver() ) {
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

void MainToolBar::jackTimebaseBtnClicked()
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

void MainToolBar::fastForwardBtnClicked() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		const int nCurrentColumn =
			pHydrogen->getAudioEngine()->getTransportPosition()->getColumn();
		if ( nCurrentColumn < pSong->getPatternGroupVector()->size() - 1 ) {
			// Not within the last column
			CoreActionController::locateToColumn(
				std::max( nCurrentColumn, 0 ) + 1 );
		}
		else {
			// Last one. If looping is enabled, we jump to the first column. If
			// not, we "reach" the end of the song by stopping.
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				CoreActionController::locateToColumn( 0 );
			} else {
				stopBtnClicked();
			}
		}
	}
}

void MainToolBar::rewindBtnClicked() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( pHydrogen->getMode() == Song::Mode::Song ) {
		const int nCurrentColumn =
			pHydrogen->getAudioEngine()->getTransportPosition()->getColumn();
		int nNewColumn;
		if ( nCurrentColumn > 0 ) {
			nNewColumn = std::max( nCurrentColumn - 1, 0 );
		}
		else {
			// Within the first column we either jump to the last one if loop
			// mode is enabled, or to the beginning of the song.
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				nNewColumn = pSong->getPatternGroupVector()->size() - 1;
			} else {
				nNewColumn = 0;
			}
		}
		CoreActionController::locateToColumn( nNewColumn );
	}
}

void MainToolBar::updateBpmSpinBox() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
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
	case H2Core::Hydrogen::Tempo::Midi:
		m_pBpmSpinBox->setToolTip(
			pCommonStrings->getTimelineDisabledMidiClock() );
		break;
	case H2Core::Hydrogen::Tempo::Timeline:
		m_pBpmSpinBox->setToolTip( m_sLCDBPMSpinboxTimelineToolTip );
		break;
	default:
		m_pBpmSpinBox->setToolTip( m_sLCDBPMSpinboxToolTip );
	}
}

void MainToolBar::updateJackTransport() {
	auto pHydrogen = Hydrogen::get_instance();
	const bool bVisible = pHydrogen->hasJackDriver();
	m_pJackTimebaseAction->setVisible( bVisible &&
		Preferences::get_instance()->m_bJackTimebaseEnabled );
	m_pJackTransportAction->setVisible( bVisible );
	m_pJackSeparator->setVisible( bVisible );

	if ( pHydrogen->hasJackTransport() ) {
		m_pJackTransportAction->setChecked( true );
	} else {
		m_pJackTransportAction->setChecked( false );
	}
}

void MainToolBar::updateJackTimebase()
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	auto pHydrogen = Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const bool bVisible = pHydrogen->hasJackDriver();
	m_pJackTimebaseAction->setVisible( bVisible &&
		Preferences::get_instance()->m_bJackTimebaseEnabled );
	m_pJackTransportAction->setVisible( bVisible );
	m_pJackSeparator->setVisible( bVisible );

	if ( ! m_pJackTimebaseAction->isVisible() ) {
		return;
	}

	m_pJackTimebaseButton->setStyleSheet( "" );
	if ( pHydrogen->hasJackTransport() ) {
		switch ( pHydrogen->getJackTimebaseState() ) {
		case JackDriver::Timebase::Controller:
			m_pJackTimebaseButton->setChecked( true );
			m_pJackTimebaseButton->setStyleSheet( QString( "\
#JackTimebaseButton {\
    background-color: %1;\
}" ).arg( pColorTheme->m_highlightColor.name() ) );
			m_pJackTimebaseButton->setToolTip(
				pCommonStrings->getJackTimebaseControllerToolTip() );
			break;

		case JackDriver::Timebase::Listener:
			m_pJackTimebaseButton->setChecked( true );
			m_pJackTimebaseButton->setStyleSheet( QString( "\
#JackTimebaseButton {\
    background-color: %1;\
}" ).arg( pColorTheme->m_buttonRedColor.name() ) );
			m_pJackTimebaseButton->setToolTip(
				pCommonStrings->getJackTimebaseListenerToolTip() );
			break;

		default:
			m_pJackTimebaseButton->setToolTip(
				pCommonStrings->getJackTimebaseToolTip() );
		}
	}
}

void MainToolBar::updateLoopMode() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
		m_pSongLoopAction->setChecked( true );
	} else {
		m_pSongLoopAction->setChecked( false );
	}
}

void MainToolBar::updateRecordMode() {
	auto pHydrogen = Hydrogen::get_instance();
	m_pRecButton->setChecked( pHydrogen->getRecordEnabled() );
}

void MainToolBar::updateSongMode() {
	auto pHydrogen = Hydrogen::get_instance();

	const bool bSongMode = pHydrogen->getMode() == Song::Mode::Song;
	m_pSongModeAction->setChecked( bSongMode );
	m_pPatternModeAction->setChecked( ! bSongMode );
	m_pRwdButton->setEnabled( bSongMode );
	m_pFfwdButton->setEnabled( bSongMode );
	m_pSongLoopAction->setEnabled( bSongMode );
}

void MainToolBar::updateTransportControl() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	m_pPlayButton->setChecked(
		pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing );
	m_pRecButton->setChecked( pHydrogen->getRecordEnabled() );
}

void MainToolBar::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::AudioTab ) {
		updateJackTransport();
		updateJackTimebase();
	}
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcons();
	}
}

void MainToolBar::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pSelectAction->setIcon( QIcon( sIconPath + "select.svg" ) );
	m_pDrawAction->setIcon( QIcon( sIconPath + "draw.svg" ) );
	m_pRwdButton->setIcon( QIcon( sIconPath + "rewind.svg" ) );
	m_pRecButton->setIcon(
		QIcon( sIconPath + "record.svg" ) );
	m_pPlayAction->setIcon( QIcon( sIconPath + "play.svg" ) );
	m_pCountInAction->setIcon( QIcon( sIconPath + "play_count_in.svg" ) );
	m_pStopButton->setIcon( QIcon( sIconPath + "stop.svg" ) );
	m_pFfwdButton->setIcon( QIcon( sIconPath + "fast_forward.svg" ) );
	m_pSongLoopAction->setIcon( QIcon( sIconPath + "loop.svg" ) );
	m_pPatternModeAction->setIcon( QIcon( sIconPath + "pattern-editor.svg" ) );
	m_pSongModeAction->setIcon( QIcon( sIconPath + "song-editor.svg" ) );
	m_pMetronomeButton->setIcon( QIcon( sIconPath + "metronome.svg" ) );
	m_pRubberBandAction->setIcon( QIcon( sIconPath + "rubberband.svg" ) );
	m_pJackTransportAction->setIcon( QIcon( sIconPath + "jack.svg" ) );
	m_pJackTimebaseButton->setIcon( QIcon( sIconPath + "jack-timebase.svg" ) );
	m_pShowPlaylistEditorAction->setIcon( QIcon( sIconPath + "playlist.svg" ) );
	m_pShowDirectorAction->setIcon( QIcon( sIconPath + "director.svg" ) );
	m_pShowMixerAction->setIcon( QIcon( sIconPath + "mixer.svg" ) );
	m_pShowRackAction->setIcon(
		QIcon( sIconPath + "rack.svg" ) );
	m_pShowAutomationAction->setIcon( QIcon( sIconPath + "automation.svg" ) );
	m_pShowPlaybackTrackAction->setIcon(
		QIcon( sIconPath + "playback-track.svg" ) );
	m_pShowPreferencesAction->setIcon( QIcon( sIconPath + "cog.svg" ) );

	m_pBpmTap->updateIcons();
	m_pMidiControlButton->updateIcons();
}

void MainToolBar::updateStyleSheet() {

	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();

	const QColor colorBackground =
		pColorTheme->m_songEditor_backgroundColor.darker( 110 );

		setStyleSheet( QString( "\
QToolBar {\
     background-color: %1; \
     border: %2px solid #000;\
     spacing: %3px;\
}")
				   .arg( colorBackground.name() ).arg( MainToolBar::nBorder )
				   .arg( MainToolBar::nSpacing ) );

	m_pBpmTap->setBackgroundColor( colorBackground );
	m_pBpmTap->updateStyleSheet();
}
