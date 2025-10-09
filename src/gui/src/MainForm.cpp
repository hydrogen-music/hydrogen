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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "MainForm.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/H2Exception.h>
#include <core/Hydrogen.h>
#include <core/Lilipond/Lilypond.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiMessage.h>
#include <core/Midi/SMF.h>
#include <core/OscServer.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Shortcuts.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Timeline.h>
#include <core/Version.h>

#include "AboutDialog.h"
#include "AudioEngineInfoForm.h"
#include "CommonStrings.h"
#include "Director.h"
#include "ExportMidiDialog.h"
#include "ExportSongDialog.h"
#include "HydrogenApp.h"
#include "InstrumentEditor/ComponentsEditor.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "InstrumentRack.h"
#include "LadspaFXProperties.h"
#include "Mixer/Mixer.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PlaylistEditor/PlaylistEditor.h"
#include "MainToolBar/MainToolBar.h"
#include "Skin.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SongPropertiesDialog.h"
#include "SoundLibrary/SoundLibraryOnlineImportDialog.h"
#include "SoundLibrary/DrumkitOpenDialog.h"
#include "SoundLibrary/DrumkitPropertiesDialog.h"
#include "UndoActions.h"
#include "Widgets/FileDialog.h"
#include "Widgets/InfoBar.h"
#include "Widgets/InputCaptureDialog.h"

#include <QtGui>
#include <QtWidgets>
#ifndef H2CORE_HAVE_QT6
  #include <QTextCodec>
#endif

#ifndef WIN32
#include <sys/time.h>
#include <sys/socket.h>
#endif

#include <memory>
#include <cassert>

using namespace H2Core;

int MainForm::sigusr1Fd[2];

MainForm::MainForm( QApplication * pQApplication, const QString& sSongFilename,
					const QString& sPlaylistFilename )
	: QMainWindow( nullptr )
	, m_sPreviousAutoSaveSongFile( "" )
	, m_bUnsavedChangesHandled( false )
{
	const auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	setObjectName( "MainForm" );

#ifndef WIN32
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr1Fd)) {
		qFatal("Couldn't create HUP socketpair");
	}
	snUsr1 = new QSocketNotifier(sigusr1Fd[1], QSocketNotifier::Read, this);
	connect(snUsr1, SIGNAL(activated(int)), this, SLOT( handleSigUsr1() ));
#endif

	m_pQApp = pQApplication;

	m_pQApp->processEvents();

	/////////// Load song and playlist
	auto openFile = [=]( const Filesystem::Type& type, const QString& sPath,
						 const QString& sLastPath ) {
		bool bRet = false;
		if ( sPath.isEmpty() ) {
			bRet = HydrogenApp::openFile( type, sLastPath );
		}
		else if ( ! sPath.isEmpty() ) {
			bRet = HydrogenApp::openFile( type, sPath );
		}
		return bRet;
	};

	//
	// When using the Non Session Management system, the new Song
	// will be loaded by the NSM client singleton itself and not
	// by the MainForm. The latter will just access the already
	// loaded Song.
	if ( ! pHydrogen->isUnderSessionManagement() ) {
		if ( ! openFile( Filesystem::Type::Song, sSongFilename,
						 pPref->getLastSongFilename() ) ) {
			// Fall back to an empty song.
			HydrogenApp::openSong( H2Core::Song::getEmptySong() );		}
	}

	// We need no fallback for the playlist as a new one corresponds to an empty
	// one.
	openFile( Filesystem::Type::Playlist, sPlaylistFilename,
			  pPref->getLastPlaylistFilename() );

	QFont font( pPref->getFontTheme()->m_sApplicationFontFamily,
			   getPointSize( pPref->getFontTheme()->m_fontSize ) );
	setFont( font );
	m_pQApp->setFont( font );

	// Setup undo stack
	auto pUndoStack = new QUndoStack( this );

	h2app = new HydrogenApp( this, pUndoStack );
	showDevelWarning();
	h2app->addEventListener( this );
	createMenuBar();
	// The menu bar will be created anew each time the shortcuts are altered in
	// the Preferences. But we need to wire the corresponding actions only once
	// or they are triggered each 1 + N times the number of shortcut changes.
	connect( pUndoStack, &QUndoStack::canUndoChanged,
			 []( bool bCanUndo ) {
				 auto pUndoAction =
					 HydrogenApp::get_instance()->getMainForm()->m_pUndoAction;
				 if ( pUndoAction != nullptr ) {
					 pUndoAction->setEnabled( bCanUndo );
				 }
			 } );
	connect( pUndoStack, &QUndoStack::canRedoChanged,
			 []( bool bCanRedo ) {
				 auto pRedoAction =
					 HydrogenApp::get_instance()->getMainForm()->m_pRedoAction;
				 if ( pRedoAction != nullptr ) {
					 pRedoAction->setEnabled( bCanRedo );
				 }
			 } );

	checkMidiSetup();
	checkMissingSamples();
	checkNecessaryDirectories();

	h2app->showStatusBarMessage( tr("Hydrogen Ready.") );

	// we need to do all this to support the keyboard playing
	// for all the window modes
	h2app->getMixer()->installEventFilter (this);
	h2app->getPatternEditorPanel()->installEventFilter (this);
	h2app->getSongEditorPanel()->installEventFilter (this);
	h2app->getMainToolBar()->installEventFilter(this);
	h2app->getInstrumentRack()->getInstrumentEditorPanel()
		->installEventFilter(this);
	h2app->getAudioEngineInfoForm()->installEventFilter(this);
	h2app->getDirector()->installEventFilter(this);
	installEventFilter( this );

	connect( &m_AutosaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimer()));
	startAutosaveTimer();

	//playlist display timer
	QTimer *playlistDisplayTimer = new QTimer(this);
	connect( playlistDisplayTimer, SIGNAL( timeout() ), this, SLOT( onPlaylistDisplayTimer() ) );
	playlistDisplayTimer->start(30000);	// update player control at
	// ~ playlist display timer

	auto pCommonStrings = h2app->getCommonStrings();

	m_pUndoView = new QUndoView( pUndoStack );
	m_pUndoView->setWindowTitle( QString( "Hydrogen - %1" )
								 .arg( pCommonStrings->getUndoHistoryTitle() ) );

	// Check whether the audio driver could be loaded based on the
	// content of the config file
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning(
			this, "Hydrogen", QString( "%1 [%2]\n%3" )
			.arg( pCommonStrings->getAudioDriverStartError() )
			.arg( Preferences::audioDriverToQString( pPref->m_audioDriver ) )
			.arg( pCommonStrings->getAudioDriverErrorHint() ) );
	}
}


MainForm::~MainForm()
{
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getAudioEngine()->getState() ==
		 H2Core::AudioEngine::State::Playing ) {
		pHydrogen->sequencerStop();
	}
	
	hide();

	delete m_pUndoView;

	if (h2app != nullptr) {
		delete h2app;
		h2app = nullptr;
	}

}

void MainForm::updateMenuBar() {
	auto pHydrogenApp = HydrogenApp::get_instance();

	m_pViewMixerAction->setChecked( pHydrogenApp->getMixer()->isVisible() );
	m_pViewInstrumentRackAction->setChecked(
		pHydrogenApp->getInstrumentRack()->isVisible() );
}

void MainForm::updateAutomationPathVisibility() {
	m_pViewAutomationPathAction->setChecked(
		HydrogenApp::get_instance()->getSongEditorPanel()
		->getAutomationPathView()->isVisible() );
}

///
/// Create the menubar
///
void MainForm::createMenuBar()
{
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pShortcuts = pPref->getShortcuts();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	// menubar
	QMenuBar *pMenubar = new QMenuBar( this );
	pMenubar->setObjectName( "MainMenu" );

	// FILE menu
	m_pFileMenu = pMenubar->addMenu( tr( "Pro&ject" ) );

	// Then under session management a couple of options will be named
	// differently and some must be even omitted. 
	const bool bUnderSessionManagement =
		H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
	QString sLabelNew, sLabelOpen, sLabelOpenRecent, sLabelSaveAs, sLabelOpenDemo;
	
	if ( bUnderSessionManagement ) {
		/*: When Hydrogen is under session management the path the
		song is stored to can not be changed by the user. This option
		allows to replace the current song with an empty one.*/
		sLabelNew = tr( "Replace With &New Song" );
		/*: When Hydrogen is under session management the path the
		song is stored to can not be changed by the user. This option
		allows to replace the current song with one chosen by the
		user via a file browser widget.*/
		sLabelOpen = tr( "Imp&ort Into Session" );
		/*: When Hydrogen is under session management the path the
		song is stored to can not be changed by the user. This option
		allows to replace the current song with one chosen recently
		used by the user.*/
		sLabelOpenRecent = tr( "Import &Recent Into Session" );
		sLabelOpenDemo = tr( "Import &Demo Into Session" );
		/*: When Hydrogen is under session management the path the
		song is stored to can not be changed by the user. This option
		allows the user store the current song in a .h2song anywhere
		on her system. The filepath of the current song won't be
		altered.*/
		sLabelSaveAs = tr( "Export From Session &As..." );
	}
	else {
		sLabelNew = tr( "&New" );
		sLabelOpen = tr( "&Open" );
		sLabelOpenRecent = tr( "Open &Recent" );
		sLabelSaveAs = tr( "Save &As..." );
		sLabelOpenDemo = tr( "Open &Demo" );
	}
	
	auto pActionFileNew = m_pFileMenu->addAction(
		sLabelNew, this, SLOT( action_file_new() ) );
	pActionFileNew->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::NewSong ) );

	auto pActionSongProperties = m_pFileMenu->addAction(
		tr( "Song Properties" ), this, SLOT( action_file_songProperties() ) );
	pActionSongProperties->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::EditSongProperties ) );
	
	auto pActionOpenSong = m_pFileMenu->addAction(
		sLabelOpen, this, SLOT( action_file_open() ) );
	pActionOpenSong->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::OpenSong ) );
	auto pActionOpenDemoSong = m_pFileMenu->addAction(
		sLabelOpenDemo, this, SLOT( action_file_openDemo() ) );
	pActionOpenDemoSong->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::OpenDemoSong ) );
	m_pRecentFilesMenu = m_pFileMenu->addMenu( sLabelOpenRecent );

	m_pFileMenu->addSeparator();				// -----

	auto pActionSongSave = m_pFileMenu->addAction(
		tr( "&Save" ), this, SLOT( action_file_save() ) );
	pActionSongSave->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::SaveSong ) );
	auto pActionSongSaveAs = m_pFileMenu->addAction(
		sLabelSaveAs, this, SLOT( action_file_save_as() ) );
	pActionSongSaveAs->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::SaveAsSong ) );
	
	m_pFileMenu->addSeparator();				// -----

	auto pActionOpenPattern = m_pFileMenu->addAction(
		tr( "Open &Pattern" ), this, SLOT ( action_file_openPattern() ) );
	pActionOpenPattern->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::OpenPattern ) );
	auto pActionExportPattern = m_pFileMenu->addAction(
		tr( "E&xport Pattern As..." ), this, SLOT( action_file_export_pattern_as() ) );
	pActionExportPattern->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ExportPattern ) );

	m_pFileMenu->addSeparator();				// -----

	auto pActionExportMidi = m_pFileMenu->addAction(
		tr( "Export &MIDI File" ), this, SLOT( action_file_export_midi() ) );
	pActionExportMidi->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ExportMIDI ) );
	auto pActionExportSong = m_pFileMenu->addAction(
		tr( "&Export Song" ), this, SLOT( action_file_export() ) );
	pActionExportSong->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ExportSong ) );
	auto pActionExportLilyPond = m_pFileMenu->addAction(
		tr( "Export &LilyPond File" ), this, SLOT( action_file_export_lilypond() ) );
	pActionExportLilyPond->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ExportLilyPond ) );


#ifndef Q_OS_MACX
	m_pFileMenu->addSeparator();				// -----

	auto pActionQuit = m_pFileMenu->addAction(
		tr("&Quit"), this, SLOT( action_file_exit() ) );
	pActionQuit->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::Quit ) );
