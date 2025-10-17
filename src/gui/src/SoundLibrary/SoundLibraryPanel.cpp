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

#include "SoundLibraryPanel.h"

#include <QtGui>
#include <QtWidgets>

#include "SoundLibraryTree.h"
#include "FileBrowser.h"

#include "SoundLibraryPropertiesDialog.h"
#include "SoundLibraryExportDialog.h"

#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../Widgets/Button.h"
#include "../Widgets/PixmapWidget.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../PatternEditor/DrumPatternEditor.h"
#include "../PatternEditor/PatternEditorInstrumentList.h"
#include "../InstrumentRack.h"
#include "../InstrumentEditor/InstrumentEditorPanel.h"

#include <core/Basics/Adsr.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/H2Exception.h>
#include <core/Hydrogen.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

#include <cassert>

SoundLibraryPanel::SoundLibraryPanel( QWidget *pParent, bool bInItsOwnDialog )
 : QWidget( pParent )
 , __sound_library_tree( nullptr )
 , __drumkit_menu( nullptr )
 , __song_menu( nullptr )
 , __pattern_menu( nullptr )
 , __pattern_menu_list( nullptr )
 , m_pTreeSystemDrumkitsItem( nullptr )
 , m_pTreeUserDrumkitsItem( nullptr )
 , m_pTreeSessionDrumkitsItem( nullptr )
 , __song_item( nullptr )
 , __pattern_item( nullptr )
 , __pattern_item_list( nullptr )
 , m_bInItsOwnDialog( bInItsOwnDialog )
{
	
	__drumkit_menu = new QMenu( this );
	__drumkit_menu->addAction( tr( "Load" ), this, SLOT( on_drumkitLoadAction() ) );
	__drumkit_menu->addAction( tr( "Export" ), this, SLOT( on_drumkitExportAction() ) );
	__drumkit_menu->addAction( tr( "Properties" ), this, SLOT( on_drumkitPropertiesAction() ) );
	__drumkit_menu->addSeparator();
	__drumkit_menu->addAction( tr( "Delete" ), this, SLOT( on_drumkitDeleteAction() ) );

	// A version with reduced functionality for read-only drumkits
	__drumkit_menu_system = new QMenu( this );
	__drumkit_menu_system->addAction( tr( "Load" ), this, SLOT( on_drumkitLoadAction() ) );
	__drumkit_menu_system->addAction( tr( "Export" ), this, SLOT( on_drumkitExportAction() ) );
	__drumkit_menu_system->addAction( tr( "Properties" ), this, SLOT( on_drumkitPropertiesAction() ) );

	__song_menu = new QMenu( this );
	__song_menu->addSeparator();
	__song_menu->addAction( tr( "Load" ), this, SLOT( on_songLoadAction() ) );

	__pattern_menu = new QMenu( this );
	__pattern_menu->addSeparator();
	__pattern_menu->addAction( tr( "Load" ), this, SLOT( on_patternLoadAction() ) );
	__pattern_menu->addAction( tr( "Delete" ), this, SLOT( on_patternDeleteAction() ) );

	__pattern_menu_list = new QMenu( this );
	__pattern_menu_list->addSeparator();
	__pattern_menu_list->addAction( tr( "Load" ), this, SLOT( on_patternLoadAction() ) );

// DRUMKIT LIST
	__sound_library_tree = new SoundLibraryTree( nullptr );
	connect( __sound_library_tree, SIGNAL( currentItemChanged ( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( on_DrumkitList_ItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
	connect( __sound_library_tree, SIGNAL( itemActivated ( QTreeWidgetItem*, int ) ), this, SLOT( on_DrumkitList_itemActivated( QTreeWidgetItem*, int ) ) );
	connect( __sound_library_tree, SIGNAL( leftClicked(QPoint) ), this, SLOT( on_DrumkitList_leftClicked(QPoint)) );
	if( ! m_bInItsOwnDialog ) {
		connect( __sound_library_tree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_DrumkitList_rightClicked(QPoint)) );
		connect( __sound_library_tree, SIGNAL( onMouseMove( QMouseEvent* ) ), this, SLOT( on_DrumkitList_mouseMove( QMouseEvent* ) ) );
	}


	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setContentsMargins( 0, 0, 0, 0 );

	pVBox->addWidget( __sound_library_tree );
	

	this->setLayout( pVBox );

	__expand_pattern_list = Preferences::get_instance()->__expandPatternItem;
	__expand_songs_list = Preferences::get_instance()->__expandSongItem;

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &SoundLibraryPanel::onPreferencesChanged );
	
	updateTree();
	
	HydrogenApp::get_instance()->addEventListener(this);
}



SoundLibraryPanel::~SoundLibraryPanel()
{
	if ( auto pH2App = HydrogenApp::get_instance() ) {
		pH2App->removeEventListener( this );
	}
}



void SoundLibraryPanel::updateTree()
{
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_patternRegistry.clear();
	__sound_library_tree->clear();

	QFont boldFont( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	boldFont.setBold( true );

	QFont childFont( pPref->getLevel2FontFamily(), getPointSize( pPref->getFontSize() ) );
	setFont( childFont );
	
	m_pTreeSystemDrumkitsItem = nullptr;
	m_pTreeUserDrumkitsItem = nullptr;
	m_pTreeSessionDrumkitsItem = nullptr;

	// top-level drumkit items found
	QList<QTreeWidgetItem*> drumkitItems;

	// drumkit list
	m_drumkitRegister.clear();
	m_drumkitLabels.clear();
	for ( const auto& pDrumkitEntry : pSoundLibraryDatabase->getDrumkitDatabase() ) {
		auto pDrumkit = pDrumkitEntry.second;
		if ( pDrumkit != nullptr ) {
			QString sItemLabel = pDrumkit->get_name();
			auto drumkitType =
				Filesystem::determineDrumkitType( pDrumkitEntry.first );

			QTreeWidgetItem* pDrumkitItem;
			if ( drumkitType == Filesystem::DrumkitType::System ) {
				if ( m_pTreeSystemDrumkitsItem == nullptr ) {
					m_pTreeSystemDrumkitsItem = new QTreeWidgetItem();
					m_pTreeSystemDrumkitsItem->setText( 0, tr( "System drumkits" ) );
					m_pTreeSystemDrumkitsItem->setFont( 0, boldFont );
				}

				pDrumkitItem = new QTreeWidgetItem( m_pTreeSystemDrumkitsItem );
				sItemLabel.append( QString( " (%1)" )
								   .arg( pCommonStrings->getSoundLibrarySystemSuffix() ) );
			}
			else if ( drumkitType == Filesystem::DrumkitType::User ) {
				if ( m_pTreeUserDrumkitsItem == nullptr ) {
					m_pTreeUserDrumkitsItem = new QTreeWidgetItem();
					m_pTreeUserDrumkitsItem->setText( 0, tr( "User drumkits" ) );
					m_pTreeUserDrumkitsItem->setFont( 0, boldFont );
				}

				pDrumkitItem = new QTreeWidgetItem( m_pTreeUserDrumkitsItem );
			} else {
				if ( m_pTreeSessionDrumkitsItem == nullptr ) {
					m_pTreeSessionDrumkitsItem = new QTreeWidgetItem();
					m_pTreeSessionDrumkitsItem->setText( 0, tr( "Session drumkits" ) );
					m_pTreeSessionDrumkitsItem->setFont( 0, boldFont );
				}
				pDrumkitItem = new QTreeWidgetItem( m_pTreeSessionDrumkitsItem );
				sItemLabel.append( QString( " (%1)" )
								   .arg( pCommonStrings->getSoundLibrarySessionSuffix() ) );
			}


			// Ensure uniqueness of the label.
			int nCount = 1;
			while ( m_drumkitLabels.contains( sItemLabel ) ) {
				sItemLabel = QString( "%1 (%2)" )
					.arg( pDrumkit->get_name() ).arg( nCount );
				nCount++;
			}

			m_drumkitLabels << sItemLabel;
			m_drumkitRegister[ sItemLabel ] = pDrumkitEntry.first;
			
			pDrumkitItem->setText( 0, sItemLabel );
			pDrumkitItem->setToolTip( 0, pDrumkitEntry.first );
			if ( ! m_bInItsOwnDialog ) {
				auto pInstrList = pDrumkit->get_instruments();
				for ( const auto& pInstrument : *pDrumkit->get_instruments() ) {
					if ( pInstrument != nullptr ) {
						QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
						pInstrumentItem->setText( 0, QString( "[%1] %2" )
												  .arg( pInstrument->get_id() )
												  .arg( pInstrument->get_name() ) );
						pInstrumentItem->setToolTip( 0, pInstrument->get_name() );
					}
				}
			}
		}
	}

	// Ensure the ordering of the top-level nodes is always
	// system > user > session
	if ( m_pTreeSystemDrumkitsItem != nullptr ) {
		drumkitItems << m_pTreeSystemDrumkitsItem;
	}
	if ( m_pTreeUserDrumkitsItem != nullptr ) {
		drumkitItems << m_pTreeUserDrumkitsItem;
	}
	if ( m_pTreeSessionDrumkitsItem != nullptr ) {
		drumkitItems << m_pTreeSessionDrumkitsItem;
	}
	__sound_library_tree->addTopLevelItems( drumkitItems );

	// Ensure drumkit nodes are expanded (necessary when added as
	// above.)
	if ( m_pTreeSystemDrumkitsItem != nullptr ) {
		m_pTreeSystemDrumkitsItem->setExpanded( true );
	}
	if ( m_pTreeUserDrumkitsItem != nullptr ) {
		m_pTreeUserDrumkitsItem->setExpanded( true );
	}
	if ( m_pTreeSessionDrumkitsItem != nullptr ) {
		m_pTreeSessionDrumkitsItem->setExpanded( true );
	}

	if ( ! m_bInItsOwnDialog ) {
		//Songlist
		QStringList songs = Filesystem::song_list_cleared();
		if ( songs.size() > 0 ) {
			__song_item = new QTreeWidgetItem( __sound_library_tree );
			__song_item->setText( 0, tr( "Songs" ) );
			__song_item->setToolTip( 0, tr("Double click to expand the list") );
			__song_item->setExpanded( __expand_songs_list );
			__song_item->setFont( 0, boldFont );
			for (uint i = 0; i < songs.size(); i++) {
				QTreeWidgetItem* pSongItem = new QTreeWidgetItem( __song_item );
				QString song = songs[i];
				pSongItem->setText( 0 , song.left( song.indexOf(".")) );
				pSongItem->setToolTip( 0, song );
			}
		}


		//Pattern list
		QStringList patternDirList = Filesystem::pattern_drumkits();
		if ( patternDirList.size() > 0 ) {
		
			__pattern_item = new QTreeWidgetItem( __sound_library_tree );
			__pattern_item->setText( 0, tr( "Patterns" ) );
			__pattern_item->setToolTip( 0, tr("Double click to expand the list") );
			__pattern_item->setExpanded( __expand_pattern_list );
			__pattern_item->setFont( 0, boldFont );
		
			auto patternInfoVector = pSoundLibraryDatabase->getPatternInfoVector();
			QStringList patternCategories =
				pSoundLibraryDatabase->getPatternCategories();

			//now sorting via category

			/*: Base tooltip displayed when hovering over a pattern in
			  the Sound Library. It indicates which drumkit the
			  pattern was created with*/
			QString sPatternTooltip = tr( "Created for drumkit" );
			for ( const auto& categoryName : patternCategories ) {

				QTreeWidgetItem* pCategoryItem = new QTreeWidgetItem( __pattern_item );
				pCategoryItem->setText( 0, categoryName  );

				for ( const auto& pInfo : patternInfoVector ) {
					QString patternCategory = pInfo->getCategory();
					if ( ( patternCategory == categoryName ) ||
						 ( patternCategory.isEmpty() && categoryName == "No category" ) ){
						QTreeWidgetItem* pPatternItem = new QTreeWidgetItem( pCategoryItem );
						pPatternItem->setText( 0, pInfo->getName());
						pPatternItem->setText( 1, pInfo->getPath() );
						pPatternItem->setToolTip( 0, QString( "%1 [%2]" )
												  .arg( sPatternTooltip )
												  .arg( pInfo->getDrumkitName() ) );
						m_patternRegistry[ pPatternItem ] = pInfo;
					}
				}
			}
		}
	}
	
	update_background_color();
}



void SoundLibraryPanel::on_DrumkitList_ItemChanged( QTreeWidgetItem * current, QTreeWidgetItem * previous )
{
	UNUSED( previous );
	
	if( current == nullptr ){
		return;
	}

	if ( current->parent() == m_pTreeSystemDrumkitsItem ||
		 current->parent() == m_pTreeUserDrumkitsItem ||
		 current->parent() == m_pTreeSessionDrumkitsItem ){
			emit item_changed( true );
	} else {
		emit item_changed( false );
	}
	
	test_expandedItems();
}



void SoundLibraryPanel::on_DrumkitList_itemActivated( QTreeWidgetItem * item, int column )
{
	UNUSED( column );

//	INFOLOG( "[on_DrumkitList_itemActivated]" );
	if ( item == m_pTreeSystemDrumkitsItem ||
		 item == m_pTreeUserDrumkitsItem ||
		 item == m_pTreeSessionDrumkitsItem ||
		 ( ( m_pTreeSystemDrumkitsItem != nullptr &&
			 item == m_pTreeSystemDrumkitsItem->parent() ) ||
		   ( m_pTreeUserDrumkitsItem != nullptr &&
			 item == m_pTreeUserDrumkitsItem->parent() ) ||
		   ( m_pTreeSessionDrumkitsItem != nullptr &&
			 item == m_pTreeSessionDrumkitsItem->parent() ) )||
		 item->parent() == __song_item ||
		 item == __song_item ||
		 item == __pattern_item ||
		 item->parent() == __pattern_item ||
		 item->parent()->parent() == __pattern_item ||
		 item == __pattern_item_list ||
		 item->parent() == __pattern_item_list ||
		 item->parent()->parent() == __pattern_item_list ) {
		return;
	}

	if ( item->parent() == m_pTreeSystemDrumkitsItem ||
		 item->parent() == m_pTreeUserDrumkitsItem  ||
		 item->parent() == m_pTreeSessionDrumkitsItem  ) {
		// Double clicking a drumkit
	}
	else {
		// Double clicking an instrument
		QString sSelectedName = item->text(0);

		QString sInstrName = sSelectedName.remove( 0, sSelectedName.indexOf( "] " ) + 2 );
		QString sDrumkitName = item->parent()->text(0);
		QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
		INFOLOG( QString( "Loading instrument [%1] from drumkit [%2] located in [%3]" )
				 .arg( sInstrName ).arg( sDrumkitName ).arg( sDrumkitPath ) );

		auto pInstrument = Instrument::load_instrument( sDrumkitPath, sInstrName );

		if ( pInstrument == nullptr ) {
			ERRORLOG( "Unable to load instrument. Abort" );
			return;
		}
		
		pInstrument->set_muted( false );

		Hydrogen::get_instance()->getAudioEngine()->getSampler()->preview_instrument( pInstrument );
	}
}







void SoundLibraryPanel::on_DrumkitList_rightClicked( QPoint pos )
{
	if( __sound_library_tree->currentItem() == nullptr ) {
		return;
	}
	
	if (
		__sound_library_tree->currentItem()->parent() == nullptr ||
		__sound_library_tree->currentItem() == m_pTreeUserDrumkitsItem ||
		__sound_library_tree->currentItem() == m_pTreeSystemDrumkitsItem ||
		__sound_library_tree->currentItem() == m_pTreeSessionDrumkitsItem ) {
		return;
	}

	if ( __sound_library_tree->currentItem()->parent() == __song_item ) {
		__song_menu->popup( pos );
	}

	if ( __sound_library_tree->currentItem()->parent()->parent() == __pattern_item && __pattern_item != nullptr ) {
		__pattern_menu->popup( pos );
	}

	if ( __sound_library_tree->currentItem()->parent() == m_pTreeUserDrumkitsItem ) {
		__drumkit_menu->popup( pos );
	}
	
	if ( __sound_library_tree->currentItem()->parent() == m_pTreeSystemDrumkitsItem ) {
		__drumkit_menu_system->popup( pos );
	}
	
	// We do not provide distinct parent items for read-only and
	// writable session drumkits as it would make the GUI unnecessary
	// complex. Instead, the level of access for the current user is
	// checked during runtime (which should be a very rare thing to do).
	if ( __sound_library_tree->currentItem()->parent() == m_pTreeSessionDrumkitsItem ) {
		const QString sDrumkitName = __sound_library_tree->currentItem()->text( 0 );
		const QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
		const auto drumkitType = Filesystem::determineDrumkitType( sDrumkitPath );
		
		if ( drumkitType == Filesystem::DrumkitType::SessionReadOnly ) {
			__drumkit_menu_system->popup( pos );
		} else {
			__drumkit_menu->popup( pos );
		}
	}

}



void SoundLibraryPanel::on_DrumkitList_leftClicked( QPoint pos )
{
	__start_drag_position = pos;
}



void SoundLibraryPanel::on_DrumkitList_mouseMove( QMouseEvent *event)
{
	if (! ( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	if ( ( event->pos() - __start_drag_position ).manhattanLength() < QApplication::startDragDistance() ) {
		return;
	}
	
	if ( !__sound_library_tree->currentItem() ) {
		return;
	}

	if ( __sound_library_tree->currentItem()->parent() == m_pTreeSystemDrumkitsItem ||
		 __sound_library_tree->currentItem()->parent() == m_pTreeUserDrumkitsItem ||
		 __sound_library_tree->currentItem()->parent() == m_pTreeSessionDrumkitsItem ) {
 		// drumkit selection
		//INFOLOG( "ho selezionato un drumkit (system)" );
		return;
	}
	else {
		//INFOLOG( "ho selezionato uno strumento" );
		// instrument selection
		if ( __sound_library_tree->currentItem() == nullptr )
		{
			return;
		}
		
		if ( __sound_library_tree->currentItem()->parent() == nullptr )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent() == __song_item )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent()->text(0) == nullptr )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent() == __pattern_item ) {
			return;
		}

		if ( __sound_library_tree->currentItem()->parent()->parent() == __pattern_item ) {

			QString sPatternPath = __sound_library_tree->currentItem()->text( 1 );
			QString dragtype = "drag pattern";
			QString sText = dragtype + "::" + sPatternPath;

			QDrag *pDrag = new QDrag(this);
			QMimeData *pMimeData = new QMimeData;

			pMimeData->setText( sText );
			pDrag->setMimeData( pMimeData);
			pDrag->exec( Qt::CopyAction | Qt::MoveAction );
			return;
		}

		const QString sDrumkitName = __sound_library_tree->currentItem()->parent()->text(0);
		const QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
		const QString sInstrumentName = ( __sound_library_tree->currentItem()->text(0) )
			.remove( 0, __sound_library_tree->currentItem()->text(0).indexOf( "] " ) + 2 );

		const QString sText = "importInstrument:" + sDrumkitPath + "::" + sInstrumentName;

		QDrag *pDrag = new QDrag(this);
		QMimeData *pMimeData = new QMimeData;

		pMimeData->setText( sText );
		pDrag->setMimeData( pMimeData);

		pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	}
}



void SoundLibraryPanel::on_drumkitLoadAction()
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);
	QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	auto pDrumkit =
		pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
	
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to find drumkit [%1] (mapped to path [%2]" )
				  .arg( sDrumkitName ).arg( sDrumkitPath ) );
		return;
	}

	auto pSongInstrList = pHydrogen->getSong()->getInstrumentList();
	auto pDrumkitInstrList = pDrumkit->get_instruments();

	int oldCount = pSongInstrList->size();
	int newCount = pDrumkitInstrList->size();

	bool conditionalLoad = false;
	bool hasNotes = false;

	INFOLOG("Old kit has " + QString::number( oldCount ) + " instruments, new one has " + QString::number( newCount ) );

	if ( newCount < oldCount )
	{
		// Check if any of the instruments that will be removed have notes
		for ( int i = 0; i < pSongInstrList->size(); i++)
		{
			if ( i >= newCount )
			{
				INFOLOG("Checking if Instrument " + QString::number( i ) + " has notes..." );

				if ( pHydrogen->instrumentHasNotes( pSongInstrList->get( i ) ) )
				{
					hasNotes = true;
					INFOLOG("Instrument " + QString::number( i ) + " has notes" );
				}
			}

		}
	
		if ( hasNotes ) {
			auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
			QMessageBox msgBox;
			msgBox.setWindowTitle("Hydrogen");
			msgBox.setIcon( QMessageBox::Warning );
			msgBox.setText( tr( "The existing kit has %1 instruments but the new one only has %2.\nThe first %2 instruments will be replaced with the new instruments and will keep their notes, but some of the remaining instruments have notes.\nWould you like to keep or discard the remaining instruments and notes?\n").arg( QString::number( oldCount ),QString::number( newCount ) ) );

			msgBox.setStandardButtons( QMessageBox::Save | QMessageBox::Discard |
									   QMessageBox::Cancel );
			msgBox.setDefaultButton(QMessageBox::Cancel);
			
			switch ( msgBox.exec() )
			{
				case QMessageBox::Save:
					// Save old instruments with notes
					conditionalLoad = true;
					break;

				case QMessageBox::Discard:
					// discard extra instruments
					conditionalLoad = false;
					break;

				case QMessageBox::Cancel:
					// Cancel
					return;
			}
		}
	}

	assert( pDrumkit );

	QApplication::setOverrideCursor(Qt::WaitCursor);

	pHydrogen->getCoreActionController()->setDrumkit( pDrumkit, conditionalLoad );

	QApplication::restoreOverrideCursor();
}

