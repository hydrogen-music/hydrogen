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

#include <core/EventQueue.h>
#include <core/Version.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/SMF/SMF.h>
#include <core/Timeline.h>
#include <core/Helpers/Files.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Playlist.h>
#include <core/Lilipond/Lilypond.h>
#include <core/IO/MidiCommon.h>
#include <core/NsmClient.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "AboutDialog.h"
#include "AudioEngineInfoForm.h"
#include "CommonStrings.h"
#include "ExportSongDialog.h"
#include "ExportMidiDialog.h"
#include "HydrogenApp.h"
#include "Skin.h"
#include "InstrumentRack.h"
#include "MainForm.h"
#include "PlayerControl.h"
#include "LadspaFXProperties.h"
#include "SongPropertiesDialog.h"
#include "UndoActions.h"
#include "Widgets/InfoBar.h"
#include "Widgets/FileDialog.h"

#include "Director.h"
#include "Mixer/Mixer.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SoundLibrary/SoundLibraryImportDialog.h"
#include "SoundLibrary/SoundLibraryOpenDialog.h"
#include "SoundLibrary/SoundLibraryExportDialog.h"
#include "SoundLibrary/SoundLibraryPropertiesDialog.h"
#include "PlaylistEditor/PlaylistDialog.h"

#include <QtGui>
#include <QtWidgets>

#ifndef WIN32
#include <sys/time.h>
#include <sys/socket.h>
#endif

#ifdef H2CORE_HAVE_LASH
#include <lash/lash.h>
#include <core/Lash/LashClient.h>
#endif

#include <memory>
#include <cassert>

using namespace H2Core;

int MainForm::sigusr1Fd[2];

MainForm::MainForm( QApplication * pQApplication, QString sSongFilename )
	: QMainWindow( nullptr )
	, m_sPreviousAutoSaveFilename( "" )
{
	auto pPref = H2Core::Preferences::get_instance();
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

	// When using the Non Session Management system, the new Song
	// will be loaded by the NSM client singleton itself and not
	// by the MainForm. The latter will just access the already
	// loaded Song.
	if ( ! pHydrogen->isUnderSessionManagement() ){
		std::shared_ptr<H2Core::Song>pSong = nullptr;

		if ( sSongFilename.isEmpty() ) {
			if ( pPref->isRestoreLastSongEnabled() ) {
				sSongFilename = pPref->getLastSongFilename();
			}
		}

		bool bRet = false;
		if ( !sSongFilename.isEmpty() ) {
			if ( sSongFilename == H2Core::Filesystem::empty_song_path() ) {
				bRet = HydrogenApp::recoverEmptySong();
			} else {
				bRet = HydrogenApp::openSong( sSongFilename );
			}
		}

		if ( sSongFilename.isEmpty() || ! bRet ) {
			pSong = H2Core::Song::getEmptySong();
			HydrogenApp::openSong( pSong );
		}
	}

	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	setFont( font );
	m_pQApp->setFont( font );

	h2app = new HydrogenApp( this );
	showDevelWarning();
	h2app->addEventListener( this );
	createMenuBar();
	checkMidiSetup();
	checkMissingSamples();
	checkNecessaryDirectories();

	h2app->showStatusBarMessage( tr("Hydrogen Ready.") );

	initKeyInstMap();

	// we need to do all this to support the keyboard playing
	// for all the window modes
	h2app->getMixer()->installEventFilter (this);
	h2app->getPatternEditorPanel()->installEventFilter (this);
	h2app->getSongEditorPanel()->installEventFilter (this);
	h2app->getPlayerControl()->installEventFilter(this);
	InstrumentEditorPanel::get_instance()->installEventFilter(this);
	h2app->getAudioEngineInfoForm()->installEventFilter(this);
	h2app->getDirector()->installEventFilter(this);
	//	h2app->getPlayListDialog()->installEventFilter(this);
	installEventFilter( this );

	connect( &m_AutosaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimer()));
	startAutosaveTimer();

#ifdef H2CORE_HAVE_LASH

	if ( pPref->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient->isConnected())
		{
			// send alsa client id now since it can only be sent
			// after the audio engine has been started.
			if ( pPref->m_sMidiDriver == "ALSA" ) {
				//			infoLog("[LASH] Sending alsa seq id to LASH server");
				lashClient->sendAlsaClientId();
			}
			// start timer for polling lash events
			lashPollTimer = new QTimer(this);
			connect( lashPollTimer, SIGNAL( timeout() ), this, SLOT( onLashPollTimer() ) );
			lashPollTimer->start(500);
		}
	}
#endif

	//playlist display timer
	QTimer *playlistDisplayTimer = new QTimer(this);
	connect( playlistDisplayTimer, SIGNAL( timeout() ), this, SLOT( onPlaylistDisplayTimer() ) );
	playlistDisplayTimer->start(30000);	// update player control at
	// ~ playlist display timer

	//beatcouter
	pHydrogen->setBcOffsetAdjust();

	m_pUndoView = new QUndoView(h2app->m_pUndoStack);
	m_pUndoView->setWindowTitle(tr("Undo history"));

	//restore last playlist
	if ( pPref->isRestoreLastPlaylistEnabled() &&
		 ! pPref->getLastPlaylistFilename().isEmpty() ) {
		bool bLoadSuccessful = h2app->getPlayListDialog()->loadListByFileName(
			pPref->getLastPlaylistFilename() );
		if ( bLoadSuccessful ) {
			if ( pPref->getPlaylistDialogProperties().visible ){
				// If there was a playlist used during the last
				// session and it was still visible/shown when closing
				// Hydrogen, bring it up again.
				action_window_showPlaylistDialog();
			}
		}
		else {
			_ERRORLOG( QString( "Unable to load last playlist [%1]" )
					   .arg( pPref->getLastPlaylistFilename() ) );
		}
	}

	// Must be done _after_ the creation of the HydrogenApp instance.
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
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
	
	// Remove the autosave file in case all modifications already have
	// been written to disk.
	if ( ! pHydrogen->getSong()->getIsModified() ) {
		QFile file( getAutoSaveFilename() );
		file.remove();
	}

	if ( (Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}
	
	hide();

	delete m_pUndoView;

	if (h2app != nullptr) {
		delete Playlist::get_instance();
		delete h2app;
		h2app = nullptr;
	}

}

