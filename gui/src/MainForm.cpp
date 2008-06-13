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
#include "config.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/playlist.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/smf/SMF.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;


#include "Skin.h"
#include "MainForm.h"
#include "AboutDialog.h"
#include "PlayerControl.h"
#include "ExportSongDialog.h"
#include "Mixer/Mixer.h"
#include "HelpBrowser.h"
#include "DrumkitManager.h"
#include "AudioEngineInfoForm.h"
#include "LadspaFXProperties.h"
#include "SongPropertiesDialog.h"
#include "InstrumentRack.h"

#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SoundLibrary/SoundLibraryImportDialog.h"
#include "SoundLibrary/SoundLibrarySaveDialog.h"
#include "SoundLibrary/SoundLibraryExportDialog.h"
#include "PlaylistEditor/PlaylistDialog.h"

#include <QtGui>

#ifndef WIN32
	#include <sys/time.h>
#endif

#ifdef LASH_SUPPORT
#include <lash-1.0/lash/lash.h>
#include <hydrogen/LashClient.h>
#endif

#include <cassert>

using std::map;
using namespace H2Core;

MainForm::MainForm( QApplication *app, const QString& songFilename )
 : QMainWindow( 0, 0 )
 , Object( "MainForm" )
{
	setMinimumSize( QSize( 1000, 600 ) );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	m_pQApp = app;

	m_pQApp->processEvents();

	// Load default song
	Song *song = NULL;
	if (songFilename != "") {
		song = Song::load( songFilename );
		if (song == NULL) {
			//QMessageBox::warning( this, "Hydrogen", trUtf8("Error loading song.") );
			song = Song::get_empty_song();
			song->set_filename( "" );
		}
	}
	else {
		Preferences *pref = Preferences::getInstance();
		bool restoreLastSong = pref->isRestoreLastSongEnabled();
		QString filename = pref->getLastSongFilename();
		if ( restoreLastSong && (filename != "" )) {
			song = Song::load( filename );
			if (song == NULL) {
				//QMessageBox::warning( this, "Hydrogen", trUtf8("Error restoring last song.") );
				song = Song::get_empty_song();
				song->set_filename( "" );
			}
		}
		else {
			song = Song::get_empty_song();
			song->set_filename( "" );
		}
	}

	h2app = new HydrogenApp( this, song );
	h2app->addEventListener( this );

	createMenuBar();

	h2app->setStatusBarMessage( trUtf8("Hydrogen Ready."), 10000 );

	initKeyInstMap();

	// we need to do all this to support the keyboard playing
	// for all the window modes
	h2app->getMixer()->installEventFilter (this);
	h2app->getPatternEditorPanel()->installEventFilter (this);
	h2app->getSongEditorPanel()->installEventFilter (this);
	h2app->getPlayerControl()->installEventFilter(this);
	InstrumentEditorPanel::getInstance()->installEventFilter(this);
	h2app->getDrumkitManager()->installEventFilter(this);
	h2app->getAudioEngineInfoForm()->installEventFilter(this);

	installEventFilter( this );

	connect(&m_http, SIGNAL(done(bool)), this, SLOT(latestVersionDone(bool)));
	getLatestVersion();


	connect( &m_autosaveTimer, SIGNAL(timeout()), this, SLOT(onAutoSaveTimer()));
	m_autosaveTimer.start( 60 * 1000 );


#ifdef LASH_SUPPORT
	LashClient* lashClient = LashClient::getInstance();
	if (lashClient->isConnected())
	{
		// send alsa client id now since it can only be sent
		// after the audio engine has been started.
		Preferences *pref = Preferences::getInstance();
		if ( pref->m_sMidiDriver == "ALSA" ) {
//			infoLog("[LASH] Sending alsa seq id to LASH server");
			lashClient->sendAlsaClientId();
		}
		// start timer for polling lash events
		lashPollTimer = new QTimer(this);
		connect( lashPollTimer, SIGNAL( timeout() ), this, SLOT( onLashPollTimer() ) );
		lashPollTimer->start(500);
	}
#endif
}