#endif

	updateRecentUsedSongList();
	connect( m_pRecentFilesMenu, SIGNAL( triggered(QAction*) ), this,
			 SLOT( action_file_open_recent(QAction*) ) );
	// ~ FILE menu

	// Undo menu
	m_pUndoMenu = pMenubar->addMenu( pCommonStrings->getUndoMenuUndo() );
	m_pUndoAction = m_pUndoMenu->addAction(
		pCommonStrings->getUndoMenuUndo(), this, SLOT( action_undo() ) );
	m_pUndoAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::Undo ) );
	m_pUndoAction->setEnabled( h2app->m_pUndoStack->canUndo() );
	m_pRedoAction = m_pUndoMenu->addAction(
		pCommonStrings->getUndoMenuRedo(), this, SLOT( action_redo() ) );
	m_pRedoAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::Redo ) );
	m_pRedoAction->setEnabled( h2app->m_pUndoStack->canRedo() );

	auto pActionUndoHistory = m_pUndoMenu->addAction(
		pCommonStrings->getUndoMenuHistory(), this, SLOT( openUndoStack() ) );
	pActionUndoHistory->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowUndoHistory ) );

	// DRUMKITS MENU
	m_pDrumkitMenu = pMenubar->addMenu( tr( "Drum&kit" ) );
	auto pActionDrumkitNew = m_pDrumkitMenu->addAction(
		tr( "&New" ), this, SLOT( action_drumkit_new() ) );
	pActionDrumkitNew->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::NewDrumkit ) );

	auto pActionDrumkitProperties = m_pDrumkitMenu->addAction(
		tr( "&Properties" ), this, SLOT( action_drumkit_properties() ) );
	pActionDrumkitProperties->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::EditDrumkitProperties ) );

	auto pActionDrumkitOpen = m_pDrumkitMenu->addAction(
		tr( "&Open" ), this, SLOT( action_drumkit_open() ) );
	pActionDrumkitOpen->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::OpenDrumkit ) );

	m_pDrumkitMenu->addSeparator();				// -----

	auto pActionAddInstrument = m_pDrumkitMenu->addAction(
		tr( "Add &Instrument" ), this, SLOT( action_drumkit_addInstrument() ) );
	pActionAddInstrument->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::AddInstrument ) );

	m_pDrumkitMenu->addSeparator();				// -----

	auto pActionDrumkitSave = m_pDrumkitMenu->addAction(
		tr( "&Save To Sound Library" ), this, SLOT( action_drumkit_save() ) );
	pActionDrumkitSave->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::SaveDrumkitToSoundLibrary ) );

	if ( bUnderSessionManagement ) {
		auto pActionDrumkitSaveSession = m_pDrumkitMenu->addAction(
			tr( "Save &To Session" ), this,
			SLOT( action_drumkit_save_to_session() ) );
		pActionDrumkitSaveSession->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::SaveDrumkitToSession ) );
	}

	auto pActionDrumkitExport = m_pDrumkitMenu->addAction(
		tr( "&Export" ), this, SLOT( action_drumkit_export() ) );
	pActionDrumkitExport->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ExportDrumkit ) );

	m_pDrumkitMenu->addSeparator();				// -----

	auto pActionDrumkitImport = m_pDrumkitMenu->addAction(
		tr( "&Import" ), this, SLOT( action_drumkit_import() ) );
	pActionDrumkitImport->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ImportDrumkit ) );
	auto pActionDrumkitOnlineImport = m_pDrumkitMenu->addAction(
		tr( "On&line Import" ), this, SLOT( action_drumkit_onlineImport() ) );
	pActionDrumkitOnlineImport->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ImportOnlineDrumkit ) );

	// VIEW MENU
	m_pViewMenu = pMenubar->addMenu( tr( "&View" ) );

	m_pViewPlaylistEditorAction = m_pViewMenu->addAction(
		tr("Play&list Editor"), this, SLOT( action_window_showPlaylistEditor() ) );
		m_pViewPlaylistEditorAction->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::ShowPlaylist ) );
	m_pViewPlaylistEditorAction->setCheckable( true );
	m_pViewDirectorAction = m_pViewMenu->addAction(
		tr("&Director"), this, SLOT( action_window_show_DirectorWidget() ) );
	m_pViewDirectorAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowDirector ) );
	m_pViewDirectorAction->setCheckable( true );

	m_pFileMenu->addSeparator();
	m_pViewMixerAction = m_pViewMenu->addAction(
		tr("&Mixer"), this, SLOT( action_window_showMixer() ) );
	m_pViewMixerAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowMixer ) );
	m_pViewMixerAction->setCheckable( true );
	m_pViewMixerAction->setChecked( pPref->getMixerProperties().visible );

	m_pViewInstrumentRackAction = m_pViewMenu->addAction(
		tr("&Instrument Rack"), this, SLOT( action_window_showInstrumentRack() ) );
	m_pViewInstrumentRackAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowInstrumentRack ) );
	m_pViewInstrumentRackAction->setCheckable( true );
	m_pViewInstrumentRackAction->setChecked(
		pPref->getInstrumentRackProperties().visible );

	m_pViewAutomationPathAction = m_pViewMenu->addAction(
		tr("&Automation Path"), this, SLOT( action_window_showAutomationArea() ) );
	m_pViewAutomationPathAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowAutomation ) );
	m_pViewAutomationPathAction->setCheckable( true );
	if ( pPref->getShowAutomationArea() ){
		m_pViewAutomationPathAction->setChecked(true);	
	} else {
		m_pViewAutomationPathAction->setChecked(false);
	}

	m_pViewMenu->addSeparator();				// -----

	m_pViewTimelineAction = m_pViewMenu->addAction(
		tr("&Timeline"), this, SLOT( action_window_showTimeline() ) );
	m_pViewTimelineAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowTimeline ) );
	m_pViewTimelineAction->setCheckable( true );
	
	m_pViewPlaybackTrackAction = m_pViewMenu->addAction(
		tr("&Playback Track"), this, SLOT( action_window_showPlaybackTrack() ) );
	m_pViewPlaybackTrackAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowPlaybackTrack ) );
	m_pViewPlaybackTrackAction->setCheckable( true );

	m_pViewPlaybackTrackActionGroup = new QActionGroup( this );
	m_pViewPlaybackTrackActionGroup->addAction( m_pViewTimelineAction );
	m_pViewPlaybackTrackActionGroup->addAction( m_pViewPlaybackTrackAction );

	// Note that the ActionGroup unchecks the other menu item automatically
	if ( pPref->getShowPlaybackTrack() ) {
		m_pViewPlaybackTrackAction->setChecked( true );
	} else {
		m_pViewTimelineAction->setChecked( true );
	}

	m_pViewMenu->addSeparator();				// -----

	auto pActionFullScreen = m_pViewMenu->addAction(
		tr("&Full screen"), this, SLOT( action_window_toggleFullscreen() ) );
	pActionFullScreen->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowFullscreen ) );


	// Options menu
	m_pOptionsMenu = pMenubar->addMenu( tr( "&Options" ));

	m_pInputModeMenu = m_pOptionsMenu->addMenu( tr( "Input &Mode" ) );
	m_pInstrumentAction = m_pInputModeMenu->addAction(
		tr( "&Instrument" ), this, SLOT( action_inputMode_instrument() ) );
	m_pInstrumentAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::InputInstrument ) );
	m_pInstrumentAction->setCheckable( true );

	m_pDrumkitAction = m_pInputModeMenu->addAction(
		tr( "&Drumkit" ), this, SLOT( action_inputMode_drumkit() ) );
	m_pDrumkitAction->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::InputDrumkit ) );
	m_pDrumkitAction->setCheckable( true );

	if ( pPref->m_bPlaySelectedInstrument ) {
		m_pInstrumentAction->setChecked( true );
		m_pDrumkitAction->setChecked (false );
	}
	else {
		m_pInstrumentAction->setChecked( false );
		m_pDrumkitAction->setChecked (true );
	}

	auto pActionPreferences = m_pOptionsMenu->addAction(
		tr("&Preferences"), this, SLOT( showPreferencesDialog() ) );
	pActionPreferences->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowPreferencesDialog ) );

	// ~ Tools menu


	Logger *pLogger = Logger::get_instance();
	if ( pLogger->bit_mask() >= 1 ) {
		// DEBUG menu
		m_pDebugMenu = pMenubar->addMenu( tr("De&bug") );
		auto pActionEngineInfo = m_pDebugMenu->addAction(
			tr( "Show &Audio Engine Info" ), this,
			SLOT( action_debug_showAudioEngineInfo() ) );
		pActionEngineInfo->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::ShowAudioEngineInfo ) );
		auto pActionFilesystemInfo = m_pDebugMenu->addAction(
			tr( "Show &Filesystem Info" ), this,
			SLOT( action_debug_showFilesystemInfo() ) );
		pActionFilesystemInfo->setShortcut(
				pShortcuts->getKeySequence( Shortcuts::Action::ShowFilesystemInfo ) );
		
		m_pLogLevelMenu = m_pDebugMenu->addMenu( tr( "&Log Level" ) );		
		auto pActionDebugLevelNone = m_pLogLevelMenu->addAction(
			tr( "&None" ), this, SLOT( action_debug_logLevel_none() ) );
		pActionDebugLevelNone->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::LogLevelNone ) );
		auto pActionDebugLevelError = m_pLogLevelMenu->addAction(
			tr( "&Error" ), this, SLOT( action_debug_logLevel_error() ) );
		pActionDebugLevelError->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::LogLevelError ) );
		auto pActionDebugLevelWarning = m_pLogLevelMenu->addAction(
			tr( "&Warning" ), this, SLOT( action_debug_logLevel_warn() ) );
		pActionDebugLevelWarning->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::LogLevelWarning ) );
		auto pActionDebugLevelInfo = m_pLogLevelMenu->addAction(
			tr( "&Info" ), this, SLOT( action_debug_logLevel_info() ) );
		pActionDebugLevelInfo->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::LogLevelInfo ) );
		auto pActionDebugLevelDebug = m_pLogLevelMenu->addAction(
			tr( "&Debug" ), this, SLOT( action_debug_logLevel_debug() ) );
		pActionDebugLevelDebug->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::LogLevelDebug ) );

		auto pActionOpenLogFile = m_pDebugMenu->addAction(
			tr( "&Open Log File" ), this, SLOT( action_debug_openLogfile() ) );
		pActionOpenLogFile->setShortcut(
			pShortcuts->getKeySequence( Shortcuts::Action::OpenLogFile ) );
		
		if ( pLogger->bit_mask() == 8 ) { // hydrogen -V8 list object map in console
			auto pActionPrintObjects = m_pDebugMenu->addAction(
				tr( "&Print Objects" ), this, SLOT( action_debug_printObjects() ) );
			pActionPrintObjects->setShortcut(
				pShortcuts->getKeySequence( Shortcuts::Action::DebugPrintObjects ) );
		}
		// ~ DEBUG menu
	}

	// INFO menu
	m_pInfoMenu = pMenubar->addMenu( tr( "I&nfo" ) );
	auto pActionUserManual = m_pInfoMenu->addAction(
		tr("User &Manual"), this, SLOT( showUserManual() ) );
	pActionUserManual->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::OpenManual ) );
	m_pInfoMenu->addSeparator();
	auto pActionAbout = m_pInfoMenu->addAction(
		tr("&About"), this, SLOT( action_help_about() ) );
	pActionAbout->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowAbout ) );
	auto pActionReportBug = m_pInfoMenu->addAction(
		tr("&Report Bug"), this, SLOT( action_report_bug() ) );
	pActionReportBug->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowReportBug ) );
	auto pActionDonate = m_pInfoMenu->addAction(
		tr("&Donate"), this, SLOT( action_donate() ) );
	pActionDonate->setShortcut(
		pShortcuts->getKeySequence( Shortcuts::Action::ShowDonate ) );
	// ~ INFO menu

	setMenuBar( pMenubar );
}

void MainForm::startAutosaveTimer() {
	int nAutosavesPerHour = Preferences::get_instance()->m_nAutosavesPerHour;

	if ( nAutosavesPerHour > 0 ) {
		if ( nAutosavesPerHour > 360 ) {
			ERRORLOG( QString( "Too many autosaves per hour set [%1]. Using 360 - once a second - instead." )
					  .arg( nAutosavesPerHour ) );
			nAutosavesPerHour = 360;
		}
		m_AutosaveTimer.start( std::round( 60 * 60 * 1000 /
										   static_cast<float>(nAutosavesPerHour) ) );
	}
}

void MainForm::action_donate()
{
	QMessageBox donationDialog;
	donationDialog.setText( tr( "Hydrogen is an open source project which is developed by multiple people in their spare time. By making a donation you can say 'thank you' to the involved persons." ) );
	donationDialog.setStandardButtons( QMessageBox::Cancel );
	donationDialog.addButton( tr( "&Donate!" ), QMessageBox::AcceptRole );
		
	int nRet = donationDialog.exec();

	if ( nRet == QMessageBox::AcceptRole ) {
		QDesktopServices::openUrl(QUrl::fromEncoded("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=sebastian%2emoors%40gmail%2ecom&lc=DE&item_name=Hydrogen%20donation&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest"));
	}
}

void MainForm::action_file_exit() {
	closeAll();
}

void MainForm::action_file_new()
{
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		pHydrogen->sequencerStop();
	}

	if ( ! HydrogenApp::handleUnsavedChanges( Filesystem::Type::Song ) ) {
		return;
	}
	
	std::shared_ptr<Song> pSong = Song::getEmptySong();

	if ( bUnderSessionManagement ) {
		// Just a single click will allow the user to discard the
		// current song and replace it with an empty one with no way
		// of undoing the action. Therefore, a warning popup will
		// check whether the action was intentional.
		QMessageBox confirmationBox;
		confirmationBox.setText( tr( "Replace current song with empty one?" ) );
		confirmationBox.setInformativeText( tr( "You won't be able to undo this action after saving the new song! Please export the current song from the session first in order to keep it." ) );
		confirmationBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		confirmationBox.setDefaultButton( QMessageBox::No );
		
		int confirmationChoice = confirmationBox.exec();
		
		if ( confirmationChoice == QMessageBox::No ) {
			return;
		}
	}

	// Since the user explicitly chooses to open an empty song, we do
	// not attempt to recover the autosave file generated while last
	// working on an empty song but, instead, remove the corresponding
	// autosave file in order to start fresh.
	QFileInfo fileInfo( Filesystem::empty_path( Filesystem::Type::Song ) );
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}
	QFileInfo autoSaveFile( QString( "%1/.%2.autosave%3" )
							.arg( fileInfo.absoluteDir().absolutePath() )
							.arg( sBaseName ).arg( Filesystem::songs_ext ) );
	if ( autoSaveFile.exists() ) {
		Filesystem::rm( autoSaveFile.absoluteFilePath() );
	}
	
	HydrogenApp::openSong( pSong );

	// Ensure we are not removing an autosave file belonging to the previous
	// song.
	m_sPreviousAutoSaveSongFile = "";
}



