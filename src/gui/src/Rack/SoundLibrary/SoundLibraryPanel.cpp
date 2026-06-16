/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "InfoView.h"
#include "SoundLibraryTree.h"
#include "../Rack.h"
#include "../../CommonStrings.h"
#include "../../HydrogenApp.h"
#include "core/SoundLibrary/SoundLibraryInfo.h"

#include <core/CoreActionController.h>
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
	  m_pOpenType( pOpenType )
{
	setMinimumWidth( Rack::nWidth );
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pPref = HydrogenApp::pPreferences();

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
	m_pSearchField->setFixedHeight( SoundLibraryPanel::nHeaderHeight - 2 );
	m_pSearchField->setPlaceholderText( pCommonStrings->getSearchPlaceholder() );

	m_pRescanButton = new QToolButton( this );
	m_pRescanButton->setFixedHeight( SoundLibraryPanel::nHeaderHeight - 2 );

	auto pSearchWidget = new QWidget( this );
	pSearchWidget->setObjectName( "SearchWidget" );
	auto pSearchLayout = new QHBoxLayout();
	pSearchLayout->setSpacing( 0 );
	pSearchLayout->setContentsMargins( 1, 1, 1, 1 );
	pSearchLayout->addWidget( m_pSearchField );
	pSearchLayout->addWidget( m_pRescanButton );
	pSearchWidget->setLayout( pSearchLayout );

	// Main layout
	QVBoxLayout* pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->addWidget( pSearchWidget );

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

		m_pTabWidget->setCurrentIndex( pPref->getSoundLibraryLastTab() );

		m_pTabWidget->tabBar()->setExpanding( true );

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
	m_pInfoView = new InfoView( this );
	pMainLayout->addWidget( m_pInfoView );

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

	updateIcons();
	updateStyleSheet();
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

void SoundLibraryPanel::updateInfoView( std::shared_ptr<SoundLibraryInfo> pInfo )
{
	m_pInfoView->updateContent( pInfo );
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
		pTopItem->setHidden( !filterTreeRecursive( pTree, pTopItem, sFilter ) );
	}
}

bool SoundLibraryPanel::filterTreeRecursive(
	SoundLibraryTree* pTree,
	QTreeWidgetItem* pItem,
	const QString& sFilter
)
{
	bool bAnyChildVisible = false;
	for ( int jj = 0; jj < pItem->childCount(); ++jj ) {
		QTreeWidgetItem* pChild = pItem->child( jj );

		bool bRecursiveVisibility = true;
		bool bMatch =
			sFilter.isEmpty() ||
			pChild->text( 0 ).contains( sFilter, Qt::CaseInsensitive );
		if ( !bMatch && pChild->childCount() > 0 ) {
			bMatch = filterTreeRecursive( pTree, pChild, sFilter );
			// In case we show this item because one of its children is visible,
			// we must not apply show all other siblings as well.
			bRecursiveVisibility = !bMatch;
		}
		if ( !bMatch ) {
			// We also filter by tags.
			auto it = pTree->getRegistry().find( pChild );
			if ( it != pTree->getRegistry().end() && it->second != nullptr &&
				 it->second->getTags().size() > 0 ) {
				bMatch = it->second->getTags().join( "" ).contains(
					sFilter, Qt::CaseInsensitive
				);
			}
		}
		if ( bRecursiveVisibility ) {
			hideRecursive( pChild, !bMatch );
		}
		else {
			pChild->setHidden( !bMatch );
		}
		if ( bMatch ) {
			bAnyChildVisible = true;
		}
	}

	return bAnyChildVisible;
}

void SoundLibraryPanel::hideRecursive( QTreeWidgetItem* pItem, bool bHidden )
{
	pItem->setHidden( bHidden );
	for ( int jj = 0; jj < pItem->childCount(); ++jj ) {
		QTreeWidgetItem* pChild = pItem->child( jj );
		hideRecursive( pChild, bHidden );
	}
}

void SoundLibraryPanel::onTabChanged( int nIndex )
{
	UNUSED( nIndex );
	getCurrentTree()->updateInfo();
	filterTree( getCurrentTree(), m_pSearchField->text() );
	HydrogenApp::pPreferences()->setSoundLibraryLastTab( nIndex );
}

void SoundLibraryPanel::onSearchTextChanged( const QString& sText )
{
	filterTree( getCurrentTree(), m_pSearchField->text() );
}

void SoundLibraryPanel::onRescanClicked()
{
	HydrogenApp::pEngine()->getSoundLibraryDatabase()->update();
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

	HydrogenApp::pEngine()->getCoreActionController()->setDrumkit( pNewDrumkit );

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
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
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
		updateIcons();
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

void SoundLibraryPanel::updateIcons()
{
	QString sIconPath( Skin::getSvgImagePath() );
	if ( HydrogenApp::pPreferences()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pRescanButton->setIcon( QIcon( sIconPath + "reload.svg" ) );

	if ( m_pTabWidget != nullptr ) {
		m_pTabWidget->setTabIcon( 0, QIcon( sIconPath + "drum.svg" ) );
		m_pTabWidget->setTabIcon( 1, QIcon( sIconPath + "pattern-editor.svg" ) );
		m_pTabWidget->setTabIcon( 2, QIcon( sIconPath + "song-editor.svg" ) );
	}
}

void SoundLibraryPanel::updateStyleSheet()
{
	const auto pColorTheme = HydrogenApp::pPreferences()->getColorTheme();

	const auto backgroundColor = pColorTheme->m_baseColor;
	const QColor textColor = Skin::moreBlackThanWhite( backgroundColor )
							   ? Qt::white
							   : Qt::black;

	setStyleSheet(
		QString( "\
QWidget#SearchWidget {                 \
    border: 1px solid #000;			   \
    border-radius: 2px;			       \
}									   \
%1				                       \
QLineEdit {						       \
    border-radius: 0px;			       \
    background: %2;         	       \
    color: %3;               	       \
}                          	           \
QTabBar {			   \
    background-color: %4;     	       \
    color: %5;              	       \
}                          	           \
" )
			.arg( Skin::getToolButtonStyle( backgroundColor ) )
			.arg( pColorTheme->m_spinBoxColor.name() )
			.arg( pColorTheme->m_spinBoxTextColor.name() )
			.arg( backgroundColor.name() )
			.arg( textColor.name() )
	);

	m_pInfoView->updateStyleSheet();
}
