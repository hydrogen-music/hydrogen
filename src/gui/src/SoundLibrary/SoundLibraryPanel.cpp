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

#include "DrumkitPropertiesDialog.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../InstrumentRack.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/H2Exception.h>
#include <core/Hydrogen.h>
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
	setMinimumWidth( InstrumentRack::nWidth );
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = Preferences::get_instance();

	auto addDrumkitActions = [&]( QMenu* pMenu, bool bWritable) {
		pMenu->addAction( pCommonStrings->getMenuActionLoad(), this,
						  SLOT( on_drumkitLoadAction() ) );
		pMenu->addAction( pCommonStrings->getMenuActionProperties(), this,
						  [=](){ editDrumkitProperties( false );} );
		pMenu->addSeparator();
		pMenu->addAction( pCommonStrings->getMenuActionDuplicate(), this,
						  [=](){ editDrumkitProperties( true );} );
		auto pDeleteAction =
			pMenu->addAction( pCommonStrings->getMenuActionDelete(), this,
							  SLOT( on_drumkitDeleteAction() ) );
		if ( ! bWritable ) {
			pDeleteAction->setEnabled( false );
		}
		pMenu->addAction( pCommonStrings->getMenuActionExport(), this,
						  SLOT( on_drumkitExportAction() ) );
		pMenu->addSeparator();
		pMenu->addAction( pCommonStrings->getMenuActionImport(), this,
						  [=](){ HydrogenApp::get_instance()->getMainForm()->
								  action_drumkit_import( false ); } );
		pMenu->addAction( pCommonStrings->getMenuActionOnlineImport(),
						  HydrogenApp::get_instance()->getMainForm(),
						  SLOT( action_drumkit_onlineImport() ) );
	};

	__drumkit_menu = new QMenu( this );
	addDrumkitActions( __drumkit_menu, true );

	__drumkit_menu_system = new QMenu( this );
	addDrumkitActions( __drumkit_menu_system, false );

	__song_menu = new QMenu( this );
	__song_menu->addSeparator();
	__song_menu->addAction( pCommonStrings->getMenuActionLoad(), this,
							SLOT( on_songLoadAction() ) );

	__pattern_menu = new QMenu( this );
	__pattern_menu->addSeparator();
	__pattern_menu->addAction( pCommonStrings->getMenuActionLoad(), this,
							   SLOT( on_patternLoadAction() ) );
	__pattern_menu->addAction( pCommonStrings->getMenuActionDelete(), this,
							   SLOT( on_patternDeleteAction() ) );

	__pattern_menu_list = new QMenu( this );
	__pattern_menu_list->addSeparator();
	__pattern_menu_list->addAction( pCommonStrings->getMenuActionLoad(), this,
									SLOT( on_patternLoadAction() ) );

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
	const auto pPref = H2Core::Preferences::get_instance();
	const auto theme = pPref->getTheme();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	__sound_library_tree->clear();

	QFont boldFont( theme.m_font.m_sApplicationFontFamily, getPointSize( theme.m_font.m_fontSize ) );
	boldFont.setBold( true );

	QFont childFont( theme.m_font.m_sLevel2FontFamily, getPointSize( theme.m_font.m_fontSize ) );
	setFont( childFont );
	
	m_pTreeSystemDrumkitsItem = nullptr;
	m_pTreeUserDrumkitsItem = nullptr;
	m_pTreeSessionDrumkitsItem = nullptr;

	// top-level drumkit items found
	QList<QTreeWidgetItem*> drumkitItems;

	// drumkit list
	m_drumkitRegister.clear();
	m_drumkitLabels.clear();
	for ( const auto& [ssPath, ppDrumkit] : pSoundLibraryDatabase->getDrumkitDatabase() ) {
		if ( ppDrumkit == nullptr ) {
			continue;
		}

		const QString sItemLabel = pSoundLibraryDatabase->getUniqueLabel( ssPath );
		if ( sItemLabel.isEmpty() ) {
			ERRORLOG( QString( "Unable to retrieve unique label for kit in path [%1]" )
					  .arg( ssPath ) );
			continue;
		}

		const auto drumkitContext = ppDrumkit->getContext();

		QTreeWidgetItem* pDrumkitItem;
		if ( drumkitContext == Drumkit::Context::System ) {
			if ( m_pTreeSystemDrumkitsItem == nullptr ) {
				m_pTreeSystemDrumkitsItem = new QTreeWidgetItem();
				m_pTreeSystemDrumkitsItem->setText( 0, tr( "System drumkits" ) );
				m_pTreeSystemDrumkitsItem->setFont( 0, boldFont );
			}

			pDrumkitItem = new QTreeWidgetItem( m_pTreeSystemDrumkitsItem );
		}
		else if ( drumkitContext == Drumkit::Context::User ) {
			if ( m_pTreeUserDrumkitsItem == nullptr ) {
				m_pTreeUserDrumkitsItem = new QTreeWidgetItem();
				m_pTreeUserDrumkitsItem->setText( 0, tr( "User drumkits" ) );
				m_pTreeUserDrumkitsItem->setFont( 0, boldFont );
			}

			pDrumkitItem = new QTreeWidgetItem( m_pTreeUserDrumkitsItem );
		}
		else if ( drumkitContext == Drumkit::Context::SessionReadOnly ||
					drumkitContext == Drumkit::Context::SessionReadWrite ) {
			if ( m_pTreeSessionDrumkitsItem == nullptr ) {
				m_pTreeSessionDrumkitsItem = new QTreeWidgetItem();
				m_pTreeSessionDrumkitsItem->setText( 0, tr( "Session drumkits" ) );
				m_pTreeSessionDrumkitsItem->setFont( 0, boldFont );
			}
			pDrumkitItem = new QTreeWidgetItem( m_pTreeSessionDrumkitsItem );
		}
		else {
			ERRORLOG( QString( "Drumkits of context [%1] should not end up in the SoundLibrary." )
					  .arg( Drumkit::ContextToString( drumkitContext ) ) );
			continue;
		}

		m_drumkitLabels << sItemLabel;
		m_drumkitRegister[ sItemLabel ] = ssPath;
			
		pDrumkitItem->setText( 0, sItemLabel );
		pDrumkitItem->setToolTip( 0, ssPath );
		if ( ! m_bInItsOwnDialog ) {
			auto pInstrList = ppDrumkit->getInstruments();
			for ( const auto& pInstrument : *ppDrumkit->getInstruments() ) {
				if ( pInstrument != nullptr ) {
					QTreeWidgetItem* pInstrumentItem = new QTreeWidgetItem( pDrumkitItem );
					pInstrumentItem->setText( 0, QString( "[%1] %2" )
											  .arg( pInstrument->getId() )
											  .arg( pInstrument->getName() ) );
					pInstrumentItem->setToolTip( 0, pInstrument->getName() );
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
			__song_item->setExpanded( pPref->m_bExpandSongItem );
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
			__pattern_item->setExpanded( pPref->m_bExpandPatternItem );
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
					}
				}
			}
		}
	}
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
		auto pHydrogen = Hydrogen::get_instance();

		// Double clicking an instrument
		QString sSelectedName = item->text(0);

		QString sInstrumentName = sSelectedName.remove( 0, sSelectedName.indexOf( "] " ) + 2 );
		QString sDrumkitName = item->parent()->text(0);
		QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];

		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			sDrumkitPath );
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve kit [%1] for instrument [%2]" )
					  .arg( sDrumkitPath ).arg( sInstrumentName ) );
			return;
		}
		const auto pTargetInstrument = pDrumkit->getInstruments()->find( sInstrumentName );
		if ( pTargetInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument [%1] from kit [%2]" )
					  .arg( sInstrumentName ).arg( sDrumkitPath ) );
			return;
		}

		auto pInstrument = std::make_shared<Instrument>( pTargetInstrument );
		pInstrument->loadSamples(
			pHydrogen->getAudioEngine()->getTransportPosition()->getBpm() );

		INFOLOG( QString( "Loading instrument [%1] from drumkit [%2] located in [%3]" )
				 .arg( sInstrumentName ).arg( sDrumkitName ).arg( sDrumkitPath ) );

		if ( pInstrument == nullptr ) {
			ERRORLOG( "Unable to load instrument. Abort" );
			return;
		}
		
		pInstrument->setMuted( false );

		Hydrogen::get_instance()->getAudioEngine()->getSampler()->previewInstrument( pInstrument );
	}
}