MainForm::~MainForm()
{
	// remove the autosave file
	QFile file( getAutoSaveFilename() );
	file.remove();

	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	// remove the autosave file
	m_autosaveTimer.stop();
	QFile autosaveFile( "hydrogen_autosave.h2song" );
	autosaveFile.remove();

	hide();

	if (h2app != NULL) {
		delete Playlist::get_instance();
		delete h2app;
		h2app = NULL;
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
	QMenu *m_pFileMenu = m_pMenubar->addMenu( trUtf8( "&Project" ) );

	m_pFileMenu->addAction( trUtf8( "&New" ), this, SLOT( action_file_new() ), QKeySequence( "Ctrl+N" ) );
	m_pFileMenu->addAction( trUtf8( "Show info" ), this, SLOT( action_file_songProperties() ), QKeySequence( "" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "&Open" ), this, SLOT( action_file_open() ), QKeySequence( "Ctrl+O" ) );
	m_pFileMenu->addAction( trUtf8( "Open &Demo" ), this, SLOT( action_file_openDemo() ), QKeySequence( "Ctrl+D" ) );

	m_pRecentFilesMenu = m_pFileMenu->addMenu( trUtf8( "Open &recent" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "&Save" ), this, SLOT( action_file_save() ), QKeySequence( "Ctrl+S" ) );
	m_pFileMenu->addAction( trUtf8( "Save &as..." ), this, SLOT( action_file_save_as() ), QKeySequence( "Ctrl+Shift+S" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction ( trUtf8 ( "Open &Pattern" ), this, SLOT ( action_file_openPattern() ), QKeySequence ( "" ) );
	m_pFileMenu->addAction( trUtf8( "Export pattern as..." ), this, SLOT( action_file_export_pattern_as() ), QKeySequence( "Ctrl+P" ) );

	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8( "Export &MIDI file" ), this, SLOT( action_file_export_midi() ), QKeySequence( "Ctrl+M" ) );
	m_pFileMenu->addAction( trUtf8( "&Export song" ), this, SLOT( action_file_export() ), QKeySequence( "Ctrl+E" ) );


#ifndef Q_OS_MACX
	m_pFileMenu->addSeparator();				// -----

	m_pFileMenu->addAction( trUtf8("&Quit"), this, SLOT( action_file_exit() ), QKeySequence( "Ctrl+Q" ) );
#endif

	updateRecentUsedSongList();
	connect( m_pRecentFilesMenu, SIGNAL( triggered(QAction*) ), this, SLOT( action_file_open_recent(QAction*) ) );
	//~ FILE menu


	// INSTRUMENTS MENU
	QMenu *m_pInstrumentsMenu = m_pMenubar->addMenu( trUtf8( "Instruments" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "Add instrument" ), this, SLOT( action_instruments_addInstrument() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "Clear all" ), this, SLOT( action_instruments_clearAll() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "Save library" ), this, SLOT( action_instruments_saveLibrary() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "Export library" ), this, SLOT( action_instruments_exportLibrary() ), QKeySequence( "" ) );
	m_pInstrumentsMenu->addAction( trUtf8( "Import library" ), this, SLOT( action_instruments_importLibrary() ), QKeySequence( "" ) );




	// Tools menu
	QMenu *m_pToolsMenu = m_pMenubar->addMenu( trUtf8( "&Tools" ));

//	if ( Preferences::getInstance()->getInterfaceMode() == Preferences::SINGLE_PANED ) {
//		m_pWindowMenu->addAction( trUtf8("Show song editor"), this, SLOT( action_window_showSongEditor() ), QKeySequence( "" ) );
//	}
	m_pToolsMenu->addAction( trUtf8("Playlist editor"), this, SLOT( action_window_showPlaylistDialog() ), QKeySequence( "" ) );

	m_pToolsMenu->addAction( trUtf8("&Mixer"), this, SLOT( action_window_showMixer() ), QKeySequence( "Alt+M" ) );

	m_pToolsMenu->addAction( trUtf8("&Instrument Rack"), this, SLOT( action_window_showDrumkitManagerPanel() ), QKeySequence( "Alt+I" ) );
	m_pToolsMenu->addAction( trUtf8("&Preferences"), this, SLOT( showPreferencesDialog() ), QKeySequence( "Alt+P" ) );

	m_pToolsMenu->addSeparator();				// -----
	m_pToolsMenu->addAction( trUtf8("OLD &drumkit manager (Obsolete)"), this, SLOT( action_window_showDrumkitManager() ), QKeySequence( "Alt+D" ) );
	//~ Tools menu


	if ( Object::is_using_verbose_log() ) {
		// DEBUG menu
		QMenu *m_pDebugMenu = m_pMenubar->addMenu( trUtf8("De&bug") );
		m_pDebugMenu->addAction( trUtf8( "Show &audio engine info" ), this, SLOT( action_debug_showAudioEngineInfo() ) );
		m_pDebugMenu->addAction( trUtf8( "debug action" ), this, SLOT( action_debug_debugCommand() ) );
		m_pDebugMenu->addAction( trUtf8( "Print Objects" ), this, SLOT( action_debug_printObjects() ) );
		//~ DEBUG menu
	}

	// INFO menu
	QMenu *m_pInfoMenu = m_pMenubar->addMenu( trUtf8( "&Info" ) );
	m_pInfoMenu->addAction( trUtf8("&User manual"), this, SLOT( showUserManual() ), QKeySequence( "Ctrl+?" ) );
	m_pInfoMenu->addSeparator();
	m_pInfoMenu->addAction( trUtf8("&About"), this, SLOT( action_help_about() ), QKeySequence( trUtf8("", "Info|About") ) );
	//~ INFO menu
}




void MainForm::onLashPollTimer()
{
#ifdef LASH_SUPPORT	
	LashClient* client = LashClient::getInstance();
	
	if (!client->isConnected())
	{
		WARNINGLOG("[LASH] Not connected to server!");
		return;
	}
	
	bool keep_running = true;

	lash_event_t* event;

	string songFilename = "";
	QString filenameSong = "";
	Song *song = Hydrogen::get_instance()->getSong();
	while (event = client->getNextEvent()) {
		
		switch (lash_event_get_type(event)) {
			
			case LASH_Save_File:
		
				INFOLOG("[LASH] Save file");
			
				songFilename.append(lash_event_get_string(event));
				songFilename.append("/hydrogen.h2song"); 
				
				filenameSong = songFilename.c_str();
				song->set_filename( filenameSong );
				action_file_save();
			  
				client->sendEvent(LASH_Save_File);
			  
				break;
		
			case LASH_Restore_File:
		
				songFilename.append(lash_event_get_string(event));
				songFilename.append("/hydrogen.h2song"); 
				
				INFOLOG("[LASH] Restore file: " + to_string( songFilename ));

				filenameSong = songFilename.c_str();
							 
				openSongFile( filenameSong );
			 
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
#endif
}


/// return true if the app needs to be closed.
bool MainForm::action_file_exit()
{
	if ( Hydrogen::get_instance()->getSong()->__is_modified ) {
		int res = QMessageBox::information(
				this,
				"Hydrogen",
				trUtf8("\nThe song has unsaved changes\n Do you want to save the changes before exiting?\n"),
				trUtf8("&Save"),
				trUtf8("&Discard"),
				trUtf8("&Cancel"),
				0,	// Enter == button 0
				2 	// Escape == button 2
		);
		switch( res ) {
			case 0:
				// Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::get_instance()->getSong()->get_filename() != "") {
					action_file_save();
				} else {
					action_file_save_as();
				}
				// save
				break;
			case 1:
				// Discard clicked or Alt+D pressed
				// don't save but exit
				break;
			case 2:
				// Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				return false;
				break;
			default:
				ERRORLOG( "Unknown return code: " + to_string( res ) );
		}
	}

	closeAll();
	return true;
}



void MainForm::action_file_new()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	if ( Hydrogen::get_instance()->getSong()->__is_modified ) {
		switch(
				QMessageBox::information( this, "Hydrogen",
						trUtf8("\nThe document contains unsaved changes\n"
						"Do you want to save the changes before exiting?\n"),
						trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
						0,      // Enter == button 0
						2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::get_instance()->getSong()->get_filename() != "") {
					action_file_save();
				} else {
					// never been saved
					action_file_save_as();
				}
				// save
				break;
			case 1: // Discard clicked or Alt+D pressed
				// don't save but exit
				break;
			case 2: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				return;
				break;
		}
	}

	Song * song = Song::get_empty_song();
	song->set_filename( "" );
	h2app->setSong(song);
 	Hydrogen::get_instance()->setSelectedPatternNumber( 0 );
}



void MainForm::action_file_save_as()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode( QFileDialog::AnyFile );
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Save song" ) );

	Song *song = Hydrogen::get_instance()->getSong();
	QString defaultFilename;
	QString lastFilename = song->get_filename();

	if (lastFilename == "") {
		defaultFilename = Hydrogen::get_instance()->getSong()->__name;
		defaultFilename += ".h2song";
	}
	else {
		defaultFilename = lastFilename;
	}

	fd->selectFile( defaultFilename );

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}

	if (filename != "") {
		QString sNewFilename = filename;
		if ( sNewFilename.endsWith(".h2song") == false ) {
			filename += ".h2song";
		}

		song->set_filename(filename);
		action_file_save();
	}
	h2app->setStatusBarMessage( trUtf8("Song saved."), 10000 );
}



