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

#include "SoundLibraryTree.h"

#include <QMimeData>

#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include "SoundLibraryPanel.h"
#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"

using namespace H2Core;

SoundLibraryTree::SoundLibraryTree(
	SoundLibraryPanel* pParent,
	Filesystem::Artifact artifact,
	bool bStandAlone
)
	: QTreeWidget( pParent ),
	  m_pSoundLibraryPanel( pParent ),
	  m_artifact( artifact ),
	  m_bStandAlone( bStandAlone )
{
	setAlternatingRowColors( true );
	setRootIsDecorated( false );
	headerItem()->setHidden( true );

	connect( this, &QTreeWidget::currentItemChanged, [&]() {
		m_pSoundLibraryPanel->updateDetailView();
	} );

}

void SoundLibraryTree::updateRegistry() {
	clear();
	m_registry.clear();
	m_pSessionItem = nullptr;
	m_pSystemItem = nullptr;
	m_pUserItem = nullptr;

	auto pPref = H2Core::Preferences::get_instance();
	auto pFontTheme = pPref->getFontTheme();
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	auto pSoundLibraryDatabase = pHydrogen->getSoundLibraryDatabase();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QFont boldFont(
		pFontTheme->m_sApplicationFontFamily,
		getPointSize( pFontTheme->m_fontSize )
	);
	boldFont.setBold( true );

	std::vector<std::shared_ptr<SoundLibraryInfo>> infos;
	if ( m_artifact == Filesystem::Artifact::DrumkitExtracted ) {
		infos = pSoundLibraryDatabase->getDrumkitInfos();
	}
	else if ( m_artifact == Filesystem::Artifact::Pattern ) {
		infos = pSoundLibraryDatabase->getPatternInfos();
	}
	else if ( m_artifact == Filesystem::Artifact::Song ) {
		infos = pSoundLibraryDatabase->getSongInfos();
	}
	else {
		ERRORLOG( QString( "Unsupported artifact [%1]" )
					  .arg( Filesystem::ArtifactToQString( m_artifact ) ) );
		return;
	}

	// Separate patterns by context
	for ( const auto& pInfo : infos ) {
		QTreeWidgetItem* pParentItem = nullptr;

		if ( pInfo->getContext() == H2Core::Filesystem::Context::System ) {
			if ( m_pSystemItem == nullptr ) {
				m_pSystemItem = new QTreeWidgetItem( this );
				m_pSystemItem->setText(
					0, pCommonStrings->getSoundLibrarySystem()
				);
				m_pSystemItem->setFont( 0, boldFont );
				m_pSystemItem->setExpanded( true );
			}
			pParentItem = m_pSystemItem;
		}
		else if ( pInfo->getContext() == H2Core::Filesystem::Context::User ) {
			if ( m_pUserItem == nullptr ) {
				m_pUserItem = new QTreeWidgetItem( this );
				m_pUserItem->setText(
					0, pCommonStrings->getSoundLibraryUser()
				);
				m_pUserItem->setFont( 0, boldFont );
				m_pUserItem->setExpanded( true );
			}
			pParentItem = m_pUserItem;
		}
		else {
			if ( m_pSessionItem == nullptr ) {
				m_pSessionItem = new QTreeWidgetItem( this );
				m_pSessionItem->setText(
					0, pCommonStrings->getSoundLibraryUser()
				);
				m_pSessionItem->setFont( 0, boldFont );
				m_pSessionItem->setExpanded( true );
			}
			pParentItem = m_pSessionItem;
		}

		auto pNewItem = new QTreeWidgetItem( pParentItem );
		QString sDisplayName = pInfo->getName();
		if ( sDisplayName.isEmpty() ) {
			// Fallback to filename without extension
			QFileInfo fi( pInfo->getPath() );
			sDisplayName = fi.completeBaseName();
		}
		pNewItem->setText( 0, sDisplayName );
		pNewItem->setText( 1, pInfo->getPath() );
		m_registry[pNewItem] = pInfo;
	}
}

void SoundLibraryTree::mousePressEvent( QMouseEvent* event )
{
	//	INFOLOG( "[mousePressEvent]" );
	QTreeWidget::mousePressEvent( event );

	auto pEv = static_cast<MouseEvent*>( event );

	if ( event->button() == Qt::RightButton ) {
		emit rightClicked( pEv->globalPosition().toPoint() );
	}
	else if ( event->button() == Qt::LeftButton ) {
		emit leftClicked( pEv->globalPosition().toPoint() );
	}
}

void SoundLibraryTree::mouseMoveEvent( QMouseEvent* pEvent )
{
	if ( m_bStandAlone ) {
		return;
	}

    // Initialize drag and drop events
	if ( !( pEvent->buttons() & Qt::LeftButton ) ) {
		return;
	}

	if ( currentItem() == nullptr ) {
		return;
	}

	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	const QString sMimeText =
		QString( "drag %1::%2" )
			.arg( Filesystem::ArtifactToQString( m_artifact ) )
			.arg( it->second->getPath() );

	auto pDrag = new QDrag( this );
	auto pMimeData = new QMimeData;
	pMimeData->setText( sMimeText );
	pDrag->setMimeData( pMimeData );
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
}
