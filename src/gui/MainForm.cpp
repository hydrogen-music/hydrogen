/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: MainForm.cpp,v 1.48 2005/07/21 13:26:47 comix Exp $
 *
 */

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

#include "lib/Hydrogen.h"
#include "lib/smf/SMF.h"
#include "lib/Preferences.h"
#include "lib/LocalFileMng.h"

#include "InstrumentEditor/InstrumentEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"

#include <qaccel.h>
#include <qinputdialog.h>
#include <qmessagebox.h>

#ifndef WIN32
	#include <sys/time.h>
#else
//	#include "timeHelper.h"
#endif

MainForm::MainForm(QApplication *app, string songFilename ) : QMainWindow( 0, 0, 0 ), Object( "MainForm   " )
{
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	m_pQApp = app;

	workspace = NULL;

	m_pQApp->processEvents( 1000 );

	// Load default song
	Song *song = NULL;
	if (songFilename != "") {
		song = Song::load( songFilename );
		if (song == NULL) {
			QMessageBox::warning( this, "Hydrogen", trUtf8("Error loading song.") );
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
				QMessageBox::warning( this, "Hydrogen", trUtf8("Error restoring last song.") );
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
	h2app->getInstrumentEditor()->installEventFilter(this);
	h2app->getDrumkitManager()->installEventFilter(this);
	h2app->getAudioEngineInfoForm()->installEventFilter(this);

// 	QAccel *a = new QAccel( this );
// 	a->connectItem( a->insertItem(Key_S + CTRL), this, SLOT( onSaveAccelEvent() ) );
// 	a->connectItem( a->insertItem(Key_O + CTRL), this, SLOT( onOpenAccelEvent() ) );


	installEventFilter( this );

	connect(&m_http, SIGNAL(done(bool)), this, SLOT(latestVersionDone(bool)));
	getLatestVersion();
}



MainForm::~MainForm() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

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
	m_pMenubar = new QMenuBar( this, "m_pMenubar" );

	// FILE menu
	m_pFilePopupMenu = new QPopupMenu( this );			// FILE MENU
	m_pMenubar->insertItem( trUtf8("&File"), m_pFilePopupMenu );

	m_pFilePopupMenu->insertItem( trUtf8("&New"), this, SLOT( action_file_new() ), QKeySequence( "Ctrl+N" ) );

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("&Open"), this, SLOT( action_file_open() ), QKeySequence( "Ctrl+O" ) );
	m_pFilePopupMenu->insertItem( trUtf8("Open &Demo"), this, SLOT( action_file_openDemo() ), QKeySequence( "Ctrl+D" ) );

	m_pRecentFilesPopupMenu = new QPopupMenu( this );
	m_pFilePopupMenu->insertItem( trUtf8( "Open &recent" ), m_pRecentFilesPopupMenu );	// FILE->OPEN_RECENT

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("&Save"), this, SLOT( action_file_save() ), QKeySequence( "Ctrl+S" ) );
	m_pFilePopupMenu->insertItem( trUtf8("Save &as"), this, SLOT( action_file_save_as() ), QKeySequence( "Ctrl+Shift+S" ) );

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("&Export song"), this, SLOT( action_file_export() ), QKeySequence( "Ctrl+E" ) );
	m_pFilePopupMenu->insertItem( trUtf8("Export &MIDI file"), this, SLOT( action_file_export_midi() ), QKeySequence( "Ctrl+M" ) );

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("Show song properties"), this, SLOT( action_file_songProperties() ), QKeySequence( "Ctrl+Shift+P" ) );

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("&Preferences"), this, SLOT( action_file_preferences() ), QKeySequence( "Ctrl+P" ) );

	m_pFilePopupMenu->insertSeparator();				// -----

	m_pFilePopupMenu->insertItem( trUtf8("&Quit"), this, SLOT( action_file_exit() ), QKeySequence( "Ctrl+Q" ) );

	updateRecentUsedSongList();
	//~ FILE menu

	// WINDOW menu
	m_pWindowPopupMenu = new QPopupMenu( this );
	m_pMenubar->insertItem( trUtf8( "&View" ), m_pWindowPopupMenu );

	m_pWindowPopupMenu->insertItem( trUtf8("Show &mixer"), this, SLOT( action_window_showMixer() ), QKeySequence( "Alt+M" ) );

	if ( ( Preferences::getInstance() )->getInterfaceMode() == Preferences::MDI ) {	// show only with child frame interface
		m_pWindowPopupMenu->insertItem( trUtf8("Show &song editor"), this, SLOT( action_window_showSongEditor() ), QKeySequence( "Alt+S" ) );
	}

	if ( ( Preferences::getInstance() )->getInterfaceMode() != Preferences::SINGLE_PANED ) {
		m_pWindowPopupMenu->insertItem( trUtf8("Show &pattern editor"), this, SLOT( action_window_showPatternEditor() ), QKeySequence( "Alt+P" ) );
	}

	m_pWindowPopupMenu->insertItem( trUtf8("Show &drumkit manager"), this, SLOT( action_window_showDrumkitManager() ), QKeySequence( "Alt+D" ) );

