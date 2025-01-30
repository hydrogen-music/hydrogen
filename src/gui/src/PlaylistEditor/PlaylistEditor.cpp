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


#include "PlaylistEditor.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../CommonStrings.h"
#include "../InstrumentRack.h"
#include "../UndoActions.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SongEditor/SongEditorPanel.h"
#include "Widgets/PixmapWidget.h"

#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Shortcuts.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Timeline.h>
#include <core/EventQueue.h>
#include <core/Basics/Playlist.h>

#include "../Widgets/Button.h"
#include "../Widgets/FileDialog.h"

#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

using namespace H2Core;


PlaylistEditor::PlaylistEditor( QWidget* pParent )
		: QDialog( pParent )
		, Object()
{

	setupUi( this );
	// Required for the widget to receive key press events (this is
	// not working within table widgets, which spans most parts).
	setFocusPolicy( Qt::StrongFocus );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	const QString sWindowTitleBase = tr( "Playlist Browser" );

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pUndoStack = new QUndoStack( this );
	m_pUndoView = new QUndoView( m_pUndoStack );
	m_pUndoView->setWindowTitle( QString( "%1 - %2" )
								 .arg( sWindowTitleBase )
								 .arg( pCommonStrings->getUndoHistoryTitle() ) );

	const auto pPref = H2Core::Preferences::get_instance();
	
	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
				getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	setFont( font );
	m_pPlaylistTable->setFont( font );
	for ( int ii = 0; ii < m_pPlaylistTable->horizontalHeader()->count(); ii++ ) {
		m_pPlaylistTable->horizontalHeaderItem( ii )->setFont( font );
	}

	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	setWindowTitle( sWindowTitleBase + QString(" - ") +
					 pPlaylist->getFilename() );

	installEventFilter( this );

	m_pMenubar = new QMenuBar( this );
	populateMenuBar();

	QHBoxLayout* pMenuBarLayout = new QHBoxLayout( menuBarWidget );
	pMenuBarLayout->setSpacing(0);
	pMenuBarLayout->setContentsMargins( 0, 0, 0, 0 );
	pMenuBarLayout->addWidget( m_pMenubar );

	// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( controlWidget );
	pControlsPanel->setFixedSize( 119, 32 );
	pControlsPanel->setPixmap( "/playerControlPanel/playlist_background_Control.png" );

	// Rewind button
	m_pRwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "rewind.svg", "", false, QSize( 13, 13 ), tr("Rewind") );
	m_pRwdBtn->move( 4, 4 );
	connect(m_pRwdBtn, SIGNAL( clicked() ), this, SLOT( rewindBtnClicked() ));
	std::shared_ptr<Action> pAction = std::make_shared<Action>("PLAYLIST_PREV_SONG");
	m_pRwdBtn->setAction( pAction );

	// Play button
	m_pPlayBtn = new Button( pControlsPanel, QSize( 30, 21 ), Button::Type::Toggle, "play.svg", "", false, QSize( 13, 13 ), tr("Play/ Pause/ Load selected song") );
	m_pPlayBtn->move( 31, 4 );
	m_pPlayBtn->setChecked(false);
	connect(m_pPlayBtn, SIGNAL( clicked() ), this, SLOT( nodePlayBTN() ));
	pAction = std::make_shared<Action>("PLAY/PAUSE_TOGGLE");
	m_pPlayBtn->setAction( pAction );

	// Stop button
	m_pStopBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "stop.svg", "", false, QSize( 11, 11 ), tr("Stop") );
	m_pStopBtn->move( 63, 4 );
	connect(m_pStopBtn, SIGNAL( clicked() ), this, SLOT( nodeStopBTN() ));
	pAction = std::make_shared<Action>("STOP");
	m_pStopBtn->setAction( pAction );

	// Fast forward button
	m_pFfwdBtn = new Button( pControlsPanel, QSize( 25, 19 ), Button::Type::Push, "fast_forward.svg", "", false, QSize( 13, 13 ), tr("Fast Forward") );
	m_pFfwdBtn->move( 90, 4 );
	connect(m_pFfwdBtn, SIGNAL( clicked() ), this, SLOT( ffWDBtnClicked() ));
	pAction = std::make_shared<Action>("PLAYLIST_NEXT_SONG");
	m_pFfwdBtn->setAction( pAction );

	QVBoxLayout *pSideBarLayout = new QVBoxLayout(sideBarWidget);
	pSideBarLayout->setSpacing(0);
	pSideBarLayout->setMargin(0);

	// zoom-in btn
	Button *pUpBtn = new Button( nullptr, QSize( 16, 16 ), Button::Type::Push, "up.svg", "", false, QSize( 9, 9 ), tr( "sort" ) );
	connect(pUpBtn, SIGNAL( clicked() ), this, SLOT(o_upBClicked()) );
	pSideBarLayout->addWidget(pUpBtn);

	// zoom-in btn
	Button *pDownBtn = new Button( nullptr, QSize( 16, 16 ), Button::Type::Push, "down.svg", "", false, QSize( 9, 9 ), tr( "sort" ) );
	connect(pDownBtn, SIGNAL( clicked() ), this, SLOT(o_downBClicked()));
	pSideBarLayout->addWidget(pDownBtn);

	update();

	HydrogenApp::get_instance()->addEventListener( this );

	connect( m_pPlaylistTable, SIGNAL( itemSelectionChanged() ),
			 this, SLOT( updateMenuActivation() ) );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PlaylistEditor::onPreferencesChanged );
}

PlaylistEditor::~PlaylistEditor()
{
	INFOLOG ( "DESTROY" );
	delete m_pUndoStack;
}

