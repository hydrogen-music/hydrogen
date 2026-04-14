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
#include <memory>

#include "SoundLibraryTree.h"
#include "../Rack.h"
#include "../../CommonStrings.h"
#include "../../HydrogenApp.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
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
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

SoundLibraryPanel::SoundLibraryPanel(
	QWidget* pParent,
	std::shared_ptr<SoundLibraryInfo::Type> pOpenType
)
	: QWidget( pParent ),
	  m_pSearchField( nullptr ),
	  m_pRescanButton( nullptr ),
	  m_pTabWidget( nullptr ),
	  m_pDrumkitTree( nullptr ),
	  m_pPatternTree( nullptr ),
	  m_pSongTree( nullptr ),
	  m_pDetailName( nullptr ),
	  m_pDetailAuthor( nullptr ),
	  m_pDetailInfo( nullptr ),
	  m_pDetailLicense( nullptr ),
	  m_pDetailPath( nullptr ),
	  m_pTreeSystemDrumkitsItem( nullptr ),
	  m_pTreeUserDrumkitsItem( nullptr ),
	  m_pTreeSessionDrumkitsItem( nullptr ),
	  __song_item( nullptr ),
	  __pattern_item( nullptr ),
	  __pattern_item_list( nullptr ),
	  m_pOpenType( pOpenType )
{
	setMinimumWidth( Rack::nWidth );
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = Preferences::get_instance();

	// DRUMKIT TREE (tab 0)
	if ( m_pOpenType == nullptr ||
		 *m_pOpenType == SoundLibraryInfo::Type::Drumkit ) {
		m_pDrumkitTree = new SoundLibraryTree(
			this, SoundLibraryInfo::Type::Drumkit, m_pOpenType != nullptr
		);
		connect(
			m_pDrumkitTree, &SoundLibraryTree::itemChanged,
			[&]( bool bSelected ) { emit itemChanged( bSelected ); }
		);
		connect(
			m_pDrumkitTree, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ),
			this, SLOT( on_DrumkitList_itemActivated( QTreeWidgetItem*, int ) )
		);
	}

	// PATTERN TREE (tab 1)
	if ( m_pOpenType == nullptr ||
		 *m_pOpenType == SoundLibraryInfo::Type::Pattern ) {
		m_pPatternTree = new SoundLibraryTree(
			this, SoundLibraryInfo::Type::Pattern, m_pOpenType != nullptr
		);
		connect(
			m_pPatternTree, &SoundLibraryTree::itemChanged,
			[&]( bool bSelected ) { emit itemChanged( bSelected ); }
		);
	}

	// SONG TREE (tab 2)
	if ( m_pOpenType == nullptr ||
		 *m_pOpenType == SoundLibraryInfo::Type::Song ) {
		m_pSongTree = new SoundLibraryTree(
			this, SoundLibraryInfo::Type::Song, m_pOpenType != nullptr
		);
		connect(
			m_pSongTree, &SoundLibraryTree::itemChanged,
			[&]( bool bSelected ) { emit itemChanged( bSelected ); }
		);
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

	// Main layout
	QVBoxLayout* pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->addLayout( pSearchLayout );

	this->setLayout( pMainLayout );

	// Tree widgets
	if ( m_pOpenType == nullptr ) {
		m_pTabWidget = new QTabWidget( this );
		pMainLayout->addWidget( m_pTabWidget );
		m_pTabWidget->setDocumentMode( true );
		m_pTabWidget->addTab(
			m_pDrumkitTree, pCommonStrings->getDrumkitsLabel()
		);
		m_pTabWidget->addTab(
			m_pPatternTree, pCommonStrings->getPatternsLabel()
		);
		m_pTabWidget->addTab( m_pSongTree, pCommonStrings->getSongsLabel() );
		connect(
			m_pTabWidget, &QTabWidget::currentChanged, this,
			&SoundLibraryPanel::onTabChanged
		);
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Drumkit ) {
		pMainLayout->addWidget( m_pDrumkitTree );
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Pattern ) {
		pMainLayout->addWidget( m_pPatternTree );
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Song ) {
		pMainLayout->addWidget( m_pSongTree );
	}

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
	pMainLayout->addWidget( pDetailContainer );

	connect(
		m_pSearchField, &QLineEdit::textChanged, this,
		&SoundLibraryPanel::onSearchTextChanged
	);
	connect(
		m_pRescanButton, &QPushButton::clicked, this,
		&SoundLibraryPanel::onRescanClicked
	);

	connect(
		HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this,
		&SoundLibraryPanel::onPreferencesChanged
	);

	updateTree();

	HydrogenApp::get_instance()->addEventListener( this );
}