bool MainForm::action_file_save_as()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPref = Preferences::get_instance();

	if ( pSong == nullptr ) {
		return false;
	}

	const bool bUnderSessionManagement = pHydrogen->isUnderSessionManagement();

	QString sPath = pPref->getLastSaveSongAsDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::songs_dir();
	}

	//std::auto_ptr<QFileDialog> fd( new QFileDialog );
	FileDialog fd(this);
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setDirectory( sPath );

	if ( bUnderSessionManagement ) {	
		fd.setWindowTitle( tr( "Export song from Session" ) );
	} else {
		fd.setWindowTitle( pCommonStrings->getActionSaveSong() );
	}
	
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::songs_dir() ) );

	QString sDefaultFilename;

	// Cache a couple of things we have to restore when under session
	// management.
	const QString sLastFilename = pSong->getFilename();

	if ( sLastFilename == Filesystem::empty_path( Filesystem::Type::Song ) ) {
		sDefaultFilename = Filesystem::default_song_name();
	}
	else if ( sLastFilename.isEmpty() ) {
		sDefaultFilename = pSong->getName();
	}
	else {
		QFileInfo fileInfo( sLastFilename );
		sDefaultFilename = fileInfo.completeBaseName();
	}
	sDefaultFilename += Filesystem::songs_ext;

	fd.selectFile( sDefaultFilename );

	if (fd.exec() == QDialog::Accepted) {
		QString sNewFilename = fd.selectedFiles().first();

		if ( ! sNewFilename.isEmpty() ) {
			pPref->setLastSaveSongAsDirectory( fd.directory().absolutePath( ) );

			if ( ! sNewFilename.endsWith( Filesystem::songs_ext ) ) {
				sNewFilename += Filesystem::songs_ext;
			}

			// We do not use the CoreActionController::saveSongAs
			// function directly since action_file_save as does some
			// additional checks and prompts the user a warning dialog
			// if required.
			if ( ! action_file_save( sNewFilename ) ) {
				ERRORLOG( "Unable to save song" );
				return false;
			}
		}

#ifdef H2CORE_HAVE_OSC
		// When Hydrogen is under session management, we only copy a
		// backup of the song to a different place but keep working on
		// the original.
		if ( bUnderSessionManagement ) {
			pSong->setFilename( sLastFilename );

			h2app->showStatusBarMessage( tr("Song exported as: ") + sDefaultFilename );
			pHydrogen->setSessionIsExported( false );
		}
		else {
			h2app->showStatusBarMessage( tr("Song saved as: ") + sDefaultFilename );
		}
#else
		h2app->showStatusBarMessage( tr("Song saved as: ") + sDefaultFilename );
#endif

		if ( sLastFilename == Filesystem::empty_path( Filesystem::Type::Song ) ) {
			// In case we stored the song for the first time, we remove the
			// autosave file corresponding to the empty one. Else, it might be
			// loaded later when clicking "New Song" while not generating a new
			// autosave file.
			const QString sAutoSaveFile = Filesystem::getAutoSaveFilename(
				Filesystem::Type::Song, sLastFilename );
			if ( Filesystem::file_exists( sAutoSaveFile, true ) ) {
				Filesystem::rm( sAutoSaveFile );
			}
		}
	}

	return true;
}



bool MainForm::action_file_save()
{
	return action_file_save( "" );
}
bool MainForm::action_file_save( const QString& sNewFilename,
								 bool bTriggerMessage )
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		return false;
	}
	
	QString sFilename = pSong->getFilename();

	if ( sNewFilename.isEmpty() &&
		 ( sFilename.isEmpty() ||
		   sFilename == Filesystem::empty_path( Filesystem::Type::Song ) ) ) {
		// The empty song is treated differently in order to allow
		// recovering changes and unsaved sessions. Therefore the
		// users are ask to store a new song using a different file
		// name.
		return action_file_save_as();
	}

	if ( pSong->hasMissingSamples() ) {
		if ( QMessageBox::information( this, "Hydrogen",
		                               tr( "Some samples used by this song failed to load. If you save the song now "
		                                   "these missing samples will be removed from the song entirely.\n"
			                               "Are you sure you want to save?" ),
		                               QMessageBox::Save | QMessageBox::Cancel,
		                               QMessageBox::Save )
		     == QMessageBox::Cancel ) {
			return false;
		}
		pSong->clearMissingSamples();
	}

	// Clear the pattern editor selection to resolve any duplicates
	HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()->clearSelection();

	bool bSaved;
	if ( sNewFilename.isEmpty() ) {
		bSaved = H2Core::CoreActionController::saveSong();
	} else {
		bSaved = H2Core::CoreActionController::saveSongAs( sNewFilename );
	}
	
	if( ! bSaved ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save song.") );
		return false;
	}

	if ( bTriggerMessage ) {
		h2app->showStatusBarMessage( tr("Song saved into") + QString(": ") +
									 sFilename );
	}

	return true;
}


void MainForm::action_inputMode_instrument() {
	auto pPref = Preferences::get_instance();

	if ( ! pPref->m_bPlaySelectedInstrument ) {
		pPref->m_bPlaySelectedInstrument = true;
		m_pDrumkitAction->setChecked( false );
	}
	m_pInstrumentAction->setChecked( true );
}

void MainForm::action_inputMode_drumkit() {
	auto pPref = Preferences::get_instance();

	if ( pPref->m_bPlaySelectedInstrument ) {
		pPref->m_bPlaySelectedInstrument = false;
		m_pInstrumentAction->setChecked( false );
	}
	m_pDrumkitAction->setChecked( true );
}

void MainForm::action_help_about() {
	AboutDialog *dialog = new AboutDialog( nullptr );
	dialog->exec();
}

void MainForm::action_report_bug()
{
	QDesktopServices::openUrl(QString("https://github.com/hydrogen-music/hydrogen/issues"));
}

// Find and open (a translation of) the manual appropriate for the user's preferences and locale
void MainForm::showUserManual()
{
	QString sDocPath = H2Core::Filesystem::doc_dir();
	QString sPreferredLanguage = Preferences::get_instance()->getPreferredLanguage();
	QStringList languages;

	if ( !sPreferredLanguage.isNull() ) {
		languages << sPreferredLanguage;
	}
	languages << QLocale::system().uiLanguages()
			  << "en"; // English as fallback

	// Find manual in filesystem
	for ( const QString& sLang : languages ) {
		QStringList sCandidates ( sLang );
		QStringList s = sLang.split('-');
		if ( s.size() != 1 ) {
			sCandidates << s[0];
		}
		for ( const QString& sCandidate : sCandidates ) {
			QString sManualPath = QString( "%1/manual_%2.html" ) .arg( sDocPath ).arg( sCandidate );
			if ( Filesystem::file_exists( sManualPath ) ) {
				QDesktopServices::openUrl( QUrl::fromLocalFile( sManualPath ) );
				return;
			}
		}
	}

	// No manual found, not even the default English one. This must be a broken installation, so let's open
	// the online manual as a sensible fallback option.

	QDesktopServices::openUrl( QString( "http://hydrogen-music.org/documentation/manual/manual_en.html" ) );

}

void MainForm::action_file_export_pattern_as( int nPatternRow )
{
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pPref = Preferences::get_instance();

	if ( Hydrogen::get_instance()->getAudioEngine()->getState() ==
		 H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencerStop();
	}

	if ( nPatternRow == -1 ) {
		nPatternRow = pHydrogen->getSelectedPatternNumber();
	}

	if ( nPatternRow == -1 ) {
		QMessageBox::warning( this, "Hydrogen", tr("No pattern selected.") );
		return;
	}

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ){
		return;
	}

	auto pPattern = pSong->getPatternList()->get( nPatternRow );
	if ( pPattern == nullptr ){
		ERRORLOG( QString( "Pattern [%1] could not be retrieved" )
				  .arg( nPatternRow ) );
		return;
	}

	QString sPath = pPref->getLastExportPatternAsDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::patterns_dir();
	}

	const QString sTitle = tr( "Save Pattern as ..." );
	FileDialog fd(this);
	fd.setWindowTitle( sTitle );
	fd.setDirectory( sPath );
	fd.selectFile( pPattern->getName() );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setSidebarUrls( fd.sidebarUrls() <<
					   QUrl::fromLocalFile( Filesystem::patterns_dir() ) );
	fd.setDefaultSuffix( Filesystem::patterns_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo( fd.selectedFiles().first() );
	pPref->setLastExportPatternAsDirectory( fileInfo.path() );
	const QString sFilePath = fileInfo.absoluteFilePath();

	QString sOriginalName = pPattern->getName();
	pPattern->setName( fileInfo.baseName() );
	if ( ! pPattern->save( sFilePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
	}
	else {
		h2app->showStatusBarMessage( tr( "Pattern saved." ) );

		if ( sFilePath.indexOf( Filesystem::patterns_dir() ) == 0 ) {
			pHydrogen->getSoundLibraryDatabase()->updatePatterns();
		}
	}

	pPattern->setName( sOriginalName );
}

void MainForm::action_file_open() {
	QString sPath = Preferences::get_instance()->getLastOpenSongDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::songs_dir();
	}

	QString sWindowTitle;
	if ( H2Core::Hydrogen::get_instance()->isUnderSessionManagement() ) {
		sWindowTitle = tr( "Import song into Session" );
	} else {
		sWindowTitle = tr( "Open song" );
	}

	openSongWithDialog( sWindowTitle, sPath, false );
}


void MainForm::action_file_openPattern()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pPref = Preferences::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	QString sPath = pPref->getLastOpenPatternDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::patterns_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode ( QFileDialog::ExistingFiles );
	fd.setDirectory ( sPath );
	fd.setNameFilter( Filesystem::patterns_filter_name );

	fd.setWindowTitle ( tr ( "Open Pattern" ) );

	if ( fd.exec() == QDialog::Accepted ) {
		pPref->setLastOpenPatternDirectory( fd.directory().absolutePath() );

		for ( const auto& ssFilename : fd.selectedFiles() ) {

			auto pNewPattern = Pattern::load( ssFilename );
			if ( pNewPattern == nullptr ) {
				QMessageBox::critical( this, "Hydrogen", HydrogenApp::get_instance()->getCommonStrings()->getPatternLoadError() );
			} else {
				int nRow;
				if ( pHydrogen->getSelectedPatternNumber() == -1 ) {
					nRow = pSong->getPatternList()->size();
				} else {
					nRow = pHydrogen->getSelectedPatternNumber() + 1;
				}
				
				SE_insertPatternAction* pAction =
					new SE_insertPatternAction( nRow, pNewPattern );
				HydrogenApp::get_instance()->pushUndoCommand( pAction );
			}
		}
	}
}

void MainForm::action_file_openDemo() {
	QString sWindowTitle;
	if ( ! H2Core::Hydrogen::get_instance()->isUnderSessionManagement() ) {
		sWindowTitle = tr( "Open Demo Song" );
	} else {
		sWindowTitle = tr( "Import Demo Song into Session" );
	}

	openSongWithDialog( sWindowTitle, Filesystem::demos_dir(), true );
}

bool MainForm::prepareSongOpening() {
	
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getAudioEngine()->getState() ==
		 H2Core::AudioEngine::State::Playing ) {
		pHydrogen->sequencerStop();
	}

	return HydrogenApp::handleUnsavedChanges( Filesystem::Type::Song );
}

void MainForm::openSongWithDialog( const QString& sWindowTitle, const QString& sPath, bool bIsDemo ) {
	// Check for unsaved changes.
	if ( ! prepareSongOpening() ) {
		return;
	}
	
	auto pHydrogen = Hydrogen::get_instance();

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( sPath );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setWindowTitle( sWindowTitle );

	QString sFilename;
	if ( fd.exec() == QDialog::Accepted ) {
		if ( ! bIsDemo ) {
			Preferences::get_instance()->setLastOpenSongDirectory( fd.directory().absolutePath() );
		}
		sFilename = fd.selectedFiles().first();
	}

	if ( !sFilename.isEmpty() ) {
		HydrogenApp::get_instance()->openFile( Filesystem::Type::Song, sFilename );
		if ( bIsDemo &&
			 ! pHydrogen->isUnderSessionManagement() ) {
			pHydrogen->getSong()->setFilename( "" );
		}
	}

	// Ensure we are not removing an autosave file belonging to the previous
	// song.
	m_sPreviousAutoSaveSongFile = "";
}

void MainForm::showPreferencesDialog()
{
	h2app->showPreferencesDialog();
}

void MainForm::action_window_showPlaylistEditor()
{
	h2app->showPlaylistEditor();
}

// function to update director status in menu bar
void MainForm::update_playlist_checkbox()
{
	bool isVisible = HydrogenApp::get_instance()->getPlaylistEditor()->isVisible();
	m_pViewPlaylistEditorAction->setChecked( isVisible );
}

void MainForm::action_window_show_DirectorWidget()
{
	h2app->showDirector();
}

// function to update director status in menu bar
void MainForm::update_director_checkbox()
{
	bool isVisible = HydrogenApp::get_instance()->getDirector()->isVisible();
	m_pViewDirectorAction->setChecked( isVisible );
}

void MainForm::action_window_toggleFullscreen()
{
	if( this->isFullScreen() ){
		this->showNormal();
	} else {
		this->showFullScreen();
	}
}

void MainForm::action_window_showMixer() {
	h2app->showMixer( ! HydrogenApp::get_instance()->getMixer()->isVisible() );
}

void MainForm::action_debug_showAudioEngineInfo()
{
	h2app->showAudioEngineInfoForm();
}

void MainForm::action_debug_showFilesystemInfo()
{
	h2app->showFilesystemInfoForm();
}

void MainForm::action_debug_logLevel_none()
{
	Logger* pLogger = Logger::get_instance();
	pLogger->set_bit_mask( Logger::None );
}

void MainForm::action_debug_logLevel_error()
{
	Logger* pLogger = Logger::get_instance();
	pLogger->set_bit_mask( Logger::Error );
}