void PlaylistEditor::populateMenuBar() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pShortcuts = pPref->getShortcuts();
	const QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	
	// menubar
	m_pMenubar->clear();
	m_actionsSelected.clear();
	m_actionsSelectedScript.clear();

	// Playlist menu
	m_pPlaylistMenu = m_pMenubar->addMenu( tr( "&Playlist" ) );

	m_pPlaylistMenu->addAction( tr( "&New Playlist" ), this,
								SLOT( newPlaylist() ),
								pShortcuts->getKeySequence( Shortcuts::Action::NewPlaylist ) );
	m_pPlaylistMenu->addAction( tr( "&Open Playlist" ), this, SLOT( openPlaylist() ),
								pShortcuts->getKeySequence( Shortcuts::Action::OpenPlaylist ) );
	m_pPlaylistMenu->addSeparator();
	m_pPlaylistMenu->addAction( tr( "&Save Playlist" ), this, SLOT( savePlaylist() ),
								pShortcuts->getKeySequence( Shortcuts::Action::SavePlaylist ) );
	m_pPlaylistMenu->addAction( tr( "Save Playlist &as" ), this,
								SLOT( savePlaylistAs() ),
								pShortcuts->getKeySequence( Shortcuts::Action::SaveAsPlaylist ) );
	m_pPlaylistMenu->addSeparator();				// -----
	m_pPlaylistMenu->addAction( tr( "Add song to Play&list" ), this,
								SLOT( addSong() ),
								pShortcuts->getKeySequence( Shortcuts::Action::PlaylistAddSong ) );
	m_pPlaylistMenu->addAction( tr( "Add &current song to Playlist" ), this,
								SLOT( addCurrentSong() ),
								pShortcuts->getKeySequence( Shortcuts::Action::PlaylistAddCurrentSong ) );
	m_pPlaylistMenu->addSeparator();				// -----
	m_actionsSelected.push_back(
		m_pPlaylistMenu->addAction(
			tr( "&Remove selected song from Playlist" ), this,
			SLOT( removeSong() ),
			pShortcuts->getKeySequence( Shortcuts::Action::PlaylistRemoveSong ) ) );
	m_pPlaylistMenu->setFont( font );

	// Undo menu
	m_pUndoMenu = m_pMenubar->addMenu( pCommonStrings->getUndoMenuUndo() );
	auto pUndoAction =
		m_pUndoMenu->addAction(
			pCommonStrings->getUndoMenuUndo(), this, SLOT( undo() ),
			pShortcuts->getKeySequence( Shortcuts::Action::Undo ) );
	pUndoAction->setEnabled( false );
	connect( m_pUndoStack, &QUndoStack::canUndoChanged,
			 [=]( bool bCanUndo ) {
				 if ( pUndoAction != nullptr ) {
					 pUndoAction->setEnabled( bCanUndo );
				 }
			 } );
	auto pRedoAction =
		m_pUndoMenu->addAction(
			pCommonStrings->getUndoMenuRedo(), this, SLOT( redo() ),
			pShortcuts->getKeySequence( Shortcuts::Action::Redo ) );
	pRedoAction->setEnabled( false );
	connect( m_pUndoStack, &QUndoStack::canRedoChanged,
			 [=]( bool bCanRedo ) {
				 if ( pRedoAction != nullptr ) {
					 pRedoAction->setEnabled( bCanRedo );
				 }
			 } );
	m_pUndoMenu->addAction( pCommonStrings->getUndoMenuHistory(), this,
							SLOT( showUndoHistory() ),
							pShortcuts->getKeySequence( Shortcuts::Action::ShowUndoHistory ) );

#ifndef WIN32
	// Script menu
	m_pScriptMenu = m_pMenubar->addMenu( tr( "&Scripts" ) );

	m_actionsSelected.push_back(
		m_pScriptMenu->addAction(
			tr( "&Create a new Script" ), this, SLOT( newScript() ),
			pShortcuts->getKeySequence( Shortcuts::Action::PlaylistCreateScript ) ) );
	m_actionsSelected.push_back(
		m_pScriptMenu->addAction(
			tr( "&Add Script to selected song" ), this, SLOT( loadScript() ),
			pShortcuts->getKeySequence( Shortcuts::Action::PlaylistAddScript ) ) );
	auto pScriptEditAction = m_pScriptMenu->addAction(
		tr( "&Edit selected Script" ), this, SLOT( editScript() ),
		pShortcuts->getKeySequence( Shortcuts::Action::PlaylistEditScript ) );
	m_actionsSelected.push_back( pScriptEditAction );
	m_actionsSelectedScript.push_back( pScriptEditAction );

	m_pScriptMenu->addSeparator();

	auto pScriptRemoveAction = m_pScriptMenu->addAction(
		tr( "&Remove selected Script" ), this, SLOT( removeScript() ),
		pShortcuts->getKeySequence( Shortcuts::Action::PlaylistRemoveScript ) );
	m_actionsSelected.push_back( pScriptRemoveAction );
	m_actionsSelectedScript.push_back( pScriptRemoveAction );

	m_pScriptMenu->setFont( font );
#endif
}

void PlaylistEditor::updateMenuActivation() {
	int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection.
		for ( auto& ppAction : m_actionsSelected ) {
			ppAction->setEnabled( false );
		}
	}
	else {
		const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
		if ( pPlaylist == nullptr || pPlaylist->size() == 0 ) {
			return;
		}

		for ( auto& ppAction : m_actionsSelected ) {
			ppAction->setEnabled( true );
		}

		// When deleting the last row, the current row can point temporarly
		// beyond the playlist.
		nIndex = std::clamp( nIndex, 0, pPlaylist->size() - 1 );

		const auto pEntry = pPlaylist->get( nIndex );
		if ( pEntry == nullptr ) {
			ERRORLOG( QString( "Unable to obtain song [%1]" ).arg( nIndex ) );
			return;
		}

		const bool bActivateScriptActions =
			! pEntry->getScriptPath().isEmpty() &&
			pEntry->getScriptPath() != PlaylistEntry::sLegacyEmptyScriptPath;
		for ( auto& ppAction : m_actionsSelectedScript ) {
			ppAction->setEnabled( bActivateScriptActions );
		}
	}
}

