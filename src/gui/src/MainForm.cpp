/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <core/EventQueue.h>
#include <core/Version.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine.h>
#include <core/Smf/SMF.h>
#include <core/Preferences.h>
#include <core/Timeline.h>
#include <core/Helpers/Files.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Playlist.h>
#include <core/Lilipond/Lilypond.h>

#include "AboutDialog.h"
#include "AudioEngineInfoForm.h"
#include "DonationDialog.h"
#include "ExportSongDialog.h"
#include "ExportMidiDialog.h"
#include "HydrogenApp.h"
#include "InstrumentRack.h"
#include "Skin.h"
#include "MainForm.h"
#include "PlayerControl.h"
#include "HelpBrowser.h"
#include "LadspaFXProperties.h"
#include "SongPropertiesDialog.h"
#include "UndoActions.h"
#include "Widgets/InfoBar.h"

#include "Director.h"
#include "Mixer/Mixer.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SoundLibrary/SoundLibraryImportDialog.h"
#include "SoundLibrary/SoundLibrarySaveDialog.h"
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

const char* MainForm::__class_name = "MainForm";

MainForm::MainForm( QApplication * pQApplication, const QString& songFilename, const bool bLoadSong )
	: QMainWindow( nullptr )
	, Object( __class_name )
{
	setMinimumSize( QSize( 1000, 500 ) );

#ifndef WIN32
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigusr1Fd)) {
		qFatal("Couldn't create HUP socketpair");
	}
	snUsr1 = new QSocketNotifier(sigusr1Fd[1], QSocketNotifier::Read, this);
	connect(snUsr1, SIGNAL(activated(int)), this, SLOT( handleSigUsr1() ));
#endif

	m_pQApp =  pQApplication;

	m_pQApp->processEvents();

	Song *pSong = nullptr;
	
	if ( bLoadSong ) {
		if ( !songFilename.isEmpty() ) {
			pSong = Song::load( songFilename );

			/*
			 * If the song could not be loaded, create
			 * a new one with the specified filename
			 */
			if ( pSong == nullptr ) {
				pSong = Song::getEmptySong();
				pSong->setFilename( songFilename );
			}
		}
		else {
			Preferences *pref = Preferences::get_instance();
			bool restoreLastSong = pref->isRestoreLastSongEnabled();
			QString filename = pref->getLastSongFilename();
			if ( restoreLastSong && ( !filename.isEmpty() )) {
				pSong = Song::load( filename );
				if ( pSong == nullptr ) {
					//QMessageBox::warning( this, "Hydrogen", tr("Error restoring last song.") );
					pSong = Song::getEmptySong();
					pSong->setFilename( "" );
				}
			}
			else {
				pSong = Song::getEmptySong();
				pSong->setFilename( "" );
			}
		}
	} else {
		// When under Non Session Management the new Song will be
		// prepared by the corresponding NSM client instance and in
		// here we will just obtain and handed to the HydrogenApp but
		// not load there.
		pSong = Hydrogen::get_instance()->getSong();

		// In case something went wrong when setting the Song to the
		// loaded by the GUI via an OSC command, load the default Song
		// instead.
		if ( pSong == nullptr ) {
			pSong = Song::getEmptySong();
			pSong->setFilename( songFilename );
		}
		
	}

	showDevelWarning();
	h2app = new HydrogenApp( this, pSong );
	h2app->addEventListener( this );
	createMenuBar();
	checkMidiSetup();
	checkMissingSamples();
	checkNecessaryDirectories();

	h2app->setStatusBarMessage( tr("Hydrogen Ready."), 10000 );

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
	m_AutosaveTimer.start( 60 * 1000 );

#ifdef H2CORE_HAVE_LASH

	if ( Preferences::get_instance()->useLash() ){
		LashClient* lashClient = LashClient::get_instance();
		if (lashClient->isConnected())
		{
			// send alsa client id now since it can only be sent
			// after the audio engine has been started.
			Preferences *pref = Preferences::get_instance();
			if ( pref->m_sMidiDriver == "ALSA" ) {
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
	Hydrogen::get_instance()->setBcOffsetAdjust();
	// director
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 1 );
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );

	m_pUndoView = new QUndoView(h2app->m_pUndoStack);
	m_pUndoView->setWindowTitle(tr("Undo history"));

	//restore last playlist
	if(		Preferences::get_instance()->isRestoreLastPlaylistEnabled()
		&& !Preferences::get_instance()->getLastPlaylistFilename().isEmpty() ){
		bool loadlist = h2app->getPlayListDialog()->loadListByFileName( Preferences::get_instance()->getLastPlaylistFilename() );
		if( !loadlist ){
			_ERRORLOG ( "Error loading the playlist" );
		}
	}
}


