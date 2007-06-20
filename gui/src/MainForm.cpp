/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <hydrogen/hydrogen.h>
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

#include <QInputDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QCloseEvent>
#include <QEvent>
#include <QKeyEvent>

#include <QFileDialog>

#ifndef WIN32
	#include <sys/time.h>
#endif

#include <cassert>

using std::string;
using std::map;
using namespace H2Core;

MainForm::MainForm( QApplication *app, const std::string& songFilename )
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
			song = Song::getEmptySong();
			song->setFilename( "" );
		}
	}
	else {
		Preferences *pref = Preferences::getInstance();
		bool restoreLastSong = pref->isRestoreLastSongEnabled();
		string filename = pref->getLastSongFilename();
		if ( restoreLastSong && (filename != "" )) {
			song = Song::load( filename );
			if (song == NULL) {
				//QMessageBox::warning( this, "Hydrogen", trUtf8("Error restoring last song.") );
				song = Song::getEmptySong();
				song->setFilename( "" );
			}
		}
		else {
			song = Song::getEmptySong();
			song->setFilename( "" );
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
}



MainForm::~MainForm()
{
	// remove the autosave file
	QFile file( getAutoSaveFilename().c_str() );
	file.remove();

	if ( (Hydrogen::getInstance()->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	// remove the autosave file
	m_autosaveTimer.stop();
	QFile autosaveFile( "hydrogen_autosave.h2song" );
	autosaveFile.remove();

	hide();

	if (h2app != NULL) {
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

	m_pToolsMenu->addAction( trUtf8("&Mixer"), this, SLOT( action_window_showMixer() ), QKeySequence( "Alt+M" ) );

	m_pToolsMenu->addAction( trUtf8("&Instrument Rack"), this, SLOT( action_window_showDrumkitManagerPanel() ), QKeySequence( "Alt+I" ) );
	m_pToolsMenu->addAction( trUtf8("&Preferences"), this, SLOT( showPreferencesDialog() ), QKeySequence( "Alt+P" ) );

	m_pToolsMenu->addSeparator();				// -----
	m_pToolsMenu->addAction( trUtf8("OLD &drumkit manager (Obsolete)"), this, SLOT( action_window_showDrumkitManager() ), QKeySequence( "Alt+D" ) );
	//~ Tools menu


	if ( Object::isUsingVerboseLog() ) {
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



/// return true if the app needs to be closed.
bool MainForm::action_file_exit()
{
	if ( Hydrogen::getInstance()->getSong()->m_bIsModified ) {
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
				if ( Hydrogen::getInstance()->getSong()->getFilename() != "") {
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
				ERRORLOG( "Unknown return code: " + toString( res ) );
		}
	}

	closeAll();
	return true;
}



void MainForm::action_file_new()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	if ( Hydrogen::getInstance()->getSong()->m_bIsModified ) {
		switch(
				QMessageBox::information( this, "Hydrogen",
						trUtf8("\nThe document contains unsaved changes\n"
						"Do you want to save the changes before exiting?\n"),
						trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
						0,      // Enter == button 0
						2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::getInstance()->getSong()->getFilename() != "") {
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

	Song * song = Song::getEmptySong();
	song->setFilename( "" );
	h2app->setSong(song);
 	Hydrogen::getInstance()->setSelectedPatternNumber( 0 );
}



void MainForm::action_file_save_as()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	QFileDialog *fd = new QFileDialog(this);
	fd->setFileMode( QFileDialog::AnyFile );
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
	fd->setAcceptMode( QFileDialog::AcceptSave );
	fd->setWindowTitle( trUtf8( "Save song" ) );

	Song *song = Hydrogen::getInstance()->getSong();
	QString defaultFilename;
	QString lastFilename = song->getFilename().c_str();

	if (lastFilename == "") {
		defaultFilename = Hydrogen::getInstance()->getSong()->m_sName.c_str();
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

		song->setFilename(filename.toStdString());
		action_file_save();
	}
	h2app->setStatusBarMessage( trUtf8("Song saved."), 10000 );
}



void MainForm::action_file_save()
{
//	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
//		(Hydrogen::getInstance())->stop();
//	}

	Song *song = Hydrogen::getInstance()->getSong();
	QString filename = (song->getFilename()).c_str();

	if (filename == "") {
		// just in case!
		return action_file_save_as();
	}

	LocalFileMng mng;
	song->save( filename.toStdString() );
	( Preferences::getInstance() )->setLastSongFilename( song->getFilename() );

	// add the new loaded song in the "last used song" vector
	Preferences *pPref = Preferences::getInstance();
	vector<string> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), filename.toStdString() );
	pPref->setRecentFiles( recentFiles );

	updateRecentUsedSongList();

	h2app->setStatusBarMessage( trUtf8("Song saved."), 10000 );
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




void MainForm::action_file_open() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	if ( Hydrogen::getInstance()->getSong()->m_bIsModified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::getInstance()->getSong()->getFilename() != "") {
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

	static QString lastUsedDir = QDir::homePath();
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
		openSongFile( filename.toStdString() );
	}
}




/// \todo parametrizzare il metodo action_file_open ed eliminare il seguente...
void MainForm::action_file_openDemo()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	if ( ( Hydrogen::getInstance()->getSong())->m_bIsModified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if ( Hydrogen::getInstance()->getSong()->getFilename() != "") {
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
	fd->setDirectory( QString( Preferences::getInstance()->getDemoPath().c_str() ) );


	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFiles().first();
	}


	if (filename != "") {
		openSongFile( filename.toStdString() );
		Hydrogen::getInstance()->getSong()->setFilename( "" );
	}
}



void MainForm::showPreferencesDialog()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
	}

	h2app->showPreferencesDialog();
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
	AudioEngine::getInstance()->lock("MainForm::action_instruments_addInstrument");
	InstrumentList* pList = Hydrogen::getInstance()->getSong()->getInstrumentList();

	// create a new valid ID for this instrument
	int nID = -1;
	for ( uint i = 0; i < pList->getSize(); ++i ) {
		Instrument* pInstr = pList->get( i );
		if ( atoi( pInstr->m_sId.c_str() ) > nID ) {
			nID = atoi( pInstr->m_sId.c_str() );
		}
	}
	++nID;

	Instrument *pNewInstr = new Instrument();
	pNewInstr->m_sName = "New instrument";
	pNewInstr->m_sId = toString( nID );
	pNewInstr->m_pADSR = new ADSR();

	pList->add( pNewInstr );
	AudioEngine::getInstance()->unlock();

	Hydrogen::getInstance()->setSelectedInstrumentNumber( pList->getSize() - 1 );

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
	AudioEngine::getInstance()->lock("MainForm::action_instruments_clearAll");
	Song *pSong = Hydrogen::getInstance()->getSong();
	InstrumentList* pList = pSong->getInstrumentList();
	for (uint i = 0; i < pList->getSize(); i++) {
		Instrument* pInstr = pList->get( i );
		pInstr->m_sName = (QString( trUtf8( "Instrument %1" ) ).arg( i + 1 )).toStdString();
		// remove all layers
		for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer* pLayer = pInstr->getLayer( nLayer );
			delete pLayer;
			pInstr->setLayer( NULL, nLayer );
		}
	}
	AudioEngine::getInstance()->unlock();
	EventQueue::getInstance()->pushEvent( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



void MainForm::action_instruments_exportLibrary()
{
  ERRORLOG( "Not implemented yet" );
  QMessageBox::warning( this, "Hydrogen", QString( "Not implemented yet.") );
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
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
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
	int nState = Hydrogen::getInstance()->getState();
	switch (nState) {
		case STATE_READY:
			Hydrogen::getInstance()->sequencer_play();
			break;

		case STATE_PLAYING:
			Hydrogen::getInstance()->sequencer_stop();
			break;

		default:
			ERRORLOG( "[MainForm::onPlayStopAccelEvent()] Unhandled case." );
	}
}



void MainForm::onRestartAccelEvent()
{
	Hydrogen* pEngine = ( Hydrogen::getInstance() );
	pEngine->setPatternPos( 0 );
}



void MainForm::onBPMPlusAccelEvent()
{
	Hydrogen* pEngine = ( Hydrogen::getInstance() );
	AudioEngine::getInstance()->lock( "MainForm::onBPMPlusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->m_fBPM  < 300) {
		pEngine->setBPM( pSong->m_fBPM + 0.1 );
	}
	AudioEngine::getInstance()->unlock();
}



void MainForm::onBPMMinusAccelEvent()
{
	Hydrogen* pEngine = ( Hydrogen::getInstance() );
	AudioEngine::getInstance()->lock( "MainForm::onBPMMinusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->m_fBPM > 40 ) {
		pEngine->setBPM( pSong->m_fBPM - 0.1 );
	}
	AudioEngine::getInstance()->unlock();
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
		( Hydrogen::getInstance() )->setTapTempo( fInterval );
	}
#endif
}



void MainForm::updateRecentUsedSongList()
{
	m_pRecentFilesMenu->clear();

	Preferences *pPref = Preferences::getInstance();
	vector<string> recentUsedSongs = pPref->getRecentFiles();

	string sFilename = "";

	for ( uint i = 0; i < recentUsedSongs.size(); ++i ) {
		sFilename = recentUsedSongs[ i ];

		if ( sFilename != "" ) {
			QAction *pAction = new QAction( this  );
			pAction->setText( QString( sFilename.c_str() ) );
			m_pRecentFilesMenu->addAction( pAction );
		}
	}
}



void MainForm::action_file_open_recent(QAction *pAction)
{
//	INFOLOG( pAction->text().toStdString() );
	openSongFile( pAction->text().toStdString() );
}



void MainForm::openSongFile( const std::string& sFilename )
{
 	Hydrogen *engine = Hydrogen::getInstance();
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
	vector<string> recentFiles = pPref->getRecentFiles();
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
				onPlayStopAccelEvent();
				return TRUE; // eat event
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
			Hydrogen* engine = Hydrogen::getInstance();

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
	Object::printObjectMap();
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
			Hydrogen *pEngine = Hydrogen::getInstance();

			std::cout << "*** print current pattern list ***" << std::endl;

			std::cout << "Pattern pos: " << pEngine->getPatternPos() << std::endl;

			std::cout << std::endl;

			std::cout << "----------------------------------------------------------------------" << std::endl;
			std::cout << "Song pattern list" << std::endl;
			std::cout << "----------------------------------------------------------------------" << std::endl;
			PatternList *pSongPatternList = pEngine->getSong()->getPatternList();
			for ( uint i = 0; i <pSongPatternList->getSize(); i++ ) {
				H2Core::Pattern *pPat = pSongPatternList->get( i );
				std::cout << "   |->[" << i << "] " << pPat->m_sName << std::endl;
			}
			std::cout << "----------------------------------------------------------------------" << std::endl;

			std::cout << std::endl;

			cout << "----------------------------------------------------------------------" << std::endl;
			cout << "Current pattern list" << std::endl;
			cout << "----------------------------------------------------------------------" << std::endl;
			PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
			for ( uint i = 0; i <pCurrentPatternList->getSize(); i++ ) {
				H2Core::Pattern *pPat = pCurrentPatternList->get( i );
				cout << "   |->[" << i << "] " << pPat->m_sName << std::endl;
			}
			cout << "----------------------------------------------------------------------" << std::endl;
		}
		else if ( cmd == "crash" ) {
			Song *pBadPointer = NULL;
			pBadPointer->m_fBPM = 1000000;
		}
		else {
			QMessageBox::warning( this, "Hydrogen", "action not found" );
		}

	}

}



void MainForm::action_file_export_midi()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		Hydrogen::getInstance()->sequencer_stop();
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

		Song *pSong = Hydrogen::getInstance()->getSong();

		// create the Standard Midi File object
		SMFWriter *pSmfWriter = new SMFWriter();
		pSmfWriter->save( sFilename.toStdString(), pSong );

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
		Hydrogen::getInstance()->getSong()->m_bIsModified = true;
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


	QString sRequest = QString("/getLatestVersion.php?UsingVersion=%1").arg(VERSION);
	sRequest += QString( "&OS=%1" ).arg( os );

	//INFOLOG( sRequest.toStdString() );

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
//	INFOLOG( "Latest available version is: " + string( sLatestVersion.ascii() ) );

	QString sCurrentVersion = VERSION;
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
		string sLatest = string(sLatest_major.toStdString()) + "." +  string(sLatest_minor.toStdString()) + "." + string(sLatest_micro.toStdString());
		WARNINGLOG( "\n\n*** A newer version (v" + sLatest + ") of Hydrogen is available at http://www.hydrogen-music.org\n" );
	}

	if ( bUsingDevelVersion ) {
	  QString msg = trUtf8( "You're using a development version of Hydrogen, please help us reporting bugs or suggestions in the hydrogen-devel mailing list.<br><br>Thank you!" );
		QMessageBox::warning( this, "Hydrogen", msg );

	}
}



std::string MainForm::getAutoSaveFilename()
{
	Song *pSong = Hydrogen::getInstance()->getSong();
	assert( pSong );
	string sOldFilename = pSong->getFilename();
	string newName = "autosave.h2song";

	if ( sOldFilename != "" ) {
		newName = sOldFilename.substr( 0, sOldFilename.length() - 7 ) + ".autosave.h2song";
	}

	return newName;
}



void MainForm::onAutoSaveTimer()
{
	//INFOLOG( "[onAutoSaveTimer]" );
	Song *pSong = Hydrogen::getInstance()->getSong();
	assert( pSong );
	string sOldFilename = pSong->getFilename();

	pSong->save( getAutoSaveFilename() );

	pSong->setFilename(sOldFilename);

/*
	Song *pSong = h2app->getSong();
	if (pSong->getFilename() == "") {
		pSong->save( "autosave.h2song" );
		return;
	}

	action_file_save();
*/
}