void PlaylistEditor::closeEvent( QCloseEvent* ev )
{
	HydrogenApp::get_instance()->showPlaylistEditor();
}

void PlaylistEditor::undo() {
	m_pUndoStack->undo();
}

void PlaylistEditor::redo() {
	m_pUndoStack->redo();
}

void PlaylistEditor::showUndoHistory() {
	m_pUndoView->show();
	m_pUndoView->setAttribute( Qt::WA_QuitOnClose, false );
}

void PlaylistEditor::addSong()
{
	auto pPref = Preferences::get_instance();
	QString sPath = pPref->getLastAddSongToPlaylistDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::songs_dir();
	}

	const QString sTitle = tr( "Add Songs to PlayList" );

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setWindowTitle( sTitle );
	fd.setFileMode( QFileDialog::ExistingFiles );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setDirectory( sPath );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}
	
	pPref->setLastAddSongToPlaylistDirectory( fd.directory().absolutePath() );

	m_pUndoStack->beginMacro( sTitle );
	for ( const auto& sPath : fd.selectedFiles() ) {
		auto pNewEntry = std::make_shared<PlaylistEntry>( sPath );
		auto pAction = new SE_addEntryToPlaylistAction( pNewEntry );
		m_pUndoStack->push( pAction );
	}
	m_pUndoStack->endMacro();
}

void PlaylistEditor::addCurrentSong()
{
	const std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	const auto sPath = pSong->getFilename();

	if ( sPath == "" ) {
		// just in case!
		QMessageBox::information(
			this, "Hydrogen", tr( "Please save your song first" ) );
		return;
	}

	auto pNewEntry = std::make_shared<PlaylistEntry>( sPath );
	auto pAction = new SE_addEntryToPlaylistAction( pNewEntry );
	m_pUndoStack->push( pAction );
}

void PlaylistEditor::removeSong() {
	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}
	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	const auto pEntry = pPlaylist->get( nIndex );

	if ( pEntry == nullptr ) {
		ERRORLOG( QString( "Entry [%1: %2] could not be found in playlist [%3]" )
				  .arg( nIndex ).arg( pPlaylist->toQString() ) );
		return;
	}

	auto pAction = new SE_removeEntryFromPlaylistAction( pEntry, nIndex );
	m_pUndoStack->push( pAction );
}

void PlaylistEditor::newPlaylist()
{
	if ( ! HydrogenApp::handleUnsavedChanges( Filesystem::Type::Playlist ) ) {
		return;
	}

	auto pNewPlaylist = std::make_shared<Playlist>();
	pNewPlaylist->setFilename(
		Filesystem::empty_path( Filesystem::Type::Playlist ) );
	auto pAction = new SE_replacePlaylistAction( pNewPlaylist );
	m_pUndoStack->push( pAction );

	// Since the user explicitly chooses to open an empty playlist, we do not
	// attempt to recover the autosave file generated while last working on an
	// empty playlist but, instead, remove the corresponding autosave file in
	// order to start fresh.
	QFileInfo fileInfo( Filesystem::empty_path( Filesystem::Type::Playlist ) );
	QString sBaseName( fileInfo.completeBaseName() );
	if ( sBaseName.startsWith( "." ) ) {
		sBaseName.remove( 0, 1 );
	}
	QFileInfo autoSaveFile( QString( "%1/.%2.autosave%3" )
							.arg( fileInfo.absoluteDir().absolutePath() )
							.arg( sBaseName )
							.arg( Filesystem::playlist_ext ) );
	if ( autoSaveFile.exists() ) {
		Filesystem::rm( autoSaveFile.absoluteFilePath() );
	}

	return;
}

void PlaylistEditor::openPlaylist() {
	if ( ! HydrogenApp::handleUnsavedChanges( Filesystem::Type::Playlist ) ) {
		return;
	}

	auto pPref = Preferences::get_instance();
	QString sPath = pPref->getLastPlaylistDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::playlists_dir();
	}

	FileDialog fd( nullptr );
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setWindowTitle( tr( "Load Playlist" ) );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( sPath );
	fd.setNameFilter( Filesystem::playlists_filter_name );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	const QString sFilePath = fd.selectedFiles().first();

	const auto sRecoverFilename = HydrogenApp::findAutoSaveFile(
		Filesystem::Type::Playlist, sFilePath );

	auto pPlaylist = CoreActionController::loadPlaylist(
		sFilePath, sRecoverFilename );
	if ( pPlaylist == nullptr ) {
		QMessageBox msgBox;
		// Not commonized in CommmonStrings as it is required before
		// HydrogenApp was instantiated.
		msgBox.setText( QString( "%1: [%2]" )
						.arg( tr( "Unable to open playlist" ) )
						.arg( sFilePath ) );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Warning );
		msgBox.exec();
		return;
	}

	auto pAction = new SE_replacePlaylistAction( pPlaylist );
	m_pUndoStack->push( pAction );

	pPref->setLastPlaylistDirectory( fd.directory().absolutePath() );
}

