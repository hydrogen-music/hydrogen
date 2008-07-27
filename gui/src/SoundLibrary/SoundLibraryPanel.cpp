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

#include "SoundLibraryPanel.h"

#include <QtGui>

#include "SoundLibraryTree.h"
#include "FileBrowser.h"

#include "SoundLibrarySaveDialog.h"
#include "SoundLibraryExportDialog.h"

#include "../HydrogenApp.h"
#include "../widgets/Button.h"
#include "../widgets/PixmapWidget.h"
#include "../Skin.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../PatternEditor/PatternEditorInstrumentList.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/data_path.h>
#include <hydrogen/sample.h>
#include <hydrogen/adsr.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/Preferences.h>
using namespace H2Core;

#include <cassert>

SoundLibraryPanel::SoundLibraryPanel( QWidget *pParent )
 : QWidget( pParent )
 , Object( "SoundLibraryPanel" )
{
	//INFOLOG( "INIT" );

	m_pDrumkitMenu = new QMenu( this );
	m_pDrumkitMenu->addAction( trUtf8( "Load" ), this, SLOT( on_drumkitLoadAction() ) );
	m_pDrumkitMenu->addAction( trUtf8( "Export" ), this, SLOT( on_drumkitExportAction() ) );
	m_pDrumkitMenu->addAction( trUtf8( "Rename" ), this, SLOT( on_drumkitRenameAction() ) );
	m_pDrumkitMenu->addSeparator();
	m_pDrumkitMenu->addAction( trUtf8( "Delete" ), this, SLOT( on_drumkitDeleteAction() ) );

	m_pInstrumentMenu = new QMenu( this );
	m_pInstrumentMenu->addSeparator();
	m_pInstrumentMenu->addAction( trUtf8( "Delete" ), this, SLOT( on_instrumentDeleteAction() ) );

	m_pSongMenu = new QMenu( this );
	m_pSongMenu->addSeparator();
	m_pSongMenu->addAction( trUtf8( "Load" ), this, SLOT( on_songLoadAction() ) );

	m_pPatternMenu = new QMenu( this );
	m_pPatternMenu->addSeparator();
	m_pPatternMenu->addAction( trUtf8( "Load" ), this, SLOT( on_patternLoadAction() ) );

// DRUMKIT LIST
	m_pSoundLibraryTree = new SoundLibraryTree( NULL );
	connect( m_pSoundLibraryTree, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( on_DrumkitList_ItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
	connect( m_pSoundLibraryTree, SIGNAL( itemActivated ( QTreeWidgetItem*, int ) ), this, SLOT( on_DrumkitList_itemActivated( QTreeWidgetItem*, int ) ) );
	connect( m_pSoundLibraryTree, SIGNAL( leftClicked(QPoint) ), this, SLOT( on_DrumkitList_leftClicked(QPoint)) );
	connect( m_pSoundLibraryTree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_DrumkitList_rightClicked(QPoint)) );
	connect( m_pSoundLibraryTree, SIGNAL( onMouseMove( QMouseEvent* ) ), this, SLOT( on_DrumkitList_mouseMove( QMouseEvent* ) ) );

	/*
	m_pFileBrowser = new FileBrowser( NULL );
	m_pFileBrowser->hide();
	*/
	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );

	pVBox->addWidget( m_pSoundLibraryTree );
	//	pVBox->addWidget( pButtonsPanel );

	this->setLayout( pVBox );

	updateDrumkitList();
}



SoundLibraryPanel::~SoundLibraryPanel()
{
	for (uint i = 0; i < m_systemDrumkitInfoList.size(); ++i ) {
		delete m_systemDrumkitInfoList[i];
	}
	m_systemDrumkitInfoList.clear();

	for (uint i = 0; i < m_userDrumkitInfoList.size(); ++i ) {
		delete m_userDrumkitInfoList[i];
	}
	m_userDrumkitInfoList.clear();

	//INFOLOG( "DESTROY" );
}