//	if ( ( Preferences::getInstance() )->getInterfaceMode() != Preferences::SINGLE_PANED ) {
		m_pWindowPopupMenu->insertItem( trUtf8("Show &instrument editor"), this, SLOT( action_window_showInstrumentEditor() ), QKeySequence( "Alt+I" ) );
//	}
	//~ WINDOW menu


//#ifdef CONFIG_DEBUG
	if ( Object::isUsingVerboseLog() ) {
		// DEBUG menu
		m_pDebugPopupMenu = new QPopupMenu( this );
		m_pMenubar->insertItem( trUtf8("De&bug"), m_pDebugPopupMenu );

		menuItem_debug_showAudioEngineInfo = new QAction( this );
		menuItem_debug_showAudioEngineInfo->setMenuText( trUtf8( "Show &audio engine info" ) );
		menuItem_debug_showAudioEngineInfo->addTo( m_pDebugPopupMenu );
		connect( menuItem_debug_showAudioEngineInfo, SIGNAL( activated() ), this, SLOT( action_debug_showAudioEngineInfo() ) );

		menuItem_debug_debugCommand = new QAction( this, "menuItem_debug_debugCommand" );
		menuItem_debug_debugCommand->setText( "debug action" );
		menuItem_debug_debugCommand->addTo( m_pDebugPopupMenu );
		connect( menuItem_debug_debugCommand, SIGNAL( activated() ), this, SLOT( action_debug_debugCommand() ) );

		menuItem_debug_printObjects = new QAction( this, "menuItem_debug_printObjects" );
		menuItem_debug_printObjects->setText( "Print Objects" );
		menuItem_debug_printObjects->addTo( m_pDebugPopupMenu );
		connect( menuItem_debug_printObjects, SIGNAL( activated() ), this, SLOT( action_debug_printObjects() ) );
		//~ DEBUG menu
	}
//#endif

	// HELP menu
	m_pHelpPopupMenu = new QPopupMenu( this );
	m_pMenubar->insertItem( trUtf8( "&Help" ), m_pHelpPopupMenu );

	m_pHelpPopupMenu->insertItem( trUtf8("&User manual"), this, SLOT( action_help_manual() ), QKeySequence( "Ctrl+?" ) );

	m_pHelpPopupMenu->insertSeparator();

	m_pHelpPopupMenu->insertItem( trUtf8("&About"), this, SLOT( action_help_about() ), QKeySequence( trUtf8("", "Help|About") ) );
	//~ HELP menu
}