void PlaylistEditor::newScript()
{
	auto pPref = Preferences::get_instance();

	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}

	QString sPath = pPref->getLastPlaylistScriptDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::scripts_dir();
	}



	FileDialog fd(this);
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::scripts_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setWindowTitle( tr( "New Script" ) );
	fd.setDirectory( sPath );

	QString defaultFilename;

	defaultFilename += ".sh";

	fd.selectFile( defaultFilename );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	const auto sFilePath = fd.selectedFiles().first();

	if ( sFilePath.contains( " ", Qt::CaseInsensitive ) ) {
		QMessageBox::information( this, "Hydrogen",
			tr( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must be without whitespaces.") );
		return;
	}

	QFile chngPerm( sFilePath );
	if ( ! chngPerm.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
		QMessageBox::critical( this, "Hydrogen",
			tr( "Unable to open selected file with write access" ) +
			QString( ": [%1]" ).arg( sFilePath ) );
		return;
	}

	pPref->setLastPlaylistScriptDirectory( fd.directory().absolutePath() );

	QTextStream out(&chngPerm);
	out <<  "#!/bin/sh\n\n#have phun";
	chngPerm.close();

	if ( chngPerm.exists() ) {
		chngPerm.setPermissions( QFile::ReadOwner | QFile::WriteOwner |
								 QFile::ExeOwner );
		QMessageBox::warning( this, "Hydrogen",
			tr( "The new file is executable by the owner of the file!" ) );
	}

	if ( pPref->getDefaultEditor().isEmpty() ){
		QMessageBox::information( this, "Hydrogen",
			tr( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";

		FileDialog fd(this);
		fd.setAcceptMode( QFileDialog::AcceptOpen );
		fd.setFileMode( QFileDialog::ExistingFile );
		fd.setDirectory( lastUsedDir );

		fd.setWindowTitle( tr( "Set your Default Editor" ) );

		QString sEditorPath;
		if ( fd.exec() == QDialog::Accepted ){
			sEditorPath = fd.selectedFiles().first();

			pPref->setDefaultEditor( sEditorPath );
		}
	}

	QString openfile = pPref->getDefaultEditor() + " " + sFilePath + "&";
	std::system( openfile.toLatin1() );

	auto pOldEntry = Hydrogen::get_instance()->getPlaylist()->get( nIndex );

	auto pNewEntry = std::make_shared<PlaylistEntry>( pOldEntry );
	pNewEntry->setScriptPath( sFilePath );

	m_pUndoStack->beginMacro( tr( "Edit playlist scripts" ) );

	auto pAction1 = new SE_removeEntryFromPlaylistAction( pOldEntry, nIndex );
	m_pUndoStack->push( pAction1 );
	auto pAction2 = new SE_addEntryToPlaylistAction( pNewEntry, nIndex );
	m_pUndoStack->push( pAction2 );

	m_pUndoStack->endMacro();

	return;
}

bool PlaylistEditor::savePlaylistAs() {
	auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pPlaylist = pHydrogen->getPlaylist();
	const auto sLastFilename = pPlaylist->getFilename();

	QString sPath = pPref->getLastPlaylistDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::playlists_dir();
	}
	
	FileDialog fd( nullptr );
	fd.setWindowTitle( tr( "Save Playlist" ) );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::playlists_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setDirectory( sPath );
	fd.selectFile( Filesystem::untitled_playlist_file_name() );
	fd.setDefaultSuffix( Filesystem::playlist_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return false;
	}

	QString filename = fd.selectedFiles().first();

	if ( ! CoreActionController::savePlaylistAs( filename ) ) {
		QMessageBox::critical( nullptr, "Hydrogen",
							   pCommonStrings->getPlaylistSaveFailure() );
		return false;
	}

	pPref->setLastPlaylistDirectory( fd.directory().absolutePath() );

	if ( sLastFilename == Filesystem::empty_path( Filesystem::Type::Playlist ) ) {
		// In case we stored the playlist for the first time, we remove the
		// autosave file corresponding to the empty one. Else, it might be
		// loaded later when clicking "New Playlist" while not generating a new
		// autosave file.
		const QString sAutoSaveFile = Filesystem::getAutoSaveFilename(
			Filesystem::Type::Playlist, sLastFilename );
		if ( Filesystem::file_exists( sAutoSaveFile, true ) ) {
			Filesystem::rm( sAutoSaveFile );
		}
	}

	return true;

}

bool PlaylistEditor::savePlaylist()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pPlaylist = pHydrogen->getPlaylist();

	if ( pPlaylist->getFilename().isEmpty() ||
		 pPlaylist->getFilename() ==
		 Filesystem::empty_path( Filesystem::Type::Playlist ) ) {
		return savePlaylistAs();
	}

	if ( ! CoreActionController::savePlaylist() ) {
		QMessageBox::critical( this, "Hydrogen",
							   pCommonStrings->getPlaylistSaveFailure() );
		return false;
	}

	return true;
}

void PlaylistEditor::loadScript() {
	auto pPref = Preferences::get_instance();
	QString sPath = pPref->getLastPlaylistScriptDirectory();

	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}

	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::scripts_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( sPath );
	fd.setNameFilter( tr( "Hydrogen Playlist (*.sh)" ) );
	fd.setWindowTitle( tr( "Add Script to selected Song" ) );

	QString filename;
	if ( fd.exec() != QDialog::Accepted ){
		return;
	}

	const auto sScriptPath = fd.selectedFiles().first();

	if ( sScriptPath.contains( " ", Qt::CaseInsensitive ) ) {
		QMessageBox::critical( this, "Hydrogen",
			  tr( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must without whitespaces.") );
		return;
	}

	pPref->setLastPlaylistScriptDirectory( fd.directory().absolutePath() );

	auto pOldEntry = Hydrogen::get_instance()->getPlaylist()->get( nIndex );

	auto pNewEntry = std::make_shared<PlaylistEntry>( pOldEntry );
	pNewEntry->setScriptPath( sScriptPath );

	m_pUndoStack->beginMacro( tr( "Edit playlist scripts" ) );

	auto pAction1 = new SE_removeEntryFromPlaylistAction( pOldEntry, nIndex );
	m_pUndoStack->push( pAction1 );
	auto pAction2 = new SE_addEntryToPlaylistAction( pNewEntry, nIndex );
	m_pUndoStack->push( pAction2 );

	m_pUndoStack->endMacro();
}

void PlaylistEditor::removeScript() {
	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}

	auto pOldEntry = Hydrogen::get_instance()->getPlaylist()->get( nIndex );
	if ( pOldEntry->getScriptPath().isEmpty() ) {
		// Nothing to do
		return;
	}

	auto pNewEntry = std::make_shared<PlaylistEntry>( pOldEntry );
	pNewEntry->setScriptPath( "" );

	m_pUndoStack->beginMacro( tr( "Edit playlist scripts" ) );

	auto pAction1 = new SE_removeEntryFromPlaylistAction( pOldEntry, nIndex );
	m_pUndoStack->push( pAction1 );
	auto pAction2 = new SE_addEntryToPlaylistAction( pNewEntry, nIndex );
	m_pUndoStack->push( pAction2 );

	m_pUndoStack->endMacro();
}

void PlaylistEditor::editScript()
{
	auto pPref = Preferences::get_instance();
	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}

	if ( pPref->getDefaultEditor().isEmpty() ) {
		QMessageBox::information( this, "Hydrogen",
			tr( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";

		FileDialog fd(this);
		fd.setAcceptMode( QFileDialog::AcceptOpen );
		fd.setFileMode( QFileDialog::ExistingFile );
		fd.setDirectory( lastUsedDir );

		fd.setWindowTitle( tr( "Set your Default Editor" ) );

		QString filename;
		if ( fd.exec() == QDialog::Accepted ){
			filename = fd.selectedFiles().first();

			pPref->setDefaultEditor( filename );
		}
	}

	auto pEntry = Hydrogen::get_instance()->getPlaylist()->get( nIndex );

	QString sCommand = pPref->getDefaultEditor() + " " +
		pEntry->getScriptPath() + " &";

	if( !QFile( pEntry->getSongPath() ).exists() ){
		QMessageBox::information( this, "Hydrogen", tr( "No Script selected!" ));
		return;
	}

	std::system( sCommand.toLatin1() );
	
	return;
}

void PlaylistEditor::o_upBClicked() {
	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 || nIndex == 0 ) {
		// No selection or already on top.
		return;
	}

	moveRow( nIndex, nIndex - 1 );
}

void PlaylistEditor::o_downBClicked() {
	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 || nIndex == m_pPlaylistTable->rowCount() - 1 ) {
		// No selection or already at the bottom.
		return;
	}

	moveRow( nIndex, nIndex + 1 );
}

