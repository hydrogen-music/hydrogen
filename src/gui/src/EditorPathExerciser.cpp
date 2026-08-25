/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#include "EditorPathExerciser.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/Lilipond/Lilypond.h>
#include <core/Logger.h>
#include <core/Midi/SMF.h>
#include <core/Preferences/Shortcuts.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "HydrogenApp.h"
#include "MainForm.h"

#include <QCoreApplication>
#include <QMetaEnum>

using namespace H2Core;

EditorPathExerciser::EditorPathExerciser( MainForm* pMainForm,
										  QObject* pParent )
	: QObject( pParent )
	, m_pMainForm( pMainForm )
{
	m_timer.setSingleShot( true );
	m_timer.setInterval( 50 );
	connect( &m_timer, &QTimer::timeout,
			 this, &EditorPathExerciser::exerciseNextAction );
}

void EditorPathExerciser::start()
{
	buildActionList();
	___INFOLOG( QString( "EditorPathExerciser: starting — %1 actions queued" )
				.arg( m_actions.size() ) );
	m_nIndex = 0;
	m_timer.start();
}

void EditorPathExerciser::buildActionList()
{
	// Actions that open modal dialogs, file dialogs, quit the app, or launch
	// external programs — they would block the headless event loop.
	static const std::set<Shortcuts::Action> skipSet = {
		// File dialogs / modal dialogs
		Shortcuts::Action::NewSong,
		Shortcuts::Action::EditSongProperties,
		Shortcuts::Action::OpenDemoSong,
		Shortcuts::Action::SaveSong,
		Shortcuts::Action::SaveAsSong,
		Shortcuts::Action::OpenPattern,
		// Drumkit dialogs
		Shortcuts::Action::NewDrumkit,
		Shortcuts::Action::OpenDrumkit,
		Shortcuts::Action::EditDrumkitProperties,
		Shortcuts::Action::SaveDrumkitToSoundLibrary,
		Shortcuts::Action::SaveDrumkitToSession,
		Shortcuts::Action::ImportDrumkit,
		Shortcuts::Action::ImportOnlineDrumkit,
		Shortcuts::Action::AddComponent,
		// Playlist dialogs
		Shortcuts::Action::NewPlaylist,
		Shortcuts::Action::OpenPlaylist,
		Shortcuts::Action::SavePlaylist,
		Shortcuts::Action::SaveAsPlaylist,
		Shortcuts::Action::PlaylistAddScript,
		Shortcuts::Action::PlaylistEditScript,
		Shortcuts::Action::PlaylistCreateScript,
		Shortcuts::Action::PlaylistRemoveScript,
		// Preferences / about / external
		Shortcuts::Action::ShowPreferencesDialog,
		Shortcuts::Action::ShowAbout,
		Shortcuts::Action::ShowReportBug,
		Shortcuts::Action::ShowDonate,
		Shortcuts::Action::OpenManual,
		Shortcuts::Action::OpenLogFile,
		// Quit
		Shortcuts::Action::Quit,
		// PlayPauseToggleAtCursor — requires a valid QObject (SongEditorPanel
		// or PatternEditorPanel) to read the cursor position; nullptr crashes.
		Shortcuts::Action::PlayPauseToggleAtCursor,
		// Fullscreen — problematic in headless offscreen
		Shortcuts::Action::ShowFullscreen,
		// ShowUndoHistory — opens a modal stack dialog
		Shortcuts::Action::ShowUndoHistory,
		// ShowAudioEngineInfo / ShowFilesystemInfo — open non-modal windows
		// that may persist; safe but noisy in headless, skip for cleanliness
		Shortcuts::Action::ShowAudioEngineInfo,
		Shortcuts::Action::ShowFilesystemInfo,
		// SaveDrumkitToSoundLibrary is already skipped above
		// ClearAllInstruments calls action_drumkit_new() which may prompt
		Shortcuts::Action::ClearAllInstruments,
		Shortcuts::Action::PlaylistAddSong,
		Shortcuts::Action::PlaylistAddCurrentSong,
	};

	// Iterate all action enum values from FirstWith0Args to the last MainForm
	// action. We use the actionInfoMap to determine valid actions.
	const auto pPref = HydrogenApp::pPreferences();
	const auto pShortcuts = pPref->getShortcuts();
	const auto& actionInfoMap = pShortcuts->getActionInfoMap();

	for ( int n = static_cast<int>( Shortcuts::Action::FirstWith0Args );
		  n < static_cast<int>( Shortcuts::Action::Null ); ++n ) {
		const auto action = static_cast<Shortcuts::Action>( n );

		// Skip sentinel/boundary values
		if ( action == Shortcuts::Action::FirstWith0Args ||
			 action == Shortcuts::Action::LastWith0Args ||
			 action == Shortcuts::Action::FirstWith1Args ||
			 action == Shortcuts::Action::LastWith1Args ||
			 action == Shortcuts::Action::FirstWith2Args ||
			 action == Shortcuts::Action::LastWith2Args ||
			 action == Shortcuts::Action::FirstWithManyArgs ||
			 action == Shortcuts::Action::LastWithManyArgs ) {
			continue;
		}

		// Skip actions not in the actionInfoMap (gaps in the enum)
		if ( actionInfoMap.find( action ) == actionInfoMap.end() ) {
			continue;
		}

		// Skip unsafe actions
		if ( skipSet.count( action ) > 0 ) {
			continue;
		}

		m_actions.push_back( action );
	}
}