MainForm::~MainForm()
{
	// remove the autosave file
	QFile file( getAutoSaveFilename() );
	file.remove();

	//if a playlist is used, we save the last playlist-path to hydrogen.conf
	Preferences::get_instance()->setLastPlaylistFilename( Playlist::get_instance()->getFilename() );

	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	// remove the autosave file
	m_AutosaveTimer.stop();
	QFile autosaveFile( "hydrogen_autosave.h2song" );
	autosaveFile.remove();


	hide();

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
	QMenuBar *m_pMenubar = new QMenuBar( this );
	setMenuBar( m_pMenubar );

	// FILE menu
	QMenu *m_pFileMenu = m_pMenubar->addMenu( tr( "Pro&ject" ) );

	// Then under session management a couple of options will be named
	// differently and some must be even omitted. 
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
	QString textFileNew, textFileOpen, textFileOpenRecent, textFileSaveAs;
	
	if ( bUnderSessionManagement ) {
		textFileNew = tr( "Replace With &New Song" );
		textFileOpen = tr( "Imp&ort Into Session" );
		textFileOpenRecent = tr( "Import &Recent Into Session" );
		textFileSaveAs = tr( "Export From Session &As..." );
	} else {
		textFileNew = tr( "&New" );
		textFileOpen = tr( "&Open" );
		textFileOpenRecent = tr( "Open &Recent" );
		textFileSaveAs = tr( "Save &As..." );
	}
	
	m_pFileMenu->addAction( textFileNew, this, SLOT( action_file_new() ), QKeySequence( "Ctrl+N" ) );
	
	m_pFileMenu->addAction( tr( "Show &Info" ), this, SLOT( action_file_songProperties() ), QKeySequence( "" ) );
	
	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( textFileOpen, this, SLOT( action_file_open() ), QKeySequence( "Ctrl+O" ) );
	if ( ! bUnderSessionManagement ) {
		m_pFileMenu->addAction( tr( "Open &Demo" ), this, SLOT( action_file_openDemo() ), QKeySequence( "Ctrl+D" ) );
	}
	m_pRecentFilesMenu = m_pFileMenu->addMenu( textFileOpenRecent );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( tr( "&Save" ), this, SLOT( action_file_save() ), QKeySequence( "Ctrl+S" ) );
	m_pFileMenu->addAction( textFileSaveAs, this, SLOT( action_file_save_as() ), QKeySequence( "Ctrl+Shift+S" ) );
	
	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction ( tr ( "Open &Pattern" ), this, SLOT ( action_file_openPattern() ), QKeySequence ( "" ) );
	m_pFileMenu->addAction( tr( "E&xport Pattern As..." ), this, SLOT( action_file_export_pattern_as() ), QKeySequence( "Ctrl+P" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( tr( "Export &MIDI File" ), this, SLOT( action_file_export_midi() ), QKeySequence( "Ctrl+M" ) );
	m_pFileMenu->addAction( tr( "&Export Song" ), this, SLOT( action_file_export() ), QKeySequence( "Ctrl+E" ) );
	m_pFileMenu->addAction( tr( "Export &LilyPond File" ), this, SLOT( action_file_export_lilypond() ), QKeySequence( "Ctrl+L" ) );


#ifndef Q_OS_MACX
	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( tr("&Quit"), this, SLOT( action_file_exit() ), QKeySequence( "Ctrl+Q" ) );
#endif

	updateRecentUsedSongList();
	connect( m_pRecentFilesMenu, SIGNAL( triggered(QAction*) ), this, SLOT( action_file_open_recent(QAction*) ) );
	//~ FILE menu

	// Undo menu
	QMenu *m_pUndoMenu = m_pMenubar->addMenu( tr( "&Undo" ) );
	m_pUndoMenu->addAction( tr( "&Undo" ), this, SLOT( action_undo() ), QKeySequence( "Ctrl+Z" ) );
	m_pUndoMenu->addAction( tr( "&Redo" ), this, SLOT( action_redo() ), QKeySequence( "Shift+Ctrl+Z" ) );
	m_pUndoMenu->addAction( tr( "Undo &History" ), this, SLOT( openUndoStack() ), QKeySequence( "" ) );

	// DRUMKITS MENU
	QMenu *m_pDrumkitsMenu = m_pMenubar->addMenu( tr( "Drum&kits" ) );
	m_pDrumkitsMenu->addAction( tr( "&New" ), this, SLOT( action_instruments_clearAll() ), QKeySequence( "" ) );
	m_pDrumkitsMenu->addAction( tr( "&Open" ), this, SLOT( action_banks_open() ), QKeySequence( "" ) );
	m_pDrumkitsMenu->addAction( tr( "&Properties" ), this, SLOT( action_banks_properties() ), QKeySequence( "" ) );

	m_pDrumkitsMenu->addSeparator();				// -----

	m_pDrumkitsMenu->addAction( tr( "&Save" ), this, SLOT( action_instruments_saveLibrary() ), QKeySequence( "" ) );
	m_pDrumkitsMenu->addAction( tr( "Save &As" ), this, SLOT( action_instruments_saveAsLibrary() ), QKeySequence( "" ) );

	m_pDrumkitsMenu->addSeparator();				// -----

	m_pDrumkitsMenu->addAction( tr( "&Export" ), this, SLOT( action_instruments_exportLibrary() ), QKeySequence( "" ) );
	m_pDrumkitsMenu->addAction( tr( "&Import" ), this, SLOT( action_instruments_importLibrary() ), QKeySequence( "" ) );
	m_pDrumkitsMenu->addAction( tr( "On&line Import" ), this, SLOT( action_instruments_onlineImportLibrary() ), QKeySequence( "" ) );

	// INSTRUMENTS MENU
	QMenu *m_pInstrumentsMenu = m_pMenubar->addMenu( tr( "In&struments" ) );
	m_pInstrumentsMenu->addAction( tr( "Add &Instrument" ), this, SLOT( action_instruments_addInstrument() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( tr( "Clea&r All" ), this, SLOT( action_instruments_clearAll() ), QKeySequence( "" ) );

	m_pInstrumentsMenu->addSeparator();				// -----

	m_pInstrumentsMenu->addAction( tr( "Add &Component" ), this, SLOT( action_instruments_addComponent() ), QKeySequence( "" ) );

	// VIEW MENU
	QMenu *m_pViewMenu = m_pMenubar->addMenu( tr( "&View" ) );

	m_pViewPlaylistEditorAction = m_pViewMenu->addAction( tr("Play&list Editor"), this, SLOT( action_window_showPlaylistDialog() ), QKeySequence( "" ) );
	m_pViewPlaylistEditorAction->setCheckable( true );
	m_pViewDirectorAction = m_pViewMenu->addAction( tr("&Director"), this, SLOT( action_window_show_DirectorWidget() ), QKeySequence( "Alt+D" ) );
	m_pViewDirectorAction->setCheckable( true );

	m_pFileMenu->addSeparator();
	m_pViewMixerAction = m_pViewMenu->addAction( tr("&Mixer"), this, SLOT( action_window_showMixer() ), QKeySequence( "Alt+M" ) );
	m_pViewMixerAction->setCheckable( true );
	update_mixer_checkbox();						// if checkbox need to be checked.

	m_pViewMixerInstrumentRackAction = m_pViewMenu->addAction( tr("&Instrument Rack"), this, SLOT( action_window_showInstrumentRack() ), QKeySequence( "Alt+I" ) );
	m_pViewMixerInstrumentRackAction->setCheckable( true );
	update_instrument_checkbox( Preferences::get_instance()->getInstrumentRackProperties().visible );

	m_pViewAutomationPathAction = m_pViewMenu->addAction( tr("&Automation Path"), this, SLOT( action_window_showAutomationArea() ), QKeySequence( "Alt+A" ) );
	m_pViewAutomationPathAction->setCheckable( true );
	update_automation_checkbox();

	m_pViewTimelineAction = m_pViewMenu->addAction( tr("&Timeline"), this, SLOT( action_window_showTimeline() ), QKeySequence( "" ) );
	m_pViewTimelineAction->setCheckable( true );
	m_pViewTimelineAction->setChecked( true );
	update_automation_checkbox();
	
	m_pViewPlaybackTrackAction = m_pViewMenu->addAction( tr("&Playback Track"), this, SLOT( action_window_showPlaybackTrack() ), QKeySequence( "" ) );
	m_pViewPlaybackTrackAction->setCheckable( true );
	m_pViewPlaybackTrackAction->setChecked( false );
	update_automation_checkbox();

	m_pViewMenu->addSeparator();				// -----

	m_pViewMenu->addAction( tr("&Full screen"), this, SLOT( action_window_toggleFullscreen() ), QKeySequence( "Alt+F" ) );


	// Options menu
	QMenu *m_pOptionsMenu = m_pMenubar->addMenu( tr( "&Options" ));

	m_pInputModeMenu = m_pOptionsMenu->addMenu( tr( "Input &Mode" ) );
	m_pInstrumentAction = m_pInputModeMenu->addAction( tr( "&Instrument" ), this, SLOT( action_inputMode_instrument() ), QKeySequence( "Ctrl+Alt+I" ) );
	m_pInstrumentAction->setCheckable( true );

	m_pDrumkitAction = m_pInputModeMenu->addAction( tr( "&Drumkit" ), this, SLOT( action_inputMode_drumkit() ), QKeySequence( "Ctrl+Alt+D" ) );
	m_pDrumkitAction->setCheckable( true );

	if( Preferences::get_instance()->__playselectedinstrument )
	{
		m_pInstrumentAction->setChecked( true );
		m_pDrumkitAction->setChecked (false );
	} else {
		m_pInstrumentAction->setChecked( false );
		m_pDrumkitAction->setChecked (true );
	}

	m_pOptionsMenu->addAction( tr("&Preferences"), this, SLOT( showPreferencesDialog() ), QKeySequence( "Alt+P" ) );

	//~ Tools menu


	Logger *pLogger = Logger::get_instance();
	if ( pLogger->bit_mask() >= 1 ) {
		// DEBUG menu
		QMenu *m_pDebugMenu = m_pMenubar->addMenu( tr("De&bug") );
		m_pDebugMenu->addAction( tr( "Show &Audio Engine Info" ), this, SLOT( action_debug_showAudioEngineInfo() ) );
		m_pDebugMenu->addAction( tr( "Show &Filesystem Info" ), this, SLOT( action_debug_showFilesystemInfo() ) );
		
		m_pLogLevelMenu = m_pDebugMenu->addMenu( tr( "&Log Level" ) );		
		m_pLogLevelMenu->addAction( tr( "&None" ), this, SLOT( action_debug_logLevel_none() ), QKeySequence( "" ) );
		m_pLogLevelMenu->addAction( tr( "&Error" ), this, SLOT( action_debug_logLevel_info() ), QKeySequence( "" ) );
		m_pLogLevelMenu->addAction( tr( "&Warning" ), this, SLOT( action_debug_logLevel_warn() ), QKeySequence( "" ) );
		m_pLogLevelMenu->addAction( tr( "&Info" ), this, SLOT( action_debug_logLevel_info() ), QKeySequence( "" ) );
		m_pLogLevelMenu->addAction( tr( "&Debug" ), this, SLOT( action_debug_logLevel_debug() ), QKeySequence( "" ) );
		
		m_pDebugMenu->addAction( tr( "&Open Log File" ), this, SLOT( action_debug_openLogfile()) );
		
		
		if(pLogger->bit_mask() == 8) { // hydrogen -V8 list object map in console 
			m_pDebugMenu->addAction( tr( "&Print Objects" ), this, SLOT( action_debug_printObjects() ) );
		}
		//~ DEBUG menu
	}

	// INFO menu
	QMenu *m_pInfoMenu = m_pMenubar->addMenu( tr( "I&nfo" ) );
	m_pInfoMenu->addAction( tr("User &Manual"), this, SLOT( showUserManual() ), QKeySequence( "Ctrl+?" ) );
	m_pInfoMenu->addSeparator();
	m_pInfoMenu->addAction( tr("&About"), this, SLOT( action_help_about() ), QKeySequence( tr("", "Info|About") ) );
	m_pInfoMenu->addAction( tr("&Report Bug"), this, SLOT( action_report_bug() ));
	m_pInfoMenu->addAction( tr("&Donate"), this, SLOT( action_donate() ));
	//~ INFO menu
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
		Song *song = Hydrogen::get_instance()->getSong();
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
	DonationDialog *pDialog = new DonationDialog( nullptr );
	pDialog->exec();
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
	
	Hydrogen * pEngine = Hydrogen::get_instance();
	if ( (pEngine->getState() == STATE_PLAYING) ) {
		pEngine->sequencer_stop();
	}

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return;
	}
	
	h2app->m_pUndoStack->clear();
	pEngine->getTimeline()->deleteAllTempoMarkers();
	pEngine->getTimeline()->deleteAllTags();
	Song* pSong = Song::getEmptySong();

	// When under session management the filename of the current Song
	// has to be preserved.
	QString currentFilename;
	if ( bUnderSessionManagement ) {
		currentFilename = H2Core::Hydrogen::get_instance()->getSong()->getFilename();
		
		// Just a single click will allow the user to discard the
		// current song and replace it with an empty one with now way
		// of undoing the action. Therefore, a warning popup will
		// check whether the action was intentional.
		QMessageBox confirmationBox;
		confirmationBox.setText( "Replace current song with empty one?" );
		confirmationBox.setInformativeText( "You won't be able to undo this action! Please export the current song from the session first in order to keep it." );
		confirmationBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
		confirmationBox.setDefaultButton( QMessageBox::No );
		
		int confirmationChoice = confirmationBox.exec();
		
		if ( confirmationChoice == QMessageBox::No ) {
			return;
		}
								 
		pSong->setFilename( currentFilename );
	} else {
		pSong->setFilename( "" );
	}

	h2app->openSong( pSong );
	h2app->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();
	h2app->getSongEditorPanel()->updatePositionRuler();

	// update director tags
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 2 );
	// update director songname
	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
}



void MainForm::action_file_save_as()
{
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
		
	Hydrogen* pHydrogen = Hydrogen::get_instance();

	if ( pHydrogen->getState() == STATE_PLAYING ) {
			pHydrogen->sequencer_stop();
	}

	//std::auto_ptr<QFileDialog> fd( new QFileDialog );
	QFileDialog fd(this);
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );

	if ( bUnderSessionManagement ) {	
		fd.setWindowTitle( tr( "Export song from Session" ) );
	} else {
		fd.setWindowTitle( tr( "Save song" ) );
	}
	
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::songs_dir() ) );

	Song* pSong = pHydrogen->getSong();
	QString defaultFilename;
	QString lastFilename = pSong->getFilename();

	if ( lastFilename.isEmpty() ) {
		defaultFilename = pHydrogen->getSong()->getName();
		defaultFilename += Filesystem::songs_ext;
	}
	else {
		defaultFilename = lastFilename;
	}

	fd.selectFile( defaultFilename );

	QString filename;
	if (fd.exec() == QDialog::Accepted) {
		filename = fd.selectedFiles().first();
	}

	if ( !filename.isEmpty() ) {
		QString sNewFilename = filename;
		if ( sNewFilename.endsWith( Filesystem::songs_ext ) == false ) {
			filename += Filesystem::songs_ext;
		}

		pSong->setFilename(filename);
		action_file_save();
	}
	
	// When Hydrogen is under session management, the file name
	// provided by the NSM server has to be preserved.
	if ( pHydrogen->isUnderSessionManagement() ) {
		pSong->setFilename( lastFilename );
		h2app->setScrollStatusBarMessage( tr("Song exported as: ") + defaultFilename, 2000 );
	} else {
		h2app->setScrollStatusBarMessage( tr("Song saved as: ") + defaultFilename, 2000 );
	}
	
	h2app->updateWindowTitle();
}