void PlaylistEditor::moveRow( int nFrom, int nTo ) {
	if ( nFrom < 0 || nFrom >= m_pPlaylistTable->rowCount() ||
		 nTo < 0 || nTo > m_pPlaylistTable->rowCount() ) {
		ERRORLOG( QString( "Provided rows [%1 -> %2] out of bound [0, %3]" )
				  .arg( nFrom ).arg( nTo )
				  .arg( m_pPlaylistTable->rowCount() ) );
		return;
	}
	if ( nFrom == nTo ) {
		return;
	}

	const auto pEntry = Hydrogen::get_instance()->getPlaylist()->get( nFrom );
	if ( pEntry == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve entry [%1]" ).arg( nFrom ));
		return;
	}

	m_pPlaylistTable->setCurrentCell( nTo, 0 );

	m_pUndoStack->beginMacro( tr( "Edit playlist" ) );

	auto pAction1 = new SE_removeEntryFromPlaylistAction( pEntry, nFrom );
	m_pUndoStack->push( pAction1 );
	auto pAction2 = new SE_addEntryToPlaylistAction( pEntry, nTo );
	m_pUndoStack->push( pAction2 );

	m_pUndoStack->endMacro();
}

void PlaylistEditor::on_m_pPlaylistTable_itemClicked( QTableWidgetItem* pItem ) {
	// Only act on "script enabled" column
	if ( pItem->column() != 2 ) {
		return;
	}

	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		WARNINGLOG( "No selection while clicking an item? Something is off..." );
		return;
	}
	auto pOldEntry = Hydrogen::get_instance()->getPlaylist()->get( nIndex );
	if ( pOldEntry == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve entry [%1]" ).arg( nIndex ));
		pItem->setCheckState( Qt::Unchecked );
		return;
	}

	if ( pOldEntry->getScriptPath().isEmpty() ||
		 pOldEntry->getScriptPath() == PlaylistEntry::sLegacyEmptyScriptPath ) {
		WARNINGLOG( QString( "No script set in entry [%1]" ).arg( nIndex ) );
		pItem->setCheckState( Qt::Unchecked );
		return;
	}

	auto pNewEntry = std::make_shared<PlaylistEntry>( pOldEntry );
	pNewEntry->setScriptEnabled( ! pOldEntry->getScriptEnabled() );

	m_pUndoStack->beginMacro( tr( "Edit playlist scripts" ) );

	auto pAction1 = new SE_removeEntryFromPlaylistAction( pOldEntry, nIndex );
	m_pUndoStack->push( pAction1 );
	auto pAction2 = new SE_addEntryToPlaylistAction( pNewEntry, nIndex );
	m_pUndoStack->push( pAction2 );

	m_pUndoStack->endMacro();
}