void MainForm::action_debug_logLevel_warn()
{
	Logger* pLogger = Logger::get_instance();
	pLogger->set_bit_mask( Logger::Error | Logger::Warning );
}

void MainForm::action_debug_logLevel_info()
{
	Logger* pLogger = Logger::get_instance();
	pLogger->set_bit_mask( Logger::Error | Logger::Warning | Logger::Info );
}

void MainForm::action_debug_logLevel_debug()
{
	Logger* pLogger = Logger::get_instance();
	pLogger->set_bit_mask( Logger::Error | Logger::Warning | Logger::Info | Logger::Debug );
}

void MainForm::action_debug_openLogfile()
{
	QDesktopServices::openUrl( Filesystem::log_file_path() );
}


///
/// Shows the song editor
///
void MainForm::action_window_showSongEditor()
{
	bool isVisible = h2app->getSongEditorPanel()->isVisible();
	h2app->getSongEditorPanel()->setHidden( isVisible );
}

void MainForm::action_window_showTimeline()
{
	h2app->getSongEditorPanel()->showTimeline();
}


void MainForm::action_window_showPlaybackTrack()
{
	h2app->getSongEditorPanel()->showPlaybackTrack();
}

void MainForm::action_window_showAutomationArea()
{
	h2app->getSongEditorPanel()->toggleAutomationAreaVisibility();
}

void MainForm::action_drumkit_open()
{
	DrumkitOpenDialog dialog( this );
	dialog.exec();
}


void MainForm::action_drumkit_new()
{
	switch( QMessageBox::information(
				this, "Hydrogen",
				tr( "Replace the drumkit of the current song with an empty one?" ),
				QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel ) ) {
	case QMessageBox::Ok:
		// ok btn pressed
		break;
	case QMessageBox::Cancel:
		// cancel btn pressed
		return;
	default:
		// Not reached
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	// Remove all instruments
	auto pNewDrumkit = Drumkit::getEmptyDrumkit();

	pHydrogenApp->pushUndoCommand(
		new SE_switchDrumkitAction(
			pNewDrumkit, Hydrogen::get_instance()->getSong()->getDrumkit(),
			SE_switchDrumkitAction::Type::NewDrumkit ) );
	pHydrogenApp->showStatusBarMessage( pCommonStrings->getActionNewDrumkit() );
}

void MainForm::action_drumkit_addInstrument(
	std::shared_ptr<H2Core::Instrument> pInstrument )
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	if ( pInstrument == nullptr ) {
		pInstrument = std::make_shared<Instrument>();
	}

	// In case we add an instrument to a row already containing types notes, the
	// instrument ID of these notes will be mapped to the one registered to the
	// new instrument. However, when removing the instrument again in an undo
	// action, this ID change is not done automatically but has to be covered in
	// a dedicated action.
	std::vector< std::pair< int, std::vector< std::shared_ptr<Note> > > > affectedNotes;
	if ( ! pInstrument->getType().isEmpty() ) {
		for ( int nn = 0; nn < pSong->getPatternList()->size(); ++nn ) {
			auto ppPattern = pSong->getPatternList()->get( nn );
			if ( ppPattern != nullptr &&
				 ppPattern->getAllNotesOfType( pInstrument->getType() ).size() > 0 ) {
				auto notes = ppPattern->getAllNotesOfType(
					pInstrument->getType() );
				// Ensure we do not loose the ID when the note is mapped.
				std::vector< std::shared_ptr<Note> > copiedNotes;
				for ( const auto& ppNote : notes ) {
					if ( ppNote != nullptr ) {
						copiedNotes.push_back( std::make_shared<Note>( ppNote ) );
					}
				}
				affectedNotes.push_back(std::make_pair( nn, copiedNotes ) );
			}
		}
	}

	if ( affectedNotes.size() > 0 ) {
		pHydrogenApp->beginUndoMacro( pCommonStrings->getActionAddInstrument() );
	}

	// If the instrument was not added to a particular row, ensure it uses an id
	// not already present in both the notes within the current song (since the
	// user action was explicitly _not_ to add an instrument to a row but a new
	// one) and the instruments in the current drumkit (to prevent supplying an
	// invalid ID and triggering the fallback in Drumkit::addInstrument which
	// could end up using an ID of a note again).
	if ( pInstrument->getType().isEmpty() &&
		 pInstrument->getId() == EMPTY_INSTR_ID ) {
		std::set<int> presentIds;
		for ( const auto& ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
			if ( ppInstrument != nullptr ) {
				presentIds.insert( ppInstrument->getId() );
			}
		}

		for ( const auto& ppPattern : *pSong->getPatternList() ) {
			for ( const auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
				// We only have to take those note not bearing a type into
				// account. A type will always take precedence in mapping and
				// even though the note could have the same ID as the empty,
				// untyped instrument, it will never be associated with it.
				if ( ppNote != nullptr && ppNote->getType().isEmpty() ) {
					presentIds.insert( ppNote->getInstrumentId() );
				}
			}
		}

		// Pick an unique ID for the new instrument.
		for ( int ii = 0; ii < presentIds.size() + 1; ++ii ) {
			if ( presentIds.find( ii ) == presentIds.end() ) {
				pInstrument->setId( ii );
				break;
			}
		}
	}

	pHydrogenApp->pushUndoCommand(
		new SE_addInstrumentAction(
			pInstrument, -1, SE_addInstrumentAction::Type::AddEmptyInstrument ) );

	if ( affectedNotes.size() > 0 ) {
		// After the action was pushed and the corresponding redo executed, the
		// instrument has its proper ID within the Drumkit. We can use it set up
		// the note undo actions.
		for ( const auto& [ nnPatternNumber, nnotes ] : affectedNotes ) {
			for ( const auto& ppNote : nnotes ) {
				if ( ppNote != nullptr ) {
					pHydrogenApp->pushUndoCommand(
						new SE_editNotePropertiesAction(
							PatternEditor::Property::InstrumentId,
							nnPatternNumber,
							ppNote->getPosition(),
							pInstrument->getId(),
							ppNote->getInstrumentId(),
							ppNote->getType(),
							ppNote->getType(),
							ppNote->getVelocity(),
							ppNote->getVelocity(),
							ppNote->getPan(),
							ppNote->getPan(),
							ppNote->getLeadLag(),
							ppNote->getLeadLag(),
							ppNote->getProbability(),
							ppNote->getProbability(),
							ppNote->getLength(),
							ppNote->getLength(),
							ppNote->getKey(),
							ppNote->getKey(),
							ppNote->getOctave(),
							ppNote->getOctave() ) );
				}
			}
		}
		pHydrogenApp->endUndoMacro();
	}

	pHydrogenApp->showStatusBarMessage(
		pCommonStrings->getActionAddInstrument() );

	// Select the new instrument. It will be appended to the instrument list of
	// the current drumkit.
	auto pPatternEditorPanel = pHydrogenApp->getPatternEditorPanel();
	pPatternEditorPanel->updateDB();
	pPatternEditorPanel->setSelectedRowDB(
		pSong->getDrumkit()->getInstruments()->size() - 1 );
	pPatternEditorPanel->updateEditors( Editor::Update::Background );
	pPatternEditorPanel->ensureCursorIsVisible();
}

void MainForm::action_drumkit_deleteInstrument( int nInstrumentIndex )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pSelectedInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentIndex );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( QString( "Could not find instrument corresponding to index [%1]" )
				  .arg( nInstrumentIndex ) );
		return;
	}

	// If there is just a single instrument, we will replace it with an empty
	// one instead of deleting it.
	if ( pSong->getDrumkit()->getInstruments()->size() == 1 ) {
		auto pAction = new SE_replaceInstrumentAction(
			std::make_shared<Instrument>(), pSelectedInstrument,
			SE_replaceInstrumentAction::Type::DeleteLastInstrument,
			pSelectedInstrument->getName() );
		pHydrogenApp->pushUndoCommand( pAction );
	}
	else {
		auto pAction = new SE_deleteInstrumentAction(
			pSelectedInstrument, nInstrumentIndex );
		pHydrogenApp->pushUndoCommand( pAction );
	}
	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionDeleteInstrument() )
		.arg( pSelectedInstrument->getName() ) );
}

void MainForm::action_drumkit_renameInstrument( int nInstrumentIndex )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrument =
		pSong->getDrumkit()->getInstruments()->get( nInstrumentIndex );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument in row [%1]" )
				  .arg( nInstrumentIndex ) );
		return;
	}

	const QString sOldName = pInstrument->getName();
	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		nullptr, "Hydrogen", pCommonStrings->getActionRenameInstrument(),
		QLineEdit::Normal, sOldName, &bIsOkPressed );
	if ( bIsOkPressed ) {
		auto pNewInstrument = std::make_shared<Instrument>(pInstrument);
		pNewInstrument->setName( sNewName );

		pHydrogenApp->pushUndoCommand(
			new SE_replaceInstrumentAction(
				pNewInstrument, pInstrument,
				SE_replaceInstrumentAction::Type::RenameInstrument,
				sNewName, sOldName ) );

		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2] -> [%3]" )
			.arg( pCommonStrings->getActionRenameInstrument() )
			.arg( sOldName ).arg( sNewName ) );
	}
}

void MainForm::action_drumkit_export() {

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	const auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ){
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// The song drumkit we want to export is a somewhat floating one. It does
	// not contain samples by itself but is composed of instruments that either
	// borrow their samples from kits in the Sound Library or access them using
	// absolute paths. To allow exporting it, we save it to a temporary folder
	// and proceed with a regular export from there.
	QTemporaryDir tmpDir;
    if ( ! tmpDir.isValid() ) {
		ERRORLOG( "Unable to create tmp folder" );
		QMessageBox::critical( nullptr, "Hydrogen",
							   pCommonStrings->getExportDrumkitFailure() );
		return;
    }

	QApplication::setOverrideCursor( Qt::WaitCursor );

	INFOLOG( QString( "Saving song kit to temporary folder [%1] for export" )
			 .arg( tmpDir.path() ) );

	auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
	pNewDrumkit->setPath( tmpDir.path() );

	if ( ! pNewDrumkit->save( tmpDir.path() ) ) {
		QApplication::restoreOverrideCursor();
		ERRORLOG( QString( "Unable to save kit to tmp folder [%1]" )
				  .arg( tmpDir.path() ) );
		QMessageBox::critical( nullptr, "Hydrogen",
							   pCommonStrings->getExportDrumkitFailure() );
		return;
	}

	QApplication::restoreOverrideCursor();

	exportDrumkit( pNewDrumkit );
}