///
/// Create the menubar
///
void MainForm::createMenuBar()
{
	// menubar
	QMenuBar *pMenubar = new QMenuBar( this );
	pMenubar->setObjectName( "MainMenu" );
	setMenuBar( pMenubar );

	// FILE menu
	m_pFileMenu = pMenubar->addMenu( tr( "Pro&ject" ) );

	// Then under session management a couple of options will be named
	// differently and some must be even omitted. 
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
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
		/*: When Hydrogen is under session management the path the
		song is stored to can not be changed by the user. This option
		allows the user store the current song in a .h2song anywhere
		on her system. The filepath of the current song won't be
		altered.*/
		sLabelSaveAs = tr( "Export From Session &As..." );
	} else {
		sLabelNew = tr( "&New" );
		sLabelOpen = tr( "&Open" );
		sLabelOpenRecent = tr( "Open &Recent" );
		sLabelSaveAs = tr( "Save &As..." );
		sLabelOpenDemo = tr( "Open &Demo" );
	}
	
	auto pActionFileNew = m_pFileMenu->addAction(
		sLabelNew, this, SLOT( action_file_new() ) );
	pActionFileNew->setShortcut( QKeySequence( "Ctrl+N" ) );
	
	m_pFileMenu->addSeparator();				// -----
	
	m_pFileMenu->addAction( tr( "Song Properties" ), this, SLOT( action_file_songProperties() ) );
	
	m_pFileMenu->addSeparator();				// -----

	auto pActionFileOpen = m_pFileMenu->addAction(
		sLabelOpen, this, SLOT( action_file_open() ) );
	pActionFileOpen->setShortcut( QKeySequence( "Ctrl+O" ) );
	  if ( ! bUnderSessionManagement ) {
		auto pActionFileDemo = m_pFileMenu->addAction(
			sLabelOpenDemo, this, SLOT( action_file_openDemo() ) );
		pActionFileDemo->setShortcut( QKeySequence( "Ctrl+D" ) );
	}
	m_pRecentFilesMenu = m_pFileMenu->addMenu( sLabelOpenRecent );

	m_pFileMenu->addSeparator();				// -----

	auto pActionFileSave = m_pFileMenu->addAction(
		tr( "&Save" ), this, SLOT( action_file_save() ) );
	pActionFileSave->setShortcut( QKeySequence( "Ctrl+S" ) );
	auto pActionFileSaveAs = m_pFileMenu->addAction(
		sLabelSaveAs, this, SLOT( action_file_save_as() ) );
	pActionFileSaveAs->setShortcut( QKeySequence( "Ctrl+Shift+S" ) );
	
	m_pFileMenu->addSeparator();				// -----

	auto pActionOpenPattern = m_pFileMenu->addAction (
		tr( "Open &Pattern" ), this, SLOT( action_file_openPattern() ) );
	pActionOpenPattern->setShortcut( QKeySequence( "Ctrl+Shift+P" ) );
	auto pActionExportPattern = m_pFileMenu->addAction(
		tr( "E&xport Pattern As..." ), this, SLOT( action_file_export_pattern_as() ) );
	pActionExportPattern->setShortcut( QKeySequence( "Ctrl+P" ) );

	m_pFileMenu->addSeparator();				// -----

	auto pActionExportMidi = m_pFileMenu->addAction(
		tr( "Export &MIDI File" ), this, SLOT( action_file_export_midi() ) );
	pActionExportMidi->setShortcut( QKeySequence( "Ctrl+M" ) );
	auto pActionExportSong = m_pFileMenu->addAction(
		tr( "&Export Song" ), this, SLOT( action_file_export() ) );
	pActionExportSong->setShortcut( QKeySequence( "Ctrl+E" ) );
	auto pActionExportLilyPond = m_pFileMenu->addAction(
		tr( "Export &LilyPond File" ), this, SLOT( action_file_export_lilypond() ) );
	pActionExportLilyPond->setShortcut( QKeySequence( "Ctrl+L" ) );


#ifndef Q_OS_MACX
	m_pFileMenu->addSeparator();				// -----

	auto pActionQuit = m_pFileMenu->addAction(
		tr("&Quit"), this, SLOT( action_file_exit() ) );
	pActionQuit->setShortcut( QKeySequence( "Ctrl+Q" ) );
#endif

	updateRecentUsedSongList();
	connect( m_pRecentFilesMenu, SIGNAL( triggered(QAction*) ), this, SLOT( action_file_open_recent(QAction*) ) );
	// ~ FILE menu

	// Undo menu
	m_pUndoMenu = pMenubar->addMenu( tr( "&Undo" ) );
	auto pActionUndo = m_pUndoMenu->addAction(
		tr( "&Undo" ), this, SLOT( action_undo() ) );
	pActionUndo->setShortcut( QKeySequence( "Ctrl+Z" ) );
	auto pActionRedo = m_pUndoMenu->addAction(
		tr( "&Redo" ), this, SLOT( action_redo() ) );
	pActionRedo->setShortcut( QKeySequence( "Shift+Ctrl+Z" ) );
	m_pUndoMenu->addAction( tr( "Undo &History" ), this, SLOT( openUndoStack() ) );

	// DRUMKITS MENU
	m_pDrumkitsMenu = pMenubar->addMenu( tr( "Drum&kits" ) );
	m_pDrumkitsMenu->addAction( tr( "&New" ), this, SLOT( action_instruments_clearAll() ) );
	m_pDrumkitsMenu->addAction( tr( "&Open" ), this, SLOT( action_banks_open() ) );
	m_pDrumkitsMenu->addAction( tr( "&Properties" ), this, SLOT( action_banks_properties() ) );

	m_pDrumkitsMenu->addSeparator();				// -----

	m_pDrumkitsMenu->addAction( tr( "&Save" ), this, SLOT( action_instruments_saveLibrary() ) );
	m_pDrumkitsMenu->addAction( tr( "Save &As" ), this, SLOT( action_instruments_saveAsLibrary() ) );

	m_pDrumkitsMenu->addSeparator();				// -----

	m_pDrumkitsMenu->addAction( tr( "&Export" ), this, SLOT( action_instruments_exportLibrary() ) );
	m_pDrumkitsMenu->addAction( tr( "&Import" ), this, SLOT( action_instruments_importLibrary() ) );
	m_pDrumkitsMenu->addAction( tr( "On&line Import" ), this, SLOT( action_instruments_onlineImportLibrary() ) );

	// INSTRUMENTS MENU
	m_pInstrumentsMenu = pMenubar->addMenu( tr( "In&struments" ) );
	m_pInstrumentsMenu->addAction( tr( "Add &Instrument" ), this, SLOT( action_instruments_addInstrument() ) );
	m_pInstrumentsMenu->addAction( tr( "Clea&r All" ), this, SLOT( action_instruments_clearAll() ) );

	m_pInstrumentsMenu->addSeparator();				// -----

	m_pInstrumentsMenu->addAction( tr( "Add &Component" ), this, SLOT( action_instruments_addComponent() ) );

	// VIEW MENU
	m_pViewMenu = pMenubar->addMenu( tr( "&View" ) );

	m_pViewPlaylistEditorAction = m_pViewMenu->addAction( tr("Play&list Editor"), this, SLOT( action_window_showPlaylistDialog() ) );
	m_pViewPlaylistEditorAction->setCheckable( true );
	m_pViewDirectorAction = m_pViewMenu->addAction(
		tr("&Director"), this, SLOT( action_window_show_DirectorWidget() ) );
	m_pViewDirectorAction->setShortcut( QKeySequence( "Alt+D" ) );
	m_pViewDirectorAction->setCheckable( true );

	m_pFileMenu->addSeparator();
	m_pViewMixerAction = m_pViewMenu->addAction(
		tr("&Mixer"), this, SLOT( action_window_showMixer() ) );
	m_pViewMixerAction->setShortcut( QKeySequence( "Alt+M" ) );
	m_pViewMixerAction->setCheckable( true );
	update_mixer_checkbox();						// if checkbox need to be checked.

	m_pViewMixerInstrumentRackAction = m_pViewMenu->addAction(
		tr("&Instrument Rack"), this, SLOT( action_window_showInstrumentRack() ) );
	m_pViewMixerInstrumentRackAction->setShortcut( QKeySequence( "Alt+I" ) );
	m_pViewMixerInstrumentRackAction->setCheckable( true );
	update_instrument_checkbox( Preferences::get_instance()->getInstrumentRackProperties().visible );

	m_pViewAutomationPathAction = m_pViewMenu->addAction(
		tr("&Automation Path"), this, SLOT( action_window_showAutomationArea() ) );
	m_pViewAutomationPathAction->setShortcut( QKeySequence( "Alt+A" ) );
	m_pViewAutomationPathAction->setCheckable( true );
	update_automation_checkbox();

	m_pViewMenu->addSeparator();				// -----

	m_pViewTimelineAction = m_pViewMenu->addAction( tr("&Timeline"), this, SLOT( action_window_showTimeline() ) );
	m_pViewTimelineAction->setCheckable( true );
	
	m_pViewPlaybackTrackAction = m_pViewMenu->addAction( tr("&Playback Track"), this, SLOT( action_window_showPlaybackTrack() ) );
	m_pViewPlaybackTrackAction->setCheckable( true );

	m_pViewPlaybackTrackActionGroup = new QActionGroup( this );
	m_pViewPlaybackTrackActionGroup->addAction( m_pViewTimelineAction );
	m_pViewPlaybackTrackActionGroup->addAction( m_pViewPlaybackTrackAction );
	update_playback_track_group();

	m_pViewMenu->addSeparator();				// -----

	auto pActionFullScreen = m_pViewMenu->addAction(
		tr("&Full screen"), this, SLOT( action_window_toggleFullscreen() ) );
	pActionFullScreen->setShortcut( QKeySequence( "Alt+F" ) );


	// Options menu
	m_pOptionsMenu = pMenubar->addMenu( tr( "&Options" ));

	m_pInputModeMenu = m_pOptionsMenu->addMenu( tr( "Input &Mode" ) );
	m_pInstrumentAction = m_pInputModeMenu->addAction(
		tr( "&Instrument" ), this, SLOT( action_inputMode_instrument() ) );
	m_pInstrumentAction->setShortcut( QKeySequence( "Ctrl+Alt+I" ) );
	m_pInstrumentAction->setCheckable( true );

	m_pDrumkitAction = m_pInputModeMenu->addAction(
		tr( "&Drumkit" ), this, SLOT( action_inputMode_drumkit() ) );
	m_pDrumkitAction->setShortcut( QKeySequence( "Ctrl+Alt+D" ) );
	m_pDrumkitAction->setCheckable( true );

	if( Preferences::get_instance()->__playselectedinstrument )
	{
		m_pInstrumentAction->setChecked( true );
		m_pDrumkitAction->setChecked (false );
	} else {
		m_pInstrumentAction->setChecked( false );
		m_pDrumkitAction->setChecked (true );
	}

	auto pActionPreferences = m_pOptionsMenu->addAction(
		tr("&Preferences"), this, SLOT( showPreferencesDialog() ) );
	pActionPreferences->setShortcut( QKeySequence( "Alt+P" ) );

	// ~ Tools menu


	Logger *pLogger = Logger::get_instance();
	if ( pLogger->bit_mask() >= 1 ) {
		// DEBUG menu
		m_pDebugMenu = pMenubar->addMenu( tr("De&bug") );
		m_pDebugMenu->addAction( tr( "Show &Audio Engine Info" ), this, SLOT( action_debug_showAudioEngineInfo() ) );
		m_pDebugMenu->addAction( tr( "Show &Filesystem Info" ), this, SLOT( action_debug_showFilesystemInfo() ) );
		
		m_pLogLevelMenu = m_pDebugMenu->addMenu( tr( "&Log Level" ) );		
		m_pLogLevelMenu->addAction( tr( "&None" ), this, SLOT( action_debug_logLevel_none() ) );
		m_pLogLevelMenu->addAction( tr( "&Error" ), this, SLOT( action_debug_logLevel_info() ) );
		m_pLogLevelMenu->addAction( tr( "&Warning" ), this, SLOT( action_debug_logLevel_warn() ) );
		m_pLogLevelMenu->addAction( tr( "&Info" ), this, SLOT( action_debug_logLevel_info() ) );
		m_pLogLevelMenu->addAction( tr( "&Debug" ), this, SLOT( action_debug_logLevel_debug() ) );
		
		m_pDebugMenu->addAction( tr( "&Open Log File" ), this, SLOT( action_debug_openLogfile()) );
		
		if(pLogger->bit_mask() == 8) { // hydrogen -V8 list object map in console 
			m_pDebugMenu->addAction( tr( "&Print Objects" ), this, SLOT( action_debug_printObjects() ) );
		}
		// ~ DEBUG menu
	}

	// INFO menu
	m_pInfoMenu = pMenubar->addMenu( tr( "I&nfo" ) );
	auto pActionUserManual = m_pInfoMenu->addAction(
		tr("User &Manual"), this, SLOT( showUserManual() ) );
	pActionUserManual->setShortcut( QKeySequence( "Ctrl+?" ) );
	m_pInfoMenu->addSeparator();
	auto pActionAbout = m_pInfoMenu->addAction( tr("&About"), this, SLOT( action_help_about() ) );
	pActionAbout->setShortcut( QKeySequence( tr("", "Info|About") ) );
	m_pInfoMenu->addAction( tr("&Report Bug"), this, SLOT( action_report_bug() ));
	m_pInfoMenu->addAction( tr("&Donate"), this, SLOT( action_donate() ));
	// ~ INFO menu
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
	} else {
		DEBUGLOG( "Autosave disabled" );
	}
}

