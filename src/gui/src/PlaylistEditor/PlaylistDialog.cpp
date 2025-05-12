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


#include "PlaylistDialog.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../InstrumentRack.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SongEditor/SongEditorPanel.h"
#include "Widgets/PixmapWidget.h"

#include <core/Helpers/Files.h>
#include <core/Helpers/Filesystem.h>
#include <core/H2Exception.h>
#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Timeline.h>
#include <core/EventQueue.h>
#include <core/Basics/Playlist.h>

#include "../Widgets/Button.h"
#include "../Widgets/FileDialog.h"

#include <QTreeWidget>
#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

using namespace H2Core;


PlaylistDialog::PlaylistDialog ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ()
{

	setupUi ( this );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	auto pPref = H2Core::Preferences::get_instance();
	
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	setFont( font );
	m_pPlaylistTree->setFont( font );
	
	setWindowTitle ( tr ( "Playlist Browser" ) + QString(" - ") + Playlist::get_instance()->getFilename() );

	installEventFilter( this );

	// menubar
	m_pMenubar = new QMenuBar( this );

	// Playlist menu
	m_pPlaylistMenu = m_pMenubar->addMenu( tr( "&Playlist" ) );

	m_pPlaylistMenu->addAction( tr( "Add song to Play&list" ), this, SLOT( addSong() ), QKeySequence( "Ctrl+A" ) );
	m_pPlaylistMenu->addAction( tr( "Add &current song to Playlist" ), this, SLOT( addCurrentSong() ), QKeySequence( "Ctrl+Alt+A" ) );
	m_pPlaylistMenu->addSeparator();				// -----
	m_pPlaylistMenu->addAction( tr( "&Remove selected song from Playlist" ), this, SLOT( removeFromList() ), QKeySequence::Delete );
	m_pPlaylistMenu->addAction( tr( "&New Playlist" ), this, SLOT( clearPlaylist() ), QKeySequence( "Ctrl+N" ) );
	m_pPlaylistMenu->addSeparator();
	m_pPlaylistMenu->addAction( tr( "&Open Playlist" ), this, SLOT( loadList() ), QKeySequence( "Ctrl+O" ) );
	m_pPlaylistMenu->addSeparator();
	m_pPlaylistMenu->addAction( tr( "&Save Playlist" ), this, SLOT( saveList() ), QKeySequence( "Ctrl+S" ) );
	m_pPlaylistMenu->addAction( tr( "Save Playlist &as" ), this, SLOT( saveListAs() ), QKeySequence( "Ctrl+Shift+S" ) );
	m_pPlaylistMenu->setFont( font );

#ifdef WIN32
	//no scripts under windows
#else
	// Script menu
	m_pScriptMenu = m_pMenubar->addMenu( tr( "&Scripts" ) );

	m_pScriptMenu->addAction( tr( "&Add Script to selected song" ), this, SLOT( loadScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addAction( tr( "&Edit selected Script" ), this, SLOT( editScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( tr( "&Remove selected Script" ), this, SLOT( removeScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( tr( "&Create a new Script" ), this, SLOT( newScript() ), QKeySequence( "" ) );
	m_pScriptMenu->setFont( font );
#endif

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

#ifdef WIN32
	QStringList headers;
	headers << tr ( "Song list" );
	QTreeWidgetItem* header = new QTreeWidgetItem ( headers );
	m_pPlaylistTree->setHeaderItem ( header );
	m_pPlaylistTree->setAlternatingRowColors( true );
	for ( int ii = 0; ii < m_pPlaylistTree->headerItem()->columnCount(); ii++ ) {
		m_pPlaylistTree->headerItem()->setFont( ii, font );
	}

		/*addSongBTN->setEnabled ( true );
	loadListBTN->setEnabled ( true );
	removeFromListBTN->setEnabled ( false );
	removeFromListBTN->setEnabled ( false );
	saveListBTN->setEnabled ( false );
	saveListAsBTN->setEnabled ( false );
	loadScriptBTN->hide();
	removeScriptBTN->hide();
	editScriptBTN->hide();
	newScriptBTN->hide();
		clearPlBTN->setEnabled ( false );*/

#else
	QStringList headers;
	headers << tr ( "Song list" ) << tr ( "Script" ) << tr ( "exec Script" );
	QTreeWidgetItem* header = new QTreeWidgetItem ( headers );
	m_pPlaylistTree->setHeaderItem ( header );

	m_pPlaylistTree->setColumnWidth( 0, 405 );
	m_pPlaylistTree->setColumnWidth( 1, 405 );
	m_pPlaylistTree->setColumnWidth( 2, 105 );

	m_pPlaylistTree->header()->setStretchLastSection( false );
	m_pPlaylistTree->header()->setSectionResizeMode( 0, QHeaderView::Stretch );
	m_pPlaylistTree->header()->setSectionResizeMode( 1, QHeaderView::Stretch );
	m_pPlaylistTree->header()->setSectionResizeMode( 2, QHeaderView::Fixed );

	m_pPlaylistTree->setAlternatingRowColors( true );
	for ( int ii = 0; ii < m_pPlaylistTree->headerItem()->columnCount(); ii++ ) {
		m_pPlaylistTree->headerItem()->setFont( ii, font );
	}
#endif

	QVBoxLayout *pSideBarLayout = new QVBoxLayout(sideBarWidget);
	pSideBarLayout->setSpacing(0);
	pSideBarLayout->setContentsMargins( 0, 0, 0, 0 );

	// zoom-in btn
	Button *pUpBtn = new Button( nullptr, QSize( 16, 16 ), Button::Type::Push, "up.svg", "", false, QSize( 9, 9 ), tr( "sort" ) );
	connect(pUpBtn, SIGNAL( clicked() ), this, SLOT(o_upBClicked()) );
	pSideBarLayout->addWidget(pUpBtn);

	// zoom-in btn
	Button *pDownBtn = new Button( nullptr, QSize( 16, 16 ), Button::Type::Push, "down.svg", "", false, QSize( 9, 9 ), tr( "sort" ) );
	connect(pDownBtn, SIGNAL( clicked() ), this, SLOT(o_downBClicked()));
	pSideBarLayout->addWidget(pDownBtn);

	//restore the playlist
	Playlist* pPlaylist = Playlist::get_instance();
	if( pPlaylist->size() > 0 ){
		for ( uint i = 0; i < pPlaylist->size(); ++i ){
			QTreeWidgetItem* pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
			
			pPlaylistItem->setText( 0, pPlaylist->get( i )->filePath );
			pPlaylistItem->setText( 1, pPlaylist->get( i )->scriptPath );
			
			if ( pPlaylist->get( i )->scriptEnabled ) {
				pPlaylistItem->setCheckState( 2, Qt::Checked );
			} else {
				pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			}
		}

		//restore the selected item
		int activeSongNumber = Playlist::get_instance()->getActiveSongNumber();
		int selectedSongNumber = Playlist::get_instance()->getSelectedSongNr();
		
		if(! (activeSongNumber == -1 && selectedSongNumber == -1) )
		{
			int aselected = 0;
			if( activeSongNumber == -1 ){
				aselected = selectedSongNumber;
			} else {
				aselected = activeSongNumber ;
			}
	
			QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->topLevelItem ( aselected );
			m_pPlaylistItem->setBackground( 0, QColor( 50, 50, 50) );
			m_pPlaylistItem->setBackground( 1, QColor( 50, 50, 50) );
			m_pPlaylistItem->setBackground( 2, QColor( 50, 50, 50) );
		}
	}

	m_pTimer = new QTimer( this );
	connect(m_pTimer, SIGNAL(timeout() ), this, SLOT( updateActiveSongNumber() ) );
	m_pTimer->start( 1000 );	// update player control at 1 fps

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &PlaylistDialog::onPreferencesChanged );
}

PlaylistDialog::~PlaylistDialog()
{
	INFOLOG ( "DESTROY" );
}

void PlaylistDialog::keyPressEvent( QKeyEvent* ev )
{
	if(ev->key() == Qt::Key_Escape) {
		HydrogenApp::get_instance()->showPlaylistDialog();
	}
}

void PlaylistDialog::closeEvent( QCloseEvent* ev )
{
	HydrogenApp::get_instance()->showPlaylistDialog();
}

void PlaylistDialog::addSong()
{
	QString sPath = Preferences::get_instance()->getLastAddSongToPlaylistDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::songs_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setWindowTitle( tr( "Add Song to PlayList" ) );
	fd.setFileMode( QFileDialog::ExistingFiles );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setDirectory( sPath );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}
	
	Preferences::get_instance()->setLastAddSongToPlaylistDirectory( fd.directory().absolutePath() );

	foreach( QString filePath, fd.selectedFiles() ) {
		updatePlayListNode( filePath );
	}
}

void PlaylistDialog::addCurrentSong()
{
	std::shared_ptr<Song> 	pSong = Hydrogen::get_instance()->getSong();
	QString filename = 	pSong->getFilename();

	if (filename == "") {
		// just in case!
		QMessageBox::information ( this, "Hydrogen", tr ( "Please save your song first" ));
		return;
	}
	updatePlayListNode ( filename );
}

void PlaylistDialog::removeFromList()
{
	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylistTree->indexOfTopLevelItem ( pPlaylistItem );
	QTreeWidgetItem * pTmpItem = m_pPlaylistTree->topLevelItem ( 1 );

	if (pPlaylistItem == nullptr){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song selected!" ));
	} else {
		if (pTmpItem == nullptr){
			m_pPlaylistTree->clear();
			Playlist::get_instance()->clear();
			Playlist::get_instance()->setSelectedSongNr( -1 );
			Playlist::get_instance()->setActiveSongNumber( -1 );
			Playlist::get_instance()->setFilename( "" );
			setWindowTitle ( tr ( "Playlist Browser" ) );
			
			return;
		} else {
			///avoid segfault if the last item will be removed!!
			/// 
			delete pPlaylistItem;
			
			updatePlayListVector();
			if ( Playlist::get_instance()->getActiveSongNumber() == index ){
				Playlist::get_instance()->setActiveSongNumber( -1 );
			} else if ( Playlist::get_instance()->getActiveSongNumber() > index  ){
				Playlist::get_instance()->setActiveSongNumber(  Playlist::get_instance()->getActiveSongNumber() -1 );
			}
		}
	}
}

void PlaylistDialog::clearPlaylist()
{
	bool DiscardChanges = false;
	bool IsModified = Playlist::get_instance()->getIsModified();

	if( IsModified ) {
		auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		switch(QMessageBox::information( this, "Hydrogen",
										 tr("\nThe current playlist contains unsaved changes.\n"
												"Do you want to discard the changes?\n"),
										 pCommonStrings->getButtonDiscard(),
										 pCommonStrings->getButtonCancel(),
										 nullptr,      // Enter == button 0
										 2 ) ) { // Escape == button 1
		case 0: // Discard clicked or Alt+D pressed
			// don't save but exit
			DiscardChanges = true;
			break;
		case 1: // Cancel clicked or Alt+C pressed or Escape pressed
			// don't exit
			DiscardChanges = false;
			break;
		}
	}

	if(!IsModified || (IsModified && DiscardChanges))
	{
		m_pPlaylistTree->clear();
		Playlist::get_instance()->clear();
		Playlist::get_instance()->setSelectedSongNr( -1 );
		Playlist::get_instance()->setActiveSongNumber( -1 );
		Playlist::get_instance()->setFilename ( "" );
		setWindowTitle ( tr ( "Playlist Browser" ) );

		Playlist::get_instance()->setIsModified(false);
	}
	return;
}

void PlaylistDialog::updatePlayListNode ( QString file )
{
	QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
	m_pPlaylistItem->setText ( 0, file );
	m_pPlaylistItem->setText ( 1, tr("no Script") );
	m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );

	updatePlayListVector();

	m_pPlaylistTree->setCurrentItem ( m_pPlaylistItem );
}

void PlaylistDialog::loadList()
{
	QString sPath = Preferences::get_instance()->getLastPlaylistDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::playlists_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setWindowTitle( tr( "Load Playlist" ) );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( sPath );
	fd.setNameFilter( Filesystem::playlists_filter_name );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QString filename = fd.selectedFiles().first();
	Preferences::get_instance()->setLastPlaylistDirectory( fd.directory().absolutePath() );

	bool relativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
	Playlist* pPlaylist = Playlist::load( filename, relativePaths);
	if ( ! pPlaylist ) {
		_ERRORLOG( "Error loading the playlist" );
		/* FIXME: get current instance (?) */
		pPlaylist = Playlist::get_instance();
	}

	Playlist* playlist = Playlist::get_instance();
	if( playlist->size() > 0 ) {
		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		m_pPlaylist->clear();

		for ( uint i = 0; i < playlist->size(); ++i ){
			QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );

			if( playlist->get( i )->fileExists ){
				m_pPlaylistItem->setText( 0, playlist->get( i )->filePath );
			} else {
				m_pPlaylistItem->setText( 0, tr("File not found: ") + playlist->get( i )->filePath );
			}

			m_pPlaylistItem->setText ( 1, playlist->get( i )->scriptPath );

			if ( playlist->get( i )->scriptEnabled ) {
				m_pPlaylistItem->setCheckState( 2, Qt::Checked );
			} else {
				m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			}
		}

		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( 0 );
		m_pPlaylist->setCurrentItem ( m_pPlaylistItem );
		pPlaylist->setSelectedSongNr( 0 );
		setWindowTitle ( tr ( "Playlist Browser" ) + QString(" - ") + pPlaylist->getFilename() );
	}
}

void PlaylistDialog::newScript()
{
	Preferences *pPref = Preferences::get_instance();

	QString sPath = Preferences::get_instance()->getLastPlaylistScriptDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::scripts_dir();
	}

	FileDialog fd(this);
	fd.setFileMode ( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::scripts_filter_name );
	fd.setAcceptMode ( QFileDialog::AcceptSave );
	fd.setWindowTitle ( tr ( "New Script" ) );
	fd.setDirectory( sPath );

	QString defaultFilename;

	defaultFilename += ".sh";

	fd.selectFile ( defaultFilename );

	QString filename;
	if ( fd.exec() != QDialog::Accepted ) return;

	filename = fd.selectedFiles().first();

	if( filename.contains(" ", Qt::CaseInsensitive)){
		QMessageBox::information ( this, "Hydrogen", tr ( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must be without whitespaces.") );
		return;
	}

	QFile chngPerm ( filename );
	if (!chngPerm.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return;
	}

	Preferences::get_instance()->setLastPlaylistScriptDirectory( fd.directory().absolutePath() );

	QTextStream out(&chngPerm);
	out <<  "#!/bin/sh\n\n#have phun";
	chngPerm.close();

	if (chngPerm.exists() ) {
		chngPerm.setPermissions( QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner );
		QMessageBox::information ( this, "Hydrogen", tr ( "WARNING, the new file is executable by the owner of the file!" ) );
	}

	if( pPref->getDefaultEditor().isEmpty() ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";

		FileDialog fd(this);
		fd.setAcceptMode( QFileDialog::AcceptOpen );
		fd.setFileMode ( QFileDialog::ExistingFile );
		fd.setDirectory ( lastUsedDir );

		fd.setWindowTitle ( tr ( "Set your Default Editor" ) );

		QString filename;
		if ( fd.exec() == QDialog::Accepted ){
			filename = fd.selectedFiles().first();

			pPref->setDefaultEditor( filename );
		}
	}

	QString  openfile = pPref->getDefaultEditor() + " " + filename + "&";
	std::system(openfile.toLatin1());

	return;
}

void PlaylistDialog::saveListAs()
{
	QString sPath = Preferences::get_instance()->getLastPlaylistDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::playlists_dir();
	}
	
	FileDialog fd(this);
	fd.setWindowTitle( tr( "Save Playlist" ) );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::playlists_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setDirectory( sPath );
	fd.selectFile( Filesystem::untitled_playlist_file_name() );
	fd.setDefaultSuffix( Filesystem::playlist_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QString filename = fd.selectedFiles().first();

	Playlist* pPlaylist = Playlist::get_instance();
	bool relativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
	if ( Files::savePlaylistPath( filename, pPlaylist, relativePaths ) == nullptr ) {
		return;
	}

	pPlaylist->setIsModified( false );
	Preferences::get_instance()->setLastPlaylistDirectory( fd.directory().absolutePath() );

	setWindowTitle( tr( "Playlist Browser" ) + QString(" - %1").arg( filename ) );
}

void PlaylistDialog::saveList()
{
	Playlist* pPlaylist = Playlist::get_instance();
	if ( pPlaylist->getFilename().isEmpty() ) {
		return saveListAs();
	}

	bool relativePaths = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
	if ( Files::savePlaylistPath( pPlaylist->getFilename(), pPlaylist, relativePaths ) == nullptr ) {
		return;
	}

	pPlaylist->setIsModified( false );
}

void PlaylistDialog::loadScript()
{

	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->currentItem();
	if ( pPlaylistItem == nullptr ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song in List or no Song selected!" ) );
		return;
	}

	QString sPath = Preferences::get_instance()->getLastPlaylistScriptDirectory();
	if ( ! Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::scripts_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode ( QFileDialog::ExistingFile );
	fd.setDirectory ( sPath );
	fd.setNameFilter ( tr ( "Hydrogen Playlist (*.sh)" ) );
	fd.setWindowTitle ( tr ( "Add Script to selected Song" ) );

	QString filename;
	if ( fd.exec() == QDialog::Accepted ){
		filename = fd.selectedFiles().first();

		if( filename.contains(" ", Qt::CaseInsensitive)){
			QMessageBox::information ( this, "Hydrogen", tr ( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must without whitespaces.") );
			return;
		}
		Preferences::get_instance()->setLastPlaylistScriptDirectory( fd.directory().absolutePath() );

		pPlaylistItem->setText ( 1, filename );
		updatePlayListVector();

	}
}

void PlaylistDialog::removeScript()
{
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();


	if (m_pPlaylistItem == nullptr){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song selected!" ));
		return;
	} else {
		QString selected;
		selected = m_pPlaylistItem->text ( 1 );
		if( !QFile( selected ).exists()  ){
			QMessageBox::information ( this, "Hydrogen", tr ( "No Script in use!" ));
			return;
		} else {
			m_pPlaylistItem->setText ( 1, tr("no Script") );

			m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			updatePlayListVector();
		}
	}

}

void PlaylistDialog::editScript()
{
	Preferences *pPref = Preferences::get_instance();
	if( pPref->getDefaultEditor().isEmpty() ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";

		FileDialog fd(this);
		fd.setAcceptMode( QFileDialog::AcceptOpen );
		fd.setFileMode ( QFileDialog::ExistingFile );
		fd.setDirectory ( lastUsedDir );

		fd.setWindowTitle ( tr ( "Set your Default Editor" ) );

		QString filename;
		if ( fd.exec() == QDialog::Accepted ){
			filename = fd.selectedFiles().first();

			pPref->setDefaultEditor( filename );
		}
	}

	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->currentItem();

	if ( pPlaylistItem == nullptr ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song selected!" ) );
		return;
	}
	QString selected;
	selected = pPlaylistItem->text ( 1 );

	QString filename = pPref->getDefaultEditor() + " " + selected + "&";

	if( !QFile( selected ).exists()  ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Script selected!" ));
		return;
	}

	std::system( filename.toLatin1() );
	
	return;
}

void PlaylistDialog::o_upBClicked()
{
	m_pTimer->stop();

	Playlist* pPlaylist = Playlist::get_instance();

	QTreeWidget* pPlaylistTree = m_pPlaylistTree;
	QTreeWidgetItem* pPlaylistTreeItem = m_pPlaylistTree->currentItem();
	int index = pPlaylistTree->indexOfTopLevelItem ( pPlaylistTreeItem );

	if (index == 0 ){
		m_pTimer->start( 1000 );
		return;
	}

	QTreeWidgetItem* tmpPlaylistItem = pPlaylistTree->takeTopLevelItem ( index );

	pPlaylistTree->insertTopLevelItem ( index -1, tmpPlaylistItem );
	pPlaylistTree->setCurrentItem ( tmpPlaylistItem );

	if ( pPlaylist->getSelectedSongNr() >= 0 ){
		pPlaylist->setSelectedSongNr( pPlaylist->getSelectedSongNr() -1 );
	}

	if ( pPlaylist->getActiveSongNumber() == index ){
		pPlaylist->setActiveSongNumber( pPlaylist->getActiveSongNumber() -1 );
	}else if ( pPlaylist->getActiveSongNumber() == index -1 ){
		pPlaylist->setActiveSongNumber( pPlaylist->getActiveSongNumber() +1 );
	}

	updatePlayListVector();
}

void PlaylistDialog::o_downBClicked()
{
	m_pTimer->stop();
	Playlist* pPlaylist = Playlist::get_instance();

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	int length = m_pPlaylist->topLevelItemCount();
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );

	if ( index == length - 1){
		m_pTimer->start( 1000 );
		return;
	}

	QTreeWidgetItem* pTmpPlaylistItem = m_pPlaylist->takeTopLevelItem ( index );

	m_pPlaylist->insertTopLevelItem ( index +1, pTmpPlaylistItem );
	m_pPlaylist->setCurrentItem ( pTmpPlaylistItem );

	if ( pPlaylist->getSelectedSongNr() >= 0 ) {
		pPlaylist->setSelectedSongNr( pPlaylist->getSelectedSongNr() +1 );
	}

	if (pPlaylist ->getActiveSongNumber() == index ){
		pPlaylist->setActiveSongNumber( pPlaylist->getActiveSongNumber() +1 );
	}else if ( pPlaylist->getActiveSongNumber() == index +1 ){
		pPlaylist->setActiveSongNumber( pPlaylist->getActiveSongNumber() -1 );
	}
	updatePlayListVector();

}

void PlaylistDialog::on_m_pPlaylistTree_itemClicked ( QTreeWidgetItem * item, int column )
{
	if ( column == 2 ){
		QString selected;
		selected = item->text ( 1 );

		if( !QFile( selected ).exists() ){
			QMessageBox::information ( this, "Hydrogen", tr ( "No Script!" ));
			item->setCheckState( 2, Qt::Unchecked );
			return;
		}
		updatePlayListVector();
	}
	return;
}

void PlaylistDialog::nodePlayBTN()
{
	Hydrogen *		pHydrogen = Hydrogen::get_instance();
	HydrogenApp *	pH2App = HydrogenApp::get_instance();

	if ( m_pPlayBtn->isChecked() ) {
		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
		if ( m_pPlaylistItem == nullptr ){
			QMessageBox::information ( this, "Hydrogen", tr ( "No valid song selected!" ) );
			m_pPlayBtn->setChecked(false);
			return;
		}
		QString sFilename = "";
		sFilename = m_pPlaylistItem->text ( 0 );

		if( sFilename == pHydrogen->getSong()->getFilename()){
			pHydrogen->sequencer_play();
			return;
		}

		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );
		Playlist::get_instance()->setActiveSongNumber( index );

		if ( ! pH2App->openSong( sFilename ) ) {
			m_pPlayBtn->setChecked(false);
		}

		pHydrogen->sequencer_play();
	}
	else {
		pHydrogen->sequencer_stop();
		pH2App->showStatusBarMessage( tr("Pause.") );
	}
}

void PlaylistDialog::nodeStopBTN()
{
	m_pPlayBtn->setChecked(false);
	Hydrogen::get_instance()->sequencer_stop();
	Hydrogen::get_instance()->getCoreActionController()->locateToColumn( 0 );
}

void PlaylistDialog::ffWDBtnClicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() + 1 );
}

void PlaylistDialog::rewindBtnClicked()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->locateToColumn( pHydrogen->getAudioEngine()->getTransportPosition()->getColumn() - 1 );
}

void PlaylistDialog::on_m_pPlaylistTree_itemDoubleClicked ()
{
	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->currentItem();
	if ( pPlaylistItem == nullptr ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song selected!" ) );
		return;
	}
	
	QString sFilename;
	sFilename = pPlaylistItem->text( 0 );

	int index = m_pPlaylistTree->indexOfTopLevelItem ( pPlaylistItem );
	Playlist::get_instance()->setSelectedSongNr( index );
	Playlist::get_instance()->setActiveSongNumber( index );

	HydrogenApp *pH2App = HydrogenApp::get_instance();

	m_pPlayBtn->setChecked(false);

	pH2App->openSong( sFilename );

	pH2App->showStatusBarMessage( tr( "Playlist: set song no. %1" ).arg( index +1 ) );

///exec script
///this is very very simple and only an experiment
#ifdef WIN32
	//I know nothing about windows scripts -wolke-
	return;
#else
	QString execscript;
	sFilename = pPlaylistItem->text ( 1 );
	bool execcheckbox = pPlaylistItem->checkState ( 2 );

	if( execcheckbox == false){
		//QMessageBox::information ( this, "Hydrogen", tr ( "No Script selected!" ));
		return;
	}

	if( execscript == "Script not used"){
		//QMessageBox::information ( this, "Hydrogen", tr ( "Script not in use!" ));
		return;
	}

	std::system( sFilename.toLatin1() );
	
	return;
#endif

}


void PlaylistDialog::updatePlayListVector()
{
	int length = m_pPlaylistTree->topLevelItemCount();

	Playlist::get_instance()->clear();

	for (int i = 0 ;i < length; i++){
		QTreeWidgetItem * pPlaylistItem = m_pPlaylistTree->topLevelItem ( i );

		Playlist::Entry* entry = new Playlist::Entry();
		entry->filePath = pPlaylistItem->text( 0 );
		entry->scriptPath = pPlaylistItem->text( 1 );
		entry->scriptEnabled = pPlaylistItem->checkState( 2 );

		Playlist::get_instance()->add( entry );
		Playlist::get_instance()->setIsModified(true);
	}
	m_pTimer->start( 1000 );
}


void PlaylistDialog::updateActiveSongNumber()
{
	for ( uint i = 0; i < Playlist::get_instance()->size(); ++i ){
		if ( !m_pPlaylistTree->topLevelItem( i ) ) {
			break;
		}
		( m_pPlaylistTree->topLevelItem( i ) )->setBackground( 0, QBrush() );
		( m_pPlaylistTree->topLevelItem( i ) )->setBackground( 1, QBrush() );
		( m_pPlaylistTree->topLevelItem( i ) )->setBackground( 2, QBrush() );

	}

	int selected = Playlist::get_instance()->getActiveSongNumber();
	if ( selected == -1 ) {
		return;
	}

	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->topLevelItem ( selected );
	if ( pPlaylistItem != nullptr ){
		pPlaylistItem->setBackground( 0, QColor( 50, 50, 50) );
		pPlaylistItem->setBackground( 1, QColor( 50, 50, 50) );
		pPlaylistItem->setBackground( 2, QColor( 50, 50, 50) );
	}
}


bool PlaylistDialog::eventFilter ( QObject *o, QEvent *e )
{
	UNUSED ( o );
	if ( e->type() == QEvent::KeyPress ) {
		QKeyEvent *k = ( QKeyEvent * ) e;

		switch ( k->key() ) {
		case  Qt::Key_F5 :
			if(    Playlist::get_instance()->size() == 0 
				|| Playlist::get_instance()->getActiveSongNumber() <=0) {
				break;
			}

			Playlist::get_instance()->setNextSongByNumber(Playlist::get_instance()->getActiveSongNumber()-1);
			return true;
			break;
		case  Qt::Key_F6 :
			if(		Playlist::get_instance()->size() == 0
				||	Playlist::get_instance()->getActiveSongNumber() >= Playlist::get_instance()->size() -1) {
				break;
			}
			
			Playlist::get_instance()->setNextSongByNumber(Playlist::get_instance()->getActiveSongNumber()+1);
			return true;
			break;
		}
	} else {
		return false; // standard event processing
	}

	return false;
}

bool PlaylistDialog::loadListByFileName( QString filename )
{
	bool bUseRelativeFilenames = Preferences::get_instance()->isPlaylistUsingRelativeFilenames();
	
	Playlist* pPlaylist = Playlist::load ( filename, bUseRelativeFilenames );
	if ( !pPlaylist ) {
		_ERRORLOG( "Error loading the playlist" );
		return false;
	}

	Preferences::get_instance()->setLastPlaylistFilename( filename );

	Playlist* playlist = Playlist::get_instance();
	if ( playlist->size() > 0 ) {
		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		m_pPlaylist->clear();

		for ( uint i = 0; i < playlist->size(); ++i ){
			QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
			m_pPlaylistItem->setText( 0, playlist->get( i )->filePath );
			m_pPlaylistItem->setText( 1, playlist->get( i )->scriptPath );

			if ( playlist->get( i )->scriptEnabled ) {
				m_pPlaylistItem->setCheckState( 2, Qt::Checked );
			} else {
				m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			}
		}

		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( 0 );
		m_pPlaylist->setCurrentItem ( m_pPlaylistItem );
		pPlaylist->setSelectedSongNr( 0 );
		setWindowTitle ( tr ( "Playlist Browser" ) + QString(" - ") + pPlaylist->getFilename() );
	}

	return true;
}

void PlaylistDialog::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
		QFont childFont( pPref->getLevel2FontFamily(), getPointSize( pPref->getFontSize() ) );
		setFont( font );
		m_pMenubar->setFont( font );
		m_pPlaylistMenu->setFont( font );
#ifndef WIN32
		m_pScriptMenu->setFont( font );
#endif

		int ii;
		
		for ( ii = 0; ii < m_pPlaylistTree->headerItem()->columnCount(); ii++ ) {
			m_pPlaylistTree->headerItem()->setFont( ii, font );
		}

		QTreeWidgetItem* pNode = m_pPlaylistTree->topLevelItem( 0 );

		while ( pNode != nullptr ) {
			for ( ii = 0; ii < pNode->columnCount(); ii++ ) {
				pNode->setFont( ii, childFont );
			}
			pNode = m_pPlaylistTree->itemBelow( pNode );
		}
		
	}
}