void PlaylistEditor::nodePlayBTN()
{
	Hydrogen *		pHydrogen = Hydrogen::get_instance();
	HydrogenApp *	pH2App = HydrogenApp::get_instance();
	const auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr || pPlaylist->size() == 0 ) {
		return;
	}

	auto onFailure = [=](){
		QMessageBox::warning( this, "Hydrogen", tr( "No valid song selected!" ) );
		m_pPlayBtn->setChecked( false );
	};

	const int nIndex = m_pPlaylistTable->currentRow();
	if ( nIndex == -1 ) {
		ERRORLOG( "No selection" );
		onFailure();
		return;
	}
	const auto pEntry = pHydrogen->getPlaylist()->get( nIndex );
	if ( pEntry == nullptr ) {
		ERRORLOG( QString( "Could not retrieve song [%1]" ).arg( nIndex ) );
		onFailure();
		return;
	}

	if ( pEntry->getSongPath() != pHydrogen->getSong()->getFilename() ) {

		if ( ! HydrogenApp::openFile( Filesystem::Type::Song,
									  pEntry->getSongPath() ) ) {
			ERRORLOG( QString( "Unable to load song [%1]" )
					  .arg( pEntry->getSongPath() ) );
			m_pPlayBtn->setChecked(false);
			return;
		}
		CoreActionController::activatePlaylistSong( nIndex );
	}

	if ( m_pPlayBtn->isChecked() ) {
		pHydrogen->sequencerPlay();
	}
	else {
		pHydrogen->sequencerStop();
		pH2App->showStatusBarMessage( tr("Pause.") );
	}
}

void PlaylistEditor::nodeStopBTN()
{
	m_pPlayBtn->setChecked(false);
	Hydrogen::get_instance()->sequencerStop();
	CoreActionController::locateToColumn( 0 );
}

void PlaylistEditor::ffWDBtnClicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1 );
}

void PlaylistEditor::rewindBtnClicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	CoreActionController::locateToColumn(
		pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() - 1 );
}

bool PlaylistEditor::eventFilter( QObject *o, QEvent *e )
{
	UNUSED( o );
	if ( e->type() == QEvent::KeyPress ) {
		// special processing for key press
		QKeyEvent* pKeyEvent = dynamic_cast<QKeyEvent*>(e);
		assert( pKeyEvent != nullptr );

		if ( ! handleKeyEvent( pKeyEvent ) ) {
			return HydrogenApp::get_instance()->getMainForm()->eventFilter( o, e );
		}
	}

	return false;
}

bool PlaylistEditor::handleKeyEvent( QKeyEvent* pKeyEvent ) {
	auto pShortcuts = Preferences::get_instance()->getShortcuts();
	auto pActionManager = MidiActionManager::get_instance();
	
	int nKey = pKeyEvent->key();

	if ( nKey == Qt::Key_Escape ) {
		// Close window when hitting ESC.
		HydrogenApp::get_instance()->showPlaylistEditor();
		return true;
	}
	else if ( nKey == Qt::Key_Enter || nKey == Qt::Key_Return ) {
		// Loading a song by seleting it via keyboard and pressing Enter.
		if ( m_pPlaylistTable->hasFocus() ) {
			m_pPlaylistTable->loadCurrentRow();
			return true;
		}
	}
	
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
	bool bHandled = false;
	
	const auto actions = pShortcuts->getActions( keySequence );
	for ( const auto& action : actions ) {
		
		switch ( action ) {
		case Shortcuts::Action::PlaylistAddSong:
			addSong();
			bHandled = true;
			break;
			
		case Shortcuts::Action::PlaylistAddCurrentSong:
			addCurrentSong();
			bHandled = true;
			break;
			
		case Shortcuts::Action::PlaylistRemoveSong:
			removeSong();
			bHandled = true;
			break;
			
		case Shortcuts::Action::NewPlaylist:
			newPlaylist();
			bHandled = true;
			break;
			
		case Shortcuts::Action::OpenPlaylist:
			openPlaylist();
			bHandled = true;
			break;
			
		case Shortcuts::Action::SavePlaylist:
			savePlaylist();
			bHandled = true;
			break;
			
		case Shortcuts::Action::SaveAsPlaylist:
			savePlaylistAs();
			bHandled = true;
			break;

		case Shortcuts::Action::Undo:
			undo();
			bHandled = true;
			break;

		case Shortcuts::Action::Redo:
			redo();
			bHandled = true;
			break;

		case Shortcuts::Action::ShowUndoHistory:
			showUndoHistory();
			bHandled = true;
			break;

#ifndef WIN32
		case Shortcuts::Action::PlaylistAddScript:
			loadScript();
			bHandled = true;
			break;
			
		case Shortcuts::Action::PlaylistEditScript:
			editScript();
			bHandled = true;
			break;
			
		case Shortcuts::Action::PlaylistRemoveScript:
			removeScript();
			bHandled = true;
			break;
			
		case Shortcuts::Action::PlaylistCreateScript:
			newScript();
			bHandled = true;
			break;
#endif
		default:
			bHandled = false;
		}
	}

	if ( bHandled ) {
		// Event consumed by the actions triggered above.
		pKeyEvent->accept();
		return true;
	}

	return false;
}

void PlaylistEditor::playlistChangedEvent( int nValue ) {
	if ( nValue == 0 ) {
		update();
	}
	else if ( nValue == 1 ) {
		updateWindowTitle();
	}
	// case 2 is handled by HydrogenApp.
}

void PlaylistEditor::playlistLoadSongEvent() {
	m_pPlaylistTable->update();
}

void PlaylistEditor::update() {
	m_pPlaylistTable->update();
	updateWindowTitle();
}

void PlaylistEditor::updateWindowTitle() {

	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "No playlist" );
		return;
	}


	QString sWindowTitle = tr( "Playlist Browser" );
	if ( ! pPlaylist->getFilename().isEmpty() &&
		 pPlaylist->getFilename() !=
		 Filesystem::empty_path( Filesystem::Type::Playlist ) ) {
		sWindowTitle.append( QString(" - %1").arg( pPlaylist->getFilename() ) );
	}

	if ( pPlaylist->getIsModified() ) {
		sWindowTitle.append( QString( " (%1)" ).arg(
			HydrogenApp::get_instance()->getCommonStrings()->getIsModified() ) );
	}

	setWindowTitle( sWindowTitle );
}