void MainForm::onLashPollTimer()
{
#ifdef H2CORE_HAVE_LASH
	if ( Preferences::get_instance()->useLash() ){
		LashClient* client = LashClient::get_instance();

		if (!client->isConnected())
		{
			WARNINGLOG("[LASH] Not connected to server!");
			return;
		}

		bool keep_running = true;

		lash_event_t* event;

		std::string songFilename;
		QString filenameSong;
		std::shared_ptr<Song> song = Hydrogen::get_instance()->getSong();
		// Extra parentheses for -Wparentheses
		while ( (event = client->getNextEvent()) ) {

			switch (lash_event_get_type(event)) {

			case LASH_Save_File:

				INFOLOG("[LASH] Save file");

				songFilename.append(lash_event_get_string(event));
				songFilename.append("/hydrogen.h2song");

				filenameSong = QString::fromLocal8Bit( songFilename.c_str() );
				song->setFilename( filenameSong );
				action_file_save();

				client->sendEvent(LASH_Save_File);

				break;

			case LASH_Restore_File:

				songFilename.append(lash_event_get_string(event));
				songFilename.append("/hydrogen.h2song");

				INFOLOG( QString("[LASH] Restore file: %1")
						 .arg( songFilename.c_str() ) );

				filenameSong = QString::fromLocal8Bit( songFilename.c_str() );

				HydrogenApp::get_instance()->openSong( filenameSong );

				client->sendEvent(LASH_Restore_File);

				break;

			case LASH_Quit:

				//				infoLog("[LASH] Quit!");
				keep_running = false;

				break;

			default:
				;
				//				infoLog("[LASH] Got unknown event!");

			}

			lash_event_destroy(event);

		}

		if (!keep_running)
		{
			lashPollTimer->stop();
			action_file_exit();
		}
	}
#endif
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

/// return true if the app needs to be closed.
bool MainForm::action_file_exit()
{
	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return false;
	}
	closeAll();
	return true;
}



void MainForm::action_file_new()
{
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		pHydrogen->sequencer_stop();
	}

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
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
	QFileInfo fileInfo( Filesystem::empty_song_path() );
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}
	QFileInfo autoSaveFile( QString( "%1/.%2.autosave.h2song" )
							.arg( fileInfo.absoluteDir().absolutePath() )
							.arg( sBaseName ) );
	if ( autoSaveFile.exists() ) {
		Filesystem::rm( autoSaveFile.absoluteFilePath() );
	}
	
	h2app->openSong( pSong );
	
	// The drumkit of the new song will linked into the session
	// folder during the next song save.
	pHydrogen->setSessionDrumkitNeedsRelinking( true );
}