void SoundLibraryPanel::drumkitLoadedEvent() {
	update_background_color();
}

void SoundLibraryPanel::selectedInstrumentChangedEvent() {
	update_background_color();
}

void SoundLibraryPanel::update_background_color()
{
	restore_background_color();
	change_background_color();
}



void SoundLibraryPanel::restore_background_color()
{
	if ( m_pTreeSystemDrumkitsItem != nullptr ) {
		for (int i = 0; i < m_pTreeSystemDrumkitsItem->childCount() ; i++){
			( m_pTreeSystemDrumkitsItem->child( i ) )->setBackground( 0, QBrush() );		
		}
	}

	if ( m_pTreeUserDrumkitsItem != nullptr ) {
		for (int i = 0; i < m_pTreeUserDrumkitsItem->childCount() ; i++){
			( m_pTreeUserDrumkitsItem->child( i ) )->setBackground(0, QBrush() );
		}
	}

	if ( m_pTreeSessionDrumkitsItem != nullptr ) {
		for (int i = 0; i < m_pTreeSessionDrumkitsItem->childCount() ; i++){
			( m_pTreeSessionDrumkitsItem->child( i ) )->setBackground( 0, QBrush() );		
		}
	}

}

QString SoundLibraryPanel::getDrumkitLabel( const QString& sDrumkitPath ) const {
	for ( const auto& entry : m_drumkitRegister ) {
		if ( entry.second == sDrumkitPath ) {
			return entry.first;
		}
	}

	return "";
}
QString SoundLibraryPanel::getDrumkitPath( const QString& sDrumkitLabel ) const {
	return m_drumkitRegister.at( sDrumkitLabel );
}