void MainForm::action_file_save()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	QString filename = pSong->getFilename();

	if ( filename.isEmpty() ) {
		// just in case!
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
			return;
		}
		pSong->clearMissingSamples();
	}

	bool saved = false;
	saved = pSong->save( filename );


	if(! saved) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save song.") );
	} else {
		Preferences::get_instance()->setLastSongFilename( pSong->getFilename() );

		// Add the new loaded song in the "last used song"
		// vector. 
		// This behavior is prohibited under session management. Only
		// songs open during normal runs will be listed.
		if ( ! H2Core::Hydrogen::get_instance()->isUnderSessionManagement() ) {
			Preferences::get_instance()->insertRecentFile( filename );
			updateRecentUsedSongList();
		}

		h2app->setScrollStatusBarMessage( tr("Song saved.") + QString(" Into: ") + filename, 2000 );
		EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );
	}
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

void MainForm::showUserManual()
{
	h2app->getHelpBrowser()->hide();
	h2app->getHelpBrowser()->show();
}

void MainForm::action_file_export_pattern_as()
{
	if ( ( Hydrogen::get_instance()->getState() == STATE_PLAYING ) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	Pattern *pPattern = pSong->getPatternList()->get( pEngine->getSelectedPatternNumber() );

	QDir dir = Preferences::get_instance()->__lastspatternDirectory;

	QString title = tr( "Save Pattern as ..." );
	QFileDialog fd(this);
	fd.setWindowTitle( title );
	fd.setDirectory( dir );
	fd.selectFile( pPattern->get_name() );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::patterns_dir() ) );
	fd.setDefaultSuffix( Filesystem::patterns_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo = fd.selectedFiles().first();
	Preferences::get_instance()->__lastspatternDirectory =  fileInfo.path();
	QString filePath = fileInfo.absoluteFilePath();

	QString originalName = pPattern->get_name();
	pPattern->set_name( fileInfo.baseName() );
	QString path = Files::savePatternPath( filePath, pPattern, pSong, pEngine->getCurrentDrumkitname() );
	pPattern->set_name( originalName );

	if ( path.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
		return;
	}

	h2app->setStatusBarMessage( tr( "Pattern saved." ), 10000 );

	if ( filePath.indexOf( Filesystem::patterns_dir() ) == 0 ) {
		SoundLibraryDatabase::get_instance()->updatePatterns();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
		HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();

	}
}