bool MainForm::action_file_save_as()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		return false;
	}

	const bool bUnderSessionManagement = pHydrogen->isUnderSessionManagement();
	if ( bUnderSessionManagement &&
		 pHydrogen->getSessionDrumkitNeedsRelinking() ) {
		// When used under session management "save as" will be used
		// for exporting and Hydrogen is allowed to
		// be in a transient state which is not ready for export. This
		// way the user is able to undo e.g. loading a drumkit by
		// closing the session without storing and the overall state
		// is not getting bricked during unexpected shut downs.
		//
		// We will prompt for saving the changes applied to the
		// drumkit usage and require the user to exit this transient
		// state first.
		if ( QMessageBox::information( this, "Hydrogen",
									   tr( "\nThere have been recent changes to the drumkit settings.\n"
										   "The session needs to be saved before exporting will can be continued.\n" ),
		                               QMessageBox::Save | QMessageBox::Cancel,
		                               QMessageBox::Save )
		     == QMessageBox::Cancel ) {
			INFOLOG( "Exporting cancelled at relinking" );
			return false;
		}

		if ( ! action_file_save() ) {
			ERRORLOG( "Unable to save file" );
			return false;
		}
	}

	QString sPath = Preferences::get_instance()->getLastSaveSongAsDirectory();
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
		fd.setWindowTitle( tr( "Save song" ) );
	}
	
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::songs_dir() ) );

	QString sDefaultFilename;

	// Cachce a couple of things we have to restore when under session
	// management.
	QString sLastFilename = pSong->getFilename();
	QString sLastLoadedDrumkitPath = pSong->getLastLoadedDrumkitPath();

	if ( sLastFilename == Filesystem::empty_song_path() ) {
		sDefaultFilename = Filesystem::default_song_name();
	} else if ( sLastFilename.isEmpty() ) {
		sDefaultFilename = pHydrogen->getSong()->getName();
	} else {
		QFileInfo fileInfo( sLastFilename );
		sDefaultFilename = fileInfo.completeBaseName();
	}
	sDefaultFilename += Filesystem::songs_ext;

	fd.selectFile( sDefaultFilename );

	if (fd.exec() == QDialog::Accepted) {
		QString sNewFilename = fd.selectedFiles().first();

		if ( ! sNewFilename.isEmpty() ) {
			Preferences::get_instance()->setLastSaveSongAsDirectory( fd.directory().absolutePath( ) );

			if ( ! sNewFilename.endsWith( Filesystem::songs_ext ) ) {
				sNewFilename += Filesystem::songs_ext;
			}

#ifdef H2CORE_HAVE_OSC
			// In a session all main samples (last loaded drumkit) are
			// taken from the session folder itself (either via a
			// symlink or a copy of the whole drumkit). When exporting
			// a song, these "local" references have to be replaced by
			// global ones (drumkits in the system's or user's data
			// folder).
			if ( bUnderSessionManagement ) {
				pHydrogen->setSessionIsExported( true );
				int nRet = NsmClient::dereferenceDrumkit( pSong );
				if ( nRet == -2 ) {
					QMessageBox::warning( this, "Hydrogen",
										  tr( "Drumkit [%1] used in session could not found on your system. Please install it in to make the exported song work properly." )
										  .arg( pSong->getLastLoadedDrumkitName() ) );
				}
			}
#endif

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
			NsmClient::replaceDrumkitPath( pSong, sLastLoadedDrumkitPath );
			h2app->showStatusBarMessage( tr("Song exported as: ") + sDefaultFilename );
			pHydrogen->setSessionIsExported( false );
		}
		else {
			h2app->showStatusBarMessage( tr("Song saved as: ") + sDefaultFilename );
		}
#else
		h2app->showStatusBarMessage( tr("Song saved as: ") + sDefaultFilename );
#endif
		
		h2app->updateWindowTitle();
	}

	return true;
}



bool MainForm::action_file_save()
{
	return action_file_save( "" );
}
bool MainForm::action_file_save( const QString& sNewFilename )
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		return false;
	}
	
	auto pCoreActionController = pHydrogen->getCoreActionController();
	QString sFilename = pSong->getFilename();

	if ( sNewFilename.isEmpty() &&
		 ( sFilename.isEmpty() ||
		   sFilename == Filesystem::empty_song_path() ) ) {
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
		bSaved = pCoreActionController->saveSong();
	} else {
		bSaved = pCoreActionController->saveSongAs( sNewFilename );
	}
	
	if( ! bSaved ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save song.") );
		return false;
	}

	h2app->showStatusBarMessage( tr("Song saved into") + QString(": ") +
									 sFilename );
	return true;
}


void MainForm::action_inputMode_instrument()
{
	if( !Preferences::get_instance()->__playselectedinstrument )
	{
		Preferences::get_instance()->__playselectedinstrument = true;
		m_pDrumkitAction->setChecked (false );
	}
	m_pInstrumentAction->setChecked( true );
}

void MainForm::action_inputMode_drumkit()
{
	if( Preferences::get_instance()->__playselectedinstrument )
	{
		Preferences::get_instance()->__playselectedinstrument = false;
		m_pInstrumentAction->setChecked( false );
	}
	m_pDrumkitAction->setChecked (true );
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
	for ( QString sLang : languages ) {
		QStringList sCandidates ( sLang );
		QStringList s = sLang.split('-');
		if ( s.size() != 1 ) {
			sCandidates << s[0];
		}
		for ( QString sCandidate : sCandidates ) {
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();
		
	if ( ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	if ( nPatternRow == -1 ) {
		nPatternRow = pHydrogen->getSelectedPatternNumber();
	}

	if ( nPatternRow == -1 ) {
		QMessageBox::warning( this, "Hydrogen", tr("No pattern selected.") );
		return;
	}

	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	
	Pattern *pPattern = pSong->getPatternList()->get( nPatternRow );

	QString sPath = Preferences::get_instance()->getLastExportPatternAsDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::patterns_dir();
	}

	QString title = tr( "Save Pattern as ..." );
	FileDialog fd(this);
	fd.setWindowTitle( title );
	fd.setDirectory( sPath );
	fd.selectFile( pPattern->get_name() );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::patterns_dir() ) );
	fd.setDefaultSuffix( Filesystem::patterns_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo( fd.selectedFiles().first() );
	Preferences::get_instance()->setLastExportPatternAsDirectory( fileInfo.path() );
	QString filePath = fileInfo.absoluteFilePath();

	QString originalName = pPattern->get_name();
	pPattern->set_name( fileInfo.baseName() );
	QString path = Files::savePatternPath( filePath, pPattern, pSong, pHydrogen->getLastLoadedDrumkitName() );
	pPattern->set_name( originalName );

	if ( path.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		return;
	}

	h2app->showStatusBarMessage( tr( "Pattern saved." ) );

	if ( filePath.indexOf( Filesystem::patterns_dir() ) == 0 ) {
		pHydrogen->getSoundLibraryDatabase()->updatePatterns();

	}
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	QString sPath = Preferences::get_instance()->getLastOpenPatternDirectory();
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
		Preferences::get_instance()->setLastOpenPatternDirectory( fd.directory().absolutePath() );

		for ( auto& ssFilename : fd.selectedFiles() ) {

			auto pNewPattern = Pattern::load_file( ssFilename, pSong->getInstrumentList() );
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
				HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
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
		pHydrogen->sequencer_stop();
	}

	return handleUnsavedChanges();
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
		HydrogenApp::get_instance()->openSong( sFilename );
		if ( bIsDemo &&
			 ! pHydrogen->isUnderSessionManagement() ) {
			pHydrogen->getSong()->setFilename( "" );
		}
	}
}

void MainForm::showPreferencesDialog()
{
	h2app->showPreferencesDialog();
}

void MainForm::action_window_showPlaylistDialog()
{
	h2app->showPlaylistDialog();
}

// function to update director status in menu bar
void MainForm::update_playlist_checkbox()
{
	bool isVisible = HydrogenApp::get_instance()->getPlayListDialog()->isVisible();
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

void MainForm::action_window_showMixer()
{
	bool isVisible = HydrogenApp::get_instance()->getMixer()->isVisible();
	h2app->showMixer( !isVisible );
}

// function to update mixer status in menu bar
void MainForm::update_mixer_checkbox()
{
	bool isVisible = HydrogenApp::get_instance()->getMixer()->isVisible();
	m_pViewMixerAction->setChecked( isVisible );
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
	pLogger->set_bit_mask( Logger::Error | Logger::Warning | Logger::Info );
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



void MainForm::action_instruments_addInstrument()
{
	SE_mainMenuAddInstrumentAction *pAction = new SE_mainMenuAddInstrumentAction();
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}


void MainForm::action_instruments_addComponent()
{
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "Component name" ), QLineEdit::Normal, "New Component", &bIsOkPressed );
	if ( bIsOkPressed  ) {
		Hydrogen *pHydrogen = Hydrogen::get_instance();

		auto pDrumkitComponent = std::make_shared<DrumkitComponent>( InstrumentEditor::findFreeDrumkitComponentId(), sNewName );
		pHydrogen->getSong()->getComponents()->push_back( pDrumkitComponent );

		selectedInstrumentChangedEvent();

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

#ifdef H2CORE_HAVE_JACK
		pHydrogen->renameJackPorts(pHydrogen->getSong());
#endif
	}
	else {
		// user entered nothing or pressed Cancel
	}
}