void SoundLibraryPanel::on_DrumkitList_rightClicked( const QPoint& pos )
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
		const auto drumkitContext = Drumkit::DetermineContext( sDrumkitPath );
		
		if ( drumkitContext == Drumkit::Context::SessionReadOnly ) {
			__drumkit_menu_system->popup( pos );
		} else {
			__drumkit_menu->popup( pos );
		}
	}

}



void SoundLibraryPanel::on_DrumkitList_leftClicked( const QPoint& pos )
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
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ){
		return;
	}
	
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);
	QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	auto pDrumkit =
		pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to find drumkit [%1] (mapped to path [%2]" )
				  .arg( sDrumkitName ).arg( sDrumkitPath ) );
		return;
	}

	// Pass a copy of the kit since we do not want to alter the settings of the
	// original one.
	MainForm::switchDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
}

void SoundLibraryPanel::switchDrumkit( std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
									   std::shared_ptr<H2Core::Drumkit> pOldDrumkit ) {
	if ( pNewDrumkit == nullptr || pOldDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit provided" );
		return;
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	H2Core::CoreActionController::setDrumkit( pNewDrumkit );

	QApplication::restoreOverrideCursor();
}

QString SoundLibraryPanel::getDrumkitLabel( const QString& sDrumkitPath ) const {
	for ( const auto& [ssLabel, ssPath] : m_drumkitRegister ) {
		if ( ssPath == sDrumkitPath ) {
			return ssLabel;
		}
	}

	return "";
}
QString SoundLibraryPanel::getDrumkitPath( const QString& sDrumkitLabel ) const {
	return m_drumkitRegister.at( sDrumkitLabel );
}

void SoundLibraryPanel::on_drumkitDeleteAction()
{
	const auto pSong = Hydrogen::get_instance()->getSong();
	QTreeWidgetItem* pItem = __sound_library_tree->currentItem();
	const QString sDrumkitName = pItem->text(0);
	const QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	const auto drumkitContext = Drumkit::DetermineContext( sDrumkitPath );
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
		 ( pItem->parent() == m_pTreeSessionDrumkitsItem &&
		   drumkitContext == Drumkit::Context::SessionReadOnly ) ) {
		QMessageBox::warning( this, "Hydrogen", QString( "\"%1\" " )
							  .arg(sDrumkitName)
							  .append( tr( "is a read-only drumkit and can't be deleted.") ) );
		return;
	}

	// If we delete a kit containing samples used and loaded in the current
	// song's drumkit, we get into trouble.
	if ( pSong == nullptr ) {
		return;
	}
	auto pDrumkit = pSong->getDrumkit();
	if ( pDrumkit == nullptr ) {
		return;
	}

	// For a sample to be contained both the instrument's drumkit path must
	// match the selected one and the instrument has to contain at least one
	// sample with a non-empty, relative path.
	bool bSampleContained = false;
	for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
		if ( ppInstrument != nullptr &&
			 ppInstrument->getDrumkitPath() == sDrumkitPath ) {
			for ( const auto& ppComponent : *ppInstrument->getComponents() ) {
				if ( ppComponent != nullptr ) {
					for ( const auto& ppLayer : ppComponent->getLayers() ) {
						if ( ppLayer != nullptr &&
							 ppLayer->getSample() != nullptr &&
							 ! ppLayer->getSample()->getFilepath().isEmpty() &&
							 ppLayer->getSample()->getFilepath().contains(
								 sDrumkitPath ) ) {
							bSampleContained = true;
							break;
						}
					}
				}

				if ( bSampleContained ) {
					break;
				}
			}
		}

		if ( bSampleContained ) {
			break;
		}
	}
	if ( bSampleContained ) {
		QMessageBox::critical( this, "Hydrogen", tr( "It is not possible to delete drumkit: \n  [%1]\nIt contains samples used and loaded in the current song kit.")
							  .arg( sDrumkitName ) );
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

	const QString sDrumkitDir = m_drumkitRegister[ pItem->text(0) ];
	INFOLOG( QString( "Removing drumkit: %1" ).arg( sDrumkitDir ) );
	const bool bOk = Filesystem::rm( sDrumkitDir, true );

	QApplication::restoreOverrideCursor();

	if ( ! bOk ) {
		QMessageBox::warning( this, "Hydrogen", tr( "Drumkit deletion failed.") );
	} else {
		Hydrogen::get_instance()->getSoundLibraryDatabase()->updateDrumkits();
	}
}