void MainForm::action_file_save()
{
//	if ( ((Hydrogen::get_instance())->getState() == STATE_PLAYING) ) {
//		(Hydrogen::get_instance())->stop();
//	}

	Song *song = Hydrogen::get_instance()->getSong();
	QString filename = song->get_filename();

	if (filename == "") {
		// just in case!
		return action_file_save_as();
	}

	LocalFileMng mng;
	bool saved = false;
	saved = song->save( filename );

	if(! saved) {
		QMessageBox::warning( this, "Hydrogen", trUtf8("Could not save song.") );
	} else {
		Preferences::getInstance()->setLastSongFilename( song->get_filename() );

		// add the new loaded song in the "last used song" vector
		Preferences *pPref = Preferences::getInstance();
		vector<QString> recentFiles = pPref->getRecentFiles();
		recentFiles.insert( recentFiles.begin(), filename );
		pPref->setRecentFiles( recentFiles );

		updateRecentUsedSongList();

		h2app->setStatusBarMessage( trUtf8("Song saved."), 10000 );
	}
}




void MainForm::action_help_about() {
	//QWidget *parent = this;
//	if (workspace) {
//		parent = workspace;
//	}

	// show modal dialog
	AboutDialog *dialog = new AboutDialog( NULL );
	dialog->exec();
}




void MainForm::showUserManual()
{
	h2app->getHelpBrowser()->hide();
	h2app->getHelpBrowser()->show();
}


void MainForm::action_file_export_pattern_as()
{
	if ( ( Hydrogen::get_instance()->getState() == STATE_PLAYING ) )
	{
		Hydrogen::get_instance()->sequencer_stop();
	}

	Hydrogen *engine = Hydrogen::get_instance();
	int selectedpattern = engine->getSelectedPatternNumber();
	Song *song = engine->getSong();
	Pattern *pat = song->get_pattern_list()->get ( selectedpattern );

	Instrument *instr = song->get_instrument_list()->get ( 0 );
	assert ( instr );

	QString sDataDir = Preferences::getInstance()->getDataDirectory();


	QDir dir ( sDataDir + "patterns" );


	QFileDialog *fd = new QFileDialog ( this );
	fd->setFileMode ( QFileDialog::AnyFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Pattern (*.h2pattern)" ) );
	fd->setAcceptMode ( QFileDialog::AcceptSave );
	fd->setWindowTitle ( trUtf8 ( "Save Pattern as ..." ) );
	fd->setDirectory ( dir );



	QString defaultPatternname = QString ( pat->get_name() );

	fd->selectFile ( defaultPatternname );

	LocalFileMng fileMng;
	QString filename = "";
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
	}

	if ( filename != "" )
	{
		QString sNewFilename = filename;
		sNewFilename += ".h2pattern";
		QString patternname = sNewFilename;
		QString realpatternname = filename;
		QString realname = realpatternname.mid( realpatternname.lastIndexOf( "/" ) + 1 );
		pat->set_name(realname);
		HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
		int err = fileMng.savePattern ( song , selectedpattern, patternname, realname, 2 );
		if ( err != 0 )
		{
			_ERRORLOG ( "Error saving the pattern" );
		}
	}
	h2app->setStatusBarMessage ( trUtf8 ( "Pattern saved." ), 10000 );
}