void MainForm::action_banks_open()
{
	SoundLibraryOpenDialog dialog( this );
	dialog.exec();
}


void MainForm::action_instruments_clearAll()
{
	switch( 
			 QMessageBox::information( 	this,					//NOLINT
						   	"Hydrogen",
							tr("Clear all instruments?"),
							QMessageBox::Cancel | QMessageBox::Ok,
							QMessageBox::Cancel)) {
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

	// Remove all instruments
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	auto pList = pSong->getInstrumentList();
	for (uint i = pList->size(); i > 0; i--) {
		functionDeleteInstrument(i - 1);
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void MainForm::functionDeleteInstrument( int nInstrument )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( nInstrument );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	std::list< Note* > noteList;
	PatternList *pPatternList = pSong->getPatternList();

	QString sInstrumentName =  pSelectedInstrument->get_name();
	QString sDrumkitPath = pSelectedInstrument->get_drumkit_path();

	for ( int i = 0; i < pPatternList->size(); i++ ) {
		const H2Core::Pattern *pPattern = pPatternList->get(i);
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				pNote->set_pattern_idx( i );
				noteList.push_back( pNote );
			}
		}
	}
	
	SE_deleteInstrumentAction *pAction =
		new SE_deleteInstrumentAction( noteList, sDrumkitPath,
									   sInstrumentName, nInstrument );
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}


void MainForm::action_instruments_exportLibrary() {

	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	auto pDrumkit = pHydrogen->getSoundLibraryDatabase()
		->getDrumkit( pHydrogen->getLastLoadedDrumkitPath() );
	
	if ( pDrumkit != nullptr ){

		auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
		pNewDrumkit->set_instruments( pSong->getInstrumentList() );
		pNewDrumkit->set_components( pSong->getComponents() );
		SoundLibraryExportDialog exportDialog( this, pNewDrumkit );
		exportDialog.exec();
	}
	else {
		QMessageBox::warning( this, "Hydrogen", QString( "%1 [%2]")
							  .arg( HydrogenApp::get_instance()->getCommonStrings()->getSoundLibraryFailedPreDrumkitLoad() )
							  .arg( pHydrogen->getLastLoadedDrumkitPath() ) );
	}		
}




void MainForm::action_instruments_importLibrary()
{
	SoundLibraryImportDialog dialog( this, false );
	dialog.exec();
}


void MainForm::action_instruments_onlineImportLibrary()
{
	SoundLibraryImportDialog dialog( this, true );
	dialog.exec();
}


void MainForm::action_instruments_saveLibrary()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->
		getDrumkit( pHydrogen->getLastLoadedDrumkitPath() );
	auto drumkitType = Filesystem::determineDrumkitType(
		pHydrogen->getLastLoadedDrumkitPath() );

	// In case the user does not have write access to the folder of
	// pDrumkit, the save as dialog will be opened.
	if ( pDrumkit != nullptr &&
		 ( drumkitType == Filesystem::DrumkitType::User ||
		   drumkitType == Filesystem::DrumkitType::SessionReadWrite ) ) {
		auto pNewDrumkit = std::make_shared<Drumkit>(pDrumkit);
		pNewDrumkit->set_instruments( pSong->getInstrumentList() );
		pNewDrumkit->set_components( pSong->getComponents() );
		
		if ( ! HydrogenApp::checkDrumkitLicense( pNewDrumkit ) ) {
			ERRORLOG( "User cancelled dialog due to licensing issues." );
			return;
		}
		
		if ( ! pNewDrumkit->save() ) {
			QMessageBox::information( this, "Hydrogen", tr( "Saving of this library failed."));
			return;
		}

		pHydrogen->getSoundLibraryDatabase()->updateDrumkits();
	}
	else {
		action_instruments_saveAsLibrary();
	}
}


void MainForm::action_instruments_saveAsLibrary()
{
	editDrumkitProperties( false );
}







///
/// Window close event
///
void MainForm::closeEvent( QCloseEvent* ev )
{
	if ( action_file_exit() == false ) {
		// don't close!!!
		ev->ignore();
		return;
	}

	ev->accept();
}



void MainForm::action_file_export()
{
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	ExportSongDialog *dialog = new ExportSongDialog(this);
	dialog->exec();
	delete dialog;
}



void MainForm::action_window_showInstrumentRack()
{
	InstrumentRack *pPanel = HydrogenApp::get_instance()->getInstrumentRack();
	pPanel->setHidden( pPanel->isVisible() );
	update_instrument_checkbox( pPanel->isVisible() );
}

void MainForm::update_instrument_checkbox( bool show )
{
	m_pViewMixerInstrumentRackAction->setChecked( show );
}

void MainForm::update_automation_checkbox()
{
	Preferences *pref = Preferences::get_instance();
	
	if(pref->getShowAutomationArea()){
		m_pViewAutomationPathAction->setChecked(true);	
	} else {
		m_pViewAutomationPathAction->setChecked(false);
	}
}

void MainForm::update_playback_track_group()
{
	Preferences *pPref = Preferences::get_instance();

	// Note that the ActionGroup unchecks the other menu item automatically
	if ( pPref->getShowPlaybackTrack() ) {
		m_pViewPlaybackTrackAction->setChecked( true );
	} else {
		m_pViewTimelineAction->setChecked( true );
	}
}

void MainForm::savePreferences() {
	// save window properties in the preferences files
	Preferences *pPreferences = Preferences::get_instance();

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

	pPreferences->setPlaylistDialogProperties(
		h2app->getWindowProperties( h2app->getPlayListDialog() ) );
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
	// Store the last playlist in the Preferences in order to allow to
	// reopen it at startup.
	Preferences::get_instance()->setLastPlaylistFilename(
		Playlist::get_instance()->getFilename() );

	savePreferences();
	m_pQApp->quit();
}


void MainForm::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
		m_pQApp->setFont( font );
		menuBar()->setFont( font );

		m_pFileMenu->setFont( font );
		m_pUndoMenu->setFont( font );
		m_pDrumkitsMenu->setFont( font );
		m_pInstrumentsMenu->setFont( font );
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
}
	

// keybindings..

void MainForm::onPlayStopAccelEvent()
{
	switch ( Hydrogen::get_instance()->getAudioEngine()->getState() ) {
	case H2Core::AudioEngine::State::Ready:
		Hydrogen::get_instance()->sequencer_play();
		break;

	case H2Core::AudioEngine::State::Playing:
		Hydrogen::get_instance()->sequencer_stop();
		break;

	default:
		ERRORLOG( "[MainForm::onPlayStopAccelEvent()] Unhandled case." );
	}
}



void MainForm::onRestartAccelEvent()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->locateToColumn( 0 );
}



void MainForm::onBPMPlusAccelEvent() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	pHydrogen->getSong()->setBpm( pAudioEngine->getTransportPosition()->getBpm() + 0.1 );

	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->setNextBpm( pAudioEngine->getTransportPosition()->getBpm() + 0.1 );
	pAudioEngine->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );
}



void MainForm::onBPMMinusAccelEvent() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	pHydrogen->getSong()->setBpm( pAudioEngine->getTransportPosition()->getBpm() - 0.1 );
	
	pAudioEngine->lock( RIGHT_HERE );
	pAudioEngine->setNextBpm( pAudioEngine->getTransportPosition()->getBpm() - 0.1 );
	pAudioEngine->unlock();
	
	EventQueue::get_instance()->push_event( EVENT_TEMPO_CHANGED, -1 );
}