void MainForm::exportDrumkit( std::shared_ptr<Drumkit> pDrumkit ) {
	if ( ! HydrogenApp::checkDrumkitLicense( pDrumkit ) ) {
		ERRORLOG( "User cancelled dialog due to licensing issues." );
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pPref = H2Core::Preferences::get_instance();

	QString sPath = pPref->getLastExportDrumkitDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	const QString sTitle = QString( "%1 [%2]" )
		.arg( tr( "Export Drumkit" ) )
		.arg( pDrumkit != nullptr ? pDrumkit->getName() : tr( "invalid drumkit" ) );

	FileDialog fd( nullptr );
	fd.setWindowTitle( sTitle );
	fd.setDirectory( sPath );
	fd.selectFile( pDrumkit->getExportName() );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::drumkit_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setDefaultSuffix( Filesystem::drumkit_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo( fd.selectedFiles().first() );
	pPref->setLastExportDrumkitDirectory( fileInfo.path() );
	QString sFilePath = fileInfo.absoluteFilePath();

	if ( ! Filesystem::dir_writable(
			 fileInfo.absoluteDir().absolutePath(), false ) ) {
		QMessageBox::warning( nullptr, "Hydrogen",
							  pCommonStrings->getFileDialogMissingWritePermissions(),
							  QMessageBox::Ok );
		return;
	}

	if ( Filesystem::file_exists( sFilePath, true ) ) {
		if ( QMessageBox::warning(
				 nullptr, "Hydrogen",
				 tr( "The file [%1] does already exist and will be overwritten.")
				 .arg( sFilePath ),
				 QMessageBox::Ok | QMessageBox::Cancel,
				 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
			return;
		}
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	if ( ! pDrumkit->exportTo( sFilePath ) ) {
		QApplication::restoreOverrideCursor();

#ifndef H2CORE_HAVE_QT6
		// Check whether encoding might be the problem in here.
		auto pCodec = QTextCodec::codecForLocale();
		if ( ! pCodec->canEncode( sFilePath ) ) {
			QMessageBox::critical(
				nullptr, "Hydrogen", QString( "%1\n\n%2\n\n%3: [%4]" )
				.arg( pCommonStrings->getExportDrumkitFailure() )
				.arg( sFilePath )
				.arg( pCommonStrings->getEncodingError() )
				.arg( QString( pCodec->name() ) ) );
		}
		else {
			QMessageBox::critical( nullptr, "Hydrogen",
								   pCommonStrings->getExportDrumkitFailure() );
		}
#else
		QMessageBox::critical( nullptr, "Hydrogen",
							   pCommonStrings->getExportDrumkitFailure() );
#endif

		return;
	}

	QApplication::restoreOverrideCursor();
	QMessageBox::information( nullptr, "Hydrogen",
							  tr("Drumkit exported to") + "\n" +
							  sFilePath );
}

bool MainForm::checkDrumkitPathEncoding( const QString& sPath,
										 const QString& sContext ) {

#ifndef H2CORE_HAVE_QT6
	// Check whether encoding might be the problem in here.
	auto pCodec = QTextCodec::codecForLocale();
	if ( ! pCodec->canEncode( sPath ) ) {
		QMessageBox::critical(
			nullptr, "Hydrogen", QString( "%1\n\n%2\n\n%3: [%4]" )
			.arg( sContext ).arg( sPath )
			.arg( HydrogenApp::get_instance()->getCommonStrings()
				  ->getEncodingError() )
			.arg( QString( pCodec->name() ) ) );
		return false;
	}
#endif

	return true;
}


void MainForm::action_drumkit_import( bool bLoad ) {
	auto pPreferences = H2Core::Preferences::get_instance();

	QString sPath = pPreferences->getLastImportDrumkitDirectory();
	if ( ! H2Core::Filesystem::dir_readable( sPath, false ) ){
		sPath = QDir::homePath();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( "Hydrogen drumkit (*.h2drumkit)" );
	fd.setDirectory( sPath );

	fd.setWindowTitle( tr( "Import drumkit" ) );

	QString sFileName = "";
	if ( fd.exec() == QDialog::Accepted ) {
		sFileName = fd.selectedFiles().first();
	} else {
		// Closed
		return;
	}

	if ( sFileName.isEmpty() ) {
		ERRORLOG( "No drumkit file selected." );
		return;
	}

	pPreferences->setLastImportDrumkitDirectory( fd.directory().absolutePath() );

	loadDrumkit( sFileName, bLoad );
}

void MainForm::loadDrumkit( const QString& sFileName, bool bLoad ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QApplication::setOverrideCursor( Qt::WaitCursor );

	QString sImportedPath;
	bool bEncodingIssues;
	if ( ! H2Core::Drumkit::install( sFileName, "", &sImportedPath,
									 &bEncodingIssues, false ) ) {
		QApplication::restoreOverrideCursor();
		if ( checkDrumkitPathEncoding(
				 sFileName, pCommonStrings->getImportDrumkitFailure() ) ) {
			// In case it was not an encoding error, we have to
			// create and error window ourselves.
			QMessageBox::critical(
				nullptr, "Hydrogen", QString( "%1\n\n%2" )
				.arg( pCommonStrings->getImportDrumkitFailure() )
				.arg( sFileName ) );
		}

		return;
	}

	// update the drumkit list
	pSoundLibraryDatabase->updateDrumkits();

	if ( bLoad ) {
#ifdef H2CORE_HAVE_LIBARCHIVE
		if ( ! sImportedPath.isEmpty() ) {
			auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sImportedPath );
			if ( pDrumkit == nullptr ) {
				ERRORLOG( QString( "Unable to load freshly imported kit [%1]" )
						  .arg( sFileName ) );
			}
			else {

				// Pass copy to allow kit in the SoundLibraryDatabase to stay in
				// a pristine shape.
				if ( ! switchDrumkit( std::make_shared<Drumkit>( pDrumkit ) ) ) {
					ERRORLOG( QString( "Unable to switch to freshly imported kit [%1]" )
							  .arg( sFileName ) );
				}
			}
		}
		else {
			ERRORLOG( QString( "Unable to determine imported path for [%1]" )
					  .arg( sFileName ) );
		}
#else
		WARNINGLOG( "Imported drumkit was not loaded. This feature is only supported when compiled with libarchive." );
#endif
	}

	QApplication::restoreOverrideCursor();
	if ( ! bEncodingIssues ) {
		QMessageBox::information( this, "Hydrogen",
								  QString( "%1 [%2]" )
								  .arg( pCommonStrings->getImportDrumkitSuccess() )
								  .arg( sImportedPath ) );
	}
	else {
		QMessageBox::warning(
			this, "Hydrogen",
			QString( "%1 [%2]%3" ).arg( pCommonStrings->getImportDrumkitSuccess() )
			.arg( sImportedPath )
			.arg( pCommonStrings->getImportDrumkitEncodingFailure() ) );
	}
}

void MainForm::action_drumkit_onlineImport()
{
	SoundLibraryOnlineImportDialog dialog( this );
	dialog.exec();
}

void MainForm::action_drumkit_save()
{
	editDrumkitProperties( true, false );
}

void MainForm::action_drumkit_save_to_session() {
	editDrumkitProperties( true, true );
}

///
/// Window close event
///
void MainForm::closeEvent( QCloseEvent* ev ) {
	if ( ! handleUnsavedChangesDuringShutdown() ) {
		// don't close!!!
		ev->ignore();
		return;
	}

	closeAll();
	ev->accept();
}



void MainForm::action_file_export()
{
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencerStop();
	}

	ExportSongDialog *dialog = new ExportSongDialog(this);
	dialog->exec();
	delete dialog;
}



void MainForm::action_window_showInstrumentRack() {
	h2app->showInstrumentRack(
		! HydrogenApp::get_instance()->getInstrumentRack()->isVisible() );
}

void MainForm::saveWindowProperties() {
	// save window properties in the preferences files
	auto pPreferences = Preferences::get_instance();

	// mainform
	pPreferences->setMainFormProperties( h2app->getWindowProperties( this ) );
	// Save mixer properties
	pPreferences->setMixerProperties( h2app->getWindowProperties( h2app->getMixer() ) );
	// save pattern editor properties
	pPreferences->setPatternEditorProperties( h2app->getWindowProperties( h2app->getPatternEditorPanel() ) );
	// save song editor properties
	pPreferences->setSongEditorProperties( h2app->getWindowProperties( h2app->getSongEditorPanel() ) );
	pPreferences->setInstrumentRackProperties( h2app->getWindowProperties( h2app->getInstrumentRack() ) );
	// save audio engine info properties
	pPreferences->setAudioEngineInfoProperties( h2app->getWindowProperties( h2app->getAudioEngineInfoForm() ) );

	pPreferences->setPlaylistEditorProperties(
		h2app->getWindowProperties( h2app->getPlaylistEditor() ) );
	pPreferences->setDirectorProperties(
		h2app->getWindowProperties( h2app->getDirector() ) );

#ifdef H2CORE_HAVE_LADSPA
	// save LADSPA FX window properties
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		pPreferences->setLadspaProperties( nFX, h2app->getWindowProperties( h2app->getLadspaFXProperties( nFX ) ) );
	}
#endif
}

void MainForm::closeAll(){
	if ( ! handleUnsavedChangesDuringShutdown() ) {
		return;
	}

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	pHydrogen->setGUIState( H2Core::Hydrogen::GUIState::shutdown );

	disconnect( h2app->m_pUndoStack, nullptr, nullptr, nullptr );

	saveWindowProperties();
	m_pQApp->quit();
}


void MainForm::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & H2Core::Preferences::Changes::Font ) {
		const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();
	
		QFont font( pFontTheme->m_sApplicationFontFamily,
					getPointSize( pFontTheme->m_fontSize ) );
		m_pQApp->setFont( font );
		menuBar()->setFont( font );

		m_pFileMenu->setFont( font );
		m_pUndoMenu->setFont( font );
		m_pDrumkitMenu->setFont( font );
		m_pViewMenu->setFont( font );
		m_pOptionsMenu->setFont( font );
		if ( m_pDebugMenu != nullptr ) {
			m_pDebugMenu->setFont( font );
		}
		m_pInfoMenu->setFont( font );

		Skin::setPalette( m_pQApp );
	}

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		Skin::setPalette( m_pQApp );
	}

	if ( changes & H2Core::Preferences::Changes::GeneralTab ) {
		startAutosaveTimer();
	}
	
	if ( changes & H2Core::Preferences::Changes::ShortcutTab ) {
		createMenuBar();
	}
}
	
bool MainForm::nullDriverCheck() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::warning( this, "Hydrogen",
							  QString( "%1\n%2" )
							  .arg( pCommonStrings->getAudioDriverNotPresent() )
							  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
		return false;
	}

	return true;
}

bool MainForm::handleUnsavedChangesDuringShutdown() {
	if ( ! m_bUnsavedChangesHandled &&
		 ( ! HydrogenApp::handleUnsavedChanges( Filesystem::Type::Song ) ||
		   ! HydrogenApp::handleUnsavedChanges( Filesystem::Type::Playlist ) ) ) {
		return false;
	}

	m_bUnsavedChangesHandled = true;

	return true;
}

void MainForm::updateRecentUsedSongList()
{
	m_pRecentFilesMenu->clear();

	const QStringList recentUsedSongs =
		Preferences::get_instance()->getRecentFiles();

	for ( const auto& ssFilename : recentUsedSongs ) {
		if ( ! ssFilename.isEmpty() ) {
			QAction *pAction = new QAction( this  );
			pAction->setText( ssFilename );
			m_pRecentFilesMenu->addAction( pAction );
		}
	}
}



void MainForm::action_file_open_recent(QAction *pAction)
{
	// Check for unsaved changes.
	if ( ! prepareSongOpening() ) {
		return;
	}
	
	HydrogenApp::get_instance()->openFile(
		Filesystem::Type::Song, pAction->text() );

	m_sPreviousAutoSaveSongFile = "";
}

void MainForm::checkMissingSamples()
{
	if ( Hydrogen::get_instance()->getSong()->hasMissingSamples() ) {
		m_pMissingSamplesInfoBar = h2app->addInfoBar();
		m_pMissingSamplesInfoBar->setTitle( tr( "Song drumkit samples" ) );
		m_pMissingSamplesInfoBar->setText( tr( "Some samples used in this song could not be loaded. This may be because it uses an older default drumkit. This might be fixed by opening a new drumkit." ) );

		QPushButton *fix = m_pMissingSamplesInfoBar->addButton( tr( "Open drumkit" ) );
		QObject::connect( fix, SIGNAL( clicked() ),
						  this, SLOT( onFixMissingSamples() ) );
		m_pMissingSamplesInfoBar->show();
	}
}


void MainForm::checkMidiSetup()
{
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	if ( pSong->getDrumkit()->getInstruments()->hasAllMidiNotesSame() ) {
		WARNINGLOG( "Incorrect MIDI setup" );

		m_pMidiSetupInfoBar = h2app->addInfoBar();
		m_pMidiSetupInfoBar->reset();
		m_pMidiSetupInfoBar->setTitle( tr("MIDI setup advice") );
		m_pMidiSetupInfoBar->setText( tr("MIDI out notes are not configured for this drumkit, so exporting this song to MIDI file may fail. Would you like Hydrogen to automatically fix this by assigning default values?") );
		QPushButton *fix = m_pMidiSetupInfoBar->addButton( tr("Set default values") );
		QObject::connect( fix, SIGNAL(clicked()), this, SLOT(onFixMidiSetup()) );
		m_pMidiSetupInfoBar->show();
	} else {
		m_pMidiSetupInfoBar = nullptr;
	}
}

void MainForm::checkNecessaryDirectories()
{
	//Make sure that all directories which are needed by Hydrogen are existing and usable.
	QString sTempDir = Filesystem::tmp_dir();
	
	if( !Filesystem::dir_writable(sTempDir))
	{
		QMessageBox::warning( this, "Hydrogen", tr("Could not write to temporary directory %1.").arg(sTempDir) );
	}
}

void MainForm::onFixMidiSetup()
{
	INFOLOG( "Fixing MIDI setup" );
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong != nullptr ) {
		pSong->getDrumkit()->getInstruments()->setDefaultMidiOutNotes();
		pHydrogen->setIsModified( true );

		m_pMidiSetupInfoBar->hide();
	}
}


void MainForm::onFixMissingSamples()
{
	INFOLOG( "Fixing MIDI setup" );
	DrumkitOpenDialog dialog( this );
	dialog.exec();

	m_pMissingSamplesInfoBar->hide();
}

bool MainForm::eventFilter( QObject *o, QEvent *e )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	
	if ( e->type() == QEvent::FileOpen ) {
		// Mac OS always opens files (including via double click in Finder) via a FileOpenEvent.
		QFileOpenEvent *fe = dynamic_cast<QFileOpenEvent*>(e);
		assert( fe != nullptr );
		QString sFileName = fe->file();

		if ( sFileName.endsWith( H2Core::Filesystem::songs_ext ) ) {
			if ( HydrogenApp::handleUnsavedChanges( Filesystem::Type::Song ) ) {
				HydrogenApp::openFile( Filesystem::Type::Song, sFileName );
			}

		}
		else if ( sFileName.endsWith( H2Core::Filesystem::drumkit_ext ) ) {
			loadDrumkit( sFileName, true );
		}
		else if ( sFileName.endsWith( H2Core::Filesystem::playlist_ext ) ) {
			if ( HydrogenApp::handleUnsavedChanges( Filesystem::Type::Playlist ) ) {
				HydrogenApp::openFile( Filesystem::Type::Playlist, sFileName );
			}
		}
		return true;

	} else if ( e->type() == QEvent::KeyPress ) {
		// special processing for key press
		QKeyEvent* pKeyEvent = dynamic_cast<QKeyEvent*>(e);
		assert( pKeyEvent != nullptr );

		return handleKeyEvent( o, pKeyEvent );
	}
	else {
		return false; // standard event processing
	}
}

/// print the object map
void MainForm::action_debug_printObjects()
{
	INFOLOG( "[action_debug_printObjects]" );
	Base::write_objects_map_to_cerr();
}






void MainForm::action_file_export_midi()
{
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencerStop();
	}

	ExportMidiDialog *dialog = new ExportMidiDialog(this);
	dialog->exec();
	delete dialog;
}