void MainForm::action_file_open() {
	if ( ((Hydrogen::get_instance())->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	if ( Hydrogen::get_instance()->getSong()->__is_modified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::get_instance()->getSong()->get_filename() != "") {
					action_file_save();
				}
				else {
					action_file_save_as();
				}
				// save
				break;
			case 1: // Discard clicked or Alt+D pressed
				// don't save but exit
				break;
			case 2: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				return;
				break;
		}
	}

	
	 

	

	static QString lastUsedDir = Preferences::getInstance()->getDataDirectory() + "songs";
	

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
	fd->setDirectory( lastUsedDir );

	fd->setWindowTitle( trUtf8( "Open song" ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
		lastUsedDir = fd->directory().absolutePath();
	}


	if (filename != "") {
		openSongFile( filename );
	}
}


void MainForm::action_file_openPattern()
{

	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();

	Instrument *instr = song->get_instrument_list()->get ( 0 );
	assert ( instr );

	QString sDataDir = Preferences::getInstance()->getDataDirectory();

	QDir dirPattern ( sDataDir + "patterns" );
	ERRORLOG( sDataDir );
	QFileDialog *fd = new QFileDialog ( this );
	fd->setFileMode ( QFileDialog::ExistingFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Song (*.h2pattern)" ) );
	fd->setDirectory ( dirPattern );

	fd->setWindowTitle ( trUtf8 ( "Open Pattern" ) );


	QString filename = "";
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
	}
	QString patternname = filename;


	LocalFileMng mng;
	LocalFileMng fileMng;
	Pattern* err = fileMng.loadPattern ( patternname );
	if ( err == 0 )
	{
		_ERRORLOG( "Error loading the pattern" );
		_ERRORLOG( patternname );
	}
	else
	{
		H2Core::Pattern *pNewPattern = err;
		pPatternList->add ( pNewPattern );
		song->__is_modified = true;
	}

	HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
}

/// \todo parametrizzare il metodo action_file_open ed eliminare il seguente...
void MainForm::action_file_openDemo()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	if ( ( Hydrogen::get_instance()->getSong())->__is_modified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::get_instance()->getSong()->get_filename() != "") {
					action_file_save_as();
				}
				else {
					action_file_save_as();
				}
				// save
				break;
			case 1: // Discard clicked or Alt+D pressed
				// don't save but exit
				break;
			case 2: // Cancel clicked or Alt+C pressed or Escape pressed
				// don't exit
				return;
				break;
		}
	}


	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );

	fd->setWindowTitle( trUtf8( "Open song" ) );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/
	fd->setDirectory( QString( Preferences::getInstance()->getDemoPath() ) );


	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}


	if (filename != "") {
		openSongFile( filename );
		Hydrogen::get_instance()->getSong()->set_filename( "" );
	}
}



void MainForm::showPreferencesDialog()
{
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	h2app->showPreferencesDialog();
}


void MainForm::action_window_showPlaylistDialog()
{
	PlaylistDialog dialog(this);
	dialog.exec();
	
}

void MainForm::action_window_showMixer()
{
	bool isVisible = HydrogenApp::getInstance()->getMixer()->isVisible();
	h2app->showMixer( !isVisible );
}




void MainForm::action_debug_showAudioEngineInfo()
{
	h2app->showAudioEngineInfoForm();
}



///
/// Shows the song editor
///
void MainForm::action_window_showSongEditor()
{
	bool isVisible = h2app->getSongEditorPanel()->isVisible();
	h2app->getSongEditorPanel()->setHidden( isVisible );
}



void MainForm::action_instruments_addInstrument()
{
	AudioEngine::get_instance()->lock("MainForm::action_instruments_addInstrument");
	InstrumentList* pList = Hydrogen::get_instance()->getSong()->get_instrument_list();

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pList->get_size(); ++i ) {
		Instrument* pInstr = pList->get( i );
		if ( pInstr->get_id().toInt() > nID ) {
			nID = pInstr->get_id().toInt();
		}
	}
	++nID;

	Instrument *pNewInstr = new Instrument(to_string( nID ), "New instrument", new ADSR());
	pList->add( pNewInstr );
	AudioEngine::get_instance()->unlock();

	Hydrogen::get_instance()->setSelectedInstrumentNumber( pList->get_size() - 1 );

	// Force an update
	//EventQueue::getInstance()->pushEvent( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



void MainForm::action_instruments_clearAll()
{
	switch(
	       QMessageBox::information( this,
					 "Hydrogen",
					 trUtf8("Clear all instruments?"),
					 trUtf8("Ok"),
					 trUtf8("Cancel"),
					 0,      // Enter == button 0
					 1 )) { // Escape == button 2
	case 0:
	  // ok btn pressed
	  break;
	case 1:
	  // cancel btn pressed
	  return;
	}

	// Remove all layers
	AudioEngine::get_instance()->lock("MainForm::action_instruments_clearAll");
	Song *pSong = Hydrogen::get_instance()->getSong();
	InstrumentList* pList = pSong->get_instrument_list();
	for (uint i = 0; i < pList->get_size(); i++) {
		Instrument* pInstr = pList->get( i );
		pInstr->set_name( (QString( trUtf8( "Instrument %1" ) ).arg( i + 1 )) );
		// remove all layers
		for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer* pLayer = pInstr->get_layer( nLayer );
			delete pLayer;
			pInstr->set_layer( NULL, nLayer );
		}
	}
	AudioEngine::get_instance()->unlock();
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



