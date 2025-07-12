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
#include "../InstrumentRack.h"
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
#include <core/IO/JackAudioDriver.h>
#include <core/Midi/MidiAction.h>
#include <core/Hydrogen.h>

using namespace H2Core;

MainToolBar::MainToolBar( QWidget* pParent) : QToolBar( pParent ) {

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
		auto pAction = new MidiLearnableToolButton( this, sText );
		pAction->setCheckable( bCheckable );

		return pAction;
	};

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
	m_pPlayButton->setMidiAction(
		std::make_shared<MidiAction>( MidiAction::Type::PlayPauseToggle ) );
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
	auto pEditorGroup = new QButtonGroup( this );
	pEditorGroup->setExclusive( true );

	m_pPatternModeButton = createButton( tr( "Pattern Mode" ), true );
	m_pPatternModeButton->setObjectName( "MainToolBarPatternModeButton" );
	connect( m_pPatternModeButton, &QToolButton::clicked,
			[=]() { activateSongMode( false ); } );
	addWidget( m_pPatternModeButton );
	pEditorGroup->addButton( m_pPatternModeButton );

	m_pSongModeButton = createButton( tr( "Song Mode" ), true );
	m_pSongModeButton->setObjectName( "MainToolBarSongModeButton" );
	connect( m_pSongModeButton, &QToolButton::clicked,
			[=]() { activateSongMode( true ); } );
	addWidget( m_pSongModeButton );
	pEditorGroup->addButton( m_pSongModeButton );

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
	m_pRubberBandAction = createAction(
		tr( "Recalculate Rubberband modified samples if bpm will change" ) );
	m_pRubberBandAction->setObjectName( "MainToolBarRubberbandButton" );
	m_pRubberBandAction->setChecked( pPref->getRubberBandBatchMode() );
	connect( m_pRubberBandAction, &QAction::triggered, [&]() {
			 rubberbandButtonToggle();
	});
	// test the path. if test fails, no button
	if ( QFile( pPref->m_sRubberBandCLIexecutable ).exists() == false) {
		m_pRubberBandAction->setVisible( false );
	}
	addAction( m_pRubberBandAction );

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
	addWidget( m_pJackTimebaseButton );

	m_pJackSeparator = addSeparator();

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

	m_pShowInstrumentRackAction = createAction( tr( "Show Instrument Rack" ) );
	connect( m_pShowInstrumentRackAction, &QAction::triggered, [&]() {
		HydrogenApp::get_instance()->showInstrumentRack(
			m_pShowInstrumentRackAction->isChecked() ); });
	addAction( m_pShowInstrumentRackAction );

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
	updateJackTimebase();
	updateJackTransport();
	updateLoopMode();
	updateSongMode();
	updateIcons();
	updateStyleSheet();
	m_pBpmTap->updateBpmTap();

	HydrogenApp::get_instance()->addEventListener( this );
}

MainToolBar::~MainToolBar() {
}

void MainToolBar::setPreferencesVisibilityState( bool bChecked ) {
	m_pShowPreferencesAction->setChecked( bChecked );
}

