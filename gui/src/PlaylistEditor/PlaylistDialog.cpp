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
#include "widgets/PixmapWidget.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/playlist.h>
#include <hydrogen/event_queue.h>

#include "../widgets/Button.h"

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
using namespace std;

PlaylistDialog::PlaylistDialog ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "PlayListDialog" )
{

	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "Playlist Browser" ) + QString(" - ") + QString( Playlist::get_instance()->__playlistName  ) );
	setFixedSize ( width(), height() );

	installEventFilter( this );

	// menubar
	QMenuBar *m_pMenubar = new QMenuBar( this );
//	setMenuBar( m_pMenubar );

	// Playlist menu
	QMenu *m_pPlaylistMenu = m_pMenubar->addMenu( trUtf8( "&Playlist" ) );

	m_pPlaylistMenu->addAction( trUtf8( "Add song to Play&list" ), this, SLOT( addSong() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addAction( trUtf8( "Add &current song to Playlist" ), this, SLOT( addCurrentSong() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addSeparator();				// -----
	m_pPlaylistMenu->addAction( trUtf8( "&Remove selected song from Playlist" ), this, SLOT( removeFromList() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addAction( trUtf8( "Remove all songs from &Playlist" ), this, SLOT( clearPlaylist() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addSeparator();
	m_pPlaylistMenu->addAction( trUtf8( "&Open Playlist" ), this, SLOT( loadList() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addSeparator();
	m_pPlaylistMenu->addAction( trUtf8( "&Save Playlist" ), this, SLOT( saveList() ), QKeySequence( "" ) );
	m_pPlaylistMenu->addAction( trUtf8( "Save Playlist &as" ), this, SLOT( saveListAs() ), QKeySequence( "" ) );

#ifdef WIN32
        //no scripts under windows
#else
	// Script menu
	QMenu *m_pScriptMenu = m_pMenubar->addMenu( trUtf8( "&Scripts" ) );

	m_pScriptMenu->addAction( trUtf8( "&Add Script to selected song" ), this, SLOT( loadScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addAction( trUtf8( "&Edit selected Script" ), this, SLOT( editScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( trUtf8( "&Remove selected Script" ), this, SLOT( removeScript() ), QKeySequence( "" ) );
	m_pScriptMenu->addSeparator();
	m_pScriptMenu->addAction( trUtf8( "&Create a new Script" ), this, SLOT( newScript() ), QKeySequence( "" ) );
#endif


// CONTROLS
	PixmapWidget *pControlsPanel = new PixmapWidget( NULL );
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
	m_pRwdBtn->setToolTip( trUtf8("Rewind") );
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
	m_pPlayBtn->setToolTip( trUtf8("Play/ Pause/ Load selected song") );
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
	m_pStopBtn->setToolTip( trUtf8("Stop") );
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
	m_pFfwdBtn->setToolTip( trUtf8("Fast Forward") );
	connect(m_pFfwdBtn, SIGNAL(clicked(Button*)), this, SLOT(ffWDBtnClicked(Button*)));


#ifdef WIN32
	QStringList headers;
	headers << trUtf8 ( "Song list" );
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

	QVBoxLayout *sideBarLayout = new QVBoxLayout(sideBarWidget);
	sideBarLayout->setSpacing(0);
	sideBarLayout->setMargin(0);

#else	
	QStringList headers;
	headers << trUtf8 ( "Song list" ) << trUtf8 ( "Script" ) << trUtf8 ( "exec Script" );
	QTreeWidgetItem* header = new QTreeWidgetItem ( headers );
	m_pPlaylistTree->setHeaderItem ( header );
	m_pPlaylistTree->header()->resizeSection ( 0, 405 );
	m_pPlaylistTree->header()->resizeSection ( 1, 405 );
	m_pPlaylistTree->header()->resizeSection ( 2, 15 );
	m_pPlaylistTree->setAlternatingRowColors( true );


	QVBoxLayout *sideBarLayout = new QVBoxLayout(sideBarWidget);
	sideBarLayout->setSpacing(0);
	sideBarLayout->setMargin(0);
#endif

	// zoom-in btn
	Button *up_btn = new Button(
			NULL,
			"/songEditor/btn_up_on.png",
			"/songEditor/btn_up_off.png",
			"/songEditor/btn_up_over.png",
			QSize(18, 13)
	);

	up_btn->setFontSize(7);
	up_btn->setToolTip( trUtf8( "sort" ) );
	connect(up_btn, SIGNAL(clicked(Button*)), this, SLOT(o_upBClicked()) );
	sideBarLayout->addWidget(up_btn);

	// zoom-in btn
	Button *down_btn = new Button(
			NULL,
			"/songEditor/btn_down_on.png",
			"/songEditor/btn_down_off.png",
			"/songEditor/btn_down_over.png",
			QSize(18, 13)
	);

	down_btn->setFontSize(7);
	down_btn->setToolTip( trUtf8( "sort" ) );
	connect(down_btn, SIGNAL(clicked(Button*)), this, SLOT(o_downBClicked()));
	sideBarLayout->addWidget(down_btn);
	

//restore the playlist
	if( Hydrogen::get_instance()->m_PlayList.size() > 0 ){
		for ( uint i = 0; i < Hydrogen::get_instance()->m_PlayList.size(); ++i ){
			QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
			m_pPlaylistItem->setText ( 0, Hydrogen::get_instance()->m_PlayList[i].m_hFile );
			m_pPlaylistItem->setText ( 1, Hydrogen::get_instance()->m_PlayList[i].m_hScript );
			if ( Hydrogen::get_instance()->m_PlayList[i].m_hScriptEnabled == "Use Script" ) {
				m_pPlaylistItem->setCheckState( 2, Qt::Checked );
			}else{
				m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			}
		}


		//restore the selected item		
		int selected = Playlist::get_instance()->getActiveSongNumber();
		int Selected = Playlist::get_instance()->getSelectedSongNr();
		if( selected == -1 && Selected == -1 ) return;
		
		int aselected = 0;
		if( selected == -1 ){
			aselected = Selected;
		}else
		{
			aselected = selected ;
		}
		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( aselected );
		m_pPlaylistItem->setBackgroundColor ( 0, QColor( 50, 50, 50) );
		m_pPlaylistItem->setBackgroundColor ( 1, QColor( 50, 50, 50) );
		m_pPlaylistItem->setBackgroundColor ( 2, QColor( 50, 50, 50) );
		}

	timer = new QTimer( this );
	connect(timer, SIGNAL(timeout() ), this, SLOT( updateActiveSongNumber() ) );
	timer->start( 1000 );	// update player control at 1 fps

}


PlaylistDialog::~PlaylistDialog()
{
	INFOLOG ( "DESTROY" );
}


void PlaylistDialog::addSong()
{
	static QString songDir = Preferences::get_instance()->getDataDirectory()  + "/songs";;

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::ExistingFiles );
	fd->setFilter ( "Hydrogen song (*.h2song)" );
	fd->setDirectory ( songDir );

	fd->setWindowTitle ( trUtf8 ( "Add Song to PlayList" ) );

	QString filename;
	if ( fd->exec() == QDialog::Accepted ){
		int i;
		for(i=0; i < fd->selectedFiles().size(); i++){ 
			filename = fd->selectedFiles().at(i);
			updatePlayListNode ( filename );
		}
	}


}


void PlaylistDialog::addCurrentSong()
{
	Song *song = Hydrogen::get_instance()->getSong();
	QString filename = song->get_filename();
	

	if (filename == "") {
		// just in case!
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Please save your song first" ));
		return;
	}
//	filename += ".h2song";
	updatePlayListNode ( filename );
}


void PlaylistDialog::removeFromList()
{

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );
	QTreeWidgetItem * m_pItem = m_pPlaylist->topLevelItem ( 1 );


	if (m_pPlaylistItem == NULL){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Song selected!" ));
		return;
	}else
	{
		if (m_pItem == 0){
			m_pPlaylist->clear();
			Hydrogen::get_instance()->m_PlayList.clear();
			Playlist::get_instance()->setSelectedSongNr( -1 );
			Playlist::get_instance()->setActiveSongNumber( -1 );
			Playlist::get_instance()->__playlistName = "";
			setWindowTitle ( trUtf8 ( "Playlist Browser" ) );
			return;
		}else
		{	
			///avoid segfault if the last item will be removed!!
			delete m_pPlaylistItem;
			updatePlayListVector();
			if (  Playlist::get_instance()->getActiveSongNumber() == index ){
				Playlist::get_instance()->setActiveSongNumber( -1 );
			}else if (  Playlist::get_instance()->getActiveSongNumber() > index  ){
				Playlist::get_instance()->setActiveSongNumber(  Playlist::get_instance()->getActiveSongNumber() -1 );
			}
			
		}
	}
}


void PlaylistDialog::clearPlaylist()
{
	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	
	m_pPlaylist->clear();
	Hydrogen::get_instance()->m_PlayList.clear();
	Playlist::get_instance()->setSelectedSongNr( -1 );
	Playlist::get_instance()->setActiveSongNumber( -1 );
	Playlist::get_instance()->__playlistName = "";
	setWindowTitle ( trUtf8 ( "Playlist Browser" ) );
	return;	
}


void PlaylistDialog::updatePlayListNode ( QString file )
{

	QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
	m_pPlaylistItem->setText ( 0, file );
        m_pPlaylistItem->setText ( 1, trUtf8 ("no Script") );
	m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );

	updatePlayListVector();

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	m_pPlaylist->setCurrentItem ( m_pPlaylistItem );

}


void PlaylistDialog::loadList()
{

	static QString sDirectory =  Preferences::get_instance()->getDataDirectory()  + "playlists/" ;

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::ExistingFile );
	fd->setDirectory ( sDirectory );

	fd->setWindowTitle ( trUtf8 ( "Load Playlist" ) );

	QString filename;
	if ( fd->exec() == QDialog::Accepted ){
		filename = fd->selectedFiles().first();

		LocalFileMng fileMng;
		int err = fileMng.loadPlayList( filename.toLocal8Bit().constData() );
		if ( err != 0 ) {
			_ERRORLOG( "Error loading the playlist" );
		}
	
		if(Hydrogen::get_instance()->m_PlayList.size() > 0){

			QTreeWidget* m_pPlaylist = m_pPlaylistTree;
			m_pPlaylist->clear();

			for ( uint i = 0; i < Hydrogen::get_instance()->m_PlayList.size(); ++i ){
				QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
				m_pPlaylistItem->setText ( 0, Hydrogen::get_instance()->m_PlayList[i].m_hFile );
				m_pPlaylistItem->setText ( 1, Hydrogen::get_instance()->m_PlayList[i].m_hScript );
				if ( Hydrogen::get_instance()->m_PlayList[i].m_hScriptEnabled == "Use Script" ) {
					m_pPlaylistItem->setCheckState( 2, Qt::Checked );
				}else{
					m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
				}
			}

			QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( 0 );
			m_pPlaylist->setCurrentItem ( m_pPlaylistItem );
			Playlist::get_instance()->setSelectedSongNr( 0 );
			Playlist::get_instance()->__playlistName = filename;
			setWindowTitle ( trUtf8 ( "Playlist Browser" ) + QString(" - ") + QString( Playlist::get_instance()->__playlistName  ) );
		}

	}
}


void PlaylistDialog::newScript()
{

	Preferences *pPref = Preferences::get_instance();

	QString sDirectory = ( Preferences::get_instance()->getDataDirectory()  + "scripts/");
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::AnyFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Scripts (*.sh)" ) );
	fd->setAcceptMode ( QFileDialog::AcceptSave );
	fd->setWindowTitle ( trUtf8 ( "New Script" ) );
	fd->setDirectory ( sDirectory );

	QString defaultFilename;

		defaultFilename += ".sh";


	fd->selectFile ( defaultFilename );

	QString filename;
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
	}else
	{
		return;
	}

	if( filename.contains(" ", Qt::CaseInsensitive)){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must without whitespaces.") );
		return;
	}
	

	QFile chngPerm ( filename );
	if (!chngPerm.open(QIODevice::WriteOnly | QIODevice::Text))
		return;

	QTextStream out(&chngPerm);
	out <<  "#!/bin/sh\n\n#have phun";
	chngPerm.close();


	if (chngPerm.exists() ) {
		chngPerm.setPermissions( QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner );
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "WARNING, the new file is executable by the owner of the file!" ) );
	}
	
	if( pPref->getDefaultEditor().isEmpty() ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";
	
		std::auto_ptr<QFileDialog> fd( new QFileDialog );
		fd->setFileMode ( QFileDialog::ExistingFile );
		fd->setDirectory ( lastUsedDir );
	
		fd->setWindowTitle ( trUtf8 ( "Set your Default Editor" ) );
	
		QString filename;
		if ( fd->exec() == QDialog::Accepted ){
			filename = fd->selectedFiles().first();
	
			pPref->setDefaultEditor( filename );
		}		
	}

	QString  openfile = pPref->getDefaultEditor() + " " + filename + "&";

	char *ofile;
	ofile = new char[openfile.length() + 1];
	strcpy(ofile, openfile.toAscii());
	std::system( ofile ); 
	delete [] ofile;
	return;

	
}


void PlaylistDialog::saveListAs()
{

	QString sDirectory =  Preferences::get_instance()->getDataDirectory()  + "playlists/";
	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::AnyFile );
	fd->setFilter ( trUtf8 ( "Hydrogen Playlist (*.h2playlist)" ) );
	fd->setAcceptMode ( QFileDialog::AcceptSave );
	fd->setWindowTitle ( trUtf8 ( "Save Playlist" ) );
	fd->setDirectory ( sDirectory );


	QString defaultFilename;

		defaultFilename += ".h2playlist";


	fd->selectFile ( defaultFilename );

	QString filename;
	if ( fd->exec() == QDialog::Accepted )
	{
		filename = fd->selectedFiles().first();
	}else
	{
		return;
	}

	LocalFileMng fileMng;
	int err = fileMng.savePlayList( filename.toLocal8Bit().constData() );
	if ( err != 0 ) {
		_ERRORLOG( "Error saving the playlist" );
	}else
	{
		Playlist::get_instance()->__playlistName = filename;
		setWindowTitle ( trUtf8 ( "Playlist Browser" ) + QString(" - ") + QString( Playlist::get_instance()->__playlistName  ) );
	}
}


void PlaylistDialog::saveList()
{

	if ( Playlist::get_instance()->__playlistName == "") {
		// just in case!
		return saveListAs();
	}

	LocalFileMng fileMng;
	int err = fileMng.savePlayList( Playlist::get_instance()->__playlistName.toStdString() );
	if ( err != 0 ) {
		_ERRORLOG( "Error saving the playlist" );
	}

}


void PlaylistDialog::loadScript()
{
	
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	if ( m_pPlaylistItem == NULL ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Song in List or no Song selected!" ) );
		return;
	}

	static QString lastUsedDir =  Preferences::get_instance()->getDataDirectory()  + "scripts/";

	std::auto_ptr<QFileDialog> fd( new QFileDialog );
	fd->setFileMode ( QFileDialog::ExistingFile );
	fd->setDirectory ( lastUsedDir );
	fd->setFilter ( trUtf8 ( "Hydrogen Playlist (*.sh)" ) );
	fd->setWindowTitle ( trUtf8 ( "Add Script to selected Song" ) );

	QString filename;
	if ( fd->exec() == QDialog::Accepted ){
		filename = fd->selectedFiles().first();
//		filename = filename.simplified();

		if( filename.contains(" ", Qt::CaseInsensitive)){
			QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Script name or path to the script contains whitespaces.\nIMPORTANT\nThe path to the script and the scriptname must without whitespaces.") );
			return;
		}

		m_pPlaylistItem->setText ( 1, filename );
		updatePlayListVector();

	}
}


void PlaylistDialog::removeScript()
{
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();


	if (m_pPlaylistItem == NULL){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Song selected!" ));
		return;
	}else{
		QString selected;
		selected = m_pPlaylistItem->text ( 1 );
                if( !QFile( selected ).exists() ){
			QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Script in use!" ));
			return;
		}else
		{
                        m_pPlaylistItem->setText ( 1, trUtf8 ("no Script") );
			m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			updatePlayListVector();
		}
	}

}


void PlaylistDialog::editScript()
{
	Preferences *pPref = Preferences::get_instance();
	if( pPref->getDefaultEditor().isEmpty() ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Default Editor Set. Please set your Default Editor\nDo not use a console based Editor\nSorry, but this will not work for the moment." ) );

		static QString lastUsedDir = "/usr/bin/";
	
		std::auto_ptr<QFileDialog> fd( new QFileDialog );
		fd->setFileMode ( QFileDialog::ExistingFile );
		fd->setDirectory ( lastUsedDir );
	
		fd->setWindowTitle ( trUtf8 ( "Set your Default Editor" ) );
	
		QString filename;
		if ( fd->exec() == QDialog::Accepted ){
			filename = fd->selectedFiles().first();
	
			pPref->setDefaultEditor( filename );
		}		
	}

	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();

	if ( m_pPlaylistItem == NULL ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Song selected!" ) );
		return;
	}
	QString selected;
	selected = m_pPlaylistItem->text ( 1 );

	QString filename = pPref->getDefaultEditor() + " " + selected + "&";

        if( !QFile( selected ).exists() ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Script selected!" ));
		return;
	}

	char *file;
	file = new char[ filename.length() + 1 ];
	strcpy( file , filename.toAscii() );
	std::system( file ); 
	delete [] file;
	return;

}


void PlaylistDialog::o_upBClicked()
{	
	timer->stop();

	Playlist* pList = Playlist::get_instance();

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );

	if (index == 0 ){
		timer->start( 1000 ); 
		return;
	}

	QTreeWidgetItem* tmpPlaylistItem = m_pPlaylist->takeTopLevelItem ( index );

	m_pPlaylist->insertTopLevelItem ( index -1, tmpPlaylistItem );
	m_pPlaylist->setCurrentItem ( tmpPlaylistItem ); 

	if ( pList->getSelectedSongNr() >= 0 )
		pList->setSelectedSongNr( pList->getSelectedSongNr() -1 );

	if ( pList->getActiveSongNumber() == index ){
		pList->setActiveSongNumber( pList->getActiveSongNumber() -1 );
	}else if ( pList->getActiveSongNumber() == index -1 ){
		pList->setActiveSongNumber( pList->getActiveSongNumber() +1 );
	}

	updatePlayListVector();

}


void PlaylistDialog::o_downBClicked()
{
	timer->stop();
	Playlist* pList = Playlist::get_instance();

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	int length = m_pPlaylist->topLevelItemCount();
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );

	if ( index == length - 1){
		timer->start( 1000 );
		return;
	}


	QTreeWidgetItem* tmpPlaylistItem = m_pPlaylist->takeTopLevelItem ( index );

	m_pPlaylist->insertTopLevelItem ( index +1, tmpPlaylistItem );
	m_pPlaylist->setCurrentItem ( tmpPlaylistItem ); 

	if ( pList->getSelectedSongNr() >= 0 )
		pList->setSelectedSongNr( pList->getSelectedSongNr() +1 );

	if (pList ->getActiveSongNumber() == index ){
		pList->setActiveSongNumber( pList->getActiveSongNumber() +1 );
	}else if ( pList->getActiveSongNumber() == index +1 ){
		pList->setActiveSongNumber( pList->getActiveSongNumber() -1 );
	}
	updatePlayListVector();

}


void PlaylistDialog::on_m_pPlaylistTree_itemClicked ( QTreeWidgetItem * item, int column )
{
	if ( column == 2 ){ 
		QString selected;
		selected = item->text ( 1 );

                if( !QFile( selected ).exists() ){
			QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Script!" ));
			item->setCheckState( 2, Qt::Unchecked );
			return;
		}
		updatePlayListVector();
	}
	return;
}


void PlaylistDialog::nodePlayBTN( Button* ref )
{
	Hydrogen *engine = Hydrogen::get_instance();
	HydrogenApp *pH2App = HydrogenApp::get_instance();

	if (ref->isPressed()) {
		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
		if ( m_pPlaylistItem == NULL ){
			QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No song selected!" ) );
			m_pPlayBtn->setPressed(false);
			return;
		}
		QString selected = "";
		selected = m_pPlaylistItem->text ( 0 );

		if( selected == engine->getSong()->get_filename()){
			engine->sequencer_play();
			return;	
		}		
	
		if ( engine->getState() == STATE_PLAYING ){
			engine->sequencer_stop();
		}
	
		LocalFileMng mng;
		Song *pSong = Song::load ( selected );
		if ( pSong == NULL ){
			QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Error loading song." ) );
			m_pPlayBtn->setPressed(false);
			return;
		}

		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );
		Playlist::get_instance()->setActiveSongNumber( index );
	
		pH2App->setSong ( pSong );
		engine->setSelectedPatternNumber ( 0 );
	
		engine->sequencer_play();
	}else 
	{
		engine->sequencer_stop();
		pH2App->setStatusBarMessage(trUtf8("Pause."), 5000);
	}
}


void PlaylistDialog::nodeStopBTN( Button* ref )
{
	UNUSED( ref );
	m_pPlayBtn->setPressed(false);
	Hydrogen::get_instance()->sequencer_stop();
	Hydrogen::get_instance()->setPatternPos ( 0 );
}


void PlaylistDialog::ffWDBtnClicked( Button* ref)
{
	UNUSED( ref );
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() + 1 );
}


void PlaylistDialog::rewindBtnClicked( Button* ref )
{
	UNUSED( ref );
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->setPatternPos( pEngine->getPatternPos() - 1 );
}


void PlaylistDialog::on_m_pPlaylistTree_itemDoubleClicked ()
{

	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylistTree->currentItem();
	if ( m_pPlaylistItem == NULL ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Song selected!" ) );
		return;
	}
	QString selected;
	selected = m_pPlaylistItem->text ( 0 );

	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	int index = m_pPlaylist->indexOfTopLevelItem ( m_pPlaylistItem );
	Playlist::get_instance()->setSelectedSongNr( index );
	Playlist::get_instance()->setActiveSongNumber( index );
	
	HydrogenApp *pH2App = HydrogenApp::get_instance();
	Hydrogen *engine = Hydrogen::get_instance();
	

	if ( engine->getState() == STATE_PLAYING ){
		engine->sequencer_stop();
	}

	m_pPlayBtn->setPressed(false);

	engine->m_timelinetagvector.clear();

	LocalFileMng mng;
	Song *pSong = Song::load ( selected );
	if ( pSong == NULL ){
		QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Error loading song." ) );
		return;
	}

	pH2App->setSong ( pSong );
	engine->setSelectedPatternNumber ( 0 );

	HydrogenApp::get_instance()->getSongEditorPanel()->updatePositionRuler();
	pH2App->setStatusBarMessage( trUtf8( "Playlist: set song no. %1" ).arg( index +1 ), 5000 );

	HydrogenApp::get_instance()->getInstrumentRack()->getSoundLibraryPanel()->update_background_color();

	EventQueue::get_instance()->push_event( EVENT_METRONOME, 3 );

///exec script
///this is very very simple and only an experiment
#ifdef WIN32
	//I know nothing about windows scripts -wolke-
	return;
#else
	QString execscript;
	selected = m_pPlaylistItem->text ( 1 );
	bool execcheckbox = m_pPlaylistItem->checkState ( 2 );

	if( execcheckbox == false){
		//QMessageBox::information ( this, "Hydrogen", trUtf8 ( "No Script selected!" ));
		return;
	}

	if( execscript == "Script not used"){
		//QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Script not in use!" ));
		return;
	}

	char *file;
	file = new char[ selected.length() + 1 ];
	strcpy( file , selected.toAscii() );
	std::system( file ); 
	delete [] file;
	return;
#endif

}


void PlaylistDialog::updatePlayListVector()
{
	QTreeWidget* m_pPlaylist = m_pPlaylistTree;
	int length = m_pPlaylist->topLevelItemCount();

	Hydrogen::get_instance()->m_PlayList.clear();



	for (int i = 0 ;i < length; i++){
		QTreeWidgetItem * m_pPlaylistItem = m_pPlaylist->topLevelItem ( i );	
		
		QString execval;
		bool execcheckbox = m_pPlaylistItem->checkState ( 2 );
		if ( execcheckbox == true ) {
			execval = "Use Script";
		}else{
			execval = "Script not used";
		} 
		Hydrogen::HPlayListNode playListItem;
		playListItem.m_hFile = m_pPlaylistItem->text ( 0 );
		playListItem.m_hScript = m_pPlaylistItem->text ( 1 );
		playListItem.m_hScriptEnabled = execval;

		Hydrogen::get_instance()->m_PlayList.push_back( playListItem );
	}
	timer->start( 1000 );
}


void PlaylistDialog::updateActiveSongNumber()
{
	QTreeWidget* m_pPlaylist = m_pPlaylistTree;

	for ( uint i = 0; i < Hydrogen::get_instance()->m_PlayList.size(); ++i ){
		if ( !m_pPlaylist->topLevelItem( i ) )
			break;
		( m_pPlaylist->topLevelItem( i ) )->setBackground( 0, QBrush() );
		( m_pPlaylist->topLevelItem( i ) )->setBackground( 1, QBrush() );
		( m_pPlaylist->topLevelItem( i ) )->setBackground( 2, QBrush() );
		
	}
		
	int selected = Playlist::get_instance()->getActiveSongNumber();
	if ( selected == -1 )
		return;
	
	QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( selected );
	if ( m_pPlaylistItem != NULL ){
	    //m_pPlaylist->setCurrentItem ( m_pPlaylistItem );
	    m_pPlaylistItem->setBackgroundColor ( 0, QColor( 50, 50, 50) );
	    m_pPlaylistItem->setBackgroundColor ( 1, QColor( 50, 50, 50) );
	    m_pPlaylistItem->setBackgroundColor ( 2, QColor( 50, 50, 50) );
	}
}


bool PlaylistDialog::eventFilter ( QObject *o, QEvent *e )
{
	
	UNUSED ( o );
	if ( e->type() == QEvent::KeyPress )
	{	
		QKeyEvent *k = ( QKeyEvent * ) e;

		switch ( k->key() )
		{
			case  Qt::Key_F5 :
				if( Hydrogen::get_instance()->m_PlayList.size() == 0)
					break;
				Playlist::get_instance()->setPrevSongPlaylist();
				return TRUE;
				break;

			case  Qt::Key_F6 :
				if( Hydrogen::get_instance()->m_PlayList.size() == 0)
					break;
				Playlist::get_instance()->setNextSongPlaylist();
				return TRUE;
				break;
		}

	}
	else
	{
		return FALSE; // standard event processing
	}

return NULL;
}

bool PlaylistDialog::loadListByFileName( QString filename )
{

	LocalFileMng fileMng;
	int err = fileMng.loadPlayList( filename.toLocal8Bit().constData() );
	if ( err != 0 ) {
		_ERRORLOG( "Error loading the playlist" );
		return 0;
	}
	Preferences::get_instance()->setLastPlaylistFilename( filename );

	if(Hydrogen::get_instance()->m_PlayList.size() > 0){

		QTreeWidget* m_pPlaylist = m_pPlaylistTree;
		m_pPlaylist->clear();

		for ( uint i = 0; i < Hydrogen::get_instance()->m_PlayList.size(); ++i ){
			QTreeWidgetItem* m_pPlaylistItem = new QTreeWidgetItem ( m_pPlaylistTree );
			m_pPlaylistItem->setText ( 0, Hydrogen::get_instance()->m_PlayList[i].m_hFile );
			m_pPlaylistItem->setText ( 1, Hydrogen::get_instance()->m_PlayList[i].m_hScript );
			if ( Hydrogen::get_instance()->m_PlayList[i].m_hScriptEnabled == "Use Script" ) {
				m_pPlaylistItem->setCheckState( 2, Qt::Checked );
			}else{
				m_pPlaylistItem->setCheckState( 2, Qt::Unchecked );
			}
		}

		QTreeWidgetItem* m_pPlaylistItem = m_pPlaylist->topLevelItem ( 0 );
		m_pPlaylist->setCurrentItem ( m_pPlaylistItem );
		Playlist::get_instance()->setSelectedSongNr( 0 );
		Playlist::get_instance()->__playlistName = filename;
		setWindowTitle ( trUtf8 ( "Playlist Browser" ) + QString(" - ") + QString( Playlist::get_instance()->__playlistName  ) );
	}
	return 1;
}