void MainForm::action_instruments_exportLibrary()
{
        SoundLibraryExportDialog exportDialog( this );
 	exportDialog.exec();
}




void MainForm::action_instruments_importLibrary()
{
  SoundLibraryImportDialog dialog( this );
  dialog.exec();
}



void MainForm::action_instruments_saveLibrary()
{
  SoundLibrarySaveDialog dialog( this );
  dialog.exec();

  HydrogenApp::getInstance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
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



void MainForm::action_file_export() {
	if ( (Hydrogen::get_instance()->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	ExportSongDialog *dialog = new ExportSongDialog(this);
	dialog->exec();
	delete dialog;
}



void MainForm::action_window_showDrumkitManager()
{
	bool isVisible = h2app->getDrumkitManager()->isVisible();
	h2app->getDrumkitManager()->setHidden( isVisible );
}



void MainForm::action_window_showDrumkitManagerPanel()
{
	InstrumentRack *pPanel = HydrogenApp::getInstance()->getInstrumentRack();
	pPanel->setHidden( pPanel->isVisible() );
}




void MainForm::closeAll() {

	// save window properties in the preferences files
	Preferences *pref = Preferences::getInstance();

	// mainform
	WindowProperties mainFormProp;
	mainFormProp.x = x();
	mainFormProp.y = y();
	mainFormProp.height = height();
	mainFormProp.width = width();
	pref->setMainFormProperties( mainFormProp );

	// Save mixer properties
	WindowProperties mixerProp;
	mixerProp.x = h2app->getMixer()->x();
	mixerProp.y = h2app->getMixer()->y();
	mixerProp.width = h2app->getMixer()->width();
	mixerProp.height = h2app->getMixer()->height();
	mixerProp.visible = h2app->getMixer()->isVisible();
	pref->setMixerProperties( mixerProp );

	// save pattern editor properties
	WindowProperties patternEditorProp;
	patternEditorProp.x = h2app->getPatternEditorPanel()->x();
	patternEditorProp.y = h2app->getPatternEditorPanel()->y();
	patternEditorProp.width = h2app->getPatternEditorPanel()->width();
	patternEditorProp.height = h2app->getPatternEditorPanel()->height();
	patternEditorProp.visible = h2app->getPatternEditorPanel()->isVisible();
	pref->setPatternEditorProperties( patternEditorProp );

	// save song editor properties
	WindowProperties songEditorProp;
	songEditorProp.x = h2app->getSongEditorPanel()->x();
	songEditorProp.y = h2app->getSongEditorPanel()->y();
	songEditorProp.width = h2app->getSongEditorPanel()->width();
	songEditorProp.height = h2app->getSongEditorPanel()->height();

	QSize size = h2app->getSongEditorPanel()->frameSize();
	songEditorProp.visible = h2app->getSongEditorPanel()->isVisible();
	pref->setSongEditorProperties( songEditorProp );


	// save drumkit manager properties
	WindowProperties drumkitMngProp;
	drumkitMngProp.x = h2app->getDrumkitManager()->x();
	drumkitMngProp.y = h2app->getDrumkitManager()->y();
	drumkitMngProp.width = h2app->getDrumkitManager()->width();
	drumkitMngProp.height = h2app->getDrumkitManager()->height();
	drumkitMngProp.visible = h2app->getDrumkitManager()->isVisible();
	pref->setDrumkitManagerProperties( drumkitMngProp );


	// save audio engine info properties
	WindowProperties audioEngineInfoProp;
	audioEngineInfoProp.x = h2app->getAudioEngineInfoForm()->x();
	audioEngineInfoProp.y = h2app->getAudioEngineInfoForm()->y();
	audioEngineInfoProp.visible = h2app->getAudioEngineInfoForm()->isVisible();
	pref->setAudioEngineInfoProperties( audioEngineInfoProp );


#ifdef LADSPA_SUPPORT
	// save LADSPA FX window properties
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		WindowProperties prop;
		prop.x = h2app->getLadspaFXProperties(nFX)->x();
		prop.y = h2app->getLadspaFXProperties(nFX)->y();
		prop.visible= h2app->getLadspaFXProperties(nFX)->isVisible();
		pref->setLadspaProperties(nFX, prop);
	}
#endif

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
	Hydrogen* pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( 0 );
}



void MainForm::onBPMPlusAccelEvent()
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( "MainForm::onBPMPlusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm  < 300) {
		pEngine->setBPM( pSong->__bpm + 0.1 );
	}
	AudioEngine::get_instance()->unlock();
}



void MainForm::onBPMMinusAccelEvent()
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioEngine::get_instance()->lock( "MainForm::onBPMMinusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->__bpm > 40 ) {
		pEngine->setBPM( pSong->__bpm - 0.1 );
	}
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



void MainForm::onTapTempoAccelEvent()
{
#ifndef WIN32
	INFOLOG( "tap tempo" );
	static timeval oldTimeVal;

	struct timeval now;
	gettimeofday(&now, NULL);

	float fInterval = (now.tv_sec - oldTimeVal.tv_sec) * 1000.0 + (now.tv_usec - oldTimeVal.tv_usec) / 1000.0;

	oldTimeVal = now;

	if ( fInterval < 1000.0 ) {
		Hydrogen::get_instance()->setTapTempo( fInterval );
	}
#endif
}



void MainForm::updateRecentUsedSongList()
{
	m_pRecentFilesMenu->clear();

	Preferences *pPref = Preferences::getInstance();
	vector<QString> recentUsedSongs = pPref->getRecentFiles();

	QString sFilename = "";

	for ( uint i = 0; i < recentUsedSongs.size(); ++i ) {
		sFilename = recentUsedSongs[ i ];

		if ( sFilename != "" ) {
			QAction *pAction = new QAction( this  );
			pAction->setText( sFilename );
			m_pRecentFilesMenu->addAction( pAction );
		}
	}
}



void MainForm::action_file_open_recent(QAction *pAction)
{
//	INFOLOG( pAction->text() );
	openSongFile( pAction->text() );
}



void MainForm::openSongFile( const QString& sFilename )
{
 	Hydrogen *engine = Hydrogen::get_instance();
	if ( engine->getState() == STATE_PLAYING ) {
                engine->sequencer_stop();
	}

	LocalFileMng mng;
	Song *pSong = Song::load( sFilename );
	if ( pSong == NULL ) {
		QMessageBox::information( this, "Hydrogen", trUtf8("Error loading song.") );
		return;
	}

	// add the new loaded song in the "last used song" vector
	Preferences *pPref = Preferences::getInstance();
	vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), sFilename );
	pPref->setRecentFiles( recentFiles );

	h2app->setSong( pSong );

	updateRecentUsedSongList();
	engine->setSelectedPatternNumber( 0 );
}