void SoundLibraryPanel::change_background_color()
{
	auto pSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		DEBUGLOG( "No instrument selected" );
		return;
	}
	QString sDrumkitPath = pSelectedInstrument->get_drumkit_path();
	QString sDrumkitLabel = getDrumkitLabel( sDrumkitPath );

	if ( sDrumkitLabel.isEmpty() ) {
		ERRORLOG( QString( "Unable to find label corresponding to drumkit [%1]. No highlighting applied" )
				  .arg( sDrumkitPath ) );
		return;
	}

	if ( m_pTreeSystemDrumkitsItem != nullptr ) {
		for ( int i = 0; i < m_pTreeSystemDrumkitsItem->childCount() ; i++){
			if ( ( m_pTreeSystemDrumkitsItem->child( i ) )->text( 0 ) == sDrumkitLabel ){
				( m_pTreeSystemDrumkitsItem->child( i ) )->setBackground( 0, QColor( 50, 50, 50)  );
				return;
			}
		}
	}

	if ( m_pTreeUserDrumkitsItem != nullptr ) {
		for (int i = 0; i < m_pTreeUserDrumkitsItem->childCount() ; i++){
			if ( ( m_pTreeUserDrumkitsItem->child( i ))->text( 0 ) == sDrumkitLabel ){
				( m_pTreeUserDrumkitsItem->child( i ) )->setBackground( 0, QColor( 50, 50, 50)  );
				break;
			}
		}
	}
	
	if ( m_pTreeSessionDrumkitsItem != nullptr ) {
		for ( int i = 0; i < m_pTreeSessionDrumkitsItem->childCount() ; i++){
			if ( ( m_pTreeSessionDrumkitsItem->child( i ) )->text( 0 ) == sDrumkitLabel ){
				( m_pTreeSessionDrumkitsItem->child( i ) )->setBackground( 0, QColor( 50, 50, 50)  );
				return;
			}
		}
	}

}