void MainForm::updateRecentUsedSongList()
{
	m_pRecentFilesMenu->clear();

	Preferences *pPref = Preferences::get_instance();
	std::vector<QString> recentUsedSongs = pPref->getRecentFiles();

	QString sFilename;

	for ( uint i = 0; i < recentUsedSongs.size(); ++i ) {
		sFilename = recentUsedSongs[ i ];

		if ( !sFilename.isEmpty() ) {
			QAction *pAction = new QAction( this  );
			pAction->setText( sFilename );
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
	
	HydrogenApp::get_instance()->openSong( pAction->text() );
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
	if ( pSong->getInstrumentList()->has_all_midi_notes_same() ) {
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
		pSong->getInstrumentList()->set_default_midi_out_notes();
		pHydrogen->setIsModified( true );

		m_pMidiSetupInfoBar->hide();
	}
}


void MainForm::onFixMissingSamples()
{
	INFOLOG( "Fixing MIDI setup" );
	SoundLibraryOpenDialog dialog( this );
	dialog.exec();

	m_pMissingSamplesInfoBar->hide();
}


void MainForm::initKeyInstMap()
{

	QString loc = QLocale::system().name();

	// Mimics a MIDI NOTE_ON event which has pitch values between
	// MidiMessage::instrumentOffset and 127.
	int nNote = MidiMessage::instrumentOffset;

	///POSIX Locale
	//locale for keyboardlayout QWERTZ
	// de_DE, de_AT, de_LU, de_CH, de

	//locale for keyboardlayout AZERTY
	// fr_BE, fr_CA, fr_FR, fr_LU, fr_CH

	//locale for keyboardlayout QWERTY
	// en_GB, en_US, en_ZA, usw.

	if ( loc.contains( "de" ) || loc.contains( "DE" )){ ///QWERTZ
		keycodeInstrumentMap[Qt::Key_Y] = nNote++;
		keycodeInstrumentMap[Qt::Key_S] = nNote++;
		keycodeInstrumentMap[Qt::Key_X] = nNote++;
		keycodeInstrumentMap[Qt::Key_D] = nNote++;
		keycodeInstrumentMap[Qt::Key_C] = nNote++;
		keycodeInstrumentMap[Qt::Key_V] = nNote++;
		keycodeInstrumentMap[Qt::Key_G] = nNote++;
		keycodeInstrumentMap[Qt::Key_B] = nNote++;
		keycodeInstrumentMap[Qt::Key_H] = nNote++;
		keycodeInstrumentMap[Qt::Key_N] = nNote++;
		keycodeInstrumentMap[Qt::Key_J] = nNote++;
		keycodeInstrumentMap[Qt::Key_M] = nNote++;

		keycodeInstrumentMap[Qt::Key_Q] = nNote++;
		keycodeInstrumentMap[Qt::Key_2] = nNote++;
		keycodeInstrumentMap[Qt::Key_W] = nNote++;
		keycodeInstrumentMap[Qt::Key_3] = nNote++;
		keycodeInstrumentMap[Qt::Key_E] = nNote++;
		keycodeInstrumentMap[Qt::Key_R] = nNote++;
		keycodeInstrumentMap[Qt::Key_5] = nNote++;
		keycodeInstrumentMap[Qt::Key_T] = nNote++;
		keycodeInstrumentMap[Qt::Key_6] = nNote++;
		keycodeInstrumentMap[Qt::Key_Z] = nNote++;
		keycodeInstrumentMap[Qt::Key_7] = nNote++;
		keycodeInstrumentMap[Qt::Key_U] = nNote++;
	}
	else if ( loc.contains( "fr" ) || loc.contains( "FR" )){ ///AZERTY
		keycodeInstrumentMap[Qt::Key_W] = nNote++;
		keycodeInstrumentMap[Qt::Key_S] = nNote++;
		keycodeInstrumentMap[Qt::Key_X] = nNote++;
		keycodeInstrumentMap[Qt::Key_D] = nNote++;
		keycodeInstrumentMap[Qt::Key_C] = nNote++;
		keycodeInstrumentMap[Qt::Key_V] = nNote++;
		keycodeInstrumentMap[Qt::Key_G] = nNote++;
		keycodeInstrumentMap[Qt::Key_B] = nNote++;
		keycodeInstrumentMap[Qt::Key_H] = nNote++;
		keycodeInstrumentMap[Qt::Key_N] = nNote++;
		keycodeInstrumentMap[Qt::Key_J] = nNote++;
		keycodeInstrumentMap[Qt::Key_Question] = nNote++;

		keycodeInstrumentMap[Qt::Key_A] = nNote++;
		keycodeInstrumentMap[Qt::Key_2] = nNote++;
		keycodeInstrumentMap[Qt::Key_Z] = nNote++;
		keycodeInstrumentMap[Qt::Key_3] = nNote++;
		keycodeInstrumentMap[Qt::Key_E] = nNote++;
		keycodeInstrumentMap[Qt::Key_R] = nNote++;
		keycodeInstrumentMap[Qt::Key_5] = nNote++;
		keycodeInstrumentMap[Qt::Key_T] = nNote++;
		keycodeInstrumentMap[Qt::Key_6] = nNote++;
		keycodeInstrumentMap[Qt::Key_Y] = nNote++;
		keycodeInstrumentMap[Qt::Key_7] = nNote++;
		keycodeInstrumentMap[Qt::Key_U] = nNote++;
	}else
	{ /// default QWERTY
		keycodeInstrumentMap[Qt::Key_Z] = nNote++;
		keycodeInstrumentMap[Qt::Key_S] = nNote++;
		keycodeInstrumentMap[Qt::Key_X] = nNote++;
		keycodeInstrumentMap[Qt::Key_D] = nNote++;
		keycodeInstrumentMap[Qt::Key_C] = nNote++;
		keycodeInstrumentMap[Qt::Key_V] = nNote++;
		keycodeInstrumentMap[Qt::Key_G] = nNote++;
		keycodeInstrumentMap[Qt::Key_B] = nNote++;
		keycodeInstrumentMap[Qt::Key_H] = nNote++;
		keycodeInstrumentMap[Qt::Key_N] = nNote++;
		keycodeInstrumentMap[Qt::Key_J] = nNote++;
		keycodeInstrumentMap[Qt::Key_M] = nNote++;

		keycodeInstrumentMap[Qt::Key_Q] = nNote++;
		keycodeInstrumentMap[Qt::Key_2] = nNote++;
		keycodeInstrumentMap[Qt::Key_W] = nNote++;
		keycodeInstrumentMap[Qt::Key_3] = nNote++;
		keycodeInstrumentMap[Qt::Key_E] = nNote++;
		keycodeInstrumentMap[Qt::Key_R] = nNote++;
		keycodeInstrumentMap[Qt::Key_5] = nNote++;
		keycodeInstrumentMap[Qt::Key_T] = nNote++;
		keycodeInstrumentMap[Qt::Key_6] = nNote++;
		keycodeInstrumentMap[Qt::Key_Y] = nNote++;
		keycodeInstrumentMap[Qt::Key_7] = nNote++;
		keycodeInstrumentMap[Qt::Key_U] = nNote++;
	}
}


bool MainForm::eventFilter( QObject *o, QEvent *e )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	
	if ( e->type() == QEvent::FileOpen ) {
		// Mac OS always opens files (including via double click in Finder) via a FileOpenEvent.
		QFileOpenEvent *fe = dynamic_cast<QFileOpenEvent*>(e);
		assert( fe != nullptr );
		QString sFileName = fe->file();

		if ( sFileName.endsWith( H2Core::Filesystem::songs_ext ) ) {
			if ( handleUnsavedChanges() ) {
				pHydrogenApp->openSong( sFileName );
			}

		} else if ( sFileName.endsWith( H2Core::Filesystem::drumkit_ext ) ) {
			H2Core::Drumkit::install( sFileName );

		} else if ( sFileName.endsWith( H2Core::Filesystem::playlist_ext ) ) {
			bool loadlist = pHydrogenApp->getPlayListDialog()->loadListByFileName( sFileName );
			if ( loadlist ) {
				H2Core::Playlist::get_instance()->setNextSongByNumber( 0 );
			}
		}
		return true;

	} else if ( e->type() == QEvent::KeyPress ) {
		// special processing for key press
		QKeyEvent *k = (QKeyEvent *)e;

		if ( k->matches( QKeySequence::StandardKey::Undo ) ) {
			k->accept();
			action_undo();
			return true;
		} else if ( k->matches( QKeySequence::StandardKey::Redo ) ) {
			k->accept();
			action_redo();
			return true;
		}


		// qDebug( "Got key press for instrument '%c'", k->ascii() );
		switch (k->key()) {
		case Qt::Key_Space:

			// Hint that something is wrong in case there is no proper audio
			// driver set.
			if ( pHydrogen->getAudioOutput() == nullptr ||
				 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
				QMessageBox::warning( this, "Hydrogen",
									  QString( "%1\n%2" )
									  .arg( pCommonStrings->getAudioDriverNotPresent() )
									  .arg( pCommonStrings->getAudioDriverErrorHint() ) );
				return true;
			}

			switch ( k->modifiers() ) {
			case Qt::NoModifier:
				onPlayStopAccelEvent();
				break;

#ifndef Q_OS_MACX
			case Qt::ControlModifier:
				startPlaybackAtCursor( o );
				break;
			}
#else
			case Qt::AltModifier:
				startPlaybackAtCursor( o );
				break;
			}
#endif
			
			return true; // eat event
			break;

		case Qt::Key_Comma:
			pHydrogen->handleBeatCounter();
			return true; // eat even
			break;

		case Qt::Key_Backspace:
			onRestartAccelEvent();
			return true; // eat event
			break;

		case Qt::Key_Plus:
			onBPMPlusAccelEvent();
			return true; // eat event
			break;

		case Qt::Key_Minus:
			onBPMMinusAccelEvent();
			return true; // eat event
			break;

		case Qt::Key_Backslash:
			pHydrogen->onTapTempoAccelEvent();
			return true; // eat event
			break;

		case Qt::Key_S:
			if ( k->modifiers() ==
				 ( Qt::ControlModifier | Qt::ShiftModifier ) ) {
				action_file_save_as();
				return true;
			} else if ( k->modifiers() == Qt::ControlModifier ) {
				action_file_save();
				return true;
			}
			break;

		case  Qt::Key_F5 :
			if( Playlist::get_instance()->size() == 0) {
				break;
			}
			return handleSelectNextPrevSongOnPlaylist( -1 );
			break;

		case  Qt::Key_F6 :
			if( Playlist::get_instance()->size() == 0) {
				break;
			}
			return handleSelectNextPrevSongOnPlaylist( 1 );
			break;

		case  Qt::Key_F12 : //panic button stop all playing notes
			pHydrogen->__panic();
			//QMessageBox::information( this, "Hydrogen", tr( "Panic" ) );
			return true;
			break;

		case  Qt::Key_F9 : // Qt::Key_Left do not work. Some ideas ?
			pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() - 1 );
			return true;
			break;

		case  Qt::Key_F10 : // Qt::Key_Right do not work. Some ideas ?
			pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1 );
			return true;
			break;
		}

		// virtual keyboard handling
		if  ( k->modifiers() == Qt::NoModifier ) {
			std::map<int,int>::iterator found = keycodeInstrumentMap.find ( k->key() );
			if ( found != keycodeInstrumentMap.end() ) {
				const int nNote = (*found).second;

				INFOLOG( QString( "[Virtual Keyboard] triggering note [%1]" )
						 .arg( nNote ) );

				pHydrogen->getCoreActionController()->handleNote(
					nNote, 0.8, false );

				return true; // eat event
			}
		}
		return false; // let it go
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
		Hydrogen::get_instance()->sequencer_stop();
	}

	ExportMidiDialog *dialog = new ExportMidiDialog(this);
	dialog->exec();
	delete dialog;
}




