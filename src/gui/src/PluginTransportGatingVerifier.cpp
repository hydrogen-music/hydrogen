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
#include "MainToolBar/MainToolBar.h"
#include "PlaylistEditor/PlaylistEditor.h"

#include <core/Object.h>

#include <QCoreApplication>
#include <QString>
#include <QToolButton>
#include <QWidget>

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

	if ( ! ( verifyMainToolBar() && verifyPlaylistEditor() ) ) {
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