void SoundLibraryPanel::on_drumkitDeleteAction()
{
	QTreeWidgetItem* pItem = __sound_library_tree->currentItem();
	const QString sDrumkitName = pItem->text(0);
	const QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	const auto drumkitType = Filesystem::determineDrumkitType( sDrumkitPath );
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
		 ( pItem->parent() == m_pTreeSessionDrumkitsItem &&
		   drumkitType == Filesystem::DrumkitType::SessionReadOnly ) ) {
		QMessageBox::warning( this, "Hydrogen", QString( "\"%1\" " )
							  .arg(sDrumkitName)
							  .append( tr( "is a read-only drumkit and can't be deleted.") ) );
		return;
	}

	// If we delete the current loaded drumkit we can get trouble with some empty pointers
	if ( pItem->text(0) == Hydrogen::get_instance()->getLastLoadedDrumkitName() ){
		QMessageBox::warning( this, "Hydrogen", tr( "It is not possible to delete the currently loaded drumkit: \n  \"%1\".\nTo delete this drumkit first load another drumkit.").arg(sDrumkitName) );
		return;
	}

	if ( QMessageBox::warning(
			 this, "Hydrogen",
			 tr( "Warning, the \"%1\" drumkit will be deleted from disk.\nAre you sure?").arg(sDrumkitName),
			 QMessageBox::Ok | QMessageBox::Cancel,
			 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	bool bSuccess = Drumkit::remove( m_drumkitRegister[ pItem->text(0) ] );
	QApplication::restoreOverrideCursor();
	if ( ! bSuccess ) {
		QMessageBox::warning( this, "Hydrogen", tr( "Drumkit deletion failed.") );
	}
}



void SoundLibraryPanel::on_drumkitExportAction()
{
	auto pSoundLibraryDatabase =
		Hydrogen::get_instance()->getSoundLibraryDatabase();
	
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);
	QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sDrumkitPath );
	
	SoundLibraryExportDialog exportDialog( this, pDrumkit );
	exportDialog.exec();
}