void MainForm::action_file_export_lilypond()
{
	auto pPref = Preferences::get_instance();

	if ( Hydrogen::get_instance()->getAudioEngine()->getState() ==
		 H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencerStop();
	}

	QMessageBox::information(
		this,
		"Hydrogen",
		tr( "\nThe LilyPond export is an experimental feature.\n"
		"It should work like a charm provided that you use the "
		"GMRockKit, and that you do not use triplet.\n" ),
		QMessageBox::Ok );

	QString sPath = pPref->getLastExportLilypondDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::usr_data_path();
	}

	FileDialog fd( this );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( tr( "LilyPond file (*.ly)" ) );
	fd.setDirectory( sPath );
	fd.setWindowTitle( tr( "Export LilyPond file" ) );
	fd.setAcceptMode( QFileDialog::AcceptSave );

	QString sFilename;
	if ( fd.exec() == QDialog::Accepted ) {
		pPref->setLastExportLilypondDirectory( fd.directory().absolutePath() );
		sFilename = fd.selectedFiles().first();
	}

	if ( !sFilename.isEmpty() ) {
		if ( sFilename.endsWith( ".ly" ) == false ) {
			sFilename += ".ly";
		}

		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();

		LilyPond ly;
		ly.extractData( *pSong );
		ly.write( sFilename );
	}
}

void MainForm::errorEvent( int nErrorCode )
{
	//ERRORLOG( "[errorEvent]" );

	QString msg;
	switch (nErrorCode) {
	case Hydrogen::UNKNOWN_DRIVER:
		msg = tr( "Unknown audio driver" );
		break;

	case Hydrogen::ERROR_STARTING_DRIVER:
		msg = tr( "Error starting audio driver" );
		break;

	case Hydrogen::JACK_SERVER_SHUTDOWN:
		msg = tr( "Jack driver: server shutdown" );
		break;

	case Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT:
		msg = tr( "Jack driver: cannot activate client" );
		break;

	case Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT:
		msg = tr( "Jack driver: cannot connect output port" );
		break;

	case Hydrogen::JACK_CANNOT_CLOSE_CLIENT:
		msg = tr( "Jack driver: cannot disconnect client" );
		break;

	case Hydrogen::JACK_ERROR_IN_PORT_REGISTER:
		msg = tr( "Jack driver: error in port register" );
		break;
		
	case Hydrogen::OSC_CANNOT_CONNECT_TO_PORT:
#ifdef H2CORE_HAVE_OSC
		msg = QString( tr( "OSC Server: Cannot connect to given port, using port %1 instead" ) )
			.arg( OscServer::get_instance()->getTemporaryPort() );
#else
		// Not translated since this one should never the triggered.
		msg = "Missing OSC support";
#endif
		break;

	case Hydrogen::PLAYBACK_TRACK_INVALID:
		msg = tr( "Playback track couldn't be read" );
		break;

	default:
		msg = QString( tr( "Unknown error %1" ) ).arg( nErrorCode );
	}
	QMessageBox::information( this, "Hydrogen", msg );
}

void MainForm::action_file_songProperties()
{
	if ( H2Core::Hydrogen::get_instance()->getSong() == nullptr ) {
		return;
	}
	
	SongPropertiesDialog *pDialog = new SongPropertiesDialog( this );
	if ( pDialog->exec() ) {
		// Ensure the update name is taken into account in the window
		// title.
		HydrogenApp::get_instance()->updateWindowTitle();
	}
	delete pDialog;
}


void MainForm::action_window_showPatternEditor()
{
	bool isVisible = HydrogenApp::get_instance()->getPatternEditorPanel()->isVisible();
	HydrogenApp::get_instance()->getPatternEditorPanel()->setHidden( isVisible );
}


void MainForm::showDevelWarning()
{
	auto pPreferences = Preferences::get_instance();
	bool isDevelWarningEnabled = pPreferences->getShowDevelWarning();

	//set this to 'false' for the case that you want to make a release..
	if ( H2CORE_IS_DEVEL_BUILD ) {
		if(isDevelWarningEnabled) {
			auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

			QString msg = tr( "You're using a development version of Hydrogen, please help us reporting bugs or suggestions in the hydrogen-devel mailing list.<br><br>Thank you!" );
			QMessageBox develMessageBox( this );
			develMessageBox.setText( msg );
			develMessageBox.addButton( pCommonStrings->getButtonOk(),
									   QMessageBox::YesRole );
			develMessageBox.addButton( pCommonStrings->getMutableDialog(),
									   QMessageBox::AcceptRole );

			if( develMessageBox.exec() == 1 ){
				//don't show warning again
				pPreferences->setShowDevelWarning( false );
			}
		}
	} else {
		// Release builds
		if ( !isDevelWarningEnabled ) {
			// Running a release build, we should re-enable the development-build warning if it's been
			// disabled, since the user might have tried a release build at some time in the past, then
			// continued working happily with a release build. They will still benefit from knowing that a
			// *new* release build they're trying is in fact a release build.
			pPreferences->setShowDevelWarning( true );
		}
	}
}

void MainForm::onAutoSaveTimer()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPlaylist = pHydrogen->getPlaylist();

	if ( pSong != nullptr && pSong->getIsModified() ) {
		const QString sOldFilename = pSong->getFilename();

		const QString sAutoSaveFilename = Filesystem::getAutoSaveFilename(
			Filesystem::Type::Song, pSong->getFilename() );
		if ( sAutoSaveFilename != m_sPreviousAutoSaveSongFile ) {
			if ( ! m_sPreviousAutoSaveSongFile.isEmpty() ) {
				QFile file( m_sPreviousAutoSaveSongFile );
				file.remove();
			}
			m_sPreviousAutoSaveSongFile = sAutoSaveFilename;
		}
			
		pSong->save( sAutoSaveFilename );

		pSong->setFilename( sOldFilename );
		pSong->setIsModified( true );
	}

	if ( pPlaylist != nullptr && pPlaylist->getIsModified() ) {
		const QString sOldFilename = pPlaylist->getFilename();

		const QString sAutoSaveFilename = Filesystem::getAutoSaveFilename(
			Filesystem::Type::Playlist, pPlaylist->getFilename() );
		if ( sAutoSaveFilename != m_sPreviousAutoSavePlaylistFile ) {
			if ( ! m_sPreviousAutoSavePlaylistFile.isEmpty() ) {
				QFile file( m_sPreviousAutoSavePlaylistFile );
				file.remove();
			}
			m_sPreviousAutoSavePlaylistFile = sAutoSaveFilename;
		}

		pPlaylist->saveAs( sAutoSaveFilename );

		pPlaylist->setFilename( sOldFilename );
		pPlaylist->setIsModified( true );

	}
}


void MainForm::onPlaylistDisplayTimer()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pPlaylist = pHydrogen->getPlaylist();
	auto pSong = pHydrogen->getSong();
	if( pPlaylist->size() == 0) {
		return;
	}
	if ( pSong == nullptr ) {
		return;
	}
	
	int songnumber = pPlaylist->getActiveSongNumber();
	QString songname;
	if ( songnumber == -1 ) {
		return;
	}

	if ( pSong->getName() == "Untitled Song" ){
		songname = pSong->getFilename();
	} else {
		songname = pSong->getName();
	}
	QString message = (tr("Playlist: Song No. %1").arg( songnumber + 1)) +
		QString("  ---  Songname: ") + songname + QString("  ---  Author: ") +
		pSong->getAuthor();
	HydrogenApp::get_instance()->showStatusBarMessage( message );
}

void MainForm::usr1SignalHandler(int)
{
	char a = 1;
	int nRes = ::write(sigusr1Fd[0], &a, sizeof(a));
	if ( nRes < 0 ) {
		ERRORLOG( "Unable to write to signal handler" );
	}
}

void MainForm::handleSigUsr1()
{
	snUsr1->setEnabled(false);
	char tmp;
	int nRes = ::read(sigusr1Fd[1], &tmp, sizeof(tmp));
	if ( nRes < 0 ) {
		ERRORLOG( "Unable to write to signal handler" );
	}

	action_file_save();
	snUsr1->setEnabled(true);
}

void MainForm::openUndoStack()
{
	m_pUndoView->show();
	m_pUndoView->setAttribute(Qt::WA_QuitOnClose, false);
}

void MainForm::action_undo(){
	// Be sure to close an existing context (cusotm macro).
	h2app->endUndoContext();
	h2app->m_pUndoStack->undo();
}

void MainForm::action_redo(){
	// Be sure to close an existing context (cusotm macro).
	h2app->endUndoContext();
	h2app->m_pUndoStack->redo();
}

void MainForm::updatePreferencesEvent( int nValue ) {
	if ( nValue == 0 ) {
		// Write the state of the GUI to the Preferences.
		saveWindowProperties();
		Preferences::get_instance()->save();
	}
	else if ( nValue == 1 ) {
		
		// Reflect the changes in the preferences in the objects
		// stored in MainForm.
		if ( Preferences::get_instance()->m_bPlaySelectedInstrument ) {
			m_pInstrumentAction->setChecked( true );
			m_pDrumkitAction->setChecked( false );
		}
		else {
			m_pInstrumentAction->setChecked( false );
			m_pDrumkitAction->setChecked( true );
		}

		updateRecentUsedSongList();

	} else {
		ERRORLOG( QString( "Unknown event parameter [%1] MainForm::updatePreferencesEvent" )
				  .arg( nValue ) );
	}
}

void MainForm::undoRedoActionEvent( int nEvent ){
	if( nEvent == 0 ) {
		h2app->m_pUndoStack->undo();
	} else if(nEvent == 1) {
		h2app->m_pUndoStack->redo();
	}
}

void MainForm::action_drumkit_properties() {
	editDrumkitProperties( false, false );
}

void MainForm::editDrumkitProperties( bool bWriteToDisk, bool bSaveToNsmSession,
									  int nInstrumentID )
{
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ){
		ERRORLOG("Invalid drumkit")
		return;
	}

	// We create a copy of the kit to assure not dirty data set in the dialog is
	// leaked into the current song.
	auto pNewDrumkit = std::make_shared<Drumkit>(pDrumkit);

	DrumkitPropertiesDialog dialog( nullptr, pNewDrumkit, ! bWriteToDisk,
									bSaveToNsmSession, nInstrumentID );
	dialog.exec();
}

void MainForm::updateSongEvent( int nValue ) {
	if ( nValue == 0 || nValue == 1 ) {
		// A new song was set.
		updateRecentUsedSongList();
	}
}

void MainForm::quitEvent( int ) {
	closeAll();
}

void MainForm::startPlaybackAtCursor( QObject* pObject ) {

	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	HydrogenApp* pHydrogenApp = HydrogenApp::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong == nullptr ) {
		return;
	}

	if ( pObject->inherits( "SongEditorPanel" ) ) {
			
		if ( pHydrogen->getMode() != Song::Mode::Song ) {
			H2Core::CoreActionController::activateSongMode( true );
		}

		const int nCursorColumn = pHydrogenApp->getSongEditorPanel()->
			getSongEditor()->getCursorPosition().getColumn();

		// Within the core locating to a position beyond the length of
		// the song with loop mode enabled is a valid
		// operation. The resulting location will the wrapped as if
		// transport was looped. This is important when allowing
		// external applications to relocate but it is not what we
		// want in here.
		if ( nCursorColumn >= pSong->getPatternGroupVector()->size() ) {
			ERRORLOG( QString( "Cursor column [%1] is outside of the current song [0,%2]" )
					  .arg( nCursorColumn )
					  .arg( pSong->getPatternGroupVector()->size() - 1 ) );
			return;
		}
		
		if ( ! H2Core::CoreActionController::locateToColumn( nCursorColumn ) ) {
			// Cursor is at a position it is not allowed to locate to.
			return;
		}
			
	} else if ( pObject->inherits( "PatternEditorPanel" ) ) {
		// Covers both the PatternEditor and the
		// NotePropertiesRuler.
			
		if ( pHydrogen->getMode() != Song::Mode::Pattern ) {
			H2Core::CoreActionController::activateSongMode( false );
		}

		// To provide a similar behaviour as when pressing
		// [backspace], transport is relocated to the beginning of
		// the song.
		const int nCursorColumn = pHydrogenApp->getPatternEditorPanel()->getCursorColumn();
		
		if ( ! H2Core::CoreActionController::locateToTick( nCursorColumn ) ) {
			// Cursor is at a position it is not allowed to locate to.
			return;
		}
	} else {
		ERRORLOG( QString( "Unknown object class" ) );
	}

	if ( pAudioEngine->getState() == H2Core::AudioEngine::State::Ready ||
		 pAudioEngine->getState() == H2Core::AudioEngine::State::CountIn ) {
		pHydrogen->sequencerPlay();
	}
}

bool MainForm::switchDrumkit( std::shared_ptr<H2Core::Drumkit> pTargetKit ) {

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	if ( pTargetKit == nullptr ) {
		ERRORLOG( "Invalid target kit" );
		return false;
	}

	pTargetKit->setContext( Drumkit::Context::Song );

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	pHydrogenApp->pushUndoCommand(
		new SE_switchDrumkitAction(
			pTargetKit, pSong->getDrumkit(),
			SE_switchDrumkitAction::Type::SwitchDrumkit ) );

	pHydrogenApp->showStatusBarMessage(
		QString( "%1 [%2]" ).arg( pCommonStrings->getActionLoadDrumkit() )
		.arg( pTargetKit->getName() ) );

	return true;
}