void MainForm::initKeyInstMap()
{
	int instr = 0;
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


	/*
	// QWERTY etc.... rows of the keyboard
	keycodeInstrumentMap[Qt::Key_Q] = instr++;
	keycodeInstrumentMap[Qt::Key_W] = instr++;
	keycodeInstrumentMap[Qt::Key_E] = instr++;
	keycodeInstrumentMap[Qt::Key_R] = instr++;
	keycodeInstrumentMap[Qt::Key_T] = instr++;
	keycodeInstrumentMap[Qt::Key_Y] = instr++;
	keycodeInstrumentMap[Qt::Key_U] = instr++;
	keycodeInstrumentMap[Qt::Key_I] = instr++;
	keycodeInstrumentMap[Qt::Key_O] = instr++;
	keycodeInstrumentMap[Qt::Key_P] = instr++;
	keycodeInstrumentMap[Qt::Key_BracketLeft] = instr++;
	keycodeInstrumentMap[Qt::Key_BracketRight] = instr++;
	keycodeInstrumentMap[Qt::Key_A] = instr++;
	keycodeInstrumentMap[Qt::Key_S] = instr++;
	keycodeInstrumentMap[Qt::Key_D] = instr++;
	keycodeInstrumentMap[Qt::Key_F] = instr++;
	keycodeInstrumentMap[Qt::Key_G] = instr++;
	keycodeInstrumentMap[Qt::Key_H] = instr++;
	keycodeInstrumentMap[Qt::Key_J] = instr++;
	keycodeInstrumentMap[Qt::Key_K] = instr++;
	keycodeInstrumentMap[Qt::Key_L] = instr++;
	keycodeInstrumentMap[Qt::Key_Semicolon] = instr++;
	keycodeInstrumentMap[Qt::Key_Apostrophe] = instr++;
	keycodeInstrumentMap[Qt::Key_Z] = instr++;
	keycodeInstrumentMap[Qt::Key_X] = instr++;
	keycodeInstrumentMap[Qt::Key_C] = instr++;
	keycodeInstrumentMap[Qt::Key_V] = instr++;
	keycodeInstrumentMap[Qt::Key_B] = instr++;
	keycodeInstrumentMap[Qt::Key_N] = instr++;
	keycodeInstrumentMap[Qt::Key_M] = instr++;
	keycodeInstrumentMap[Qt::Key_Comma] = instr++;
	keycodeInstrumentMap[Qt::Key_Period] = instr++;
*/
}



bool MainForm::eventFilter( QObject *o, QEvent *e )
{
	UNUSED( o );

	if ( e->type() == QEvent::KeyPress) {
		// special processing for key press
		QKeyEvent *k = (QKeyEvent *)e;

		// qDebug( "Got key press for instrument '%c'", k->ascii() );

		switch (k->key()) {
			case Qt::Key_Space:
				if ( Preferences::getInstance()->m_spacebeatcounter == Preferences::SPACE_BEATCOUNTER_ON &&  Hydrogen::get_instance()->getState() == STATE_READY )
				{
					Hydrogen::get_instance()->handleBeatCounter();
					return TRUE; // eat event
				}
				else
				{
					onPlayStopAccelEvent();
					return TRUE; // eat event
				}
				break;

			case Qt::Key_Comma:
				Hydrogen::get_instance()->handleBeatCounter();
				return TRUE; // eat even
				break;

			case Qt::Key_Backspace:
				onRestartAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Plus:
				onBPMPlusAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Minus:
				onBPMMinusAccelEvent();
				return TRUE; // eat event
				break;

			case Qt::Key_Backslash:
				onTapTempoAccelEvent();
				return TRUE; // eat event
				break;

			case  Qt::Key_S | Qt::CTRL:
				onSaveAccelEvent();
				return TRUE;
				break;
			
			case  Qt::Key_PageUp :
				if( Hydrogen::get_instance()->m_PlayList.size() == 0)
					break;
				Playlist::get_instance()->setPrevSongPlaylist();
				return TRUE;
				break;

			case  Qt::Key_PageDown :
				if( Hydrogen::get_instance()->m_PlayList.size() == 0)
					break;
				Playlist::get_instance()->setNextSongPlaylist();
				return TRUE;
				break;
			
		// 	QAccel *a = new QAccel( this );
// 	a->connectItem( a->insertItem(Key_S + CTRL), this, SLOT( onSaveAccelEvent() ) );
// 	a->connectItem( a->insertItem(Key_O + CTRL), this, SLOT( onOpenAccelEvent() ) );

		}

		// virtual keyboard handling
		map<int,int>::iterator found = keycodeInstrumentMap.find ( k->key() );
		if (found != keycodeInstrumentMap.end()) {
//			INFOLOG( "[eventFilter] virtual keyboard event" );
			// insert note at the current column in time
			// if event recording enabled
			int row = (*found).second;
			Hydrogen* engine = Hydrogen::get_instance();

			float velocity = 0.8;
			float pan_L = 1.0;
			float pan_R = 1.0;

			engine->addRealtimeNote (row, velocity, pan_L, pan_R);

			return TRUE; // eat event
		}
		else {
			return FALSE; // let it go
		}
        }
	else {
		return FALSE; // standard event processing
        }
}