void SoundLibraryPanel::on_drumkitExportAction()
{
	auto pSoundLibraryDatabase =
		Hydrogen::get_instance()->getSoundLibraryDatabase();
	
	QString sDrumkitName = __sound_library_tree->currentItem()->text(0);
	QString sDrumkitPath = m_drumkitRegister[ sDrumkitName ];
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sDrumkitPath );

	MainForm::exportDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
}

void SoundLibraryPanel::editDrumkitProperties( bool bDuplicate ) {
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
	if ( bDuplicate ) {
		// Suggest an unique drumkit name.
		pNewDrumkit->setName(
			Filesystem::appendNumberOrIncrement( sDrumkitName ) );
		pNewDrumkit->setPath(
			H2Core::Filesystem::usr_drumkits_dir() + pNewDrumkit->getName() );
	}

	DrumkitPropertiesDialog dialog( this, pNewDrumkit, ! bDuplicate, false );
	dialog.exec();
}

void SoundLibraryPanel::on_songLoadAction()
{
	const QString sFilename = Filesystem::song_path(
		__sound_library_tree->currentItem()->text( 0 ) );

	HydrogenApp::openFile( Filesystem::Type::Song, sFilename );
}



void SoundLibraryPanel::on_patternLoadAction() {

	QString sPatternName = __sound_library_tree->currentItem()->text( 0 );
	QString sDrumkitName = __sound_library_tree->currentItem()->toolTip( 0 );
	H2Core::CoreActionController::openPattern(
		Filesystem::pattern_path( sDrumkitName, sPatternName ) );
}


