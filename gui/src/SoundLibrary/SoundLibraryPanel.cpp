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

#include "SoundLibraryPanel.h"

#include <QtGui>

#include "SoundLibraryTree.h"
#include "FileBrowser.h"

#include "SoundLibrarySaveDialog.h"

#include "../HydrogenApp.h"
#include "../widgets/Button.h"
#include "../widgets/PixmapWidget.h"
#include "../Skin.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../PatternEditor/PatternEditorInstrumentList.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/data_path.h>
#include <hydrogen/Sample.h>
#include <hydrogen/adsr.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/H2Exception.h>
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
	m_pSoundLibraryTree->clear();

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


	LocalFileMng mng;
	vector<string> userList = Drumkit::getUserDrumkitList();
	for (uint i = 0; i < userList.size(); ++i) {
	  string absPath = Preferences::getInstance()->getDataDirectory() + "/" + userList[i];
		Drumkit *pInfo = mng.loadDrumkit( absPath );
		if (pInfo) {
			m_userDrumkitInfoList.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( m_pUserDrumkitsItem );
			pDrumkitItem->setText( 0, QString( pInfo->getName().c_str() ) );

			InstrumentList *pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				Instrument *pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + QString( pInstr->get_name().c_str() ) );
			}
		}
	}


	vector<string> systemList = Drumkit::getSystemDrumkitList();
	for (uint i = 0; i < systemList.size(); i++) {
		string absPath = DataPath::get_data_path() + "/drumkits/" + systemList[i];
		Drumkit *pInfo = mng.loadDrumkit( absPath );
		if (pInfo) {
			m_systemDrumkitInfoList.push_back( pInfo );

			QTreeWidgetItem* pDrumkitItem = new QTreeWidgetItem( m_pSystemDrumkitsItem );
			pDrumkitItem->setText( 0, QString( pInfo->getName().c_str() ) );

			InstrumentList *pInstrList = pInfo->getInstrumentList();
			for ( uint nInstr = 0; nInstr < pInstrList->get_size(); ++nInstr ) {
				Instrument *pInstr = pInstrList->get( nInstr );

				QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
				pInstrumentItem->setText( 0, QString( "[%1] " ).arg( nInstr + 1 ) + QString( pInstr->get_name().c_str() ) );
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
	if ( item == m_pSystemDrumkitsItem || item == m_pUserDrumkitsItem || item == m_pSystemDrumkitsItem->parent() ) {
		return;
	}

	if ( item->parent() == m_pSystemDrumkitsItem || item->parent() == m_pUserDrumkitsItem ) {
		// e' stato selezionato un drumkit
	}
	else {
		// e' stato selezionato uno strumento
		QString selectedName = item->text(0);

		string sInstrName = ( selectedName.remove( 0, selectedName.indexOf( "] " ) + 2 ) ).toStdString();
		string sDrumkitName = item->parent()->text(0).toStdString();
		INFOLOG( sDrumkitName + string(", instr:") + sInstrName );

		Instrument *pInstrument = Instrument::load_instrument( sDrumkitName, sInstrName );
		pInstrument->set_muted( false );

		AudioEngine::get_instance()->get_sampler()->preview_instrument( pInstrument );
	}
}







void SoundLibraryPanel::on_DrumkitList_rightClicked( QPoint pos )
{
	if (
		( m_pSoundLibraryTree->currentItem()->parent() == NULL ) ||
		( m_pSoundLibraryTree->currentItem() == m_pUserDrumkitsItem ) ||
		( m_pSoundLibraryTree->currentItem() == m_pSystemDrumkitsItem )
	) {
		return;
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
		if ( QString( pInfo->getName().c_str() ) == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	for ( uint i = 0; i < m_userDrumkitInfoList.size(); i++ ) {
		Drumkit*pInfo = m_userDrumkitInfoList[i];
		if ( QString( pInfo->getName().c_str() ) == sDrumkitName ) {
			drumkitInfo = pInfo;
			break;
		}
	}
	assert( drumkitInfo );


	setCursor( QCursor( Qt::WaitCursor ) );

	Hydrogen::get_instance()->loadDrumkit( drumkitInfo );
	Hydrogen::get_instance()->getSong()->m_bIsModified = true;
	HydrogenApp::getInstance()->setStatusBarMessage( trUtf8( "Drumkit loaded: [%1]" ).arg( drumkitInfo->getName().c_str() ), 2000 );

	setCursor( QCursor( Qt::ArrowCursor ) );

	// update drumkit info in save tab
	//saveTab_nameTxt ->setText( QString( drumkitInfo->getName().c_str() ) );
	//saveTab_authorTxt->setText( QString( drumkitInfo->getAuthor().c_str() ) );
	//saveTab_infoTxt->append( QString( drumkitInfo->getInfo().c_str() ) );

//	HydrogenApp::getInstance()->getPatternEditorPanel()->getPatternEditor()->updateEditor( true );
}



void SoundLibraryPanel::on_drumkitDeleteAction()
{
	QString sSoundLibrary = m_pSoundLibraryTree->currentItem()->text( 0 );

	bool bIsUserSoundLibrary =false;
	vector<string> userList = Drumkit::getUserDrumkitList();
	for ( uint i = 0; i < userList.size(); ++i ) {
		if ( userList[ i ] == sSoundLibrary.toStdString() ) {
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

	Drumkit::removeDrumkit( sSoundLibrary.toStdString() );

	HydrogenApp::getInstance()->getInstrumentRack()->getSoundLibraryPanel()->updateDrumkitList();
}



void SoundLibraryPanel::on_drumkitExportAction()
{
	QMessageBox::warning( this, "Hydrogen", QString( "Not implemented yet.") );
	ERRORLOG( "[on_drumkitExportAction] not implemented yet" );
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





