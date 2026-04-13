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
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/InstrumentInfo.h>
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

	const auto pFontTheme = Preferences::get_instance()->getFontTheme();
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

    std::vector<std::shared_ptr<SoundLibraryInfo>> sessionInfos;
    std::vector<std::shared_ptr<SoundLibraryInfo>> systemInfos;
    std::vector<std::shared_ptr<SoundLibraryInfo>> userInfos;

	// Separate patterns by context
	for ( const auto& ppInfo : infos ) {
		if ( ppInfo == nullptr ) {
			continue;
		}
		else if ( ppInfo->getContext() == H2Core::Filesystem::Context::System ) {
			systemInfos.push_back( ppInfo );
		}
		else if ( ppInfo->getContext() == H2Core::Filesystem::Context::User ) {
			userInfos.push_back( ppInfo );
		}
		else {
			sessionInfos.push_back( ppInfo );
		}
	}

	if ( sessionInfos.size() > 0 ) {
		m_pSessionItem = new QTreeWidgetItem( this );
		m_pSessionItem->setText( 0, pCommonStrings->getSoundLibrarySession() );
		m_pSessionItem->setFont( 0, boldFont );
		m_pSessionItem->setExpanded( true );
		addNodes( m_pSessionItem, sessionInfos, "" );
	}
	if ( userInfos.size() > 0 ) {
		m_pUserItem = new QTreeWidgetItem( this );
		m_pUserItem->setText( 0, pCommonStrings->getSoundLibraryUser() );
		m_pUserItem->setFont( 0, boldFont );
		m_pUserItem->setExpanded( true );
		addNodes( m_pUserItem, userInfos, "" );
	}
	if ( systemInfos.size() > 0 ) {
		m_pSystemItem = new QTreeWidgetItem( this );
		m_pSystemItem->setText( 0, pCommonStrings->getSoundLibrarySystem() );
		m_pSystemItem->setFont( 0, boldFont );
		m_pSystemItem->setExpanded( true );
		addNodes( m_pSystemItem, systemInfos, "" );
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

void SoundLibraryTree::addNodes(
	QTreeWidgetItem* pParent,
	std::vector<std::shared_ptr<SoundLibraryInfo>> infos,
	const QString& sBasePath
)
{
	const auto pFontTheme = Preferences::get_instance()->getFontTheme();
	QFont dirFont(
		pFontTheme->m_sApplicationFontFamily,
		getPointSize( pFontTheme->m_fontSize )
	);
	dirFont.setItalic( true );

	// Let's be sure to write platform-independent code.
	auto splitCleanly = []( const QString& sPath ) {
		QString sCleanedPath( sPath );
		sCleanedPath.replace( "\\", QDir::separator() );
		sCleanedPath.replace( "/", QDir::separator() );
		return sCleanedPath.split( QDir::separator() );
	};

	QString sCurrentDir( sBasePath );
	// During the initial call of this function we have to figure out the
	// common demoniator of all supplied path as the root of this tree
	// section.
	if ( sBasePath.isEmpty() ) {
		QString sCommonPart = infos[0]->getPath();
		for ( const auto& ppInfo : infos ) {
			while ( !ppInfo->getPath().contains( sCommonPart ) ) {
				auto commonParts = splitCleanly( sCommonPart );
				if ( commonParts.length() < 2 ) {
					break;
				}
				commonParts.removeLast();
				sCommonPart = commonParts.join( QDir::separator() );
			}
		}
		sCurrentDir = sCommonPart;
	}

	// Split content into subfolders and files. We store them in maps using
	// their path relative to the current folder to harness automatic
	// alphanumeric ordering.
	std::map<QString, std::vector<std::shared_ptr<SoundLibraryInfo>>> dirInfos;
	std::map<QString, std::shared_ptr<SoundLibraryInfo>> fileInfos;
	for ( const auto& ppInfo : infos ) {
		QString sPath = ppInfo->getPath();
		sPath.remove( sCurrentDir );
		if ( sPath.startsWith( "/" ) || sPath.startsWith( "\\" ) ) {
			sPath.removeFirst();
		}
		if ( sPath.contains( "/" ) || sPath.contains( "\\" ) ) {
			auto ppathSplit = splitCleanly( sPath );
			if ( ppathSplit.first().isEmpty() ) {
				// We deal with an absolute path and the leading `/` causes the
				// first element to be empty.
				ppathSplit.removeFirst();
			}
			// The folder containing the drumkit files will be treated as
			// the drumkit itself.
			const int nMinLength =
				m_artifact == Filesystem::Artifact::DrumkitExtracted ? 2 : 1;
			if ( ppathSplit.length() <= nMinLength ) {
				fileInfos[sPath] = ppInfo;
				continue;
			}
			const QString sFolderName = ppathSplit.first();
			if ( dirInfos.find( sFolderName ) != dirInfos.end() ) {
				dirInfos.at( sFolderName ).push_back( ppInfo );
			}
			else {
				std::vector<std::shared_ptr<SoundLibraryInfo>> infos{ ppInfo };
				dirInfos[sFolderName] = std::move( infos );
			}
		}
		else {
			fileInfos[sPath] = ppInfo;
		}
	}

	for ( const auto& [ssFolderName, iinfos] : dirInfos ) {
		auto pDirItem = new QTreeWidgetItem( pParent );
		pDirItem->setText( 0, ssFolderName );
		pDirItem->setFont( 0, dirFont );
		pDirItem->setExpanded( false );
		addNodes(
			pDirItem, iinfos,
			QString( "%1%2%3" )
				.arg( sCurrentDir )
				.arg( QDir::separator() )
				.arg( ssFolderName )
		);
	}

	for ( const auto& [ssPath, ppInfo] : fileInfos ) {
		auto pFileItem = new QTreeWidgetItem( pParent );
		QString sDisplayName = ppInfo->getName();
		if ( sDisplayName.isEmpty() ) {
			// Fallback to filename without extension
			QFileInfo fi( ppInfo->getPath() );
			sDisplayName = fi.completeBaseName();
		}
		pFileItem->setText( 0, sDisplayName );
		pFileItem->setText( 1, ppInfo->getPath() );

		if ( ppInfo->getArtifact() == Filesystem::Artifact::DrumkitExtracted ) {
			auto pDrumkitInfo =
				std::dynamic_pointer_cast<DrumkitInfo>( ppInfo );
			if ( pDrumkitInfo != nullptr ) {
				for ( const auto& ppInstrumentInfo :
					  pDrumkitInfo->getInstrumentInfos() ) {
					auto pInstrumentItem = new QTreeWidgetItem( pFileItem );
					QString sDisplayName = ppInstrumentInfo->getName();
					if ( sDisplayName.isEmpty() ) {
						sDisplayName = ppInstrumentInfo->getType();
					}
					pInstrumentItem->setText( 0, sDisplayName );
					pInstrumentItem->setText( 1, ppInstrumentInfo->getPath() );
				}
			}
		}
	}
}