void MainForm::action_file_open() {
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
		
	if ( Hydrogen::get_instance()->getState() == STATE_PLAYING ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	bool bProceed = handleUnsavedChanges();
	if( !bProceed ) {
		return;
	}

	static QString sLastUsedDir = Filesystem::songs_dir();

	QFileDialog fd(this);
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( sLastUsedDir );
	fd.setNameFilter( Filesystem::songs_filter_name );

	if ( bUnderSessionManagement ) {
		fd.setWindowTitle( tr( "Import song into Session" ) );
	} else {
		fd.setWindowTitle( tr( "Open song" ) );
	}

	QString sFilename;
	if ( fd.exec() == QDialog::Accepted ) {
		sFilename = fd.selectedFiles().first();
		sLastUsedDir = fd.directory().absolutePath();
	}

	// When under session management the filename of the current Song
	// has to be preserved.
	QString sCurrentFilename;
	if ( bUnderSessionManagement ) {
		sCurrentFilename = H2Core::Hydrogen::get_instance()->getSong()->getFilename();
	}
	if ( !sFilename.isEmpty() ) {
		HydrogenApp::get_instance()->openSong( sFilename );
	}

	if ( bUnderSessionManagement ) {
		
		Song* pSong = H2Core::Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr ) {
			ERRORLOG( QString( "No song present while under session management" ) );
			return;
		}
		pSong->setFilename( sCurrentFilename );
	}

	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();
}