void MainForm::action_file_export_lilypond()
{
	if ( Hydrogen::get_instance()->getAudioEngine()->getState() == H2Core::AudioEngine::State::Playing ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	QMessageBox::information(
		this,
		"Hydrogen",
		tr( "\nThe LilyPond export is an experimental feature.\n"
		"It should work like a charm provided that you use the "
		"GMRockKit, and that you do not use triplet.\n" ),
		QMessageBox::Ok );

	QString sPath = Preferences::get_instance()->getLastExportLilypondDirectory();
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
		Preferences::get_instance()->setLastExportLilypondDirectory( fd.directory().absolutePath() );
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
		msg = QString( tr( "OSC Server: Cannot connect to given port, using port %1 instead" ) ).arg( Preferences::get_instance()->m_nOscTemporaryPort );
		break;

	case Hydrogen::PLAYBACK_TRACK_INVALID:
		msg = tr( "Playback track couldn't be read" );
		break;

	default:
		msg = QString( tr( "Unknown error %1" ) ).arg( nErrorCode );
	}
	QMessageBox::information( this, "Hydrogen", msg );
}

void MainForm::playlistLoadSongEvent (int nIndex)
{
	Playlist* pPlaylist = Playlist::get_instance();

	QString songFilename;
	if( !pPlaylist->getSongFilenameByNumber( nIndex, songFilename ) ) {
		return;
	}
	
	HydrogenApp::get_instance()->openSong( songFilename );
	
	pPlaylist->activateSong( nIndex );

	HydrogenApp::get_instance()->showStatusBarMessage( tr( "Playlist: Set song No. %1" )
													   .arg( nIndex +1 ) );
}

void MainForm::jacksessionEvent( int nEvent )
{
	switch (nEvent){
	case 0:
		action_file_save();
		break;
	case 1:
		action_file_exit();
		break;
	}

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
	Preferences *pPreferences = Preferences::get_instance();
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



QString MainForm::getAutoSaveFilename()
{
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	assert( pSong );
	QString sOldFilename = pSong->getFilename();
	QString sNewName;

	if ( !sOldFilename.isEmpty() ) {

		QFileInfo fileInfo( sOldFilename );

		// In case the user did open a hidden file, the baseName()
		// will be an empty string.
		QString sBaseName( fileInfo.completeBaseName() );
		if ( sBaseName.startsWith( "." ) ) {
			sBaseName.remove( 0, 1 );
		}

		QString sAbsoluteDir( fileInfo.absoluteDir().absolutePath() );
		if ( ! Filesystem::file_writable( sOldFilename, true ) ) {

			sNewName = QString( "%1%2.autosave.h2song" )
				.arg( Filesystem::songs_dir() ).arg( sBaseName );
		
			WARNINGLOG( QString( "Path of current song [%1] is not writable. Autosave will store the song as [%2] instead." )
						.arg( sOldFilename ).arg( sNewName ) );
		} else {
			sNewName = QString( "%1/.%2.autosave.h2song" )
				.arg( sAbsoluteDir ).arg( sBaseName );
		}
	} else {
		// Store the default autosave file in the user's song data
		// folder to not clutter their working directory.
		sNewName = QString( "%1autosave.h2song" )
			.arg( Filesystem::songs_dir() );
	}

	return sNewName;
}



void MainForm::onAutoSaveTimer()
{
	auto pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	assert( pSong );
	if ( pSong->getIsModified() ) {
		QString sOldFilename = pSong->getFilename();

		QString sAutoSaveFilename = getAutoSaveFilename();
		if ( sAutoSaveFilename != m_sPreviousAutoSaveFilename ) {
			if ( ! m_sPreviousAutoSaveFilename.isEmpty() ) {
				QFile file( m_sPreviousAutoSaveFilename );
				file.remove();
			}
			m_sPreviousAutoSaveFilename = sAutoSaveFilename;
		}
			
		pSong->save( sAutoSaveFilename );

		pSong->setFilename( sOldFilename );
		pHydrogen->setIsModified( true );
	}
}


void MainForm::onPlaylistDisplayTimer()
{
	if( Playlist::get_instance()->size() == 0) {
		return;
	}
	
	int songnumber = Playlist::get_instance()->getActiveSongNumber();
	QString songname;
	if ( songnumber == -1 ) {
		return;
	}

	if ( Hydrogen::get_instance()->getSong()->getName() == "Untitled Song" ){
		songname = Hydrogen::get_instance()->getSong()->getFilename();
	} else {
		songname = Hydrogen::get_instance()->getSong()->getName();
	}
	QString message = (tr("Playlist: Song No. %1").arg( songnumber + 1)) + QString("  ---  Songname: ") + songname + QString("  ---  Author: ") + Hydrogen::get_instance()->getSong()->getAuthor();
	HydrogenApp::get_instance()->showStatusBarMessage( message );
}

// Returns true if unsaved changes are successfully handled (saved, discarded, etc.)
// Returns false if not (i.e. Cancel)
bool MainForm::handleUnsavedChanges()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	bool done = false;
	bool rv = true;
	while ( !done && Hydrogen::get_instance()->getSong()->getIsModified() ) {
		switch(
				 QMessageBox::information( this, "Hydrogen",
										 tr("\nThe document contains unsaved changes.\n"
												"Do you want to save the changes?\n"),
										   pCommonStrings->getButtonSave(),
										   pCommonStrings->getButtonDiscard(),
										   pCommonStrings->getButtonCancel(),
										   0,      // Enter == button 0
										   2 ) ) { // Escape == button 2
		case 0: // Save clicked or Alt+S pressed or Enter pressed.
			// If the save fails, the __is_modified flag will still be true
			if ( ! Hydrogen::get_instance()->getSong()->getFilename().isEmpty() ) {
				if ( ! action_file_save() ) {
					return false;
				}
			}
			else {
				// never been saved
				if ( ! action_file_save_as() ) {
					return false;
				}
			}
			// save
			break;
		case 1: // Discard clicked or Alt+D pressed
			// don't save but exit
			done = true;
			break;
		case 2: // Cancel clicked or Alt+C pressed or Escape pressed
			// don't exit
			return false;
		}
	}

	while ( !done && Playlist::get_instance()->getIsModified() ) {
		switch(
			QMessageBox::information(
				this, 
				"Hydrogen",
				tr("\nThe current playlist contains unsaved changes.\n"
				   "Do you want to discard the changes?\n"),
				pCommonStrings->getButtonDiscard(),
				pCommonStrings->getButtonCancel(),
				nullptr,      // Enter == button 0
				2 ) ) { // Escape == button 1
		case 0: // Discard clicked or Alt+D pressed
				// don't save but exit
			done = true;
			break;
		case 1: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
			return false;
		}
	}

	return true;
}