/// print the object map
void MainForm::action_debug_printObjects()
{
	INFOLOG( "[action_debug_printObjects]" );
	Object::print_object_map();
}



/// Riceve un comando testuale ed esegue una azione di debug
void MainForm::action_debug_debugCommand()
{
	INFOLOG( "[action_debug_debugCommand]" );

	static QString sLastCommand = "";
	bool bIsOkPressed;
	QString cmd = QInputDialog::getText( this, "Hydrogen", "Command", QLineEdit::Normal, sLastCommand, &bIsOkPressed );
	if (bIsOkPressed) {
		sLastCommand = cmd;

		if ( cmd == "print current pattern list" ) {
			Hydrogen *pEngine = Hydrogen::get_instance();

			std::cout << "*** print current pattern list ***" << std::endl;

			std::cout << "Pattern pos: " << pEngine->getPatternPos() << std::endl;

			std::cout << std::endl;

			std::cout << "----------------------------------------------------------------------" << std::endl;
			std::cout << "Song pattern list" << std::endl;
			std::cout << "----------------------------------------------------------------------" << std::endl;
			PatternList *pSongPatternList = pEngine->getSong()->get_pattern_list();
			for ( uint i = 0; i <pSongPatternList->get_size(); i++ ) {
				H2Core::Pattern *pPat = pSongPatternList->get( i );
				std::cout << "   |->[" << i << "] " << pPat->get_name().toStdString() << std::endl;
			}
			std::cout << "----------------------------------------------------------------------" << std::endl;

			std::cout << std::endl;

			cout << "----------------------------------------------------------------------" << std::endl;
			cout << "Current pattern list" << std::endl;
			cout << "----------------------------------------------------------------------" << std::endl;
			PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
			for ( uint i = 0; i <pCurrentPatternList->get_size(); i++ ) {
				H2Core::Pattern *pPat = pCurrentPatternList->get( i );
				cout << "   |->[" << i << "] " << pPat->get_name().toStdString() << std::endl;
			}
			cout << "----------------------------------------------------------------------" << std::endl;
		}
		else if ( cmd == "crash" ) {
			Song *pBadPointer = NULL;
			pBadPointer->__bpm = 1000000;
		}
		else {
			QMessageBox::warning( this, "Hydrogen", "action not found" );
		}

	}

}



void MainForm::action_file_export_midi()
{
	if ( ((Hydrogen::get_instance())->getState() == STATE_PLAYING) ) {
		Hydrogen::get_instance()->sequencer_stop();
	}

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Midi file (*.mid)") );
	fd->setDirectory( QDir::homePath() );
	fd->setWindowTitle( trUtf8( "Export MIDI file" ) );
	fd->setAcceptMode( QFileDialog::AcceptSave );
//	fd->setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	QString sFilename = "";
	if ( fd->exec() == QDialog::Accepted ) {
		sFilename = fd->selectedFiles().first();
	}

	if ( sFilename != "" ) {
		if ( sFilename.endsWith(".mid") == false ) {
			sFilename += ".mid";
		}

		Song *pSong = Hydrogen::get_instance()->getSong();

		// create the Standard Midi File object
		SMFWriter *pSmfWriter = new SMFWriter();
		pSmfWriter->save( sFilename, pSong );

		delete pSmfWriter;
	}
}



void MainForm::errorEvent( int nErrorCode )
{
	//ERRORLOG( "[errorEvent]" );

	QString msg;
	switch (nErrorCode) {
		case Hydrogen::UNKNOWN_DRIVER:
			msg = trUtf8( "Unknown audio driver" );
			break;

		case Hydrogen::ERROR_STARTING_DRIVER:
			msg = trUtf8( "Error starting audio driver" );
			break;

		case Hydrogen::JACK_SERVER_SHUTDOWN:
			msg = trUtf8( "Jack driver: server shutdown" );
			break;

		case Hydrogen::JACK_CANNOT_ACTIVATE_CLIENT:
			msg = trUtf8( "Jack driver: cannot activate client" );
			break;

		case Hydrogen::JACK_CANNOT_CONNECT_OUTPUT_PORT:
			msg = trUtf8( "Jack driver: cannot connect output port" );
			break;

		case Hydrogen::JACK_ERROR_IN_PORT_REGISTER:
			msg = trUtf8( "Jack driver: error in port register" );
			break;

		default:
			msg = QString( trUtf8( "Unknown error %1" ) ).arg( nErrorCode );
	}
	QMessageBox::information( this, "Hydrogen", msg );
}


void MainForm::action_file_songProperties()
{
	SongPropertiesDialog *pDialog = new SongPropertiesDialog( this );
	if ( pDialog->exec() == QDialog::Accepted ) {
		Hydrogen::get_instance()->getSong()->__is_modified = true;
	}
	delete pDialog;
}