void SoundLibraryPanel::on_drumkitPropertiesAction()
{
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);
	QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sDrumkitPath );
	
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to find drumkit [%1] (mapped to path [%2]" )
				  .arg( sDrumkitName ).arg( sDrumkitPath ) );
		return;
	}
	
	// We provide a copy of the recent drumkit to ensure the drumkit
	// is not getting dirty upon saving (in case new properties are
	// stored in the kit but writing it to disk fails).
	auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
	SoundLibraryPropertiesDialog dialog( this, pNewDrumkit, true );
	dialog.exec();
}

void SoundLibraryPanel::on_songLoadAction()
{
	QString sFilename = Filesystem::song_path( __sound_library_tree->currentItem()->text( 0 ) );

	HydrogenApp::get_instance()->openSong( sFilename );
}



void SoundLibraryPanel::on_patternLoadAction() {
	if ( m_patternRegistry.find( __sound_library_tree->currentItem() ) ==
		 m_patternRegistry.end() ) {
		ERRORLOG( QString( "Unable to find pattern corresponding to [%1]" )
				  .arg( __sound_library_tree->currentItem()->text( 0 ) ) );
		return;
	}

	auto pInfo = m_patternRegistry.at( __sound_library_tree->currentItem() );
	if ( pInfo == nullptr ) {
		ERRORLOG( QString( "Invalid pattern info for [%1]" )
				  .arg( __sound_library_tree->currentItem()->text( 0 ) ) );
		return;
	}

	Hydrogen::get_instance()->getCoreActionController()
		->openPattern( pInfo->getPath() );
}