bool MainForm::handleKeyEvent( QObject* pQObject, QKeyEvent* pKeyEvent ) {
	
	const auto pPref = Preferences::get_instance();
	auto pShortcuts = pPref->getShortcuts();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pMidiActionManager = pHydrogen->getMidiActionManager();
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();
	const auto pSoundLibraryDataBase = pHydrogen->getSoundLibraryDatabase();

	if ( pSong == nullptr ) {
		ERRORLOG( "no song" );
		return false;
	}

	int nKey = pKeyEvent->key();
	const Qt::KeyboardModifiers modifiers = pKeyEvent->modifiers();
    if ( modifiers & Qt::ShiftModifier ) {
        nKey += Qt::SHIFT;
	}
    if ( modifiers & Qt::ControlModifier ) {
        nKey += Qt::CTRL;
	}
    if ( modifiers & Qt::AltModifier ) {
        nKey += Qt::ALT;
	}
    if ( modifiers & Qt::MetaModifier ) {
        nKey += Qt::META;
	}
	const auto keySequence = QKeySequence( nKey );
	if ( keySequence == QKeySequence( "" ) ) {
		return false;
	}
	
	const auto actions = pShortcuts->getActions( keySequence );
	const auto actionInfoMap = pShortcuts->getActionInfoMap();
	for ( const auto& action : actions ) {

		if ( actionInfoMap.find( action ) == actionInfoMap.end() ) {
			continue;
		}

		const QString sTitle = actionInfoMap.at( action ).sDescription;
		
		if ( static_cast<int>(action) >= static_cast<int>(Shortcuts::Action::VK_36_C2) &&
			 static_cast<int>(action) <= static_cast<int>(Shortcuts::Action::VK_59_B3) ) {
			// Virtual keyboard

			CoreActionController::handleNote(
				static_cast<int>(action) - 400 + MidiMessage::instrumentOffset,
				VELOCITY_DEFAULT, false );
		}
		else if ( static_cast<int>(action) >
				  static_cast<int>(Shortcuts::Action::FirstWith1Args) &&
				  static_cast<int>(action) <
				  static_cast<int>(Shortcuts::Action::LastWith1Args) ) {
			// Core actions with a single input argument
			
			auto inputType = InputCaptureDialog::Type::IntMidi;
			float fMax = 1;
			float fMin = 0;
			QString sLabel;
			auto midiActionType = MidiAction::Type::Null;
			
			switch ( action ) {
			case Shortcuts::Action::BPM:
				inputType = InputCaptureDialog::Type::Float;
				sLabel = pCommonStrings->getInputCaptureBpm();
				fMin = static_cast<float>(MIN_BPM);
				fMax = static_cast<float>(MAX_BPM);
				break;

			case Shortcuts::Action::MasterVolume:
				midiActionType = MidiAction::Type::MasterVolumeAbsolute;
				sLabel = pCommonStrings->getInputCaptureVolume();
				break;

			case Shortcuts::Action::JumpToBar:
				inputType = InputCaptureDialog::Type::Int;
				sLabel = pCommonStrings->getInputCaptureColumn();
				fMax = static_cast<float>(pSong->getPatternGroupVector()->size()) - 1;
				break;
				
			case Shortcuts::Action::SelectNextPattern:
			case Shortcuts::Action::SelectOnlyNextPattern:
			case Shortcuts::Action::SelectAndPlayPattern:
				if ( action == Shortcuts::Action::SelectNextPattern ) {
					midiActionType = MidiAction::Type::SelectNextPattern;
				}
				else if ( action == Shortcuts::Action::SelectOnlyNextPattern ) {
					midiActionType = MidiAction::Type::SelectOnlyNextPattern;
				}
				else if ( action == Shortcuts::Action::SelectAndPlayPattern ) {
					midiActionType = MidiAction::Type::SelectAndPlayPattern;
				}
				inputType = InputCaptureDialog::Type::Int;
				sLabel = pCommonStrings->getInputCapturePattern();
				fMax = static_cast<float>(pSong->getPatternList()->size()) - 1;
				break;

			case Shortcuts::Action::PlaylistSong:
				midiActionType = MidiAction::Type::PlaylistSong;
				inputType = InputCaptureDialog::Type::Int;
				sLabel = pCommonStrings->getInputCapturePattern();
				fMax = static_cast<float>(pHydrogen->getPlaylist()->size()) - 1;
				break;

			case Shortcuts::Action::TimelineDeleteMarker:
			case Shortcuts::Action::TimelineDeleteTag:
				inputType = InputCaptureDialog::Type::Int;
				sLabel = pCommonStrings->getInputCaptureColumn();
				fMax = static_cast<float>(pPref->getMaxBars()) - 1;
				break;
				
			case Shortcuts::Action::SelectInstrument:
			case Shortcuts::Action::StripVolumeIncrease:
			case Shortcuts::Action::StripVolumeDecrease:
			case Shortcuts::Action::StripMuteToggle:
			case Shortcuts::Action::StripSoloToggle:
				if ( action == Shortcuts::Action::SelectInstrument ) {
					midiActionType = MidiAction::Type::SelectInstrument;
				}
				else if ( action == Shortcuts::Action::StripMuteToggle ) {
					midiActionType = MidiAction::Type::StripMuteToggle;
				}
				else if ( action == Shortcuts::Action::StripSoloToggle ) {
					midiActionType = MidiAction::Type::StripSoloToggle;
				}
				else {
					midiActionType = MidiAction::Type::StripVolumeRelative;
				}
				inputType = InputCaptureDialog::Type::Int;
				sLabel = pCommonStrings->getInputCaptureInstrument();
				fMax = static_cast<float>(pSong->getDrumkit()->getInstruments()->size()) - 1;
				break;
			default:
				WARNINGLOG( QString( "Action [%1] not properly prepared" )
							.arg( static_cast<int>(action) ) );
			}

			auto pInputCaptureDialog =
				new InputCaptureDialog( this, sTitle, sLabel, inputType, fMin, fMax );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sArg = pInputCaptureDialog->text();
			delete pInputCaptureDialog;

			switch ( action ) {
			case Shortcuts::Action::BPM:
				H2Core::CoreActionController::setBpm( sArg.toFloat() );
				break;
			case Shortcuts::Action::JumpToBar:
				H2Core::CoreActionController::locateToColumn( sArg.toInt() );
				break;
			case Shortcuts::Action::SelectInstrument:
			case Shortcuts::Action::MasterVolume: {
				auto pAction = std::make_shared<MidiAction>( midiActionType );
				pAction->setValue( sArg );
				pMidiActionManager->handleMidiActionSync( pAction );
				break;
			}
				
			case Shortcuts::Action::SelectNextPattern:
			case Shortcuts::Action::SelectOnlyNextPattern:
			case Shortcuts::Action::SelectAndPlayPattern:
			case Shortcuts::Action::PlaylistSong:
			case Shortcuts::Action::StripVolumeIncrease:
			case Shortcuts::Action::StripVolumeDecrease:
			case Shortcuts::Action::StripMuteToggle:
			case Shortcuts::Action::StripSoloToggle: {
				auto pAction = std::make_shared<MidiAction>( midiActionType );
				pAction->setParameter1( sArg );
				if ( action == Shortcuts::Action::StripVolumeIncrease ) {
					pAction->setValue( QString::number( 1 ) );
				} else if ( action == Shortcuts::Action::StripVolumeDecrease ) {
					pAction->setValue( QString::number( -1 ) );
				}
				pMidiActionManager->handleMidiActionSync( pAction );
				break;
			}

			case Shortcuts::Action::TimelineDeleteMarker:
				H2Core::CoreActionController::deleteTempoMarker( sArg.toInt() );
				break;
			case Shortcuts::Action::TimelineDeleteTag:
				H2Core::CoreActionController::deleteTag( sArg.toInt() );
				break;
			default:
				WARNINGLOG( QString( "Action [%1] not properly handled" )
							.arg( static_cast<int>(action) ) );
			}
		}
		else if ( static_cast<int>(action) >
				  static_cast<int>(Shortcuts::Action::FirstWith2Args) &&
				  static_cast<int>(action) <
				  static_cast<int>(Shortcuts::Action::LastWith2Args) ) {
			// Core actions with two input arguments
			
			auto inputType1 = InputCaptureDialog::Type::IntMidi;
			auto inputType2 = InputCaptureDialog::Type::IntMidi;
			float fMax1 = 1;
			float fMin1 = 0;
			float fMax2 = 1;
			float fMin2 = 0;
			QString sLabel1, sLabel2;
			auto midiActionType = MidiAction::Type::Null;

			switch ( action ) {
			case Shortcuts::Action::StripVolume:
			case Shortcuts::Action::StripPan:
			case Shortcuts::Action::StripFilterCutoff:
				if ( action == Shortcuts::Action::StripVolume ) {
					midiActionType = MidiAction::Type::StripVolumeAbsolute;
					sLabel1 = pCommonStrings->getInputCaptureVolume();
				}
				else if ( action == Shortcuts::Action::StripPan ) {
					midiActionType = MidiAction::Type::PanAbsolute;
					sLabel1 = pCommonStrings->getNotePropertyPan();
				}
				else if ( action == Shortcuts::Action::StripFilterCutoff ) {
					midiActionType = MidiAction::Type::FilterCutoffLevelAbsolute;
					sLabel1 = pCommonStrings->getInputCaptureFilterCutoff();
				}
				inputType2 = InputCaptureDialog::Type::Int;
				sLabel2 = pCommonStrings->getInputCaptureInstrument();
				fMax2 = static_cast<float>(pSong->getDrumkit()->getInstruments()->size()) - 1;
				break;

			case Shortcuts::Action::TimelineAddMarker:
			case Shortcuts::Action::TimelineAddTag:
			case Shortcuts::Action::ToggleGridCell:
				inputType1 = InputCaptureDialog::Type::Int;
				sLabel1 = pCommonStrings->getInputCaptureColumn();
				fMax1 = static_cast<float>(pSong->getPatternGroupVector()->size()) - 1;

				if ( action == Shortcuts::Action::TimelineAddTag ) {
					inputType2 = InputCaptureDialog::Type::String;
					sLabel2 = pCommonStrings->getInputCaptureTag();
				}
				else if ( action == Shortcuts::Action::TimelineAddMarker ) {
					inputType2 = InputCaptureDialog::Type::Float;
					sLabel2 = pCommonStrings->getInputCaptureBpm();
					fMin2 = static_cast<float>(MIN_BPM);
					fMax2 = static_cast<float>(MAX_BPM);
				}
				else if ( action == Shortcuts::Action::ToggleGridCell ) {
					inputType2 = InputCaptureDialog::Type::Int;
					sLabel2 = pCommonStrings->getInputCapturePattern();
					fMax2 = static_cast<float>(pSong->getPatternList()->size()) - 1;
				}
				break;
			default:
				WARNINGLOG( QString( "Action [%1] not properly prepared" )
							.arg( static_cast<int>(action) ) );
			}

			// Capture arguments
			auto pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, sLabel1, inputType1, fMin1, fMax1 );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sArg1 = pInputCaptureDialog->text();
			delete pInputCaptureDialog;
			
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, sLabel2, inputType2, fMin2, fMax2 );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sArg2 = pInputCaptureDialog->text();
			delete pInputCaptureDialog;

			switch ( action ) {
			case Shortcuts::Action::StripVolume:
			case Shortcuts::Action::StripPan:
			case Shortcuts::Action::StripFilterCutoff: {
				auto pAction = std::make_shared<MidiAction>( midiActionType );
				pAction->setValue( sArg1 );
				pAction->setParameter1( sArg2 );
				pMidiActionManager->handleMidiActionSync( pAction );
				break;
			}

			case Shortcuts::Action::TimelineAddMarker:
				H2Core::CoreActionController::addTempoMarker(
					sArg1.toInt(), sArg2.toFloat() );
				break;
			case Shortcuts::Action::TimelineAddTag:
				H2Core::CoreActionController::addTag( sArg1.toInt(), sArg2 );
				break;
			case Shortcuts::Action::ToggleGridCell:
				H2Core::CoreActionController::toggleGridCell(
					GridPoint( sArg1.toInt(), sArg2.toInt() ) );
				break;
			default:
				WARNINGLOG( QString( "Action [%1] not properly handled" )
							.arg( static_cast<int>(action) ) );
			}
		}
		else if ( action == Shortcuts::Action::StripEffectLevel ) {
			// Core actions with three input arguments

			// Capture new parameter value
			auto pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureFXLevel(),
				InputCaptureDialog::Type::IntMidi );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sValue = pInputCaptureDialog->text();
			delete pInputCaptureDialog;

			// Capture instrument number
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureInstrument(),
				InputCaptureDialog::Type::Int, 0,
				static_cast<float>(pSong->getDrumkit()->getInstruments()->size()) - 1 );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const int nInstrument = pInputCaptureDialog->text().toInt();
			delete pInputCaptureDialog;
			auto pInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrument );
			if ( pInstrument == nullptr ) {
				ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
						  .arg( nInstrument ) );
				return true;
			}

			// Capture FX number
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureFXNumber(),
				InputCaptureDialog::Type::Int, 0, static_cast<float>(MAX_FX) - 1);
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sFX = pInputCaptureDialog->text();
			delete pInputCaptureDialog;
			
			// Deploy action
			auto pAction = std::make_shared<MidiAction>(
				MidiAction::Type::EffectLevelAbsolute );
			pAction->setValue( sValue );
			pAction->setParameter1( QString::number( nInstrument ) );
			pAction->setParameter2( sFX );
			pMidiActionManager->handleMidiActionSync( pAction );
		}
		else if ( action == Shortcuts::Action::LayerPitch ||
				  action == Shortcuts::Action::LayerGain ) {
			// Core actions with more than three input arguments
			
			QString sLabel;
			auto midiActionType = MidiAction::Type::Null;

			if ( action == Shortcuts::Action::LayerPitch ) {
				midiActionType = MidiAction::Type::PitchLevelAbsolute;
				sLabel = pCommonStrings->getPitchLabel();
			}
			else if ( action == Shortcuts::Action::LayerGain ) {
				midiActionType = MidiAction::Type::GainLevelAbsolute;
				sLabel = pCommonStrings->getGainLabel();
			}

			// Capture new parameter value
			auto pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, sLabel, InputCaptureDialog::Type::IntMidi );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const QString sValue = pInputCaptureDialog->text();
			delete pInputCaptureDialog;

			// Capture instrument number
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureInstrument(),
				InputCaptureDialog::Type::Int, 0,
				static_cast<float>(pSong->getDrumkit()->getInstruments()->size()) - 1 );
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const int nInstrument = pInputCaptureDialog->text().toInt();
			delete pInputCaptureDialog;
			auto pInstrument = pSong->getDrumkit()->getInstruments()->get( nInstrument );
			if ( pInstrument == nullptr ) {
				ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
						  .arg( nInstrument ) );
				return true;
			}

			// Capture component number
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureComponent(),
				InputCaptureDialog::Type::Int, 0,
				pInstrument->getComponents()->size() - 1);
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const int nComponent = pInputCaptureDialog->text().toInt();
			delete pInputCaptureDialog;
			auto pComponent = pInstrument->getComponents()->at( nComponent );
			if ( pComponent == nullptr ) {
				ERRORLOG( QString( "Unable to retrieve component [%1] of instrument [%2]" )
						  .arg( nComponent ).arg( nInstrument ) );
				return true;
			}

			// Capture layer number
			pInputCaptureDialog = new InputCaptureDialog(
				this, sTitle, pCommonStrings->getInputCaptureLayer(),
				InputCaptureDialog::Type::Int, 0,
				pComponent->getLayers().size() - 1);
			if ( pInputCaptureDialog->exec() == QDialog::Rejected ) {
				return true;
			}
			const int nLayer = pInputCaptureDialog->text().toInt();
			delete pInputCaptureDialog;
			auto pLayer = pComponent->getLayer( nLayer );
			if ( pLayer == nullptr ) {
				ERRORLOG( QString( "Unable to retrieve layer [%1] of component [%2] of instrument [%3]" )
						  .arg( nLayer ).arg( nComponent ).arg( nInstrument ) );
				return true;
			}

			// Deploy action
			auto pAction = std::make_shared<MidiAction>( midiActionType );
			pAction->setValue( sValue );
			pAction->setParameter1( QString::number( nInstrument ) );
			pAction->setParameter2( QString::number( nComponent ) );
			pAction->setParameter3( QString::number( nLayer )  );
			pMidiActionManager->handleMidiActionSync( pAction );
		}
		else {
			std::shared_ptr<MidiAction> pAction = nullptr;
			
			// Actions without input arguments
			switch ( action ) {
				
			case Shortcuts::Action::Panic:
				//panic button stop all playing notes
				pHydrogen->panic();
				break;

			case Shortcuts::Action::Play:
				pAction = std::make_shared<MidiAction>( MidiAction::Type::Play );
				break;
			case Shortcuts::Action::Pause:
				pAction = std::make_shared<MidiAction>( MidiAction::Type::Pause );
				break;
			case Shortcuts::Action::Stop:
				pAction = std::make_shared<MidiAction>( MidiAction::Type::Stop );
				break;
			case Shortcuts::Action::PlayPauseToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::PlayPauseToggle );
				break;
			case Shortcuts::Action::PlayStopToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::PlayStopToggle );
				break;
			case Shortcuts::Action::PlayPauseToggleAtCursor:
				if ( nullDriverCheck() ) {
					startPlaybackAtCursor( pQObject );
				}
				break;
				
			case Shortcuts::Action::RecordReady:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::RecordReady );
				break;
			case Shortcuts::Action::RecordStrobe:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::RecordStrobe );
				break;
			case Shortcuts::Action::RecordStrobeToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::RecordStrobeToggle );
				break;
			case Shortcuts::Action::RecordExit:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::RecordExit );
				break;

			case Shortcuts::Action::MasterMute:
				pAction = std::make_shared<MidiAction>( MidiAction::Type::Mute );
				break;
			case Shortcuts::Action::MasterUnmute:
				pAction = std::make_shared<MidiAction>( MidiAction::Type::Unmute );
				break;
			case Shortcuts::Action::MasterMuteToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::MuteToggle );
				break;
			case Shortcuts::Action::MasterVolumeIncrease:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::MasterVolumeRelative );
				pAction->setValue( QString::number( 1 ) );
				break;
			case Shortcuts::Action::MasterVolumeDecrease:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::MasterVolumeRelative );
				pAction->setValue( QString::number( -1 ) );
				break;

			case Shortcuts::Action::JumpToStart:
				H2Core::CoreActionController::locateToColumn( 0 );
				break;
			case Shortcuts::Action::JumpBarForward:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::NextBar );
				break;
			case Shortcuts::Action::JumpBarBackward:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::PreviousBar );
				break;

			case Shortcuts::Action::BPMIncreaseCoarse:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::BpmIncr );
				pAction->setParameter1( QString::number( 0.1 ) );
				break;
			case Shortcuts::Action::BPMDecreaseCoarse:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::BpmDecr );
				pAction->setParameter1( QString::number( 0.1 ) );
				break;
			case Shortcuts::Action::BPMIncreaseFine:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::BpmIncr );
				pAction->setParameter1( QString::number( 0.01 ) );
				break;
			case Shortcuts::Action::BPMDecreaseFine:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::BpmDecr );
				pAction->setParameter1( QString::number( 0.01 ) );
				break;

			case Shortcuts::Action::BeatCounter:
				pHydrogen->handleBeatCounter();
				break;

			case Shortcuts::Action::TapTempo:
				pHydrogen->onTapTempoAccelEvent();
				break;

			case Shortcuts::Action::PlaylistNextSong:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::PlaylistNextSong );
				break;
			case Shortcuts::Action::PlaylistPrevSong:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::PlaylistPrevSong );
				break;

			case Shortcuts::Action::TimelineToggle:
				H2Core::CoreActionController::toggleTimeline();
				break;
			case Shortcuts::Action::MetronomeToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::ToggleMetronome );
				break;
			case Shortcuts::Action::JackTransportToggle:
				H2Core::CoreActionController::toggleJackTransport();
				break;
			case Shortcuts::Action::JackTimebaseToggle:
				H2Core::CoreActionController::toggleJackTimebaseControl();
				break;
			case Shortcuts::Action::SongModeToggle:
				H2Core::CoreActionController::toggleSongMode();
				break;
			case Shortcuts::Action::LoopModeToggle:
				H2Core::CoreActionController::toggleLoopMode();
				break;

			case Shortcuts::Action::LoadNextDrumkit:
				// Pass copy to not alter the original kit.
				switchDrumkit( std::shared_ptr<Drumkit>(
								   pSoundLibraryDataBase->getNextDrumkit() ) );
				break;
			case Shortcuts::Action::LoadPrevDrumkit:
				// Pass copy to not alter the original kit.
				switchDrumkit( std::shared_ptr<Drumkit>(
								   pSoundLibraryDataBase->getPreviousDrumkit() ) );
				break;

			case Shortcuts::Action::CountIn:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::CountIn );
				break;
			case Shortcuts::Action::CountInPauseToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::CountInPauseToggle );
				break;
			case Shortcuts::Action::CountInStopToggle:
				pAction = std::make_shared<MidiAction>(
					MidiAction::Type::CountInStopToggle );
				break;

				//////////////////////////////////////////////////////
				// GUI Actions

			case Shortcuts::Action::NewSong:
				action_file_new();
				break;
			case Shortcuts::Action::OpenSong:
				action_file_open();
				break;
			case Shortcuts::Action::EditSongProperties:
				action_file_songProperties();
				break;
			case Shortcuts::Action::OpenDemoSong:
				action_file_openDemo();
				break;
			case Shortcuts::Action::SaveSong:
				action_file_save();
				break;
			case Shortcuts::Action::SaveAsSong:
				action_file_save_as();
				break;
			case Shortcuts::Action::OpenPattern:
				action_file_openPattern();
				break;
			case Shortcuts::Action::ExportPattern:
				action_file_export_pattern_as();
				break;
			case Shortcuts::Action::ExportSong:
				action_file_export();
				break;
			case Shortcuts::Action::ExportMIDI:
				action_file_export_midi();
				break;
			case Shortcuts::Action::ExportLilyPond:
				action_file_export_lilypond();
				break;
			case Shortcuts::Action::Quit:
				action_file_exit();
				break;

			case Shortcuts::Action::Undo:
				action_undo();
				break;
			case Shortcuts::Action::Redo:
				action_redo();
				break;
			case Shortcuts::Action::ShowUndoHistory:
				openUndoStack();
				break;

			case Shortcuts::Action::NewDrumkit:
				action_drumkit_new();
				break;
			case Shortcuts::Action::OpenDrumkit:
				action_drumkit_open();
				break;
			case Shortcuts::Action::EditDrumkitProperties:
				action_drumkit_properties();
				break;
			case Shortcuts::Action::SaveDrumkitToSoundLibrary:
				action_drumkit_save();
				break;
			case Shortcuts::Action::ExportDrumkit:
				action_drumkit_export();
				break;
			case Shortcuts::Action::ImportDrumkit:
				action_drumkit_import();
				break;
			case Shortcuts::Action::ImportOnlineDrumkit:
				action_drumkit_onlineImport();
				break;

			case Shortcuts::Action::AddInstrument:
				action_drumkit_addInstrument();
				break;
			case Shortcuts::Action::ClearAllInstruments:
				action_drumkit_new();
				break;
			case Shortcuts::Action::AddComponent:
				pHydrogenApp->getInstrumentRack()->getInstrumentEditorPanel()->
					getComponentsEditor()->addComponent();
				break;

			case Shortcuts::Action::ShowPlaylist:
				action_window_showPlaylistEditor();
				break;
			case Shortcuts::Action::ShowDirector:
				action_window_show_DirectorWidget();
				break;
			case Shortcuts::Action::ShowMixer:
				action_window_showMixer();
				break;
			case Shortcuts::Action::ShowInstrumentRack:
				action_window_showInstrumentRack();
				break;
			case Shortcuts::Action::ShowAutomation:
				action_window_showAutomationArea();
				break;
			case Shortcuts::Action::ShowTimeline:
				action_window_showTimeline();
				break;
			case Shortcuts::Action::ShowPlaybackTrack:
				action_window_showPlaybackTrack();
				break;
			case Shortcuts::Action::ShowFullscreen:
				action_window_toggleFullscreen();
				break;

			case Shortcuts::Action::InputInstrument:
				action_inputMode_instrument();
				break;
			case Shortcuts::Action::InputDrumkit:
				action_inputMode_drumkit();
				break;
			case Shortcuts::Action::ShowPreferencesDialog:
				showPreferencesDialog();
				break;

			case Shortcuts::Action::ShowAudioEngineInfo:
				action_debug_showAudioEngineInfo();
				break;
			case Shortcuts::Action::ShowFilesystemInfo:
				action_debug_showFilesystemInfo();
				break;
			case Shortcuts::Action::LogLevelNone:
				action_debug_logLevel_none();
				break;
			case Shortcuts::Action::LogLevelError:
				action_debug_logLevel_error();
				break;
			case Shortcuts::Action::LogLevelWarning:
				action_debug_logLevel_warn();
				break;
			case Shortcuts::Action::LogLevelInfo:
				action_debug_logLevel_info();
				break;
			case Shortcuts::Action::LogLevelDebug:
				action_debug_logLevel_debug();
				break;
			case Shortcuts::Action::OpenLogFile:
				action_debug_openLogfile();
				break;
			case Shortcuts::Action::DebugPrintObjects:
				action_debug_printObjects();
				break;

			case Shortcuts::Action::OpenManual:
				showUserManual();
				break;
			case Shortcuts::Action::ShowAbout:
				action_help_about();
				break;
			case Shortcuts::Action::ShowReportBug:
				action_report_bug();
				break;
			case Shortcuts::Action::ShowDonate:
				action_donate();
				break;

			// Playlist dialog related actions
			case Shortcuts::Action::PlaylistAddSong:
				pHydrogenApp->getPlaylistEditor()->addSong();
				break;
			case Shortcuts::Action::PlaylistAddCurrentSong:
				pHydrogenApp->getPlaylistEditor()->addCurrentSong();
				break;
			case Shortcuts::Action::PlaylistRemoveSong:
				pHydrogenApp->getPlaylistEditor()->removeSong();
				break;
			case Shortcuts::Action::NewPlaylist:
				pHydrogenApp->getPlaylistEditor()->newPlaylist();
				break;
			case Shortcuts::Action::OpenPlaylist:
				pHydrogenApp->getPlaylistEditor()->openPlaylist();
				break;
			case Shortcuts::Action::SavePlaylist:
				pHydrogenApp->getPlaylistEditor()->savePlaylist();
				break;
			case Shortcuts::Action::SaveAsPlaylist:
				pHydrogenApp->getPlaylistEditor()->savePlaylistAs();
				break;
			case Shortcuts::Action::PlaylistAddScript:
				pHydrogenApp->getPlaylistEditor()->loadScript();
				break;
			case Shortcuts::Action::PlaylistEditScript:
				pHydrogenApp->getPlaylistEditor()->editScript();
				break;
			case Shortcuts::Action::PlaylistRemoveScript:
				pHydrogenApp->getPlaylistEditor()->removeScript();
				break;
			case Shortcuts::Action::PlaylistCreateScript:
				pHydrogenApp->getPlaylistEditor()->newScript();
				break;

			default:
				WARNINGLOG( QString( "Action [%1] not properly handled" )
							.arg( static_cast<int>(action) ) );
			}

			if ( pAction != nullptr ) {
				pMidiActionManager->handleMidiActionSync( pAction );
			}
		}
	}

	if ( actions.size() > 0 ) {
		// Event consumed by the actions triggered above.
		pKeyEvent->accept();
		return true;
	}

	return false;
}