void MainForm::action_file_exit() {
	if ( (h2app->getSong())->m_bIsModified ) {
		switch(
				QMessageBox::information( this, "Hydrogen",
						trUtf8("\nThe song has unsaved changes\n"
						"Do you want to save the changes before exiting?\n"),
						trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
						0,      // Enter == button 0
						2 ) ) { // Escape == button 2
			case 0:
				// Save clicked or Alt+S pressed or Enter pressed.
				if (h2app->getSong()->getFilename() != "") {
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
				return;
				break;
		}
	}

	closeAll();
}



void MainForm::action_file_new() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	if ( (h2app->getSong())->m_bIsModified ) {
		switch(
				QMessageBox::information( this, "Hydrogen",
						trUtf8("\nThe document contains unsaved changes\n"
						"Do you want to save the changes before exiting?\n"),
						trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
						0,      // Enter == button 0
						2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if (h2app->getSong()->getFilename() != "") {
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

}



void MainForm::action_file_save_as()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );

	fd->setCaption( trUtf8( "Save song" ) );
	fd->setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	Song *song = h2app->getSong();
	QString defaultFilename;
	QString lastFilename = (song->getFilename()).c_str();

	if (lastFilename == "") {
		defaultFilename = Hydrogen::getInstance()->getSong()->m_sName.c_str();
		defaultFilename += ".h2song";
	}
	else {
		defaultFilename = lastFilename;
	}

	fd->setSelection(defaultFilename);

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}

	if (filename != "") {


		QString sNewFilename = filename;
		if ( sNewFilename.endsWith(".h2song") == false ) {
			filename += ".h2song";
		}

		song->setFilename(filename.latin1());
		action_file_save();
	}
}



void MainForm::action_file_save()
{
//	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
//		(Hydrogen::getInstance())->stop();
//	}

	Song *song = h2app->getSong();
	QString filename = (song->getFilename()).c_str();

	if (filename == "") {
		// just in case!
		return action_file_save_as();
	}

	LocalFileMng mng;
	song->save( filename.latin1() );
	( Preferences::getInstance() )->setLastSongFilename( song->getFilename() );

	// add the new loaded song in the "last used song" vector
	Preferences *pPref = Preferences::getInstance();
	vector<string> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), filename.latin1() );
	pPref->setRecentFiles( recentFiles );

	updateRecentUsedSongList();
}




void MainForm::action_help_about() {
	QWidget *parent = this;
	if (workspace) {
		parent = workspace;
	}

	// show modal dialog
	AboutDialog *dialog = new AboutDialog( NULL );
	dialog->exec();
}




void MainForm::action_help_manual() {
	QWidget *parent = this;
	if (workspace) {
		parent = workspace;
	}

	h2app->getHelpBrowser()->hide();
	h2app->getHelpBrowser()->show();
}




void MainForm::action_file_open() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	if ( (h2app->getSong())->m_bIsModified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if (h2app->getSong()->getFilename() != "") {
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

	static QString lastUsedDir = "";

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );
	fd->setDir( lastUsedDir );

	fd->setCaption( trUtf8( "Open song" ) );
	fd->setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/

	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
		lastUsedDir = fd->dir()->absPath();
	}


	if (filename != "") {
		openSongFile( filename.latin1() );
	}
}