SoundLibraryPanel::~SoundLibraryPanel()
{
	if ( auto pH2App = HydrogenApp::get_instance() ) {
		pH2App->removeEventListener( this );
	}
}

void SoundLibraryPanel::updateTree()
{
	if ( m_pDrumkitTree != nullptr ) {
		m_pDrumkitTree->updateRegistry();
	}
	if ( m_pPatternTree != nullptr ) {
		m_pPatternTree->updateRegistry();
	}
	if ( m_pSongTree != nullptr ) {
		m_pSongTree->updateRegistry();
	}
}

void SoundLibraryPanel::updateDetailView()
{
	// Determine which tree is active
	SoundLibraryTree* pActiveTree = nullptr;
	if ( m_pTabWidget != nullptr ) {
		switch ( m_pTabWidget->currentIndex() ) {
			case 0:
				pActiveTree = m_pDrumkitTree;
				break;
			case 1:
				pActiveTree = m_pPatternTree;
				break;
			case 2:
				pActiveTree = m_pSongTree;
				break;
			default:
				ERRORLOG( "Invalid tab" );
				return;
		}
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Drumkit ) {
		pActiveTree = m_pDrumkitTree;
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Pattern ) {
		pActiveTree = m_pPatternTree;
	}
	else {
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

	if ( pActiveTree == m_pDrumkitTree ) {
		// Drumkit tab: look up the selected drumkit
		if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
			 pItem->parent() == m_pTreeUserDrumkitsItem ||
			 pItem->parent() == m_pTreeSessionDrumkitsItem ) {
			auto it = m_pDrumkitTree->getRegistry().find( pItem );
			if ( it != m_pDrumkitTree->getRegistry().end() && it->second != nullptr ) {
				auto pInfo = it->second;
				m_pDetailName->setText( pInfo->getName() );
				m_pDetailAuthor->setText( pInfo->getAuthor() );
				m_pDetailInfo->setText( pInfo->getInfo() );
				m_pDetailLicense->setText(
					pInfo->getLicense().toQString( "", true )
				);
				m_pDetailPath->setText( pInfo->getPath() );
			}
		}
	}
	else if ( pActiveTree == m_pPatternTree || pActiveTree == m_pSongTree ) {
		auto it = pActiveTree->getRegistry().find( pItem );
		if ( it != pActiveTree->getRegistry().end() && it->second != nullptr ) {
			auto pInfo = it->second;
			m_pDetailName->setText( pInfo->getName() );
			m_pDetailAuthor->setText( pInfo->getAuthor() );
			m_pDetailInfo->setText( pInfo->getInfo() );
			m_pDetailLicense->setText( pInfo->getLicense().toQString( "", true )
			);
			m_pDetailPath->setText( pInfo->getPath() );
		}
    }
}