void SoundLibraryPanel::updateDrumkitList()
{
	LocalFileMng mng;

	m_pSoundLibraryTree->clear();

	std::vector<QString> songList = mng.getSongList();

	if ( songList.size() > 0 )
	{

		m_pSongItem = new QTreeWidgetItem( m_pSoundLibraryTree );
		m_pSongItem->setText( 0, trUtf8( "Songs" ) );
		//m_pSoundLibraryTree->setItemExpanded( m_pSongItem, false );
		m_pSongItem->setToolTip( 0, "double-click expand Songs list" );

		for (uint i = 0; i < songList.size(); i++) 
		{
			QString absPath = DataPath::get_data_path() + "/songs/" + songList[i];
			QTreeWidgetItem* pSongItem = new QTreeWidgetItem( m_pSongItem );
			pSongItem->setText( 0 , songList[ i ] );
			pSongItem->setToolTip( 0, songList[ i ] );
		}
	}

	std::vector<QString> patternList = mng.getPatternList();

	if ( patternList.size() > 0 )
	{
				
		m_pPatternItem = new QTreeWidgetItem( m_pSoundLibraryTree );
		m_pPatternItem->setText( 0, trUtf8( "Patterns" ) );
		//m_pSoundLibraryTree->setItemExpanded( m_pPatternItem, false );
		m_pPatternItem->setToolTip( 0, "double-click expand Patterns list" );

		for (uint i = 0; i < patternList.size(); i++) 
		{
			QString absPath = DataPath::get_data_path() + "/patterns/" + patternList[i];
			QTreeWidgetItem* pPatternItem = new QTreeWidgetItem( m_pPatternItem );
			pPatternItem->setText( 0 , patternList[ i ] );
			QString patternPath = Preferences::getInstance()->getDataDirectory() + "/patterns/" + patternList[i] + ".h2pattern";
			QString drumkitName = mng.getDrumkitNameForPattern( patternPath );
			pPatternItem->setToolTip( 0, drumkitName );
		}
	}

	m_pSystemDrumkitsItem = new QTreeWidgetItem( m_pSoundLibraryTree );
	m_pSystemDrumkitsItem->setText( 0, trUtf8( "System drumkits" ) );
	m_pSoundLibraryTree->setItemExpanded( m_pSystemDrumkitsItem, true );

	m_pUserDrumkitsItem = new QTreeWidgetItem( m_pSoundLibraryTree );
	m_pUserDrumkitsItem->setText( 0, trUtf8( "User drumkits" ) );
	m_pSoundLibraryTree->setItemExpanded( m_pUserDrumkitsItem, true );

	

	for (uint i = 0; i < m_systemDrumkitInfoList.size(); ++i ) {
		delete m_systemDrumkitInfoList[i];
	}
	m_systemDrumkitInfoList.clear();

	for (uint i = 0; i < m_userDrumkitInfoList.size(); ++i ) {
		delete m_userDrumkitInfoList[i];
	}
	m_userDrumkitInfoList.clear();

	std::vector<QString> userList = Drumkit::getUserDrumkitList();
	for (uint i = 0; i < userList.size(); ++i) {
		//QString absPath = Preferences::getInstance()->getDataDirectory() + "/drumkits/" + userList[i];
		QString absPath =  userList[i];
		Drumkit *pInfo = mng.loadDrumkit( absPath );
		if (pInfo) {
			m_userDrumkitInfoList.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( m_pUserDrumkitsItem );
			pDrumkitItem->setText( 0, pInfo->getName() );

			InstrumentList *pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				Instrument *pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + pInstr->get_name() );
				pInstrumentItem->setToolTip( 0, pInstr->get_name() );
			}
		}
	}

	std::vector<QString> systemList = Drumkit::getSystemDrumkitList();

	for (uint i = 0; i < systemList.size(); i++) {
		QString absPath = systemList[i];
		Drumkit *pInfo = mng.loadDrumkit( absPath );
		if (pInfo) {
			m_systemDrumkitInfoList.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( m_pSystemDrumkitsItem );
			pDrumkitItem->setText( 0, pInfo->getName() );

			InstrumentList *pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				Instrument *pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + pInstr->get_name() );
				pInstrumentItem->setToolTip( 0, pInstr->get_name() );
			}
		}
	}
}



void SoundLibraryPanel::on_DrumkitList_ItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous )
{
	UNUSED( current );
	UNUSED( previous );
//	INFOLOG( "[on_DrumkitList_ItemChanged]" );
}



void SoundLibraryPanel::on_DrumkitList_itemActivated( QTreeWidgetItem * item, int column )
{
	UNUSED( column );

//	INFOLOG( "[on_DrumkitList_itemActivated]" );
	if ( item == m_pSystemDrumkitsItem || item == m_pUserDrumkitsItem || item == m_pSystemDrumkitsItem->parent() || item->parent() == m_pSongItem || item == m_pSongItem || item == m_pPatternItem || item->parent() == m_pPatternItem ) {
		return;
	}

	if ( item->parent() == m_pSystemDrumkitsItem || item->parent() == m_pUserDrumkitsItem  ) {
		// e' stato selezionato un drumkit
	}
	else {
		// e' stato selezionato uno strumento
		QString selectedName = item->text(0);

		QString sInstrName = selectedName.remove( 0, selectedName.indexOf( "] " ) + 2 );
		QString sDrumkitName = item->parent()->text(0);
		INFOLOG( QString(sDrumkitName) + ", instr:" + sInstrName );

		Instrument *pInstrument = Instrument::load_instrument( sDrumkitName, sInstrName );
		pInstrument->set_muted( false );

		AudioEngine::get_instance()->get_sampler()->preview_instrument( pInstrument );
	}
}