void PlaylistEditor::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		QFont childFont( pPref->getTheme().m_font.m_sLevel2FontFamily,
						 getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		setFont( font );
		m_pMenubar->setFont( font );
		m_pPlaylistMenu->setFont( font );
		m_pUndoMenu->setFont( font );
#ifndef WIN32
		m_pScriptMenu->setFont( font );
#endif

		for ( int ii = 0; ii < m_pPlaylistTable->columnCount(); ii++ ) {
			m_pPlaylistTable->horizontalHeaderItem( ii )->setFont( font );
		}

		for ( int ii = 0; ii < m_pPlaylistTable->rowCount(); ii++ ) {
			m_pPlaylistTable->item( ii, 0 )->setFont( font );
#ifndef WIN32
			m_pPlaylistTable->item( ii, 1 )->setFont( font );
			m_pPlaylistTable->item( ii, 2 )->setFont( font );
#endif
		}
	}
	
	if ( changes & H2Core::Preferences::Changes::ShortcutTab ) {
		populateMenuBar();
	}
}

PlaylistTableWidget::PlaylistTableWidget( QWidget* pParent )
	: QTableWidget( pParent )
	, m_pLastSelectedEntry( nullptr ) {

	setDropIndicatorShown( true );
	setDragEnabled( true );
	setAcceptDrops( true );
	setMouseTracking( true );
	setDragDropMode( DragDropMode::InternalMove );
	setSelectionBehavior( QAbstractItemView::SelectRows );
	setSelectionMode( QAbstractItemView::SingleSelection );

#ifdef WIN32
	setColumnCount( 1 );

	QStringList headers;
	headers << tr( "Song list" );
	setHorizontalHeaderLabels( headers );
	horizontalHeader()->setSectionResizeMode( 0, QHeaderView::Stretch );

#else
	setColumnCount( 3 );

	QStringList headers;
	headers << tr( "Song list" ) << tr( "Script" ) << tr( "exec Script" );
	setHorizontalHeaderLabels( headers );

	horizontalHeader()->setStretchLastSection( false );
	horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Stretch );
	horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::ResizeToContents );
	horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::ResizeToContents );

#endif
	setSelectionBehavior( QAbstractItemView::SelectRows );
	setSelectionMode( QAbstractItemView::SingleSelection );

	setAlternatingRowColors( true );

	// Remember the playlist entry corresponding to the last selected row (in
	// order to restore it when redrawing the whole tree).
	connect( this, &QTableWidget::itemSelectionChanged,
			 [=]() { int nRow = currentRow();
				 const auto pPlaylist =
					 H2Core::Hydrogen::get_instance()->getPlaylist();
				 if ( nRow == -1 || pPlaylist == nullptr ||
					  pPlaylist->size() == 0 ) {
					 m_pLastSelectedEntry = nullptr;
				 }
				 else {
					 // When deleting the last row, the current row can point
					 // temporarly beyond the playlist.
					 nRow = std::clamp( nRow, 0, pPlaylist->size() - 1 );
					 m_pLastSelectedEntry = pPlaylist->get( nRow );
				 }
			 });
}

void PlaylistTableWidget::mousePressEvent( QMouseEvent* pEvent ) {
	// Select the row the user just clicked.
	const auto pItem = itemAt( pEvent->pos() );

	// In case no item was found the selection is cleared.
	setCurrentItem( pItem );
	if ( pItem == nullptr ) {
		return;
	}

    if ( pEvent->button() == Qt::LeftButton ) {
		if ( pItem == nullptr ) {
			return;
		}

		if ( currentRow() != -1 ) {
			m_dragStartPosition = pEvent->pos();;
		} else {
			// No row selected
			m_dragStartPosition = QPoint( 0, 0 );
		}
	}

	QTableWidget::mousePressEvent( pEvent );
}

void PlaylistTableWidget::mouseMoveEvent( QMouseEvent* pEvent ) {
    if ( ! ( pEvent->buttons() & Qt::LeftButton ) ||
		 m_dragStartPosition.isNull() ) {
		return;
	}

	if ( ( pEvent->pos() - m_dragStartPosition ).manhattanLength() <
		 QApplication::startDragDistance() ) {
        return;
	}

	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist == nullptr || pPlaylist->size() == 0 ) {
		return;
	}
	const auto pItem = itemAt( m_dragStartPosition );
	if ( pItem == nullptr ) {
		return;
	}
	const auto nIndex = row( pItem );
	const auto pEntry = pPlaylist->get( nIndex );
	if ( pEntry == nullptr ) {
		ERRORLOG( QString( "Unable to obtain song [%1]" ).arg( nIndex ) );
		return;
	}

    QDrag* pDrag = new QDrag( this );
    QMimeData* pMimeData = new QMimeData;
    pMimeData->setText( pEntry->toMimeText() );
	pDrag->setMimeData( pMimeData );

    pDrag->exec( Qt::CopyAction );
}

void PlaylistTableWidget::mouseDoubleClickEvent( QMouseEvent* pEvent ) {
	loadCurrentRow();
}

void PlaylistTableWidget::dragEnterEvent( QDragEnterEvent* pEvent ) {
	pEvent->acceptProposedAction();
}

void PlaylistTableWidget::dropEvent( QDropEvent* pEvent ) {
	const auto pFromItem = itemAt( m_dragStartPosition );
	if ( pFromItem == nullptr ) {
		ERRORLOG( QString( "No valid source of dragging at [y: %1]" )
				  .arg( m_dragStartPosition.y() ) );
		return;
	}
	int nFrom = row( pFromItem );

	const auto pToItem = itemAt( pEvent->pos() );
	int nTo;
	if ( pToItem == nullptr ) {
		// Dragged beyond the last row.
		nTo = rowCount() - 1;
	}
	else {
		nTo = row( pToItem );
	}

	HydrogenApp::get_instance()->getPlaylistEditor()->moveRow( nFrom, nTo );
}