void MainForm::action_file_openPattern()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	int selectedPatternPosition = pEngine->getSelectedPatternNumber();

	Instrument *pInstrument = pSong->getInstrumentList()->get ( 0 );
	assert ( pInstrument );

	QFileDialog fd(this);
	fd.setFileMode ( QFileDialog::ExistingFile );
	fd.setDirectory ( Filesystem::patterns_dir() );
	fd.setNameFilter( Filesystem::patterns_filter_name );

	fd.setWindowTitle ( tr ( "Open Pattern" ) );


	QString filename;
	if ( fd.exec() == QDialog::Accepted )
	{
		filename = fd.selectedFiles().first();
	}
	QString patternname = filename;

	Pattern* err = Pattern::load_file( patternname, pSong->getInstrumentList() );
	if ( err == nullptr )
	{
		_ERRORLOG( "Error loading the pattern" );
		_ERRORLOG( patternname );
	}
	else
	{
		H2Core::Pattern *pNewPattern = err;

		if(!pPatternList->check_name( pNewPattern->get_name() ) ){
			pNewPattern->set_name( pPatternList->find_unused_pattern_name( pNewPattern->get_name() ) );
		}
		SE_insertPatternAction* pAction =
				new SE_insertPatternAction( selectedPatternPosition + 1, pNewPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
	}
}

/// \todo parametrizzare il metodo action_file_open ed eliminare il seguente...
void MainForm::action_file_openDemo()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	bool proceed = handleUnsavedChanges();
	if(!proceed) {
		return;
	}

	h2app->m_pUndoStack->clear();
	QFileDialog fd(this);
	fd.setFileMode(QFileDialog::ExistingFile);
	fd.setNameFilter( Filesystem::songs_filter_name );

	fd.setWindowTitle( tr( "Open song" ) );

	fd.setDirectory( Filesystem::demos_dir() );

	QString filename;
	if (fd.exec() == QDialog::Accepted) {
		filename = fd.selectedFiles().first();
	}

	if ( !filename.isEmpty() ) {
		HydrogenApp::get_instance()->openSong( filename );
		Hydrogen::get_instance()->getSong()->setFilename( "" );
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
	m_pViewPlaybackTrackAction->setChecked( false );	
	m_pViewTimelineAction->setChecked( true );	
}


void MainForm::action_window_showPlaybackTrack()
{
	h2app->getSongEditorPanel()->showPlaybackTrack();
	m_pViewPlaybackTrackAction->setChecked( true );	
	m_pViewTimelineAction->setChecked( false );	
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
		Hydrogen *pEngine = Hydrogen::get_instance();

		DrumkitComponent* pDrumkitComponent = new DrumkitComponent( InstrumentEditor::findFreeDrumkitComponentId(), sNewName );
		pEngine->getSong()->getComponents()->push_back( pDrumkitComponent );

		selectedInstrumentChangedEvent();

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

#ifdef H2CORE_HAVE_JACK
		pEngine->renameJackPorts(pEngine->getSong());
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
	Song *pSong = Hydrogen::get_instance()->getSong();
	InstrumentList* pList = pSong->getInstrumentList();
	for (uint i = pList->size(); i > 0; i--) {
		functionDeleteInstrument(i - 1);
	}

	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}

void MainForm::functionDeleteInstrument(int instrument)
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	Instrument *pSelectedInstrument = pEngine->getSong()->getInstrumentList()->get( instrument );

	std::list< Note* > noteList;
	Song* pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	QString instrumentName =  pSelectedInstrument->get_name();
	QString drumkitName = pEngine->getCurrentDrumkitname();

	for ( int i = 0; i < pPatternList->size(); i++ ) {
		const H2Core::Pattern *pPattern = pSong->getPatternList()->get(i);
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
	
	SE_deleteInstrumentAction *pAction = new SE_deleteInstrumentAction( noteList, drumkitName, instrumentName, instrument );
	HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
}


void MainForm::action_instruments_exportLibrary()
{
	SoundLibraryExportDialog exportDialog( this, QString() );
	exportDialog.exec();
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
	QString sDrumkitName = Hydrogen::get_instance()->getCurrentDrumkitname();
	Drumkit *pDrumkitInfo = nullptr;

	//User drumkit list
	QStringList usr_dks = Filesystem::usr_drumkit_list();
	for (int i = 0; i < usr_dks.size(); ++i) {
		QString absPath = Filesystem::usr_drumkits_dir() + usr_dks[i];
		Drumkit *pInfo = Drumkit::load( absPath, false );
		if (pInfo) {
			if ( QString(pInfo->get_name() ) == sDrumkitName ){
				pDrumkitInfo = pInfo;
				break;
			}
		}
		
		// Since Drumkit::load() calls New Drumkit() internally, we
		// have to take care of destroying it manually.
		delete pInfo;
	}

	// System drumkit list
	QStringList sys_dks = Filesystem::sys_drumkit_list();
	for (int i = 0; i < sys_dks.size(); ++i) {
		QString absPath = Filesystem::sys_drumkits_dir() + sys_dks[i];
		Drumkit *pInfo = Drumkit::load( absPath, false );
		if (pInfo) {
			if ( QString( pInfo->get_name() ) == sDrumkitName ){
				pDrumkitInfo = pInfo;
				break;
			}
		}
		
		// Since Drumkit::load() calls New Drumkit() internally, we
		// have to take care of destroying it manually.
		delete pInfo;
	}

	if ( pDrumkitInfo != nullptr ){
		if( !H2Core::Drumkit::save( QString( pDrumkitInfo->get_name() ),
									QString( pDrumkitInfo->get_author() ),
									QString( pDrumkitInfo->get_info() ),
									QString( pDrumkitInfo->get_license() ),
									QString( pDrumkitInfo->get_image() ),
									QString( pDrumkitInfo->get_image_license() ),
									H2Core::Hydrogen::get_instance()->getSong()->getInstrumentList(),
									H2Core::Hydrogen::get_instance()->getSong()->getComponents(),
									true ) ) {
			QMessageBox::information( this, "Hydrogen", tr ( "Saving of this library failed."));
		}
	}
	else {
		action_instruments_saveAsLibrary();
	}

	//HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();

	// Cleaning up the last pInfo we did not deleted due to the break
	// statement.
	delete pDrumkitInfo;

}