void SoundLibraryPanel::filterTree(
	SoundLibraryTree* pTree,
	const QString& sFilter
)
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
					bool bMatch =
						sFilter.isEmpty() || pLeaf->text( 0 ).contains(
												 sFilter, Qt::CaseInsensitive
											 );
					pLeaf->setHidden( !bMatch );
					if ( bMatch ) {
						bAnyCatChildVisible = true;
					}
				}
				pChild->setHidden( !bAnyCatChildVisible );
				if ( bAnyCatChildVisible ) {
					bAnyChildVisible = true;
				}
			}
			else {
				bool bMatch =
					sFilter.isEmpty() ||
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

void SoundLibraryPanel::onSearchTextChanged( const QString& sText )
{
	filterTree( m_pDrumkitTree, sText );
	filterTree( m_pPatternTree, sText );
	filterTree( m_pSongTree, sText );
}

void SoundLibraryPanel::onRescanClicked()
{
	H2Core::Hydrogen::get_instance()->getSoundLibraryDatabase()->update();
}

void SoundLibraryPanel::on_DrumkitList_itemActivated(
	QTreeWidgetItem* pItem,
	int column
)
{
	UNUSED( column );

	//	INFOLOG( "[on_DrumkitList_itemActivated]" );
	if ( pItem == m_pTreeSystemDrumkitsItem || pItem == m_pTreeUserDrumkitsItem ||
		 pItem == m_pTreeSessionDrumkitsItem ||
		 ( ( m_pTreeSystemDrumkitsItem != nullptr &&
			 pItem == m_pTreeSystemDrumkitsItem->parent() ) ||
		   ( m_pTreeUserDrumkitsItem != nullptr &&
			 pItem == m_pTreeUserDrumkitsItem->parent() ) ||
		   ( m_pTreeSessionDrumkitsItem != nullptr &&
			 pItem == m_pTreeSessionDrumkitsItem->parent() ) ) ||
		 pItem->parent() == __song_item || pItem == __song_item ||
		 pItem == __pattern_item || pItem->parent() == __pattern_item ||
		 pItem->parent()->parent() == __pattern_item ||
		 pItem == __pattern_item_list || pItem->parent() == __pattern_item_list ||
		 pItem->parent()->parent() == __pattern_item_list ) {
		return;
	}

	if ( pItem->parent() == m_pTreeSystemDrumkitsItem ||
		 pItem->parent() == m_pTreeUserDrumkitsItem ||
		 pItem->parent() == m_pTreeSessionDrumkitsItem ) {
		// Double clicking a drumkit
	}
	else {
		auto pHydrogen = Hydrogen::get_instance();

		// Double clicking an instrument
		QString sSelectedName = pItem->text( 0 );

		QString sInstrumentName =
			sSelectedName.remove( 0, sSelectedName.indexOf( "] " ) + 2 );
		QString sDrumkitName = pItem->parent()->text( 0 );
		auto it = m_pDrumkitTree->getRegistry().find( pItem );
		if ( it == m_pDrumkitTree->getRegistry().end() || it->second == nullptr ) {
			ERRORLOG( "Unable to retrieve drumkit" );
			return;
		}
		const QString sDrumkitPath = it->second->getPath();

		auto pDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve kit [%1] for instrument [%2]"
			)
						  .arg( sDrumkitPath )
						  .arg( sInstrumentName ) );
			return;
		}
		const auto pTargetInstrument =
			pDrumkit->getInstruments()->find( sInstrumentName );
		if ( pTargetInstrument == nullptr ) {
			ERRORLOG(
				QString( "Unable to retrieve instrument [%1] from kit [%2]" )
					.arg( sInstrumentName )
					.arg( sDrumkitPath )
			);
			return;
		}

		auto pPreviewInstrument =
			std::make_shared<Instrument>( pTargetInstrument );
		pPreviewInstrument->loadSamples(
			pHydrogen->getAudioEngine()->getPlayhead()->getBpm()
		);

		INFOLOG( QString(
					 "Loading instrument [%1] from drumkit [%2] located in [%3]"
		)
					 .arg( sInstrumentName )
					 .arg( sDrumkitName )
					 .arg( sDrumkitPath ) );

		if ( pPreviewInstrument == nullptr ) {
			ERRORLOG( "Unable to load instrument. Abort" );
			return;
		}

		pPreviewInstrument->setMuted( false );
		auto pNote = std::make_shared<Note>(
			pPreviewInstrument, 0, VELOCITY_MAX, PAN_DEFAULT,
			LENGTH_ENTIRE_SAMPLE
		);

		pHydrogen->getAudioEngine()->getSampler()->previewInstrument(
			pPreviewInstrument, pNote
		);
	}
}

void SoundLibraryPanel::switchDrumkit(
	std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
	std::shared_ptr<H2Core::Drumkit> pOldDrumkit
)
{
	if ( pNewDrumkit == nullptr || pOldDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit provided" );
		return;
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	H2Core::CoreActionController::setDrumkit( pNewDrumkit );

	QApplication::restoreOverrideCursor();
}

SoundLibraryTree* SoundLibraryPanel::getCurrentTree()
{
	if ( m_pOpenType == nullptr ) {
		switch ( m_pTabWidget->currentIndex() ) {
			case 0:
				return m_pDrumkitTree;
			case 1:
				return m_pPatternTree;
			default:
				return m_pSongTree;
		}
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Drumkit ) {
		return m_pDrumkitTree;
	}
	else if ( *m_pOpenType == SoundLibraryInfo::Type::Pattern ) {
		return m_pPatternTree;
	}
	else {
		return m_pSongTree;
	}
}

void SoundLibraryPanel::soundLibraryChangedEvent()
{
	updateTree();
}

void SoundLibraryPanel::updateSongEvent( int nValue )
{
	if ( nValue == 1 ) {
		// A song was saved.
		updateTree();
	}
}

void SoundLibraryPanel::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	if ( changes & H2Core::Preferences::Changes::Font ) {
		if ( m_pDrumkitTree != nullptr ) {
			m_pDrumkitTree->updateFont();
		}
		if ( m_pPatternTree != nullptr ) {
			m_pPatternTree->updateFont();
		}
		if ( m_pSongTree != nullptr ) {
			m_pSongTree->updateFont();
		}
	}
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		// Not the most efficient way to icon update. But this operation is most
		// probably done very rarely. So, it should be fine.
		if ( m_pDrumkitTree != nullptr ) {
			m_pDrumkitTree->updateRegistry();
		}
		if ( m_pPatternTree != nullptr ) {
			m_pPatternTree->updateRegistry();
		}
		if ( m_pSongTree != nullptr ) {
			m_pSongTree->updateRegistry();
		}
	}
}