void MainToolBar::updateActions()
{
	const auto pPref = Preferences::get_instance();
	HydrogenApp *pH2App = HydrogenApp::get_instance();
	const auto pHydrogen = Hydrogen::get_instance();

	m_pShowPlaylistEditorAction->setChecked(
		pH2App->getPlaylistEditor()->isVisible() );
	m_pShowDirectorAction->setChecked( pH2App->getDirector()->isVisible() );
	m_pShowMixerAction->setChecked( pH2App->getMixer()->isVisible() );
	m_pShowInstrumentRackAction->setChecked(
		pH2App->getInstrumentRack()->isVisible() );
	m_pShowAutomationAction->setChecked(
		pH2App->getSongEditorPanel()->getAutomationPathView()->isVisible() );
	m_pShowPlaybackTrackAction->setChecked(
		pH2App->getSongEditorPanel()->getPlaybackTrackWaveDisplay()->isVisible() );

	m_pMetronomeButton->setChecked( pPref->m_bUseMetronome );

	// Rubberband
	if ( m_pRubberBandAction->isChecked() != pPref->getRubberBandBatchMode() ) {
		m_pRubberBandAction->setChecked( pPref->getRubberBandBatchMode());
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

	m_pBpmTap->updateBpmTap();
	updateBpmSpinBox();
}

void MainToolBar::loopModeActivationEvent() {
	updateLoopMode();
}

void MainToolBar::metronomeEvent( int nValue ) {
	const auto pPref = H2Core::Preferences::get_instance();

	// Value 2 corresponds to the metronome being turned on or off.
	if ( nValue == 2 ) {
		updateActions();
		return;
	}

	if ( ! pPref->m_bUseMetronome ) {
		return;
	}

	if ( nValue == 0 ) {
		m_pMetronomeButton->setStyleSheet( QString( "\
#MetronomeButton {				 \
   background-color: %1;\
}" ).arg( pPref->getTheme().m_color.m_highlightColor.name() ) );
	}
	else {
		m_pMetronomeButton->setStyleSheet( QString( "\
#MetronomeButton {\
   background-color: %1;\
}" ).arg( pPref->getTheme().m_color.m_accentColor.name() ) );
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
void MainToolBar::playBtnClicked() {
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
	
	if ( m_pPlayButton->isChecked() ) {
		pHydrogen->sequencerPlay();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Playing.") );
	}
	else {
		pHydrogen->sequencerStop();
		(HydrogenApp::get_instance())->showStatusBarMessage( tr("Pause.") );
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
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1 );
}

void MainToolBar::rewindBtnClicked() {
	auto pHydrogen = Hydrogen::get_instance();
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() - 1 );
}

void MainToolBar::updateBpmSpinBox() {
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

void MainToolBar::updateJackTransport() {
	auto pHydrogen = Hydrogen::get_instance();
	const bool bVisible = pHydrogen->hasJackAudioDriver();
	m_pJackTimebaseButton->setVisible( bVisible );
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
	const auto theme = Preferences::get_instance()->getTheme();
	auto pHydrogen = Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const bool bVisible = pHydrogen->hasJackAudioDriver();
	m_pJackTimebaseButton->setVisible( bVisible );
	m_pJackTransportAction->setVisible( bVisible );
	m_pJackSeparator->setVisible( bVisible );

	m_pJackTimebaseButton->setStyleSheet( "" );
	if ( ! Preferences::get_instance()->m_bJackTimebaseEnabled ) {
		m_pJackTimebaseButton->setChecked( false );
		m_pJackTimebaseButton->setEnabled( false );
		m_pJackTimebaseButton->setToolTip(
			pCommonStrings->getJackTimebaseDisabledToolTip() );
		return;
	}
	else {
		m_pJackTimebaseButton->setEnabled( true );
		m_pJackTimebaseButton->setChecked( false );
		m_pJackTimebaseButton->setToolTip(
			pCommonStrings->getJackTimebaseToolTip() );
	}

	if ( pHydrogen->hasJackTransport() ) {
		switch ( pHydrogen->getJackTimebaseState() ) {
		case JackAudioDriver::Timebase::Controller:
			m_pJackTimebaseButton->setChecked( true );
			m_pJackTimebaseButton->setStyleSheet( QString( "\
#JackTimebaseButton {\
    background-color: %1;\
}" ).arg( theme.m_color.m_highlightColor.name() ) );
			break;

		case JackAudioDriver::Timebase::Listener:
			m_pJackTimebaseButton->setChecked( true );
			m_pJackTimebaseButton->setStyleSheet( QString( "\
#JackTimebaseButton {\
    background-color: %1;\
}" ).arg( theme.m_color.m_buttonRedColor.name() ) );
			m_pJackTimebaseButton->setToolTip(
				pCommonStrings->getJackTimebaseListenerToolTip() );
			break;
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

void MainToolBar::updateSongMode() {
	auto pHydrogen = Hydrogen::get_instance();

	const bool bSongMode = pHydrogen->getMode() == Song::Mode::Song;
	m_pSongModeButton->setChecked( bSongMode );
	m_pPatternModeButton->setChecked( ! bSongMode );
	m_pSongLoopAction->setEnabled( bSongMode );
}

void MainToolBar::updateTransportControl() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();

	m_pPlayButton->setChecked(
		pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Playing );
	m_pRecButton->setChecked( pPref->getRecordEvents() );
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
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pRwdButton->setIcon( QIcon( sIconPath + "rewind.svg" ) );
	m_pRecButton->setIcon(
		QIcon( sIconPath + "record.svg" ) );
	m_pPlayButton->setIcon( QIcon( sIconPath + "play.svg" ) );
	m_pStopButton->setIcon( QIcon( sIconPath + "stop.svg" ) );
	m_pFfwdButton->setIcon( QIcon( sIconPath + "fast_forward.svg" ) );
	m_pSongLoopAction->setIcon( QIcon( sIconPath + "loop.svg" ) );
	m_pPatternModeButton->setIcon( QIcon( sIconPath + "pattern-editor.svg" ) );
	m_pSongModeButton->setIcon( QIcon( sIconPath + "song-editor.svg" ) );
	m_pMetronomeButton->setIcon( QIcon( sIconPath + "metronome.svg" ) );
	m_pRubberBandAction->setIcon( QIcon( sIconPath + "rubberband.svg" ) );
	m_pJackTransportAction->setIcon( QIcon( sIconPath + "jack.svg" ) );
	m_pJackTimebaseButton->setIcon( QIcon( sIconPath + "jack-timebase.svg" ) );
	m_pShowPlaylistEditorAction->setIcon( QIcon( sIconPath + "playlist.svg" ) );
	m_pShowDirectorAction->setIcon( QIcon( sIconPath + "director.svg" ) );
	m_pShowMixerAction->setIcon( QIcon( sIconPath + "mixer.svg" ) );
	m_pShowInstrumentRackAction->setIcon(
		QIcon( sIconPath + "component-editor.svg" ) );
	m_pShowAutomationAction->setIcon( QIcon( sIconPath + "automation.svg" ) );
	m_pShowPlaybackTrackAction->setIcon(
		QIcon( sIconPath + "playback-track.svg" ) );
	m_pShowPreferencesAction->setIcon( QIcon( sIconPath + "cog.svg" ) );

	m_pBpmTap->updateIcons();
	m_pMidiControlButton->updateIcons();
}

void MainToolBar::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorBackground = colorTheme.m_baseColor;

	QColor colorBackgroundChecked, colorBackgroundHovered;
	if ( Skin::moreBlackThanWhite( colorBackground ) ) {
		colorBackgroundChecked = colorBackground.lighter(
			Skin::nToolBarCheckedScaling );
		colorBackgroundHovered = colorBackground.lighter(
			Skin::nToolBarHoveredScaling );
	}
	else {
		colorBackgroundChecked = colorBackground.darker(
			Skin::nToolBarCheckedScaling );
		colorBackgroundHovered = colorBackground.darker(
			Skin::nToolBarHoveredScaling );
	}

	setStyleSheet( QString( "\
QToolBar {\
     background-color: %1; \
     border: %2px solid #000;\
     spacing: %3px;\
}\
QToolButton {\
    background-color: %1; \
}\
QToolButton:checked {\
    background-color: %4;\
}\
QToolButton:hover {\
    background-color: %5;\
}\
QToolButton:hover, QToolButton:pressed {\
    background-color: %4;\
}\
QToolButton:hover, QToolButton:checked {\
    background-color: %4;\
}")
				   .arg( colorBackground.name() ).arg( MainToolBar::nBorder )
				   .arg( MainToolBar::nSpacing )
				   .arg( colorBackgroundChecked.name() )
				   .arg( colorBackgroundHovered.name() ) );

	m_pBpmTap->setBackgroundColor( colorBackground );
	m_pBpmTap->updateStyleSheet();
}