/// \todo parametrizzare il metodo action_file_open ed eliminare il seguente...
void MainForm::action_file_openDemo() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	if ( (h2app->getSong())->m_bIsModified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe document contains unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("&Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0: // Save clicked or Alt+S pressed or Enter pressed.
				if (h2app->getSong()->getFilename() != "") {
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


	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::ExistingFile);
	fd->setFilter( trUtf8("Hydrogen Song (*.h2song)") );

	fd->setCaption( trUtf8( "Open song" ) );
	fd->setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	/// \todo impostare il preview
	/*
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( "uno", "due" );
	fd->setPreviewMode( QFileDialog::Contents );
	*/
	string demoPath = (Preferences::getInstance())->getDemoPath();
	fd->setSelection( QString( demoPath.c_str() ) );


	QString filename = "";
	if (fd->exec() == QDialog::Accepted) {
		filename = fd->selectedFile();
	}


	if (filename != "") {
		openSongFile( filename.latin1() );
		( h2app->getSong() )->setFilename( "" );
	}
}



void MainForm::action_file_preferences() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
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




///
/// Window close event
///
void MainForm::closeEvent( QCloseEvent* ev ) {
	if ( (h2app->getSong())->m_bIsModified ) {
		switch(
			QMessageBox::information( this, "Hydrogen",
					trUtf8("\nThe song has unsaved changes\n"
					"Do you want to save the changes before exiting?\n"),
					trUtf8("&Save"), trUtf8("&Discard"), trUtf8("Cancel"),
					0,      // Enter == button 0
					2 ) ) { // Escape == button 2
			case 0:
				// Save clicked or Alt+S pressed or Enter pressed.
				if (h2app->getSong()->getFilename() != "") {
					action_file_save();
				}
				else {
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
				return;
				break;
		}
	}


	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}


	closeAll();
//	this->app->quit();
	ev->accept();
}



void MainForm::action_file_export() {
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
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

	// Instrument properties
	WindowProperties instrumentEditorProp;
	if ( h2app->getInstrumentEditor()->parentWidget() != 0 ) {	// use the new MDI interface
		instrumentEditorProp.x = h2app->getInstrumentEditor()->parentWidget()->x();
		instrumentEditorProp.y = h2app->getInstrumentEditor()->parentWidget()->y();
	}
	else {
		instrumentEditorProp.x = h2app->getInstrumentEditor()->x();
		instrumentEditorProp.y = h2app->getInstrumentEditor()->y();
	}
	instrumentEditorProp.visible = h2app->getInstrumentEditor()->isVisible();
	pref->setInstrumentEditorProperties( instrumentEditorProp );

	// Save mixer properties
	WindowProperties mixerProp;
	if ( h2app->getMixer()->parentWidget() != 0 ) {	// use the new MDI interface
		mixerProp.x = h2app->getMixer()->parentWidget()->x();
		mixerProp.y = h2app->getMixer()->parentWidget()->y();
//		mixerProp.width = h2app->getMixer()->parentWidget()->width();
//		mixerProp.height = h2app->getMixer()->parentWidget()->height();
		mixerProp.width = h2app->getMixer()->width();
		mixerProp.height = h2app->getMixer()->height();
	}
	else {
		mixerProp.x = h2app->getMixer()->x();
		mixerProp.y = h2app->getMixer()->y();
		mixerProp.width = h2app->getMixer()->width();
		mixerProp.height = h2app->getMixer()->height();
	}
	mixerProp.visible = h2app->getMixer()->isVisible();
	pref->setMixerProperties( mixerProp );

	// save pattern editor properties
	WindowProperties patternEditorProp;
	if ( h2app->getPatternEditorPanel()->parentWidget() != 0 ) {	// use the new MDI interface
		patternEditorProp.x = h2app->getPatternEditorPanel()->parentWidget()->x();
		patternEditorProp.y = h2app->getPatternEditorPanel()->parentWidget()->y();
		patternEditorProp.width = h2app->getPatternEditorPanel()->width();
		patternEditorProp.height = h2app->getPatternEditorPanel()->height();
	}
	else {
		patternEditorProp.x = h2app->getPatternEditorPanel()->x();
		patternEditorProp.y = h2app->getPatternEditorPanel()->y();
		patternEditorProp.width = h2app->getPatternEditorPanel()->width();
		patternEditorProp.height = h2app->getPatternEditorPanel()->height();
	}
	patternEditorProp.visible = h2app->getPatternEditorPanel()->isVisible();
	pref->setPatternEditorProperties( patternEditorProp );

	// save song editor properties
	WindowProperties songEditorProp;
	if ( h2app->getSongEditorPanel()->parentWidget() != 0 ) {	// use the new MDI interface
		songEditorProp.x = h2app->getSongEditorPanel()->parentWidget()->x();
		songEditorProp.y = h2app->getSongEditorPanel()->parentWidget()->y();
	}
	else {
		songEditorProp.x = h2app->getSongEditorPanel()->x();
		songEditorProp.y = h2app->getSongEditorPanel()->y();
	}
	songEditorProp.width = h2app->getSongEditorPanel()->width();
	songEditorProp.height = h2app->getSongEditorPanel()->height();

//	cout << "song editor size=" << h2app->getSongEditorPanel()->width() << "x" << h2app->getSongEditorPanel()->height() << endl;
	QSize size = h2app->getSongEditorPanel()->frameSize();
//	cout << "song editor size2=" << size.width() << "x" << size.height() << endl;

	songEditorProp.visible = h2app->getSongEditorPanel()->isVisible();
	pref->setSongEditorProperties( songEditorProp );


	// save drumkit manager properties
	WindowProperties drumkitMngProp;
	if ( h2app->getDrumkitManager()->parentWidget() != 0 ) {	// use the new MDI interface
		drumkitMngProp.x = h2app->getDrumkitManager()->parentWidget()->x();
		drumkitMngProp.y = h2app->getDrumkitManager()->parentWidget()->y();
		drumkitMngProp.width = h2app->getDrumkitManager()->parentWidget()->width();
		drumkitMngProp.height = h2app->getDrumkitManager()->parentWidget()->height();
	}
	else {
		drumkitMngProp.x = h2app->getDrumkitManager()->x();
		drumkitMngProp.y = h2app->getDrumkitManager()->y();
		drumkitMngProp.width = h2app->getDrumkitManager()->width();
		drumkitMngProp.height = h2app->getDrumkitManager()->height();
	}
	drumkitMngProp.visible = h2app->getDrumkitManager()->isVisible();
	pref->setDrumkitManagerProperties( drumkitMngProp );


	// save audio engine info properties
	WindowProperties audioEngineInfoProp;
	if ( h2app->getAudioEngineInfoForm()->parentWidget() != 0 ) {	// use the new MDI interface
		audioEngineInfoProp.x = h2app->getAudioEngineInfoForm()->parentWidget()->x();
		audioEngineInfoProp.y = h2app->getAudioEngineInfoForm()->parentWidget()->y();
	}
	else {
		audioEngineInfoProp.x = h2app->getAudioEngineInfoForm()->x();
		audioEngineInfoProp.y = h2app->getAudioEngineInfoForm()->y();
	}
	audioEngineInfoProp.visible = h2app->getAudioEngineInfoForm()->isVisible();
	pref->setAudioEngineInfoProperties( audioEngineInfoProp );


#ifdef LADSPA_SUPPORT
	// save LADSPA FX window properties
	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		WindowProperties prop;
		if (h2app->getAudioEngineInfoForm()->parentWidget() != 0 ) {	// use MDI interface
			prop.x = h2app->getLadspaFXProperties(nFX)->parentWidget()->x();
			prop.y = h2app->getLadspaFXProperties(nFX)->parentWidget()->y();
		}
		else {
			prop.x = h2app->getLadspaFXProperties(nFX)->x();
			prop.y = h2app->getLadspaFXProperties(nFX)->y();
		}
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
			Hydrogen::getInstance()->start();
			break;

		case STATE_PLAYING:
			Hydrogen::getInstance()->stop();
			break;

		default:
			cerr << "[MainForm::onPlayStopAccelEvent()] Unhandled case." << endl;
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
	pEngine->lockEngine( "MainForm::onBPMPlusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->m_fBPM  < 300) {
		pEngine->setBPM( pSong->m_fBPM + 1 );
	}
	pEngine->unlockEngine();
}



void MainForm::onBPMMinusAccelEvent()
{
	Hydrogen* pEngine = ( Hydrogen::getInstance() );
	pEngine->lockEngine( "MainForm::onBPMMinusAccelEvent" );

	Song* pSong = pEngine->getSong();
	if (pSong->m_fBPM > 40 ) {
		pEngine->setBPM( pSong->m_fBPM - 1 );
	}
	pEngine->unlockEngine();
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
	infoLog( "tap tempo" );
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
//	infoLog( "updateRecentUsedSongList" );
	m_pRecentFilesPopupMenu->clear();

	Preferences *pPref = Preferences::getInstance();
	vector<string> recentUsedSongs = pPref->getRecentFiles();

	string sFilename = "";

	if ( recentUsedSongs.size() > 0 ) {
		sFilename = recentUsedSongs[ 0 ];
		if ( sFilename != "" ) {
			m_pRecentFileAction0 = new QAction( this, "recent_file0" );
			m_pRecentFileAction0->setText( QString( sFilename.c_str() ) );
			m_pRecentFileAction0->addTo( m_pRecentFilesPopupMenu );
			connect( m_pRecentFileAction0, SIGNAL( activated() ), this, SLOT( action_file_open_recent0() ) );
		}
	}

	if ( recentUsedSongs.size() > 1 ) {
		sFilename = recentUsedSongs[ 1 ];
		if ( sFilename != "" ) {
			m_pRecentFileAction1 = new QAction( this, "recent_file1" );
			m_pRecentFileAction1->setText( QString( sFilename.c_str() ) );
			m_pRecentFileAction1->addTo( m_pRecentFilesPopupMenu );
			connect( m_pRecentFileAction1, SIGNAL( activated() ), this, SLOT( action_file_open_recent1() ) );
		}
	}

	if ( recentUsedSongs.size() > 2 ) {
		sFilename = recentUsedSongs[ 2 ];
		if ( sFilename != "" ) {
			m_pRecentFileAction2 = new QAction( this, "recent_file2" );
			m_pRecentFileAction2->setText( QString( sFilename.c_str() ) );
			m_pRecentFileAction2->addTo( m_pRecentFilesPopupMenu );
			connect( m_pRecentFileAction2, SIGNAL( activated() ), this, SLOT( action_file_open_recent2() ) );
		}
	}

	if ( recentUsedSongs.size() > 3 ) {
		sFilename = recentUsedSongs[ 3 ];
		if ( sFilename != "" ) {
			m_pRecentFileAction3 = new QAction( this, "recent_file3" );
			m_pRecentFileAction3->setText( QString( sFilename.c_str() ) );
			m_pRecentFileAction3->addTo( m_pRecentFilesPopupMenu );
			connect( m_pRecentFileAction3, SIGNAL( activated() ), this, SLOT( action_file_open_recent3() ) );
		}
	}

	if ( recentUsedSongs.size() > 4 ) {
		sFilename = recentUsedSongs[ 4 ];
		if ( sFilename != "" ) {
			m_pRecentFileAction4 = new QAction( this, "recent_file4" );
			m_pRecentFileAction4->setText( QString( sFilename.c_str() ) );
			m_pRecentFileAction4->addTo( m_pRecentFilesPopupMenu );
			connect( m_pRecentFileAction4, SIGNAL( activated() ), this, SLOT( action_file_open_recent4() ) );
		}
	}
}



void MainForm::action_file_open_recent0()
{
	openSongFile( m_pRecentFileAction0->text().latin1() );
}



void MainForm::action_file_open_recent1()
{
	openSongFile( m_pRecentFileAction1->text().latin1() );
}



void MainForm::action_file_open_recent2()
{
	openSongFile( m_pRecentFileAction2->text().latin1() );
}



void MainForm::action_file_open_recent3()
{
	openSongFile( m_pRecentFileAction3->text().latin1() );
}



void MainForm::action_file_open_recent4()
{
	openSongFile( m_pRecentFileAction4->text().latin1() );
}



void MainForm::openSongFile( string sFilename )
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	LocalFileMng mng;
	Song *pSong = mng.loadSong( sFilename );
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
	if ( e->type() == QEvent::KeyPress) {
		// special processing for key press
		QKeyEvent *k = (QKeyEvent *)e;

		// qDebug( "Got key press for instrument '%c'", k->ascii() );

		switch (k->key()) {
			case Key_Space:
				onPlayStopAccelEvent();
				return TRUE; // eat event
				break;

			case Key_Backspace:
				onRestartAccelEvent();
				return TRUE; // eat event
				break;

			case Key_Plus:
				onBPMPlusAccelEvent();
				return TRUE; // eat event
				break;

			case Key_Minus:
				onBPMMinusAccelEvent();
				return TRUE; // eat event
				break;

			case Key_Backslash:
				onTapTempoAccelEvent();
				return TRUE; // eat event
				break;

			case  Key_S | CTRL:
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
//			infoLog( "[eventFilter] virtual keyboard event" );
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



void MainForm::action_window_showInstrumentEditor()
{
	InstrumentEditor *pInstrEditor = HydrogenApp::getInstance()->getInstrumentEditor();
	pInstrEditor->setHidden( pInstrEditor->isVisible() );
//	HydrogenApp::getInstance()->getInstrumentEditor()->show();
}



/// print the object map
void MainForm::action_debug_printObjects()
{
	infoLog( "[action_debug_printObjects]" );
	Object::printObjectMap();
}



/// Riceve un comando testuale ed esegue una azione di debug
void MainForm::action_debug_debugCommand()
{
	infoLog( "[action_debug_debugCommand]" );

	static QString sLastCommand = "";
	bool bIsOkPressed;
	QString cmd = QInputDialog::getText( "Hydrogen", "Command", QLineEdit::Normal, sLastCommand, &bIsOkPressed, this );
	if (bIsOkPressed) {
		sLastCommand = cmd;

		if ( cmd == "print current pattern list" ) {
			Hydrogen *pEngine = Hydrogen::getInstance();

			cout << "*** print current pattern list ***" << endl;

			cout << "Pattern pos: " << pEngine->getPatternPos() << endl;

			cout << endl;

			cout << "----------------------------------------------------------------------" << endl;
			cout << "Song pattern list" << endl;
			cout << "----------------------------------------------------------------------" << endl;
			PatternList *pSongPatternList = pEngine->getSong()->getPatternList();
			for ( uint i = 0; i <pSongPatternList->getSize(); i++ ) {
				Pattern *pPat = pSongPatternList->get( i );
				cout << "   |->[" << i << "] " << pPat->m_sName << endl;
			}
			cout << "----------------------------------------------------------------------" << endl;

			cout << endl;

			cout << "----------------------------------------------------------------------" << endl;
			cout << "Current pattern list" << endl;
			cout << "----------------------------------------------------------------------" << endl;
			PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
			for ( uint i = 0; i <pCurrentPatternList->getSize(); i++ ) {
				Pattern *pPat = pCurrentPatternList->get( i );
				cout << "   |->[" << i << "] " << pPat->m_sName << endl;
			}
			cout << "----------------------------------------------------------------------" << endl;
		}
		else {
			QMessageBox::warning( this, "Hydrogen", "action not found" );
		}

	}

}



void MainForm::action_file_export_midi()
{
	if ( ((Hydrogen::getInstance())->getState() == STATE_PLAYING) ) {
		(Hydrogen::getInstance())->stop();
	}

	QFileDialog *fd = new QFileDialog(this, "File Dialog", TRUE);
	fd->setMode(QFileDialog::AnyFile);
	fd->setFilter( trUtf8("Midi file (*.mid)") );

	fd->setCaption( trUtf8( "Export MIDI file" ) );
	fd->setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	QString sFilename = "";
	if ( fd->exec() == QDialog::Accepted ) {
		sFilename = fd->selectedFile();
	}

	if ( sFilename != "" ) {
		if ( sFilename.endsWith(".mid") == false ) {
			sFilename += ".mid";
		}

		Song *pSong = Hydrogen::getInstance()->getSong();

		// create the Standard Midi File object
		SMFWriter *pSmfWriter = new SMFWriter();
		pSmfWriter->save( sFilename.latin1(), pSong );

		delete pSmfWriter;
	}
}



void MainForm::errorEvent( int nErrorCode )
{
	errorLog( "[errorEvent]" );

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
			msg = trUtf8( QString( "Unknown error %1" ).arg( nErrorCode ) );
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

	#ifdef Q_OS_MACX
	QString os = "Mac";
	#endif
	#ifdef WIN32
	QString os = "Win";
	#else
	QString os = "Linux";
	#endif

	QString sRequest = QString("/getLatestVersion.php?UsingVersion=%1").arg(VERSION);
	sRequest += QString( "&OS=%1" ).arg( os );
	QHttpRequestHeader header( "GET", sRequest );
	header.setValue( "Host", "www.hydrogen-music.org" );

	m_http.setHost( "www.hydrogen-music.org" );
	m_http.request( header );
}

void MainForm::latestVersionDone(bool bError)
{
	if ( bError ) {
		infoLog( "[MainForm::latestVersionDone] Error." );
		return;
	}

	QString sLatestVersion( m_http.readAll() );
	sLatestVersion = sLatestVersion.simplifyWhiteSpace();
	QString sLatest_major = sLatestVersion.section( '.', 0, 0 );
	QString sLatest_minor = sLatestVersion.section( '.', 1, 1 );
	QString sLatest_micro = sLatestVersion.section( '.', 2, 2 );
//	infoLog( "Latest available version is: " + string( sLatestVersion.ascii() ) );

	QString sCurrentVersion = VERSION;
	QString sCurrent_major = sCurrentVersion.section( '.', 0, 0 );
	QString sCurrent_minor = sCurrentVersion.section( '.', 1, 1 );
	QString sCurrent_micro = sCurrentVersion.section( '.', 2, 2 );
	if ( sCurrent_micro.section( '-', 0, 0 ) ) {
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
		string sLatest = string(sLatest_major.ascii()) + "." +  string(sLatest_minor.ascii()) + "." + string(sLatest_micro.ascii());
		warningLog( "\n\n*** A newer version (v" + sLatest + ") of Hydrogen is available at http://www.hydrogen-music.org\n" );
	}

	if ( bUsingDevelVersion ) {
		warningLog( "\n\n*** You're using a development version of Hydrogen, please help us reporting bugs in the hydrogen-devel mailing list. Thank you!" );
	}
}