ShortcutArgs
EditorPathExerciser::defaultArgsFor( Shortcuts::Action action ) const
{
	ShortcutArgs args;

	// 1-arg actions
	if ( static_cast<int>( action ) > static_cast<int>( Shortcuts::Action::FirstWith1Args ) &&
		 static_cast<int>( action ) < static_cast<int>( Shortcuts::Action::LastWith1Args ) ) {
		switch ( action ) {
		case Shortcuts::Action::BPM:
			args.sArg1 = "120";
			break;
		case Shortcuts::Action::MasterVolume:
			args.sArg1 = "0.5";
			break;
		default:
			args.sArg1 = "0";
			break;
		}
	}
	// 2-arg actions
	else if ( static_cast<int>( action ) > static_cast<int>( Shortcuts::Action::FirstWith2Args ) &&
			  static_cast<int>( action ) < static_cast<int>( Shortcuts::Action::LastWith2Args ) ) {
		switch ( action ) {
		case Shortcuts::Action::StripVolume:
			args.sArg1 = "0.5";
			args.sArg2 = "0";
			break;
		case Shortcuts::Action::StripPan:
			args.sArg1 = "0.5";
			args.sArg2 = "0";
			break;
		case Shortcuts::Action::StripFilterCutoff:
			args.sArg1 = "0.5";
			args.sArg2 = "0";
			break;
		case Shortcuts::Action::TimelineAddMarker:
			args.sArg1 = "0";
			args.sArg2 = "120";
			break;
		case Shortcuts::Action::TimelineAddTag:
			args.sArg1 = "0";
			args.sArg2 = "test";
			break;
		case Shortcuts::Action::ToggleGridCell:
			args.sArg1 = "0";
			args.sArg2 = "0";
			break;
		default:
			args.sArg1 = "0";
			args.sArg2 = "0";
			break;
		}
	}
	// Many-arg actions (LayerPitch / LayerGain)
	else if ( static_cast<int>( action ) > static_cast<int>( Shortcuts::Action::FirstWithManyArgs ) &&
			  static_cast<int>( action ) < static_cast<int>( Shortcuts::Action::LastWithManyArgs ) ) {
		// args: value, instrument, component, layer
		args.sArg1 = "0";
		args.sArg2 = "0";
		args.sArg3 = "0";
		args.sArg4 = "0";
	}

	return args;
}

QString EditorPathExerciser::actionName( Shortcuts::Action action )
{
	const auto pPref = HydrogenApp::pPreferences();
	const auto pShortcuts = pPref->getShortcuts();
	const auto& actionInfoMap = pShortcuts->getActionInfoMap();
	auto it = actionInfoMap.find( action );
	if ( it != actionInfoMap.end() ) {
		return it->second.sDescription;
	}
	return QString::number( static_cast<int>( action ) );
}

