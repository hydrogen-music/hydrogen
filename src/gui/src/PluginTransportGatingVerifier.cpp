/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#include "PluginTransportGatingVerifier.h"

#include "HydrogenApp.h"
#include "MainForm.h"
#include "MainToolBar/MainToolBar.h"
#include "PlaylistEditor/PlaylistEditor.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Object.h>
#include <core/Preferences/Shortcuts.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QString>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <vector>

using namespace H2Core;

// Look up a transport button by its object name and compare its enablement
// against the expectation. Logs the mismatch (or a missing button) and
// returns false so the caller can short-circuit.
static bool checkButton( const QWidget* pWidget, const QString& sObjectName,
						 bool bExpectedEnabled ) {
	auto pButton = pWidget->findChild< QToolButton* >( sObjectName );
	if ( pButton == nullptr ) {
		___ERRORLOG( QString( "Transport gating verification: no button [%1] "
							  "in widget [%2]" )
						 .arg( sObjectName )
						 .arg( pWidget->objectName() ) );
		return false;
	}
	if ( pButton->isEnabled() != bExpectedEnabled ) {
		___ERRORLOG( QString( "Transport gating verification: button [%1] is "
							  "%2 but should be %3" )
						 .arg( sObjectName )
						 .arg( pButton->isEnabled() ? "enabled" : "disabled" )
						 .arg( bExpectedEnabled ? "enabled" : "disabled" ) );
		return false;
	}
	return true;
}

PluginTransportGatingVerifier::PluginTransportGatingVerifier(
	QObject* pParent )
	: QObject( pParent ) {
}

void PluginTransportGatingVerifier::start()
{
	// The assertions below only mean something against a plugin-host
	// engine; a broken test setup (or a stale predicate sync) must not
	// silently pass them.
	if ( ! HydrogenApp::pHydrogen()->isUnderPluginHost() ) {
		___ERRORLOG( "Transport gating verification: the editor is not "
					 "attached to a plugin-host engine — the test setup is "
					 "broken" );
		QCoreApplication::exit( 1 );
		return;
	}

	if ( ! ( verifyMainToolBar() && verifyPlaylistEditor() &&
			 verifyShortcutGating() ) ) {
		QCoreApplication::exit( 1 );
		return;
	}

	___INFOLOG( "Transport gating verification passed" );
	QCoreApplication::quit();
}

bool PluginTransportGatingVerifier::verifyMainToolBar()
{
	auto pToolBar = HydrogenApp::get_instance()->getMainToolBar();
	if ( pToolBar == nullptr ) {
		___ERRORLOG( "Transport gating verification: no MainToolBar" );
		return false;
	}

	// The host owns transport (ADR 0026): play/stop/rewind/ffwd are
	// deactivated; record stays available.
	return checkButton( pToolBar, "MainToolBarPlayButton", false )
		&& checkButton( pToolBar, "MainToolBarStopButton", false )
		&& checkButton( pToolBar, "MainToolBarRewindButton", false )
		&& checkButton( pToolBar, "MainToolBarForwardButton", false )
		&& checkButton( pToolBar, "MainToolBarRecordButton", true );
}

bool PluginTransportGatingVerifier::verifyPlaylistEditor()
{
	// The editor is constructed at startup (HydrogenApp); showing it keeps
	// the check close to what a user sees.
	HydrogenApp::get_instance()->showPlaylistEditor();
	auto pPlaylistEditor = HydrogenApp::get_instance()->getPlaylistEditor();
	if ( pPlaylistEditor == nullptr ) {
		___ERRORLOG( "Transport gating verification: no PlaylistEditor" );
		return false;
	}

	// Seeking and stopping is the host's; the play button stays active as
	// the song selector (playButtonClicked selects without starting
	// playback under a plugin host).
	return checkButton( pPlaylistEditor, "PlaylistEditorRewindButton", false )
		&& checkButton( pPlaylistEditor, "PlaylistEditorStopButton", false )
		&& checkButton( pPlaylistEditor, "PlaylistEditorForwardButton", false )
		&& checkButton( pPlaylistEditor, "PlaylistEditorPlayButton", true );
}