void SoundLibraryPanel::on_DrumkitList_rightClicked( QPoint pos )
{
	if( m_pSoundLibraryTree->currentItem() == NULL )
		return;
	

	if (
		( m_pSoundLibraryTree->currentItem()->parent() == NULL ) ||
		( m_pSoundLibraryTree->currentItem() == m_pUserDrumkitsItem ) ||
		( m_pSoundLibraryTree->currentItem() == m_pSystemDrumkitsItem )
	) {
		return;
	}

	if ( m_pSoundLibraryTree->currentItem()->parent() == m_pSongItem ) {
		m_pSongMenu->popup( pos );
	}

	if ( m_pSoundLibraryTree->currentItem()->parent() == m_pPatternItem ) {
		m_pPatternMenu->popup( pos );
	}

	if ( m_pSoundLibraryTree->currentItem()->parent() == m_pUserDrumkitsItem ) {
		m_pDrumkitMenu->popup( pos );
	}
	else if ( m_pSoundLibraryTree->currentItem()->parent()->parent() == m_pUserDrumkitsItem ) {
		m_pInstrumentMenu->popup( pos );
	}

	if ( m_pSoundLibraryTree->currentItem()->parent() == m_pSystemDrumkitsItem ) {
		m_pDrumkitMenu->popup( pos );
	}
	else if ( m_pSoundLibraryTree->currentItem()->parent()->parent() == m_pSystemDrumkitsItem ) {
		m_pInstrumentMenu->popup( pos );
	}
}



void SoundLibraryPanel::on_DrumkitList_leftClicked( QPoint pos )
{
	m_startDragPosition = pos;
}



