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

#include "DrumkitPropertiesDialog.h"
#include "SoundLibraryTree.h"
#include "../Rack.h"
#include "../../CommonStrings.h"
#include "../../HydrogenApp.h"
#include "../../MainForm.h"
#include "../../UndoActions.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
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
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

#include <cassert>

SoundLibraryPanel::SoundLibraryPanel( QWidget *pParent, bool bInItsOwnDialog )
 : QWidget( pParent )
 , m_pSearchField( nullptr )
 , m_pRescanButton( nullptr )
 , m_pTabWidget( nullptr )
 , __sound_library_tree( nullptr )
 , m_pPatternTree( nullptr )
 , m_pSongTree( nullptr )
 , m_pDetailName( nullptr )
 , m_pDetailAuthor( nullptr )
 , m_pDetailInfo( nullptr )
 , m_pDetailLicense( nullptr )
 , m_pDetailPath( nullptr )
 , __drumkit_menu( nullptr )
 , __drumkit_menu_system( nullptr )
 , __song_menu( nullptr )
 , __pattern_menu( nullptr )
 , __pattern_menu_list( nullptr )
 , m_pTreeSystemDrumkitsItem( nullptr )
 , m_pTreeUserDrumkitsItem( nullptr )
 , m_pTreeSessionDrumkitsItem( nullptr )
 , m_pPatternSystemItem( nullptr )
 , m_pPatternUserItem( nullptr )
 , m_pSongSystemItem( nullptr )
 , m_pSongUserItem( nullptr )
 , __song_item( nullptr )
 , __pattern_item( nullptr )
 , __pattern_item_list( nullptr )
 , m_bInItsOwnDialog( bInItsOwnDialog )
{
	setMinimumWidth( Rack::nWidth );
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

	// DRUMKIT TREE (tab 0)
	__sound_library_tree = new SoundLibraryTree( nullptr );
	connect( __sound_library_tree, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( on_DrumkitList_ItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
	connect( __sound_library_tree, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( on_DrumkitList_itemActivated( QTreeWidgetItem*, int ) ) );
	connect( __sound_library_tree, SIGNAL( leftClicked(QPoint) ), this, SLOT( on_DrumkitList_leftClicked(QPoint)) );
	connect( __sound_library_tree, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( onTreeItemSelected() ) );
	if( ! m_bInItsOwnDialog ) {
		connect( __sound_library_tree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_DrumkitList_rightClicked(QPoint)) );
		connect( __sound_library_tree, SIGNAL( onMouseMove( QMouseEvent* ) ), this, SLOT( on_DrumkitList_mouseMove( QMouseEvent* ) ) );
	}

	// PATTERN TREE (tab 1)
	m_pPatternTree = new SoundLibraryTree( nullptr );
	connect( m_pPatternTree, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( onTreeItemSelected() ) );
	if ( ! m_bInItsOwnDialog ) {
		connect( m_pPatternTree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_PatternTree_rightClicked(QPoint)) );
		connect( m_pPatternTree, SIGNAL( onMouseMove( QMouseEvent* ) ), this, SLOT( on_PatternTree_mouseMove( QMouseEvent* ) ) );
	}

	// SONG TREE (tab 2)
	m_pSongTree = new SoundLibraryTree( nullptr );
	connect( m_pSongTree, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), this, SLOT( onTreeItemSelected() ) );
	if ( ! m_bInItsOwnDialog ) {
		connect( m_pSongTree, SIGNAL( rightClicked(QPoint) ), this, SLOT( on_SongTree_rightClicked(QPoint)) );
	}

	// Search bar
	m_pSearchField = new QLineEdit( this );
	m_pSearchField->setPlaceholderText( "Search..." );

	m_pRescanButton = new QPushButton( "Rescan", this );

	QHBoxLayout* pSearchLayout = new QHBoxLayout();
	pSearchLayout->setSpacing( 4 );
	pSearchLayout->setContentsMargins( 0, 0, 0, 0 );
	pSearchLayout->addWidget( m_pSearchField );
	pSearchLayout->addWidget( m_pRescanButton );

	// Tab widget
	m_pTabWidget = new QTabWidget( this );
	m_pTabWidget->setDocumentMode( true );
	m_pTabWidget->addTab(
		__sound_library_tree, pCommonStrings->getDrumkitsLabel()
	);
	m_pTabWidget->addTab( m_pPatternTree, pCommonStrings->getPatternsLabel() );
	m_pTabWidget->addTab( m_pSongTree, pCommonStrings->getSongsLabel() );

	// Detail view
	m_pDetailName = new QLabel( this );
	m_pDetailName->setWordWrap( true );
	m_pDetailAuthor = new QLabel( this );
	m_pDetailAuthor->setWordWrap( true );
	m_pDetailInfo = new QLabel( this );
	m_pDetailInfo->setWordWrap( true );
	m_pDetailLicense = new QLabel( this );
	m_pDetailLicense->setWordWrap( true );
	m_pDetailPath = new QLabel( this );
	m_pDetailPath->setWordWrap( true );

	QFormLayout* pFormLayout = new QFormLayout();
	pFormLayout->addRow( pCommonStrings->getNameDialog(), m_pDetailName );
	pFormLayout->addRow( pCommonStrings->getAuthorDialog(), m_pDetailAuthor );
	pFormLayout->addRow( pCommonStrings->getNotesDialog(), m_pDetailInfo );
	pFormLayout->addRow( pCommonStrings->getLicenseDialog(), m_pDetailLicense );
	pFormLayout->addRow( "Path:", m_pDetailPath );

	QWidget* pDetailContainer = new QWidget( this );
	pDetailContainer->setLayout( pFormLayout );

	// Main layout
	QVBoxLayout* pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setContentsMargins( 0, 0, 0, 0 );
	pVBox->addLayout( pSearchLayout );
	pVBox->addWidget( m_pTabWidget );
	pVBox->addWidget( pDetailContainer );

	this->setLayout( pVBox );

	connect( m_pSearchField, &QLineEdit::textChanged, this, &SoundLibraryPanel::onSearchTextChanged );
	connect( m_pRescanButton, &QPushButton::clicked, this, &SoundLibraryPanel::onRescanClicked );
	connect( m_pTabWidget, &QTabWidget::currentChanged, this, &SoundLibraryPanel::onTabChanged );

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
	updateDrumkitTree();
	updatePatternTree();
	updateSongTree();
}