void MainForm::action_instruments_saveAsLibrary()
{
	SoundLibrarySaveDialog dialog( this );
	dialog.exec();
	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->test_expandedItems();
	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
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
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
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

void MainForm::savePreferences() {
	// save window properties in the preferences files
	Preferences *pPreferences = Preferences::get_instance();

	// mainform
	WindowProperties mainFormProp;
	mainFormProp.x = x();
	mainFormProp.y = y();
	mainFormProp.height = height();
	mainFormProp.width = width();
	pPreferences->setMainFormProperties( mainFormProp );

	// Save mixer properties
	WindowProperties mixerProp;
	mixerProp.x = h2app->getMixer()->x();
	mixerProp.y = h2app->getMixer()->y();
	mixerProp.width = h2app->getMixer()->width();
	mixerProp.height = h2app->getMixer()->height();
	mixerProp.visible = h2app->getMixer()->isVisible();
	pPreferences->setMixerProperties( mixerProp );

	// save pattern editor properties
	WindowProperties patternEditorProp;
	patternEditorProp.x = h2app->getPatternEditorPanel()->x();
	patternEditorProp.y = h2app->getPatternEditorPanel()->y();
	patternEditorProp.width = h2app->getPatternEditorPanel()->width();
	patternEditorProp.height = h2app->getPatternEditorPanel()->height();
	patternEditorProp.visible = h2app->getPatternEditorPanel()->isVisible();
	pPreferences->setPatternEditorProperties( patternEditorProp );

	// save song editor properties
	WindowProperties songEditorProp;
	songEditorProp.x = h2app->getSongEditorPanel()->x();
	songEditorProp.y = h2app->getSongEditorPanel()->y();
	songEditorProp.width = h2app->getSongEditorPanel()->width();
	songEditorProp.height = h2app->getSongEditorPanel()->height();

	QSize size = h2app->getSongEditorPanel()->frameSize();
	songEditorProp.visible = h2app->getSongEditorPanel()->isVisible();
	pPreferences->setSongEditorProperties( songEditorProp );


	WindowProperties instrumentRackProp;
	instrumentRackProp.x = h2app->getInstrumentRack()->x();
	instrumentRackProp.y = h2app->getInstrumentRack()->y();
	instrumentRackProp.width = h2app->getInstrumentRack()->width();
	instrumentRackProp.height = h2app->getInstrumentRack()->height();
	instrumentRackProp.visible = h2app->getInstrumentRack()->isVisible();
	pPreferences->setInstrumentRackProperties( instrumentRackProp );

	// save audio engine info properties
	WindowProperties audioEngineInfoProp;
	audioEngineInfoProp.x = h2app->getAudioEngineInfoForm()->x();
	audioEngineInfoProp.y = h2app->getAudioEngineInfoForm()->y();
	audioEngineInfoProp.visible = h2app->getAudioEngineInfoForm()->isVisible();
	pPreferences->setAudioEngineInfoProperties( audioEngineInfoProp );


#ifdef H2CORE_HAVE_LADSPA
	// save LADSPA FX window properties
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		WindowProperties prop;
		prop.x = h2app->getLadspaFXProperties(nFX)->x();
		prop.y = h2app->getLadspaFXProperties(nFX)->y();
		prop.visible= h2app->getLadspaFXProperties(nFX)->isVisible();
		pPreferences->setLadspaProperties(nFX, prop);
	}
#endif
}

void MainForm::closeAll(){
	savePreferences();
	m_pQApp->quit();
}



// keybindings..

void MainForm::onPlayStopAccelEvent()
{
	int nState = Hydrogen::get_instance()->getState();
	switch (nState) {
	case STATE_READY:
		Hydrogen::get_instance()->sequencer_play();
		break;

	case STATE_PLAYING:
		Hydrogen::get_instance()->sequencer_stop();
		break;

	default:
		ERRORLOG( "[MainForm::onPlayStopAccelEvent()] Unhandled case." );
	}
}



void MainForm::onRestartAccelEvent()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->relocate( 0 );
}



void MainForm::onBPMPlusAccelEvent()
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song* pSong = pEngine->getSong();
	pEngine->setBPM( pSong->getBpm() + 0.1 );
	AudioEngine::get_instance()->unlock();
}



void MainForm::onBPMMinusAccelEvent()
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song* pSong = pEngine->getSong();
	pEngine->setBPM( pSong->getBpm() - 0.1 );
	AudioEngine::get_instance()->unlock();
}



void MainForm::onSaveAsAccelEvent()
{
	action_file_save_as();
}



void MainForm::onSaveAccelEvent()
{
	action_file_save();
}