void SoundLibraryPanel::on_DrumkitList_mouseMove( QMouseEvent *event)
{
	
	
	if (! ( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	
	if ( ( event->pos() - m_startDragPosition ).manhattanLength() < QApplication::startDragDistance() ) {
		return;
	}
	
	if ( !m_pSoundLibraryTree->currentItem() ) {
		return;
	}
	
	if (
		( m_pSoundLibraryTree->currentItem()->parent() == m_pSystemDrumkitsItem ) ||
		( m_pSoundLibraryTree->currentItem()->parent() == m_pUserDrumkitsItem )
	) {
 		// drumkit selection
		//INFOLOG( "ho selezionato un drumkit (system)" );
		return;
	}
	
	else {
		//INFOLOG( "ho selezionato uno strumento" );
		// instrument selection
		if ( m_pSoundLibraryTree->currentItem() == NULL )
		{
			return;
		}
		
		if ( m_pSoundLibraryTree->currentItem()->parent() == NULL )
		{
			return;
		}	

		if ( m_pSoundLibraryTree->currentItem()->parent()->text(0) == NULL )
		{
			return;
		}
		

		QString sDrumkitName = m_pSoundLibraryTree->currentItem()->parent()->text(0);
		QString sInstrumentName = ( m_pSoundLibraryTree->currentItem()->text(0) ).remove( 0, m_pSoundLibraryTree->currentItem()->text(0).indexOf( "] " ) + 2 );

		QString sText = sDrumkitName + "::" + sInstrumentName;

		QDrag *pDrag = new QDrag(this);
		QMimeData *pMimeData = new QMimeData;

		pMimeData->setText( sText );
		pDrag->setMimeData( pMimeData);
		//drag->setPixmap(iconPixmap);

		pDrag->start( Qt::CopyAction | Qt::MoveAction );
	}
	
}



void SoundLibraryPanel::on_drumkitLoadAction()
{
	ERRORLOG( "[on_drumkitLoadAction] not implemented yet" );

	QString sDrumkitName = m_pSoundLibraryTree->currentItem()->text(0);

	Drumkit *drumkitInfo = NULL;

	// find the drumkit in the list
	for ( uint i = 0; i < m_systemDrumkitInfoList.size(); i++ ) {
		Drumkit *pInfo = m_systemDrumkitInfoList[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	for ( uint i = 0; i < m_userDrumkitInfoList.size(); i++ ) {
		Drumkit*pInfo = m_userDrumkitInfoList[i];
		if ( pInfo->getName() == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	assert( drumkitInfo );


	setCursor( QCursor( Qt::WaitCursor ) );

	Hydrogen::get_instance()->loadDrumkit( drumkitInfo );
	Hydrogen::get_instance()->getSong()->__is_modified = true;
	HydrogenApp::getInstance()->setStatusBarMessage( trUtf8( "Drumkit loaded: [%1]" ).arg( drumkitInfo->getName() ), 2000 );

	setCursor( QCursor( Qt::ArrowCursor ) );

	// update drumkit info in save tab
	//saveTab_nameTxt ->setText( QString( drumkitInfo->getName().c_str() ) );
	//saveTab_authorTxt->setText( QString( drumkitInfo->getAuthor().c_str() ) );
	//saveTab_infoTxt->append( QString( drumkitInfo->getInfo().c_str() ) );

//	HydrogenApp::getInstance()->getPatternEditorPanel()->getPatternEditor()->updateEditor( true );

//HydrogenApp::getInstance()->getPatternEditorPanel()->getDrumPatternEditor()->updateEditor();
}



void SoundLibraryPanel::on_drumkitDeleteAction()
{
	QString sSoundLibrary = m_pSoundLibraryTree->currentItem()->text( 0 );


	bool bIsUserSoundLibrary =false;
	std::vector<QString> userList = Drumkit::getUserDrumkitList();
	for ( uint i = 0; i < userList.size(); ++i ) {
		if ( userList[ i ].endsWith( sSoundLibrary ) ) {
			bIsUserSoundLibrary = true;
			break;
		}
	}

	if ( bIsUserSoundLibrary == false ) {
		QMessageBox::warning( this, "Hydrogen", QString( "A system drumkit can't be deleted.") );
		return;
	}

	int res = QMessageBox::information( this, "Hydrogen", tr( "Warning, the selected drumkit will be deleted from disk.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( res == 1 ) {
		return;
	}

	Drumkit::removeDrumkit( sSoundLibrary );

	HydrogenApp::getInstance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}



void SoundLibraryPanel::on_drumkitExportAction()
{
	SoundLibraryExportDialog exportDialog( this );
	exportDialog.exec();
}



void SoundLibraryPanel::on_drumkitRenameAction()
{
	QMessageBox::warning( this, "Hydrogen", QString( "Not implemented yet.") );
	ERRORLOG( "not implemented yet" );
}



void SoundLibraryPanel::on_instrumentDeleteAction()
{
	QMessageBox::warning( this, "Hydrogen", QString( "Not implemented yet.") );
	ERRORLOG( "[on_instrumentDeleteAction] not implemented yet" );
}

void SoundLibraryPanel::on_songLoadAction()
{
	QString songName = m_pSoundLibraryTree->currentItem()->text( 0 );
	QString sDirectory = Preferences::getInstance()->getDataDirectory()  + "songs";

	QString sFilename = sDirectory + "/" + songName + ".h2song";
	

	Hydrogen *engine = Hydrogen::get_instance();
	if ( engine->getState() == STATE_PLAYING ) {
                engine->sequencer_stop();
	}

	H2Core::LocalFileMng mng;
	Song *pSong = Song::load( sFilename );
	if ( pSong == NULL ) {
		QMessageBox::information( this, "Hydrogen", trUtf8("Error loading song.") );
		return;
	}

	// add the new loaded song in the "last used song" vector
	Preferences *pPref = Preferences::getInstance();

	std::vector<QString> recentFiles = pPref->getRecentFiles();
	recentFiles.insert( recentFiles.begin(), sFilename );
	pPref->setRecentFiles( recentFiles );

	HydrogenApp* h2app = HydrogenApp::getInstance();

	h2app->setSong( pSong );

	//updateRecentUsedSongList();
	engine->setSelectedPatternNumber( 0 );
}


void SoundLibraryPanel::on_patternLoadAction()
{
	QString patternName = m_pSoundLibraryTree->currentItem()->text( 0 );
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *pPatternList = song->get_pattern_list();
	
	QString sDirectory = Preferences::getInstance()->getDataDirectory()  + "patterns";
	
	LocalFileMng mng;
	LocalFileMng fileMng;
	Pattern* err = fileMng.loadPattern (sDirectory + "/" +  patternName + ".h2pattern" );

	if ( err == 0 )
	{
		_ERRORLOG ( "Error loading the pattern" );
	}
	else
	{
		H2Core::Pattern *pNewPattern = err;
		pPatternList->add ( pNewPattern );
		song->__is_modified = true;
	}

	HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
}