void SoundLibraryPanel::updateDrumkitTree()
{
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pFontTheme = pPref->getFontTheme();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	__sound_library_tree->clear();
	m_drumkitRegister.clear();
	m_drumkitLabels.clear();
	m_patternRegistry.clear();

	// Legacy pointers are not populated in the new design
	__song_item = nullptr;
	__pattern_item = nullptr;
	__pattern_item_list = nullptr;

	QFont boldFont( pFontTheme->m_sApplicationFontFamily,
				   getPointSize( pFontTheme->m_fontSize ) );
	boldFont.setBold( true );

	QFont childFont( pFontTheme->m_sLevel2FontFamily,
					getPointSize( pFontTheme->m_fontSize ) );
	setFont( childFont );

	m_pTreeSystemDrumkitsItem = nullptr;
	m_pTreeUserDrumkitsItem = nullptr;
	m_pTreeSessionDrumkitsItem = nullptr;

	// top-level drumkit items found
	QList<QTreeWidgetItem*> drumkitItems;

	// drumkit list
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
		if ( drumkitContext == Filesystem::Context::System ) {
			if ( m_pTreeSystemDrumkitsItem == nullptr ) {
				m_pTreeSystemDrumkitsItem = new QTreeWidgetItem();
				m_pTreeSystemDrumkitsItem->setText(
					0, pCommonStrings->getSoundLibrarySystem()
				);
				m_pTreeSystemDrumkitsItem->setFont( 0, boldFont );
			}

			pDrumkitItem = new QTreeWidgetItem( m_pTreeSystemDrumkitsItem );
		}
		else if ( drumkitContext == Filesystem::Context::User ) {
			if ( m_pTreeUserDrumkitsItem == nullptr ) {
				m_pTreeUserDrumkitsItem = new QTreeWidgetItem();
				m_pTreeUserDrumkitsItem->setText(
					0, pCommonStrings->getSoundLibraryUser()
				);
				m_pTreeUserDrumkitsItem->setFont( 0, boldFont );
			}

			pDrumkitItem = new QTreeWidgetItem( m_pTreeUserDrumkitsItem );
		}
		else if ( drumkitContext == Filesystem::Context::SessionReadOnly || drumkitContext == Filesystem::Context::SessionReadWrite ) {
			if ( m_pTreeSessionDrumkitsItem == nullptr ) {
				m_pTreeSessionDrumkitsItem = new QTreeWidgetItem();
				m_pTreeSessionDrumkitsItem->setText(
					0, pCommonStrings->getSoundLibrarySession()
				);
				m_pTreeSessionDrumkitsItem->setFont( 0, boldFont );
			}
			pDrumkitItem = new QTreeWidgetItem( m_pTreeSessionDrumkitsItem );
		}
		else {
			ERRORLOG( QString( "Drumkits of context [%1] should not end up in "
							   "the SoundLibrary." )
						  .arg( Filesystem::ContextToQString( drumkitContext ) )
			);
			continue;
		}

		m_drumkitLabels << sItemLabel;
		m_drumkitRegister[ sItemLabel ] = ssPath;

		pDrumkitItem->setText( 0, sItemLabel );
		pDrumkitItem->setToolTip( 0, ssPath );
		if ( !m_bInItsOwnDialog ) {
			auto pInstrList = ppDrumkit->getInstruments();
			for ( const auto& pInstrument : *ppDrumkit->getInstruments() ) {
				if ( pInstrument != nullptr ) {
					QTreeWidgetItem* pInstrumentItem =
						new QTreeWidgetItem( pDrumkitItem );
					pInstrumentItem->setText(
						0, QString( "[%1] %2" )
							   .arg( static_cast<int>( pInstrument->getId() ) )
							   .arg( pInstrument->getName() )
					);
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
}



void SoundLibraryPanel::updatePatternTree()
{
	m_pPatternTree->clear();
	m_patternRegistry.clear();
	m_pPatternSystemItem = nullptr;
	m_pPatternUserItem = nullptr;

	auto pPref = H2Core::Preferences::get_instance();
	auto pFontTheme = pPref->getFontTheme();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QFont boldFont( pFontTheme->m_sApplicationFontFamily,
				   getPointSize( pFontTheme->m_fontSize ) );
	boldFont.setBold( true );

	auto patternInfoVector = pSoundLibraryDatabase->getPatternInfos();

	// Separate patterns by context
	for ( const auto& pInfo : patternInfoVector ) {
		QTreeWidgetItem* pParentItem = nullptr;

		if ( pInfo->getContext() == H2Core::Filesystem::Context::System ) {
			if ( m_pPatternSystemItem == nullptr ) {
				m_pPatternSystemItem = new QTreeWidgetItem( m_pPatternTree );
				m_pPatternSystemItem->setText(
					0, pCommonStrings->getSoundLibrarySystem()
				);
				m_pPatternSystemItem->setFont( 0, boldFont );
				m_pPatternSystemItem->setExpanded( true );
			}
			pParentItem = m_pPatternSystemItem;
		}
		else {
			if ( m_pPatternUserItem == nullptr ) {
				m_pPatternUserItem = new QTreeWidgetItem( m_pPatternTree );
				m_pPatternUserItem->setText(
					0, pCommonStrings->getSoundLibraryUser()
				);
				m_pPatternUserItem->setFont( 0, boldFont );
				m_pPatternUserItem->setExpanded( true );
			}
			pParentItem = m_pPatternUserItem;
		}

		QTreeWidgetItem* pPatternItem = new QTreeWidgetItem( pParentItem );
		pPatternItem->setText( 0, pInfo->getName() );
		pPatternItem->setText( 1, pInfo->getPath() );
		m_patternRegistry[ pPatternItem ] = pInfo;
	}
}



void SoundLibraryPanel::updateSongTree()
{
	m_pSongTree->clear();
	m_songRegistry.clear();
	m_pSongSystemItem = nullptr;
	m_pSongUserItem = nullptr;

	auto pPref = H2Core::Preferences::get_instance();
	auto pFontTheme = pPref->getFontTheme();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QFont boldFont( pFontTheme->m_sApplicationFontFamily,
				   getPointSize( pFontTheme->m_fontSize ) );
	boldFont.setBold( true );

	auto songInfoVector = pSoundLibraryDatabase->getSongInfos();

	for ( const auto& pInfo : songInfoVector ) {
		QTreeWidgetItem* pParentItem = nullptr;

		if ( pInfo->getContext() == H2Core::Filesystem::Context::System ) {
			if ( m_pSongSystemItem == nullptr ) {
				m_pSongSystemItem = new QTreeWidgetItem( m_pSongTree );
				m_pSongSystemItem->setText(
					0, pCommonStrings->getSoundLibrarySystem()
				);
				m_pSongSystemItem->setFont( 0, boldFont );
				m_pSongSystemItem->setExpanded( true );
			}
			pParentItem = m_pSongSystemItem;
		}
		else {
			if ( m_pSongUserItem == nullptr ) {
				m_pSongUserItem = new QTreeWidgetItem( m_pSongTree );
				m_pSongUserItem->setText(
					0, pCommonStrings->getSoundLibraryUser()
				);
				m_pSongUserItem->setFont( 0, boldFont );
				m_pSongUserItem->setExpanded( true );
			}
			pParentItem = m_pSongUserItem;
		}

		QTreeWidgetItem* pSongItem = new QTreeWidgetItem( pParentItem );
		QString sDisplayName = pInfo->getName();
		if ( sDisplayName.isEmpty() ) {
			// Fallback to filename without extension
			QFileInfo fi( pInfo->getPath() );
			sDisplayName = fi.completeBaseName();
		}
		pSongItem->setText( 0, sDisplayName );
		pSongItem->setToolTip( 0, pInfo->getPath() );
		m_songRegistry[ pSongItem ] = pInfo;
	}
}



void SoundLibraryPanel::updateDetailView()
{
	// Determine which tree is active
	int nTab = m_pTabWidget->currentIndex();
	SoundLibraryTree* pActiveTree = nullptr;

	if ( nTab == 0 ) {
		pActiveTree = __sound_library_tree;
	} else if ( nTab == 1 ) {
		pActiveTree = m_pPatternTree;
	} else if ( nTab == 2 ) {
		pActiveTree = m_pSongTree;
	}

	// Clear all fields
	m_pDetailName->clear();
	m_pDetailAuthor->clear();
	m_pDetailInfo->clear();
	m_pDetailLicense->clear();
	m_pDetailPath->clear();

	if ( pActiveTree == nullptr || pActiveTree->currentItem() == nullptr ) {
		return;
	}

	QTreeWidgetItem* pItem = pActiveTree->currentItem();

	if ( nTab == 0 ) {
		// Drumkit tab: look up the selected drumkit
		if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
			 pItem->parent() == m_pTreeUserDrumkitsItem ||
			 pItem->parent() == m_pTreeSessionDrumkitsItem ) {
			QString sDrumkitLabel = pItem->text( 0 );
			auto it = m_drumkitRegister.find( sDrumkitLabel );
			if ( it != m_drumkitRegister.end() ) {
				auto pDB = H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase();
				auto pDrumkit = pDB->getDrumkit( it->second );
				if ( pDrumkit != nullptr ) {
					m_pDetailName->setText( pDrumkit->getName() );
					m_pDetailAuthor->setText( pDrumkit->getAuthor() );
					m_pDetailInfo->setText( pDrumkit->getInfo() );
					m_pDetailLicense->setText( pDrumkit->getLicense().toQString( "", true ) );
					m_pDetailPath->setText( it->second );
				}
			}
		}
	} else if ( nTab == 1 ) {
		// Pattern tab
		auto it = m_patternRegistry.find( pItem );
		if ( it != m_patternRegistry.end() && it->second != nullptr ) {
			auto pInfo = it->second;
			m_pDetailName->setText( pInfo->getName() );
			m_pDetailAuthor->setText( pInfo->getAuthor() );
			m_pDetailInfo->setText( pInfo->getInfo() );
			m_pDetailLicense->setText( pInfo->getLicense().toQString( "", true ) );
			m_pDetailPath->setText( pInfo->getPath() );
		}
	} else if ( nTab == 2 ) {
		// Song tab
		auto it = m_songRegistry.find( pItem );
		if ( it != m_songRegistry.end() && it->second != nullptr ) {
			auto pInfo = it->second;
			m_pDetailName->setText( pInfo->getName() );
			m_pDetailAuthor->setText( pInfo->getAuthor() );
			m_pDetailInfo->setText( pInfo->getInfo() );
			m_pDetailLicense->setText( pInfo->getLicense().toQString( "", true ) );
			m_pDetailPath->setText( pInfo->getPath() );
		}
	}
}



void SoundLibraryPanel::filterTree( SoundLibraryTree* pTree, const QString& sFilter )
{
	if ( pTree == nullptr ) {
		return;
	}

	for ( int ii = 0; ii < pTree->topLevelItemCount(); ++ii ) {
		QTreeWidgetItem* pTopItem = pTree->topLevelItem( ii );
		bool bAnyChildVisible = false;

		for ( int jj = 0; jj < pTopItem->childCount(); ++jj ) {
			QTreeWidgetItem* pChild = pTopItem->child( jj );

			// Check if pChild itself has children
			if ( pChild->childCount() > 0 ) {
				bool bAnyCatChildVisible = false;
				for ( int kk = 0; kk < pChild->childCount(); ++kk ) {
					QTreeWidgetItem* pLeaf = pChild->child( kk );
					bool bMatch = sFilter.isEmpty() ||
						pLeaf->text( 0 ).contains( sFilter, Qt::CaseInsensitive );
					pLeaf->setHidden( !bMatch );
					if ( bMatch ) {
						bAnyCatChildVisible = true;
					}
				}
				pChild->setHidden( !bAnyCatChildVisible );
				if ( bAnyCatChildVisible ) {
					bAnyChildVisible = true;
				}
			} else {
				bool bMatch = sFilter.isEmpty() ||
					pChild->text( 0 ).contains( sFilter, Qt::CaseInsensitive );
				pChild->setHidden( !bMatch );
				if ( bMatch ) {
					bAnyChildVisible = true;
				}
			}
		}
		pTopItem->setHidden( !bAnyChildVisible );
	}
}



void SoundLibraryPanel::onTabChanged( int nIndex )
{
	UNUSED( nIndex );
	updateDetailView();
}

void SoundLibraryPanel::onTreeItemSelected()
{
	updateDetailView();
}

void SoundLibraryPanel::onSearchTextChanged( const QString& sText )
{
	filterTree( __sound_library_tree, sText );
	filterTree( m_pPatternTree, sText );
	filterTree( m_pSongTree, sText );
}

void SoundLibraryPanel::onRescanClicked()
{
	H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase()->update();
}



void SoundLibraryPanel::on_PatternTree_rightClicked( const QPoint& pos )
{
	if ( m_pPatternTree->currentItem() == nullptr ) {
		return;
	}

	// Only show menu on leaf items (patterns, not folders)
	auto it = m_patternRegistry.find( m_pPatternTree->currentItem() );
	if ( it != m_patternRegistry.end() ) {
		__pattern_menu->popup( pos );
	}
}

void SoundLibraryPanel::on_PatternTree_mouseMove( QMouseEvent* event )
{
	if ( !( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	if ( m_pPatternTree->currentItem() == nullptr ) {
		return;
	}

	auto it = m_patternRegistry.find( m_pPatternTree->currentItem() );
	if ( it == m_patternRegistry.end() || it->second == nullptr ) {
		return;
	}

	QString sPatternPath = it->second->getPath();
	QString dragtype = "drag pattern";
	QString sText = dragtype + "::" + sPatternPath;

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;
	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData );
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
}

void SoundLibraryPanel::on_SongTree_rightClicked( const QPoint& pos )
{
	if ( m_pSongTree->currentItem() == nullptr ) {
		return;
	}

	auto it = m_songRegistry.find( m_pSongTree->currentItem() );
	if ( it != m_songRegistry.end() ) {
		__song_menu->popup( pos );
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

		auto pPreviewInstrument = std::make_shared<Instrument>( pTargetInstrument );
		pPreviewInstrument->loadSamples(
			pHydrogen->getAudioEngine()->getPlayhead()->getBpm() );

		INFOLOG( QString( "Loading instrument [%1] from drumkit [%2] located in [%3]" )
				 .arg( sInstrumentName ).arg( sDrumkitName ).arg( sDrumkitPath ) );

		if ( pPreviewInstrument == nullptr ) {
			ERRORLOG( "Unable to load instrument. Abort" );
			return;
		}

		pPreviewInstrument->setMuted( false );
		auto pNote = std::make_shared<Note>(
			pPreviewInstrument, 0, VELOCITY_MAX, PAN_DEFAULT, LENGTH_ENTIRE_SAMPLE );

		pHydrogen->getAudioEngine()->getSampler()->previewInstrument(
			pPreviewInstrument, pNote );
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
		const auto drumkitContext = Filesystem::DetermineContext( sDrumkitPath );

		if ( drumkitContext == Filesystem::Context::SessionReadOnly ) {
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
		return;
	}
	else {
		// instrument selection
		if ( __sound_library_tree->currentItem() == nullptr )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent() == nullptr )
		{
			return;
		}

		if ( __sound_library_tree->currentItem()->parent()->text(0) == nullptr )
		{
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
	const auto drumkitContext = Filesystem::DetermineContext( sDrumkitPath );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
		 ( pItem->parent() == m_pTreeSessionDrumkitsItem &&
		   drumkitContext == Filesystem::Context::SessionReadOnly ) ) {
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
							 ! ppLayer->getSample()->getFilePath().isEmpty() &&
							 ppLayer->getSample()->getFilePath().contains(
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
	// Support loading from the new song tree
	if ( m_pSongTree != nullptr && m_pSongTree->currentItem() != nullptr ) {
		auto it = m_songRegistry.find( m_pSongTree->currentItem() );
		if ( it != m_songRegistry.end() && it->second != nullptr ) {
			HydrogenApp::openFile( Filesystem::Type::Song, it->second->getPath() );
			return;
		}
	}

	// Fallback: legacy path (shouldn't be reached in new design)
	if ( __sound_library_tree->currentItem() != nullptr ) {
		const QString sFileName = Filesystem::song_path(
			__sound_library_tree->currentItem()->text( 0 ) );
		HydrogenApp::openFile( Filesystem::Type::Song, sFileName );
	}
}

void SoundLibraryPanel::on_patternLoadAction()
{
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	// Check pattern tree first, then fall back to the drumkit tree
	QTreeWidgetItem* pCurrentItem = nullptr;
	if ( m_pPatternTree != nullptr && m_pPatternTree->currentItem() != nullptr ) {
		pCurrentItem = m_pPatternTree->currentItem();
	} else if ( __sound_library_tree->currentItem() != nullptr ) {
		pCurrentItem = __sound_library_tree->currentItem();
	}

	if ( pCurrentItem == nullptr ) {
		return;
	}

	if ( m_patternRegistry.find( pCurrentItem ) == m_patternRegistry.end() ) {
		ERRORLOG( QString( "Unable to find pattern corresponding to [%1]" )
					  .arg( pCurrentItem->text( 0 ) ) );
		return;
	}

	auto pInfo = m_patternRegistry.at( pCurrentItem );
	if ( pInfo == nullptr ) {
		ERRORLOG( QString( "Invalid pattern info for [%1]" )
					  .arg( pCurrentItem->text( 0 ) ) );
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPattern =
		H2Core::CoreActionController::loadPattern( pInfo->getPath() );
	if ( pPattern == nullptr ) {
		QMessageBox::critical(
			this, "Hydrogen", pCommonStrings->getPatternLoadError()
		);
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand( new SE_insertPatternAction(
		SE_insertPatternAction::Type::Insert, pSong->getPatternList()->size(),
		pPattern, nullptr
	) );
}

void SoundLibraryPanel::on_patternDeleteAction() {
	QTreeWidgetItem* pCurrentItem = nullptr;
	if ( m_pPatternTree != nullptr && m_pPatternTree->currentItem() != nullptr ) {
		pCurrentItem = m_pPatternTree->currentItem();
	} else if ( __sound_library_tree->currentItem() != nullptr ) {
		pCurrentItem = __sound_library_tree->currentItem();
	}

	if ( pCurrentItem == nullptr ) {
		return;
	}

	if ( m_patternRegistry.find( pCurrentItem ) == m_patternRegistry.end() ) {
		ERRORLOG( QString( "Unable to find pattern corresponding to [%1]" )
				  .arg( pCurrentItem->text( 0 ) ) );
		return;
	}

	auto pInfo = m_patternRegistry.at( pCurrentItem );
	if ( pInfo == nullptr ) {
		ERRORLOG( QString( "Invalid pattern info for [%1]" )
				  .arg( pCurrentItem->text( 0 ) ) );
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

void SoundLibraryPanel::updateSongEvent( int nValue ) {
	if ( nValue == 1 ) {
		// A song was saved.
		test_expandedItems();
		updateTree();
	}
}

void SoundLibraryPanel::test_expandedItems()
{
	assert( __sound_library_tree );

	// __song_item and __pattern_item are always nullptr in the new design;
	// the preference booleans are left unchanged.
}

void SoundLibraryPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	if ( changes & H2Core::Preferences::Changes::Font ) {

		QFont font( pFontTheme->m_sLevel2FontFamily,
				   getPointSize( pFontTheme->m_fontSize ) );
		QFont boldFont( pFontTheme->m_sApplicationFontFamily,
					   getPointSize( pFontTheme->m_fontSize ) );
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

		// Pattern tree fonts
		if ( m_pPatternTree != nullptr ) {
			for ( int ii = 0; ii < m_pPatternTree->topLevelItemCount(); ++ii ) {
				QTreeWidgetItem* pTop = m_pPatternTree->topLevelItem( ii );
				pTop->setFont( 0, boldFont );
				for ( int jj = 0; jj < pTop->childCount(); ++jj ) {
					QTreeWidgetItem* pChild = pTop->child( jj );
					pChild->setFont( 0, font );
					for ( int kk = 0; kk < pChild->childCount(); ++kk ) {
						pChild->child( kk )->setFont( 0, font );
					}
				}
			}
		}

		// Song tree fonts
		if ( m_pSongTree != nullptr ) {
			for ( int ii = 0; ii < m_pSongTree->topLevelItemCount(); ++ii ) {
				QTreeWidgetItem* pTop = m_pSongTree->topLevelItem( ii );
				pTop->setFont( 0, boldFont );
				for ( int jj = 0; jj < pTop->childCount(); ++jj ) {
					pTop->child( jj )->setFont( 0, font );
				}
			}
		}
	}
}