void SoundLibraryPanel::on_patternDeleteAction() {
	if ( m_patternRegistry.find( __sound_library_tree->currentItem() ) ==
		 m_patternRegistry.end() ) {
		ERRORLOG( QString( "Unable to find pattern corresponding to [%1]" )
				  .arg( __sound_library_tree->currentItem()->text( 0 ) ) );
		return;
	}

	auto pInfo = m_patternRegistry.at( __sound_library_tree->currentItem() );
	if ( pInfo == nullptr ) {
		ERRORLOG( QString( "Invalid pattern info for [%1]" )
				  .arg( __sound_library_tree->currentItem()->text( 0 ) ) );
		return;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( QMessageBox::information(
			 this, "Hydrogen",
			 tr( "Warning, the selected pattern will be deleted from disk.\nAre you sure?") +
								  QString( "\n\n%1" ).arg( pInfo->getPath() ),
			 QMessageBox::Ok | QMessageBox::Cancel,
			 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
		return;
	}

	if ( Filesystem::rm( pInfo->getPath() ) ) {
		ERRORLOG( QString( "Error removing the pattern [%1]" )
				.arg( pInfo->getPath() ) );
	}

	H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase()->updatePatterns();
}

void SoundLibraryPanel::soundLibraryChangedEvent() {
	test_expandedItems();
	updateTree();
}

void SoundLibraryPanel::test_expandedItems()
{
	assert( __sound_library_tree );
	if ( __song_item == nullptr) {
		__expand_songs_list = false;
	} else {
		__expand_songs_list = __song_item->isExpanded();
	}
	if ( __pattern_item == nullptr) {
		__expand_pattern_list = false;
	} else {
		__expand_pattern_list = __pattern_item->isExpanded();
	}
	Preferences::get_instance()->__expandSongItem = __expand_songs_list;
	Preferences::get_instance()->__expandPatternItem = __expand_pattern_list;
	//ERRORLOG( QString("songs %1 patterns %2").arg(__expand_songs_list).arg(__expand_pattern_list) );
}

void SoundLibraryPanel::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		QFont font( pPref->getLevel2FontFamily(), getPointSize( pPref->getFontSize() ) );
		QFont boldFont( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
		boldFont.setBold( true );

		int ii, jj;
		QTreeWidgetItem* childNode;
		if ( m_pTreeSystemDrumkitsItem != nullptr ) {
			m_pTreeSystemDrumkitsItem->setFont( 0, boldFont );
			for ( ii = 0; ii < m_pTreeSystemDrumkitsItem->childCount(); ii++ ){ 
				childNode = m_pTreeSystemDrumkitsItem->child( ii );
				childNode->setFont( 0, font );
				for ( jj = 0; jj < childNode->childCount(); jj++ ) {
					childNode->child( jj )->setFont( 0, font );
				}
			}
		}

		if ( m_pTreeUserDrumkitsItem != nullptr ) {
			m_pTreeUserDrumkitsItem->setFont( 0, boldFont );
			for ( ii = 0; ii < m_pTreeUserDrumkitsItem->childCount(); ii++ ){ 
				childNode = m_pTreeUserDrumkitsItem->child( ii );
				childNode->setFont( 0, font );
				for ( jj = 0; jj < childNode->childCount(); jj++ ) {
					childNode->child( jj )->setFont( 0, font );
				}
			}
		}
		
		if ( m_pTreeSessionDrumkitsItem != nullptr ) {
			m_pTreeSessionDrumkitsItem->setFont( 0, boldFont );
			for ( ii = 0; ii < m_pTreeSessionDrumkitsItem->childCount(); ii++ ){ 
				childNode = m_pTreeSessionDrumkitsItem->child( ii );
				childNode->setFont( 0, font );
				for ( jj = 0; jj < childNode->childCount(); jj++ ) {
					childNode->child( jj )->setFont( 0, font );
				}
			}
		}

		if ( __song_item != nullptr ) {
			__song_item->setFont( 0, boldFont );
			for ( ii = 0; ii < __song_item->childCount(); ii++ ){ 
				__song_item->child( ii )->setFont( 0, font );
				__song_item->setFont( ii, font );
			}
		}

		if ( __pattern_item != nullptr ) {
			__pattern_item->setFont( 0, boldFont );
			for ( ii = 0; ii < __pattern_item->childCount(); ii++ ){ 
				childNode = __pattern_item->child( ii );
				childNode->setFont( 0, font );
				for ( jj = 0; jj < childNode->childCount(); jj++ ) {
					childNode->child( jj )->setFont( 0, font );
				}
			}
		}
	}
}

void SoundLibraryPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		update_background_color();
	}
}