void MainForm::usr1SignalHandler(int)
{
	char a = 1;
	::write(sigusr1Fd[0], &a, sizeof(a));
}

void MainForm::handleSigUsr1()
{
	snUsr1->setEnabled(false);
	char tmp;
	::read(sigusr1Fd[1], &tmp, sizeof(tmp));

	action_file_save();
	snUsr1->setEnabled(true);
}

void MainForm::openUndoStack()
{
	m_pUndoView->show();
	m_pUndoView->setAttribute(Qt::WA_QuitOnClose, false);
}

void MainForm::action_undo(){
	h2app->m_pUndoStack->undo();
}

void MainForm::action_redo(){
	h2app->m_pUndoStack->redo();
}

void MainForm::updatePreferencesEvent( int nValue ) {
	
	if ( nValue == 0 ) {
		// Write the state of the GUI to the Preferences.
		savePreferences();
		Preferences::get_instance()->savePreferences();
		
	} else if ( nValue == 1 ) {
		
		// Reflect the changes in the preferences in the objects
		// stored in MainForm.
		if( Preferences::get_instance()->__playselectedinstrument ) {
			m_pInstrumentAction->setChecked( true );
			m_pDrumkitAction->setChecked (false );
		} else {
			m_pInstrumentAction->setChecked( false );
			m_pDrumkitAction->setChecked (true );
		}

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

bool MainForm::handleSelectNextPrevSongOnPlaylist( int step )
{
	int nPlaylistSize = Playlist::get_instance()->size();
	int nSongnumber = Playlist::get_instance()->getActiveSongNumber();
	
	if( nSongnumber+step >= 0 && nSongnumber+step <= nPlaylistSize-1 ){
		Playlist::get_instance()->setNextSongByNumber( nSongnumber + step );
	} else {
		return false;
	}

	return true;
}

void MainForm::action_banks_properties() {
	editDrumkitProperties( true );
}

void MainForm::editDrumkitProperties( bool bDrumkitNameLocked )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	auto pDrumkit = pHydrogen->getSoundLibraryDatabase()
		->getDrumkit( pHydrogen->getLastLoadedDrumkitPath() );

	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to find drumkit at path [%1]. Trying drumkit name [%2] instead." )
				  .arg( pHydrogen->getLastLoadedDrumkitPath() )
				  .arg( pHydrogen->getLastLoadedDrumkitName() ) );
		// No luck when searching for the kit using the absolute path found in
		// the .h2song. Let's try the last loaded drumkit name.
		const QString sDrumkitPath =
			Filesystem::drumkit_path_search( pHydrogen->getLastLoadedDrumkitName(),
											 Filesystem::Lookup::stacked, true );
		pDrumkit = pHydrogen->getSoundLibraryDatabase()
			->getDrumkit( sDrumkitPath );
	}

	if ( pDrumkit == nullptr && ! bDrumkitNameLocked ) {
		ERRORLOG( QString( "Unable to find drumkit of name [%1] either. Falling back to empty one." )
				  .arg( pHydrogen->getLastLoadedDrumkitName() ) );
		// If that didn't worked either and the user wants to "Save As", we fall
		// back to the default kit.
		pDrumkit = std::make_shared<Drumkit>();
	}
	
	if ( pDrumkit != nullptr ){

		auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
		pNewDrumkit->set_instruments( pSong->getInstrumentList() );
		pNewDrumkit->set_components( pSong->getComponents() );
		
		SoundLibraryPropertiesDialog dialog( this, pNewDrumkit, bDrumkitNameLocked );
		if ( dialog.exec() == QDialog::Accepted ) {
			// Saving was successful.

			if ( pNewDrumkit->get_path() != pDrumkit->get_path() ) {
				// A new drumkit was created based on the original
				// one. We call the drumkit setter to ensure
				// everything in the Song and GUI is still in sync.
				pHydrogen->getCoreActionController()->setDrumkit( pNewDrumkit, false );
			}
		}
	}
	else {
		QMessageBox::warning( this, "Hydrogen", QString( "%1 [%2]")
							  .arg( HydrogenApp::get_instance()->getCommonStrings()->getSoundLibraryFailedPreDrumkitLoad() )
							  .arg( pHydrogen->getLastLoadedDrumkitPath() ) );
	}
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
	HydrogenApp* pApp = HydrogenApp::get_instance();
	auto pCoreActionController = pHydrogen->getCoreActionController();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong == nullptr ) {
		return;
	}

	if ( pObject->inherits( "SongEditorPanel" ) ) {
			
		if ( pHydrogen->getMode() != Song::Mode::Song ) {
			pCoreActionController->activateSongMode( true );
		}

		const int nCursorColumn =
			pApp->getSongEditorPanel()->getSongEditor()->getCursorColumn();

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
		
		if ( ! pCoreActionController->locateToColumn( nCursorColumn ) ) {
			// Cursor is at a position it is not allowed to locate to.
			return;
		}
			
	} else if ( pObject->inherits( "PatternEditorPanel" ) ) {
		// Covers both the PatternEditor and the
		// NotePropertiesRuler.
			
		if ( pHydrogen->getMode() != Song::Mode::Pattern ) {
			pCoreActionController->activateSongMode( false );
		}

		// To provide a similar behaviour as when pressing
		// [backspace], transport is relocated to the beginning of
		// the song.
		const int nCursorColumn = pApp->getPatternEditorPanel()->getCursorPosition();
		
		if ( ! pCoreActionController->locateToTick( nCursorColumn ) ) {
			// Cursor is at a position it is not allowed to locate to.
			return;
		}
	} else {
		ERRORLOG( QString( "Unknown object class" ) );
	}

	if ( pAudioEngine->getState() == H2Core::AudioEngine::State::Ready ) {
		pHydrogen->sequencer_play();
		pApp->showStatusBarMessage( tr("Playing.") );
	}
}