void MainForm::action_window_showPatternEditor()
{
	bool isVisible = HydrogenApp::getInstance()->getPatternEditorPanel()->isVisible();
	HydrogenApp::getInstance()->getPatternEditorPanel()->setHidden( isVisible );
}


///
/// Retrieve from the website the latest version available.
///
/// Warning: Hydrogen is not a spyware!!
/// Hydrogen sends only the current version and the OS used in order to let possible
/// to use an auto-updater in the future (this feature is not ready yet).
///
/// *** No user data will be stored in the server ***
///
void MainForm::getLatestVersion()
{
	#if defined( Q_OS_MACX )
	QString os = "Mac";
	#elif defined( Q_OS_WIN32 )
	QString os = "Windows";
	#elif defined( Q_OS_WIN64 )
	QString os = "Windows64";
	#elif defined( Q_OS_LINUX )
	QString os = "Linux";
	#elif defined( Q_OS_FREEBSD )
	QString os = "FreeBSD";
	#elif defined( Q_OS_UNIX )
	QString os = "Unix";
	#else
	QString os = "Unknown";
	#endif


	QString sRequest = QString("/getLatestVersion.php?UsingVersion=%1").arg(VERSION.c_str());
	sRequest += QString( "&OS=%1" ).arg( os );

	//INFOLOG( sRequest );

	QHttpRequestHeader header( "GET", sRequest );
	header.setValue( "Host", "www.hydrogen-music.org" );

	m_http.setHost( "www.hydrogen-music.org" );
	m_http.request( header );
}



void MainForm::latestVersionDone(bool bError)
{
	if ( bError ) {
		INFOLOG( "[MainForm::latestVersionDone] Error." );
		return;
	}

	QString sLatestVersion( m_http.readAll() );
	sLatestVersion = sLatestVersion.simplified();
	QString sLatest_major = sLatestVersion.section( '.', 0, 0 );
	QString sLatest_minor = sLatestVersion.section( '.', 1, 1 );
	QString sLatest_micro = sLatestVersion.section( '.', 2, 2 );
//	INFOLOG( "Latest available version is: " + QString( sLatestVersion.ascii() ) );

	QString sCurrentVersion = VERSION.c_str();
	QString sCurrent_major = sCurrentVersion.section( '.', 0, 0 );
	QString sCurrent_minor = sCurrentVersion.section( '.', 1, 1 );
	QString sCurrent_micro = sCurrentVersion.section( '.', 2, 2 );
	if ( sCurrent_micro.section( '-', 0, 0 ) != "" ) {
		sCurrent_micro = sCurrent_micro.section( '-', 0, 0 );
	}

	bool bExistsNewVersion = false;
	if ( sLatest_major > sCurrent_major ) {
		bExistsNewVersion = true;
	}
	else if ( sLatest_minor > sCurrent_minor ) {
			bExistsNewVersion = true;
	}
	else if ( sLatest_micro > sCurrent_micro ) {
		bExistsNewVersion = true;
	}

	bool bUsingDevelVersion = false;
	if ( sLatest_major < sCurrent_major ) {
		bUsingDevelVersion = true;
	}
	else if ( sLatest_major == sCurrent_major && sLatest_minor < sCurrent_minor ) {
		bUsingDevelVersion = true;
	}
	else if ( sLatest_major == sCurrent_major && sLatest_minor == sCurrent_minor && sLatest_micro < sCurrent_micro ) {
		bUsingDevelVersion = true;
	}

	if ( bExistsNewVersion ) {
		QString sLatest = QString(sLatest_major) + "." +  QString(sLatest_minor) + "." + QString(sLatest_micro);
		WARNINGLOG( "\n\n*** A newer version (v" + sLatest + ") of Hydrogen is available at http://www.hydrogen-music.org\n" );
	}

	if ( bUsingDevelVersion ) {
		Preferences *pref = Preferences::getInstance();
		bool isDevelWarningEnabled = pref->getShowDevelWarning();
		if(isDevelWarningEnabled) {

			QString msg = trUtf8( "You're using a development version of Hydrogen, please help us reporting bugs or suggestions in the hydrogen-devel mailing list.<br><br>Thank you!" );
			QMessageBox develMessageBox( this );
			develMessageBox.setText( msg );
			develMessageBox.addButton( QMessageBox::Ok );
			develMessageBox.addButton( trUtf8( "Don't show this message anymore" ) , QMessageBox::AcceptRole );

			if( develMessageBox.exec() == 0 ){
				//don't show warning again
				pref->setShowDevelWarning( false );
			}
	  }


	}
}



QString MainForm::getAutoSaveFilename()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	assert( pSong );
	QString sOldFilename = pSong->get_filename();
	QString newName = "autosave.h2song";

	if ( sOldFilename != "" ) {
		newName = sOldFilename.left( sOldFilename.length() - 7 ) + ".autosave.h2song";
	}

	return newName;
}



void MainForm::onAutoSaveTimer()
{
	//INFOLOG( "[onAutoSaveTimer]" );
	Song *pSong = Hydrogen::get_instance()->getSong();
	assert( pSong );
	QString sOldFilename = pSong->get_filename();

	pSong->save( getAutoSaveFilename() );

	pSong->set_filename(sOldFilename);

/*
	Song *pSong = h2app->getSong();
	if (pSong->getFilename() == "") {
		pSong->save( "autosave.h2song" );
		return;
	}

	action_file_save();
*/
}