void PlaylistTableWidget::loadCurrentRow() {
	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist == nullptr || pPlaylist->size() == 0 ) {
		return;
	}
	const int nIndex = currentRow();
	if ( nIndex == -1 ) {
		// No selection
		return;
	}
	const auto pEntry = pPlaylist->get( nIndex );
	if ( pEntry == nullptr ) {
		ERRORLOG( QString( "Unable to obtain song [%1]" ).arg( nIndex ) );
		return;
	}

	HydrogenApp *pH2App = HydrogenApp::get_instance();
	if ( ! HydrogenApp::openFile( Filesystem::Type::Song,
								  pEntry->getSongPath() ) ) {
		return;
	}
	// We don't want to overload HydrogenApp::openFile too much with
	// optional functionality. Therefore, we trigger the event in here
	// directly (to tell the remainder of H2 that the loaded song is
	// associated to a playlist).
	CoreActionController::activatePlaylistSong( nIndex );
}


void PlaylistTableWidget::update() {
	const auto pPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "No playlist" );
		return;
	}
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	clearContents();
	setRowCount( pPlaylist->size() );

	// Highlight cells corresponding to non-existing song files to indicate to
	// the user that something is wrong.
	auto colorNonExisting = [=]( QTableWidgetItem* pItem ) {
		const auto colorTheme =
			H2Core::Preferences::get_instance()->getTheme().m_color;
		pItem->setBackground( QBrush( colorTheme.m_buttonRedColor ) );
		pItem->setForeground( QBrush( colorTheme.m_buttonRedTextColor ) );
	};
	auto colorDefault = [=]( QTableWidgetItem* pItem ) {
		pItem->setBackground( QBrush( ) );
		pItem->setForeground( QBrush( ) );
	};
	auto colorActive = [=]( QTableWidgetItem* pItem ) {
		const auto colorTheme =
			H2Core::Preferences::get_instance()->getTheme().m_color;

		// Highlighting of non-existing file has higher priority.
		if ( pItem->background().color() != colorTheme.m_buttonRedColor ) {
			pItem->setBackground( QBrush( colorTheme.m_accentColor ) );
			pItem->setForeground( QBrush( colorTheme.m_accentTextColor ) );
		}
	};

	if ( pPlaylist->size() > 0 ) {

		int nSelectedRow = -1;
		int nnRowCount = 0;
		for ( const auto& ppEntry : *pPlaylist ){
			auto pSongItem = new QTableWidgetItem();
			if ( ppEntry->getSongExists() ) {
				pSongItem->setText( ppEntry->getSongPath() );
				colorDefault( pSongItem );
			}
			else {
				pSongItem->setText( QString( "%1: %2" )
					.arg( pCommonStrings->getErrorNotFoundShort() )
					.arg( ppEntry->getSongPath() ) );
				colorNonExisting( pSongItem );
			}
			pSongItem->setFlags( Qt::ItemIsDragEnabled | Qt::ItemIsSelectable |
								 Qt::ItemIsEnabled | Qt::ItemIsDropEnabled );
			setItem( nnRowCount, 0, pSongItem );

			if ( m_pLastSelectedEntry == ppEntry ) {
				nSelectedRow = nnRowCount;
			}
#ifndef WIN32
			// In order to not break existing UX, we display a fallback string
			// instead of an empty cell.
			auto pScriptItem = new QTableWidgetItem();
			QString sScriptText( ppEntry->getScriptPath() );
			if ( sScriptText.isEmpty() ||
				 sScriptText == PlaylistEntry::sLegacyEmptyScriptPath ) {
				pScriptItem->setText( tr( "no Script" ) );
				colorDefault( pScriptItem );
			}
			else if ( ppEntry->getScriptExists() ) {
				pScriptItem->setText( ppEntry->getScriptPath() );
				colorDefault( pScriptItem );
			}
			else {
				pScriptItem->setText( QString( "%1: %2" )
					.arg( pCommonStrings->getErrorNotFoundShort() )
					.arg( ppEntry->getScriptPath() ) );
				colorNonExisting( pScriptItem );
			}
			pScriptItem->setFlags( Qt::ItemIsDragEnabled | Qt::ItemIsSelectable |
								 Qt::ItemIsEnabled | Qt::ItemIsDropEnabled );
				setItem( nnRowCount, 1, pScriptItem );

			auto pCheckboxItem = new QTableWidgetItem();
			pCheckboxItem->setFlags( Qt::ItemIsDragEnabled | Qt::ItemIsSelectable |
								 Qt::ItemIsEnabled | Qt::ItemIsDropEnabled );
			if ( ppEntry->getScriptEnabled() ) {
				pCheckboxItem->setCheckState( Qt::Checked );
			} else {
				pCheckboxItem->setCheckState( Qt::Unchecked );
			}
			colorDefault( pCheckboxItem );
			setItem( nnRowCount, 2, pCheckboxItem );
#endif
			nnRowCount++;
		}

		// restore the selected item
		if ( nSelectedRow != -1 ) {
			setCurrentCell( nSelectedRow, 0 );
		} else {
			setCurrentCell( 0, 0 );
		}

		const int nActiveSongNumber = pPlaylist->getActiveSongNumber();
		if ( nActiveSongNumber != -1 ) {
			for ( int nnColumn = 0; nnColumn < columnCount(); nnColumn++ ) {
				auto pItem = item( nActiveSongNumber, nnColumn );
				if ( pItem == nullptr ) {
					continue;
				}

				colorActive( pItem );
			}
		}
	}
}