void EditorPathExerciser::exerciseNextAction()
{
	if ( m_nIndex >= m_actions.size() ) {
		___INFOLOG( "EditorPathExerciser: all actions exercised successfully — "
					"quitting" );
		// Avoid  handle unsaved changes modal on shutdown
		auto pHydrogen = HydrogenApp::pHydrogen();
		pHydrogen->setSongModified( false );
		pHydrogen->getEventQueue()->pushEvent( Event::Type::Quit, 0 );
		return;
	}

	const auto action = m_actions[ m_nIndex ];
	const auto args = defaultArgsFor( action );

	___INFOLOG( QString( "EditorPathExerciser: [%1/%2] %3 (enum %4)" )
				.arg( m_nIndex + 1 )
				.arg( m_actions.size() )
				.arg( actionName( action ) )
				.arg( static_cast<int>( action ) ) );

	switch( action ) {
		case Shortcuts::Action::OpenSong: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-open-song.h2song"
			);
			HydrogenApp::pEngine()->isUnderSessionManagement();
			auto pSong = HydrogenApp::pHydrogen()->getSong();
			// Ensure the path of the song is writable. Else a modal will open
			// an block the test.
			pSong->setPath( sTmpFile );
			HydrogenApp::pEngine()->getCoreActionController()->setSong(
				pSong
			);
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportPattern: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-pattern.h2pattern"
			);
			auto pNewPattern = std::make_shared<Pattern>(
				HydrogenApp::pHydrogen()->getSong()->getPatternList()->get( 0 )
			);
			pNewPattern->setPath( sTmpFile );
			pNewPattern->save(
				sTmpFile, HydrogenApp::pHydrogen()->getSong()->getDrumkit()
			);
			HydrogenApp::pHydrogen()->getSoundLibraryDatabase()->updatePatterns(
				Event::Trigger::Default
			);
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportSong: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-song.h2song"
			);
			HydrogenApp::pHydrogen()->startExportSession( 48000, 16, 0 );
			HydrogenApp::pHydrogen()->startExportSong( sTmpFile );
			HydrogenApp::pHydrogen()->stopExportSong();
			HydrogenApp::pHydrogen()->stopExportSession();
			HydrogenApp::pHydrogen()
				->getSong()
				->getDrumkit()
				->recalculateRubberband( 120, HydrogenApp::pHydrogen() );
			HydrogenApp::pHydrogen()->getSoundLibraryDatabase()->updateSongs(
				Event::Trigger::Default
			);
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportProject: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-project.h2project"
			);
			H2Project::save( HydrogenApp::pHydrogen()->getSong(), sTmpFile, true );
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportMIDI: {
			const QString sTmpFile =
				Filesystem::tmpFilePath( "editor-path-exerciser-export-midi.mid"
				);
			auto pSmfWriter = std::make_shared<SMF1WriterSingle>();
			pSmfWriter->save(
				sTmpFile, HydrogenApp::pHydrogen()->getSong(), true,
				HydrogenApp::pHydrogen()
			);
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportLilyPond: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-lilypond.ly"
			);
			LilyPond ly;
			ly.extractData( *HydrogenApp::pHydrogen()->getSong() );
			ly.write( sTmpFile );
			Filesystem::rm( sTmpFile );
			break;
		}

		case Shortcuts::Action::ExportDrumkit: {
			const QString sTmpFile = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-drumkit.xml"
			);
			const QString sTmpArchive = Filesystem::tmpFilePath(
				"editor-path-exerciser-export-drumkit.h2drumkit"
			);
			auto pNewDrumkit = std::make_shared<Drumkit>(
				HydrogenApp::pHydrogen()->getSong()->getDrumkit()
			);
			pNewDrumkit->setPath( sTmpFile );
			pNewDrumkit->save( sTmpFile );
			pNewDrumkit->exportTo( sTmpArchive );
			HydrogenApp::pHydrogen()->getSoundLibraryDatabase()->updateDrumkits(
				Event::Trigger::Default
			);
			Filesystem::rm( sTmpArchive );
			Filesystem::rm( sTmpFile );
			break;
		}

		default:
			m_pMainForm->executeShortcut( action, args );
	}

	++m_nIndex;
	m_timer.start();
}