void SoundLibraryPanel::on_patternDeleteAction()
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	QString patternPath = __sound_library_tree->currentItem()->text( 1 );

	if ( QMessageBox::information(
			 this, "Hydrogen",
			 tr( "Warning, the selected pattern will be deleted from disk.\nAre you sure?"),
			 QMessageBox::Ok | QMessageBox::Cancel,
			 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
		return;
	}

	QFile rmfile( patternPath );
	bool err = rmfile.remove();
	if ( err == false ) {
		ERRORLOG( "Error removing the pattern" );
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

	auto pPref = Preferences::get_instance();

	if ( __song_item == nullptr) {
		pPref->m_bExpandSongItem = false;
	} else {
		pPref->m_bExpandSongItem = __song_item->isExpanded();
	}
	if ( __pattern_item == nullptr) {
		pPref->m_bExpandPatternItem = false;
	} else {
		pPref->m_bExpandPatternItem = __pattern_item->isExpanded();
	}
}

void SoundLibraryPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto theme = H2Core::Preferences::get_instance()->getTheme();
	
	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		QFont font( theme.m_font.m_sLevel2FontFamily, getPointSize( theme.m_font.m_fontSize ) );
		QFont boldFont( theme.m_font.m_sApplicationFontFamily, getPointSize( theme.m_font.m_fontSize ) );
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