bool PluginTransportGatingVerifier::verifyShortcutGating()
{
	auto pMainForm = HydrogenApp::get_instance()->getMainForm();
	if ( pMainForm == nullptr ) {
		___ERRORLOG( "Transport gating verification: no MainForm" );
		return false;
	}
	const auto pHydrogen = HydrogenApp::pHydrogen();
	const auto pAudioEngine = pHydrogen->getAudioEngine();

	// Baselines captured before the first dispatch. Every dispatched
	// shortcut must leave all three of them untouched.
	const auto initialState = pAudioEngine->getState();
	const double fInitialBpm = pAudioEngine->getPlayhead()->getBpm();
	const int nInitialColumn = pAudioEngine->getPlayhead()->getColumn();

	// Drain the event loop so the IPC round trip (shortcut forwarded to
	// the engine, state telemetry back into the mirror) can surface
	// before comparing.
	const auto pumpEvents = []( int nMs ) {
		QEventLoop loop;
		QTimer::singleShot( nMs, &loop, &QEventLoop::quit );
		loop.exec();
	};

	const auto actionLabel = []( Shortcuts::Action action ) -> QString {
		const auto pPref = HydrogenApp::pPreferences();
		const auto pShortcuts = pPref->getShortcuts();
		const auto& actionInfoMap = pShortcuts->getActionInfoMap();
		const auto it = actionInfoMap.find( action );
		if ( it != actionInfoMap.end() ) {
			return QString( "%1 [%2]" )
				.arg( it->second.sDescription )
				.arg( static_cast<int>( action ) );
		}
		return QString::number( static_cast<int>( action ) );
	};

	const auto checkUnchanged = [&]( const QString& sAction,
									 int nPumpMs ) -> bool {
		pumpEvents( nPumpMs );
		const auto state = pAudioEngine->getState();
		const double fBpm = pAudioEngine->getPlayhead()->getBpm();
		const int nColumn = pAudioEngine->getPlayhead()->getColumn();
		if ( state != initialState || fBpm != fInitialBpm ||
			 nColumn != nInitialColumn ) {
			___ERRORLOG( QString( "Transport gating verification: shortcut "
								  "[%1] must be deactivated under a plugin "
								  "host but changed the mirrored engine "
								  "state [state %2 -> %3, bpm %4 -> %5, "
								  "column %6 -> %7]" )
							 .arg( sAction )
							 .arg( static_cast<int>( initialState ) )
							 .arg( static_cast<int>( state ) )
							 .arg( fInitialBpm )
							 .arg( fBpm )
							 .arg( nInitialColumn )
							 .arg( nColumn ) );
			return false;
		}
		return true;
	};

	// Only the gated shortcuts whose effect is observable in the mirror
	// are dispatched: transport state, tempo, and playhead. The remaining
	// gated ones are already inert engine-side under a plugin host
	// (playlist transport is rejected by the engine's MidiActionManager,
	// taps are no-ops with a remote tempo source), need a valid focus
	// QObject (PlayPauseToggleAtCursor), or only toggle deactivated
	// features (timeline, JACK, loop) — the gate in
	// MainForm::executeShortcut covers them all the same. Count-in
	// actions get a longer pump since their effect may only surface
	// after the count-in lead-in.
	struct GatedDispatch {
		Shortcuts::Action action;
		QString sArg1;
		int nPumpMs;
	};
	const std::vector<GatedDispatch> dispatches = {
		{ Shortcuts::Action::Play, "", 300 },
		{ Shortcuts::Action::PlayPauseToggle, "", 300 },
		{ Shortcuts::Action::PlayStopToggle, "", 300 },
		{ Shortcuts::Action::JumpToStart, "", 300 },
		{ Shortcuts::Action::JumpBarForward, "", 300 },
		{ Shortcuts::Action::JumpBarBackward, "", 300 },
		{ Shortcuts::Action::JumpToBar, "3", 300 },
		{ Shortcuts::Action::BPMIncreaseCoarse, "", 300 },
		{ Shortcuts::Action::BPMDecreaseCoarse, "", 300 },
		{ Shortcuts::Action::BPMIncreaseFine, "", 300 },
		{ Shortcuts::Action::BPMDecreaseFine, "", 300 },
		{ Shortcuts::Action::BPM,
		  QString::number( fInitialBpm + 10 ), 300 },
		{ Shortcuts::Action::CountIn, "", 2000 },
		{ Shortcuts::Action::CountInPauseToggle, "", 2000 },
		{ Shortcuts::Action::CountInStopToggle, "", 2000 },
		{ Shortcuts::Action::SelectAndPlayPattern, "0", 300 },
	};

	for ( const auto& dispatch : dispatches ) {
		ShortcutArgs args;
		args.sArg1 = dispatch.sArg1;
		pMainForm->executeShortcut( dispatch.action, args );
		if ( ! checkUnchanged( actionLabel( dispatch.action ),
							   dispatch.nPumpMs ) ) {
			return false;
		}
	}

	return true;
}