void MainForm::onOpenAccelEvent()
{
	action_file_open();
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
	// When under session management the filename of the current Song
	// has to be preserved.
	const bool bUnderSessionManagement = H2Core::Hydrogen::get_instance()->isUnderSessionManagement();
	
	QString currentFilename;
	if ( bUnderSessionManagement ) {
		currentFilename = H2Core::Hydrogen::get_instance()->getSong()->getFilename();
	}
	
	HydrogenApp::get_instance()->openSong( pAction->text() );
	
	if ( bUnderSessionManagement ) {
		H2Core::Hydrogen::get_instance()->getSong()->setFilename( currentFilename );
	}
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
	Song *pSong = Hydrogen::get_instance()->getSong();
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
	Song *pSong = Hydrogen::get_instance()->getSong();
	pSong->getInstrumentList()->set_default_midi_out_notes();
	pSong->setIsModified( true );

	m_pMidiSetupInfoBar->hide();
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
	int instr = 0;

	///POSIX Locale
	//locale for keyboardlayout QWERTZ
	// de_DE, de_AT, de_LU, de_CH, de

	//locale for keyboardlayout AZERTY
	// fr_BE, fr_CA, fr_FR, fr_LU, fr_CH

	//locale for keyboardlayout QWERTY
	// en_GB, en_US, en_ZA, usw.

	if ( loc.contains( "de" ) || loc.contains( "DE" )){ ///QWERTZ
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_M] = instr++;

		keycodeInstrumentMap[Qt::Key_Q] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}
	else if ( loc.contains( "fr" ) || loc.contains( "FR" )){ ///AZERTY
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_Question] = instr++;

		keycodeInstrumentMap[Qt::Key_A] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}else
	{ /// default QWERTY
		keycodeInstrumentMap[Qt::Key_Z] = instr++;
		keycodeInstrumentMap[Qt::Key_S] = instr++;
		keycodeInstrumentMap[Qt::Key_X] = instr++;
		keycodeInstrumentMap[Qt::Key_D] = instr++;
		keycodeInstrumentMap[Qt::Key_C] = instr++;
		keycodeInstrumentMap[Qt::Key_V] = instr++;
		keycodeInstrumentMap[Qt::Key_G] = instr++;
		keycodeInstrumentMap[Qt::Key_B] = instr++;
		keycodeInstrumentMap[Qt::Key_H] = instr++;
		keycodeInstrumentMap[Qt::Key_N] = instr++;
		keycodeInstrumentMap[Qt::Key_J] = instr++;
		keycodeInstrumentMap[Qt::Key_M] = instr++;

		keycodeInstrumentMap[Qt::Key_Q] = instr++;
		keycodeInstrumentMap[Qt::Key_2] = instr++;
		keycodeInstrumentMap[Qt::Key_W] = instr++;
		keycodeInstrumentMap[Qt::Key_3] = instr++;
		keycodeInstrumentMap[Qt::Key_E] = instr++;
		keycodeInstrumentMap[Qt::Key_R] = instr++;
		keycodeInstrumentMap[Qt::Key_5] = instr++;
		keycodeInstrumentMap[Qt::Key_T] = instr++;
		keycodeInstrumentMap[Qt::Key_6] = instr++;
		keycodeInstrumentMap[Qt::Key_Y] = instr++;
		keycodeInstrumentMap[Qt::Key_7] = instr++;
		keycodeInstrumentMap[Qt::Key_U] = instr++;
	}
}


bool MainForm::eventFilter( QObject *o, QEvent *e )
{
	UNUSED( o );
	if ( e->type() == QEvent::FileOpen ) {
		// Mac OS always opens files (including via double click in Finder) via a FileOpenEvent.
		QFileOpenEvent *fe = dynamic_cast<QFileOpenEvent*>(e);
		assert( fe != nullptr );
		QString sFileName = fe->file();

		if ( sFileName.endsWith( H2Core::Filesystem::songs_ext ) ) {
			if ( handleUnsavedChanges() ) {
				HydrogenApp::get_instance()->openSong( sFileName );
			}

		} else if ( sFileName.endsWith( H2Core::Filesystem::drumkit_ext ) ) {
			H2Core::Drumkit::install( sFileName );

		} else if ( sFileName.endsWith( H2Core::Filesystem::playlist_ext ) ) {
			bool loadlist = HydrogenApp::get_instance()->getPlayListDialog()->loadListByFileName( sFileName );
			if ( loadlist ) {
				H2Core::Playlist::get_instance()->setNextSongByNumber( 0 );
			}
		}
		return true;

	} else if ( e->type() == QEvent::KeyPress ) {
		// special processing for key press
		QKeyEvent *k = (QKeyEvent *)e;

		// qDebug( "Got key press for instrument '%c'", k->ascii() );
		//int songnumber = 0;
		HydrogenApp* app = HydrogenApp::get_instance();
		Hydrogen* pHydrogen = Hydrogen::get_instance();
		switch (k->key()) {
		case Qt::Key_Space:
			onPlayStopAccelEvent();
			return true; // eat event


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

		case  Qt::Key_S | Qt::CTRL:
			onSaveAccelEvent();
			return true;
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
			pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() - 1 );
			return true;
			break;

		case  Qt::Key_F10 : // Qt::Key_Right do not work. Some ideas ?
			pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() + 1 );
			return true;
			break;
		}

		// virtual keyboard handling
		std::map<int,int>::iterator found = keycodeInstrumentMap.find ( k->key() );
		if (found != keycodeInstrumentMap.end()) {
			//			INFOLOG( "[eventFilter] virtual keyboard event" );
			// insert note at the current column in time
			// if event recording enabled
			int row = (*found).second;

			float velocity = 0.8;
			float pan_L = 0.5f;
			float pan_R = 0.5f;

			pHydrogen->addRealtimeNote (row, velocity, pan_L, pan_R, 0, false, false , row + 36);

			return true; // eat event
		}
		else {
			return false; // let it go
		}
	}
	else {
		return false; // standard event processing
	}
}





/// print the object map
void MainForm::action_debug_printObjects()
{
	INFOLOG( "[action_debug_printObjects]" );
	Object::write_objects_map_to_cerr();
}






void MainForm::action_file_export_midi()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	ExportMidiDialog *dialog = new ExportMidiDialog(this);
	dialog->exec();
	delete dialog;
}




