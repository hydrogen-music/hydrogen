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


#include "PlaylistDialog.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "SongEditor/SongEditorPanel.h"
#include "Widgets/PixmapWidget.h"

#include <hydrogen/helpers/files.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/timeline.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/basics/playlist.h>

#include "../Widgets/Button.h"

#include <QTreeWidget>
#include <QDomDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>

using namespace H2Core;

const char* PlaylistDialog::__class_name = "PlaylistDialog";

PlaylistDialog::PlaylistDialog ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( __class_name )
{

	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( tr ( "Playlist Browser" ) + QString(" - ") + Playlist::get_instance()->getFilename() );
	setFixedSize ( width(), height() );

	installEventFilter( this );

	// menubar
	QMenuBar *m_pMenubar = new QMenuBar( this );

	// Playlist menu
	QMenu *m_pPlaylistMenu = m_pMenubar->addMenu( tr( "&Playlist" ) );

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

#ifdef WIN32
	//no scripts under windows
#else
	// Script menu
	QMenu *m_pScriptMenu = m_pMenubar->addMenu( tr( "&Scripts" ) );

	m_pScriptMenu->addAction( tr( "&Add Script to selected song" ), this, SLOT( loadScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addAction( tr( "&Edit selected Script" ), this, SLOT( editScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( tr( "&Remove selected Script" ), this, SLOT( removeScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( tr( "&Create a new Script" ), this, SLOT( newScript() ), QKeySequence( "" ) );
#endif

	// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( nullptr );
	pControlsPanel->setFixedSize( 119, 32 );
	pControlsPanel->setPixmap( "/playerControlPanel/playlist_background_Control.png" );
	vboxLayout->addWidget( pControlsPanel );

	// Rewind button
	m_pRwdBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_rwd_on.png",
			"/playerControlPanel/btn_rwd_off.png",
			"/playerControlPanel/btn_rwd_over.png",
			QSize(21, 15)
	);
	m_pRwdBtn->move(6, 6);
	m_pRwdBtn->setToolTip( tr("Rewind") );
	connect(m_pRwdBtn, SIGNAL(clicked(Button*)), this, SLOT(rewindBtnClicked(Button*)));

	// Play button
	m_pPlayBtn = new ToggleButton(
			pControlsPanel,
			"/playerControlPanel/btn_play_on.png",
			"/playerControlPanel/btn_play_off.png",
			"/playerControlPanel/btn_play_over.png",
			QSize(33, 17)
	);
	m_pPlayBtn->move(33, 6);
	m_pPlayBtn->setPressed(false);
	m_pPlayBtn->setToolTip( tr("Play/ Pause/ Load selected song") );
	connect(m_pPlayBtn, SIGNAL(clicked(Button*)), this, SLOT(nodePlayBTN(Button*)));

	// Stop button
	m_pStopBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_stop_on.png",
			"/playerControlPanel/btn_stop_off.png",
			"/playerControlPanel/btn_stop_over.png",
			QSize(21, 15)
	);
	m_pStopBtn->move(65, 6);
	m_pStopBtn->setToolTip( tr("Stop") );
	connect(m_pStopBtn, SIGNAL(clicked(Button*)), this, SLOT(nodeStopBTN(Button*)));

	// Fast forward button
	m_pFfwdBtn = new Button(
			pControlsPanel,
			"/playerControlPanel/btn_ffwd_on.png",
			"/playerControlPanel/btn_ffwd_off.png",
			"/playerControlPanel/btn_ffwd_over.png",
			QSize(21, 15)
	);
	m_pFfwdBtn->move(92, 6);
	m_pFfwdBtn->setToolTip( tr("Fast Forward") );
	connect(m_pFfwdBtn, SIGNAL(clicked(Button*)), this, SLOT(ffWDBtnClicked(Button*)));

#ifdef WIN32
	QStringList headers;
	headers << tr ( "Song list" );
	QTreeWidgetItem* header = new QTreeWidgetItem ( headers );
	m_pPlaylistTree->setHeaderItem ( header );
	m_pPlaylistTree->setAlternatingRowColors( true );

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

	QVBoxLayout *pSideBarLayout = new QVBoxLayout(sideBarWidget);
	pSideBarLayout->setSpacing(0);
	pSideBarLayout->setMargin(0);

#else
	QStringList headers;
	headers << tr ( "Song list" ) << tr ( "Script" ) << tr ( "exec Script" );
	QTreeWidgetItem* header = new QTreeWidgetItem ( headers );
	m_pPlaylistTree->setHeaderItem ( header );
	m_pPlaylistTree->header()->resizeSection ( 0, 405 );
	m_pPlaylistTree->header()->resizeSection ( 1, 405 );
	m_pPlaylistTree->header()->resizeSection ( 2, 15 );
	m_pPlaylistTree->setAlternatingRowColors( true );


	QVBoxLayout *pSideBarLayout = new QVBoxLayout(sideBarWidget);
	pSideBarLayout->setSpacing(0);
	pSideBarLayout->setMargin(0);
#endif

	// zoom-in btn
	Button *pUpBtn = new Button(
			nullptr,
			"/songEditor/btn_up_on.png",
			"/songEditor/btn_up_off.png",
			"/songEditor/btn_up_over.png",
			QSize(18, 13)
	);

	pUpBtn->setFontSize(7);
	pUpBtn->setToolTip( tr( "sort" ) );
	connect(pUpBtn, SIGNAL(clicked(Button*)), this, SLOT(o_upBClicked()) );
	pSideBarLayout->addWidget(pUpBtn);

	// zoom-in btn
	Button *pDownBtn = new Button(
			nullptr,
			"/songEditor/btn_down_on.png",
			"/songEditor/btn_down_off.png",
			"/songEditor/btn_down_over.png",
			QSize(18, 13)
	);

	pDownBtn->setFontSize(7);
	pDownBtn->setToolTip( tr( "sort" ) );
	connect(pDownBtn, SIGNAL(clicked(Button*)), this, SLOT(o_downBClicked()));
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
		int selected = Playlist::get_instance()->getActiveSongNumber();
		int Selected = Playlist::get_instance()->getSelectedSongNr();
		if( selected == -1 && Selected == -1 ) return;

		int aselected = 0;
		if( selected == -1 ){
			aselected = Selected;
		} else {
			aselected = selected ;
		}

		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->topLevelItem ( aselected );
		m_pPlaylistItem->setBackground( 0, QColor( 50, 50, 50) );
		m_pPlaylistItem->setBackground( 1, QColor( 50, 50, 50) );
		m_pPlaylistItem->setBackground( 2, QColor( 50, 50, 50) );
	}

	timer = new QTimer( this );
	connect(timer, SIGNAL(timeout() ), this, SLOT( updateActiveSongNumber() ) );
	timer->start( 1000 );	// update player control at 1 fps
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
	QFileDialog fd(this);
	fd.setWindowTitle( tr( "Add Song to PlayList" ) );
	fd.setFileMode( QFileDialog::ExistingFiles );
	fd.setNameFilter( Filesystem::songs_filter_name );
	fd.setDirectory( Filesystem::songs_dir() );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	foreach( QString filePath, fd.selectedFiles() ) {
		updatePlayListNode( filePath );
	}
}

void PlaylistDialog::addCurrentSong()
{
	Song *	pSong = Hydrogen::get_instance()->getSong();
	QString filename = 	pSong->get_filename();

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

	if( IsModified )
	{
		switch(QMessageBox::information( this, "Hydrogen",
										 tr("\nThe current playlist contains unsaved changes.\n"
												"Do you want to discard the changes?\n"),
										tr("&Discard"), tr("&Cancel"),
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
	QFileDialog fd(this);
	fd.setWindowTitle( tr( "Load Playlist" ) );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setDirectory( Filesystem::playlists_dir() );
	fd.setNameFilter( Filesystem::playlists_filter_name );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QString filename = fd.selectedFiles().first();

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

	QFileDialog fd(this);
	fd.setFileMode ( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::scripts_filter_name );
	fd.setAcceptMode ( QFileDialog::AcceptSave );
	fd.setWindowTitle ( tr ( "New Script" ) );
	fd.setDirectory( Filesystem::scripts_dir() );

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

		QFileDialog fd(this);
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
	QFileDialog fd(this);
	fd.setWindowTitle( tr( "Save Playlist" ) );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::playlists_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setDirectory( Filesystem::playlists_dir() );
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

	static QString lastUsedDir = Filesystem::scripts_dir();

	QFileDialog fd(this);
	fd.setFileMode ( QFileDialog::ExistingFile );
	fd.setDirectory ( lastUsedDir );
	fd.setNameFilter ( tr ( "Hydrogen Playlist (*.sh)" ) );
	fd.setWindowTitle ( tr ( "Add Script to selected Song" ) );

	QString filename;
	if ( fd.exec() == QDialog::Accepted ){
		filename = fd.selectedFiles().first();

		if( filename.contains(" ", Qt::CaseInsensitive)){
			QMessageBox::information ( this, "Hydrogen", tr ( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must without whitespaces.") );
			return;
		}

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

		QFileDialog fd(this);
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
	timer->stop();

	Playlist* pPlaylist = Playlist::get_instance();

	QTreeWidget* pPlaylistTree = m_pPlaylistTree;
	QTreeWidgetItem* pPlaylistTreeItem = m_pPlaylistTree->currentItem();
	int index = pPlaylistTree->indexOfTopLevelItem ( pPlaylistTreeItem );

	if (index == 0 ){
		timer->start( 1000 );
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
	timer->stop();
	Playlist* pPlaylist = Playlist::get_instance();

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	int length = m_pPlaylist->topLevelItemCount();
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );

	if ( index == length - 1){
		timer->start( 1000 );
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

void PlaylistDialog::nodePlayBTN( Button* ref )
{
	Hydrogen *		pEngine = Hydrogen::get_instance();
	HydrogenApp *	pH2App = HydrogenApp::get_instance();

	if (ref->isPressed()) {
		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
		if ( m_pPlaylistItem == nullptr ){
			QMessageBox::information ( this, "Hydrogen", tr ( "No valid song selected!" ) );
			m_pPlayBtn->setPressed(false);
			return;
		}
		QString selected = "";
		selected = m_pPlaylistItem->text ( 0 );

		if( selected == pEngine->getSong()->get_filename()){
			pEngine->sequencer_play();
			return;
		}

		if ( pEngine->getState() == STATE_PLAYING ){
			pEngine->sequencer_stop();
		}

		Song *pSong = Song::load ( selected );
		if ( pSong == nullptr ){
			QMessageBox::information ( this, "Hydrogen", tr ( "Error loading song." ) );
			m_pPlayBtn->setPressed(false);
			return;
		}

		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );
		Playlist::get_instance()->setActiveSongNumber( index );

		pH2App->setSong ( pSong );
		pEngine->setSelectedPatternNumber ( 0 );

		pEngine->sequencer_play();
	}else
	{
		pEngine->sequencer_stop();
		pH2App->setStatusBarMessage(tr("Pause."), 5000);
	}
}

void PlaylistDialog::nodeStopBTN( Button* ref )
{
	UNUSED( ref );
	m_pPlayBtn->setPressed(false);
	Hydrogen::get_instance()->sequencer_stop();
	Hydrogen::get_instance()->getCoreActionController()->relocate( 0 );
}

void PlaylistDialog::ffWDBtnClicked( Button* ref)
{
	UNUSED( ref );
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() + 1 );
}

void PlaylistDialog::rewindBtnClicked( Button* ref )
{
	UNUSED( ref );
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	pHydrogen->getCoreActionController()->relocate( pHydrogen->getPatternPos() - 1 );
}

void PlaylistDialog::on_m_pPlaylistTree_itemDoubleClicked ()
{
	QTreeWidgetItem* pPlaylistItem = m_pPlaylistTree->currentItem();
	if ( pPlaylistItem == nullptr ){
		QMessageBox::information ( this, "Hydrogen", tr ( "No Song selected!" ) );
		return;
	}
	
	QString selected;
	selected = pPlaylistItem->text ( 0 );

	int index = m_pPlaylistTree->indexOfTopLevelItem ( pPlaylistItem );
	Playlist::get_instance()->setSelectedSongNr( index );
	Playlist::get_instance()->setActiveSongNumber( index );

	HydrogenApp *pH2App = HydrogenApp::get_instance();
	Hydrogen *pEngine = Hydrogen::get_instance();

	if ( pEngine->getState() == STATE_PLAYING ){
		pEngine->sequencer_stop();
	}

	m_pPlayBtn->setPressed(false);

	Timeline* pTimeline = pEngine->getTimeline();
	pTimeline->deleteAllTags();

	Song *pSong = Song::load ( selected );
	if ( pSong == nullptr ){
		QMessageBox::information ( this, "Hydrogen", tr ( "Error loading song." ) );
		return;
	}

	pH2App->setSong ( pSong );
	pEngine->setSelectedPatternNumber ( 0 );

	HydrogenApp::get_instance()->getSongEditorPanel()->updatePositionRuler();
	pH2App->setStatusBarMessage( tr( "Playlist: set song no. %1" ).arg( index +1 ), 5000 );

	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );

///exec script
///this is very very simple and only an experiment
#ifdef WIN32
	//I know nothing about windows scripts -wolke-
	return;
#else
	QString execscript;
	selected = pPlaylistItem->text ( 1 );
	bool execcheckbox = pPlaylistItem->checkState ( 2 );

	if( execcheckbox == false){
		//QMessageBox::information ( this, "Hydrogen", tr ( "No Script selected!" ));
		return;
	}

	if( execscript == "Script not used"){
		//QMessageBox::information ( this, "Hydrogen", tr ( "Script not in use!" ));
		return;
	}

	std::system( selected.toLatin1() );
	
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
	timer->start( 1000 );
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