void MainForm::action_file_export_lilypond()
{
	if ( ( ( Hydrogen::get_instance() )->getState() == STATE_PLAYING ) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	QMessageBox::information(
		this,
		"Hydrogen",
		tr( "\nThe LilyPond export is an experimental feature.\n"
		"It should work like a charm provided that you use the "
		"GMRockKit, and that you do not use triplet.\n" ),
		QMessageBox::Ok ); 

	QFileDialog fd( this );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( tr( "LilyPond file (*.ly)" ) );
	fd.setDirectory( QDir::homePath() );
	fd.setWindowTitle( tr( "Export LilyPond file" ) );
	fd.setAcceptMode( QFileDialog::AcceptSave );

	QString sFilename;
	if ( fd.exec() == QDialog::Accepted ) {
		sFilename = fd.selectedFiles().first();
	}

	if ( !sFilename.isEmpty() ) {
		if ( sFilename.endsWith( ".ly" ) == false ) {
			sFilename += ".ly";
		}

		Song *pSong = Hydrogen::get_instance()->getSong();

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

	HydrogenApp::get_instance()->setScrollStatusBarMessage( tr( "Playlist: Set song No. %1" ).arg( nIndex +1 ), 5000 );
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
	SongPropertiesDialog *pDialog = new SongPropertiesDialog( this );
	if ( pDialog->exec() == QDialog::Accepted ) {
		Hydrogen::get_instance()->getSong()->setIsModified( true );
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
	//set this to 'false' for the case that you want to make a release..
	if ( true ) {
		Preferences *pPreferences = Preferences::get_instance();
		bool isDevelWarningEnabled = pPreferences->getShowDevelWarning();
		if(isDevelWarningEnabled) {

			QString msg = tr( "You're using a development version of Hydrogen, please help us reporting bugs or suggestions in the hydrogen-devel mailing list.<br><br>Thank you!" );
			QMessageBox develMessageBox( this );
			develMessageBox.setText( msg );
			develMessageBox.addButton( tr( "Ok" ), QMessageBox::YesRole );
			develMessageBox.addButton( tr( "Don't show this message anymore" ) , QMessageBox::AcceptRole );

			if( develMessageBox.exec() == 1 ){
				//don't show warning again
				pPreferences->setShowDevelWarning( false );
			}
		}
	}
}



QString MainForm::getAutoSaveFilename()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	assert( pSong );
	QString sOldFilename = pSong->getFilename();
	QString newName = "autosave.h2song";

	if ( !sOldFilename.isEmpty() ) {
		newName = sOldFilename.left( sOldFilename.length() - 7 ) + ".autosave.h2song";
	}

	return newName;
}



void MainForm::onAutoSaveTimer()
{
	//INFOLOG( "[onAutoSaveTimer]" );
	Song *pSong = Hydrogen::get_instance()->getSong();
	assert( pSong );
	QString sOldFilename = pSong->getFilename();

	pSong->save( getAutoSaveFilename() );

	pSong->setFilename(sOldFilename);
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
	HydrogenApp::get_instance()->setScrollStatusBarMessage( message, 2000 );
}

// Returns true if unsaved changes are successfully handled (saved, discarded, etc.)
// Returns false if not (i.e. Cancel)
bool MainForm::handleUnsavedChanges()
{
	bool done = false;
	bool rv = true;
	while ( !done && Hydrogen::get_instance()->getSong()->getIsModified() ) {
		switch(
				 QMessageBox::information( this, "Hydrogen",
										 tr("\nThe document contains unsaved changes.\n"
												"Do you want to save the changes?\n"),
										 tr("&Save"), tr("&Discard"), tr("&Cancel"),
										 0,      // Enter == button 0
										 2 ) ) { // Escape == button 2
		case 0: // Save clicked or Alt+S pressed or Enter pressed.
			// If the save fails, the __is_modified flag will still be true
			if ( ! Hydrogen::get_instance()->getSong()->getFilename().isEmpty() ) {
				action_file_save();
			} else {
				// never been saved
				action_file_save_as();
			}
			// save
			break;
		case 1: // Discard clicked or Alt+D pressed
			// don't save but exit
			done = true;
			break;
		case 2: // Cancel clicked or Alt+C pressed or Escape pressed
			// don't exit
			done = true;
			rv = false;
			break;
		}
	}

	if(rv != false)
	{
		while ( !done && Playlist::get_instance()->getIsModified() ) {
			switch(
					QMessageBox::information(
								this, 
								"Hydrogen",
								tr("\nThe current playlist contains unsaved changes.\n"
								"Do you want to discard the changes?\n"),
								tr("&Discard"), tr("&Cancel"),
								 nullptr,      // Enter == button 0
								 2 ) ) { // Escape == button 1
			case 0: // Discard clicked or Alt+D pressed
				// don't save but exit
				done = true;
				break;
			case 1: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				done = true;
				rv = false;
				break;
			}
		}
	}


	return rv;
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

void MainForm::action_banks_properties()
{
	QString sDrumkitName = Hydrogen::get_instance()->getCurrentDrumkitname();
	Drumkit *pDrumkitInfo = nullptr;

	//User drumkit list
	QStringList usr_dks = Filesystem::usr_drumkit_list();
	for (int i = 0; i < usr_dks.size(); ++i) {
		QString absPath = Filesystem::usr_drumkits_dir() + usr_dks[i];
		Drumkit *pInfo = Drumkit::load( absPath );
		if (pInfo) {
			if ( QString(pInfo->get_name() ) == sDrumkitName ){
				pDrumkitInfo = pInfo;
				break;
			}
		}
		
		// Since Drumkit::load() calls New Drumkit() internally, we
		// have to take care of destroying it manually.
		delete pInfo;
	}

	//System drumkit list
	QStringList sys_dks = Filesystem::sys_drumkit_list();
	for (int i = 0; i < sys_dks.size(); ++i) {
		QString absPath = Filesystem::sys_drumkits_dir() + sys_dks[i];
		Drumkit *pInfo = Drumkit::load( absPath );
		if (pInfo) {
			if ( QString( pInfo->get_name() ) == sDrumkitName ){
				pDrumkitInfo = pInfo;
				break;
			}
		}
		
		// Since Drumkit::load() calls New Drumkit() internally, we
		// have to take care of destroying it manually.
		delete pInfo;
	}

	if( pDrumkitInfo )
	{
		SoundLibraryPropertiesDialog dialog( this , pDrumkitInfo, pDrumkitInfo );
		dialog.exec();
	}
	else
	{
		QMessageBox::information( this, "Hydrogen", tr("Retrieving information about drumkit '%1' failed: drumkit does not exist.").arg( sDrumkitName ) );
	}

	// Cleaning up the last pInfo we did not deleted due to the break
	// statement.
	delete pDrumkitInfo;
}
