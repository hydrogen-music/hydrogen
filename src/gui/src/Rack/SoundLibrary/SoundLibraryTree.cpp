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

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/InstrumentInfo.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "DrumkitPropertiesDialog.h"
#include "SoundLibraryPanel.h"
#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../../Skin.h"
#include "../../UndoActions.h"
#include "core/SoundLibrary/SoundLibraryInfo.h"

using namespace H2Core;

SoundLibraryTree::SoundLibraryTree(
	SoundLibraryPanel* pParent,
	SoundLibraryInfo::Type type,
	bool bStandAlone
)
	: QTreeWidget( pParent ),
	  m_pSoundLibraryPanel( pParent ),
	  m_type( type ),
	  m_bStandAlone( bStandAlone ),
	  m_dragStartPosition( QPoint() )
{
	setAlternatingRowColors( true );
	setRootIsDecorated( false );
	headerItem()->setHidden( true );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	auto addDrumkitActions = [&]( QMenu* pMenu, bool bWritable ) {
		if ( m_bStandAlone ) {
			return;
		}
		pMenu->addAction(
			pCommonStrings->getMenuActionLoad(), this, SLOT( actionLoad() )
		);
		pMenu->addAction(
			pCommonStrings->getMenuActionProperties(), this,
			SLOT( actionProperties() )
		);
		pMenu->addSeparator();
		pMenu->addAction(
			pCommonStrings->getMenuActionDuplicate(), this,
			SLOT( actionDuplicate() )
		);
		auto pDeleteAction = pMenu->addAction(
			pCommonStrings->getMenuActionDelete(), this, SLOT( actionDelete() )
		);
		if ( !bWritable ) {
			pDeleteAction->setEnabled( false );
		}
		pMenu->addAction(
			pCommonStrings->getMenuActionExport(), this, SLOT( actionExport() )
		);
		pMenu->addSeparator();
		pMenu->addAction(
			pCommonStrings->getMenuActionImport(), this, SLOT( actionImport() )
		);
		pMenu->addAction(
			pCommonStrings->getMenuActionOnlineImport(), this,
			SLOT( actionOnlineImport() )
		);
	};

	m_pPopupMenu = new QMenu( this );
	addDrumkitActions( m_pPopupMenu, true );

	m_pPopupMenuReadOnly = new QMenu( this );
	addDrumkitActions( m_pPopupMenuReadOnly, false );

	connect( this, &QTreeWidget::currentItemChanged, [&]() {
		m_pSoundLibraryPanel->updateDetailView();
		if ( m_bStandAlone && currentItem() != nullptr ) {
			emit itemChanged(
				m_registry.find( currentItem() ) != m_registry.end()
			);
		}
	} );
}

void SoundLibraryTree::updateFont() {
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();
	QFont boldFont(
		pFontTheme->m_sApplicationFontFamily,
		getPointSize( pFontTheme->m_fontSize )
	);
	boldFont.setBold( true );
	if ( m_pSessionItem != nullptr ) {
		m_pSessionItem->setFont( 0, boldFont );
		recursivelyUpdateFont( m_pSessionItem );
	}
	if ( m_pSystemItem != nullptr ) {
		m_pSystemItem->setFont( 0, boldFont );
		recursivelyUpdateFont( m_pSystemItem );
	}
	if ( m_pUserItem != nullptr ) {
		m_pUserItem->setFont( 0, boldFont );
		recursivelyUpdateFont( m_pUserItem );
	}
}

void SoundLibraryTree::updateRegistry()
{
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
	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		infos = pSoundLibraryDatabase->getDrumkitInfos();
	}
	else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
		infos = pSoundLibraryDatabase->getPatternInfos();
	}
	else if ( m_type == SoundLibraryInfo::Type::Song ) {
		infos = pSoundLibraryDatabase->getSongInfos();
	}
	else {
		ERRORLOG( QString( "Unsupported type [%1]" )
					  .arg( SoundLibraryInfo::TypeToQString( m_type ) ) );
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

void SoundLibraryTree::actionLoad()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}
	if ( pHydrogen->getSong() == nullptr ) {
		return;
	}

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getName() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// Pass a copy of the kit since we do not want to alter the settings of
		// the original one.
		MainForm::switchDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
	}
	else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
		const auto pCommonStrings =
			HydrogenApp::get_instance()->getCommonStrings();
		const auto pPattern =
			H2Core::CoreActionController::loadPattern( it->second->getPath() );
		if ( pPattern == nullptr ) {
			QMessageBox::critical(
				this, "Hydrogen", pCommonStrings->getPatternLoadError()
			);
			return;
		}

		HydrogenApp::get_instance()->pushUndoCommand(
			new SE_insertPatternAction(
				SE_insertPatternAction::Type::Insert,
				pHydrogen->getSong()->getPatternList()->size(), pPattern,
				nullptr
			)
		);
	}
	else {
		// Error handling and dialog is handled within openFile.
		HydrogenApp::get_instance()->openFile(
			Filesystem::Artifact::Song, it->second->getPath()
		);
	}
}

void SoundLibraryTree::actionProperties()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getName() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// We provide a copy of the recent drumkit to ensure the drumkit
		// is not getting dirty upon saving (in case new properties are
		// stored in the kit but writing it to disk fails).
		auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
		DrumkitPropertiesDialog dialog( this, pNewDrumkit, true, false );
		dialog.exec();
	}
	else {
		INFOLOG( "not implemented" );
	}
}
void SoundLibraryTree::actionDuplicate()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getName() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// We provide a copy of the recent drumkit to ensure the drumkit
		// is not getting dirty upon saving (in case new properties are
		// stored in the kit but writing it to disk fails).
		auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
		// Suggest an unique drumkit name.
		pNewDrumkit->setName(
			Filesystem::appendNumberOrIncrement( it->second->getName() )
		);
		pNewDrumkit->setPath(
			H2Core::Filesystem::userDrumkitsDir() + pNewDrumkit->getName()
		);

		DrumkitPropertiesDialog dialog( this, pNewDrumkit, false, false );
		dialog.exec();
	}
	else {
		INFOLOG( "not implemented" );
	}
}

void SoundLibraryTree::actionDelete()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	if ( it->second->getContext() == Filesystem::Context::System ||
		 it->second->getContext() == Filesystem::Context::SessionReadOnly ) {
		QMessageBox::warning(
			this, "Hydrogen",
			QString( "%1 [%2] " )
				.arg( it->second->getName() )
				.arg( it->second->getPath() )
				.append( tr( "is a read-only and can't be deleted." ) )
		);
		return;
	}

	QString sTargetPath = it->second->getPath();

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		// If we delete a kit containing samples used and loaded in the current
		// song's drumkit, we get into trouble.
		if ( pHydrogen->getSong() == nullptr ||
			 pHydrogen->getSong()->getDrumkit() ) {
			return;
		}
		auto pDrumkit = pHydrogen->getSong()->getDrumkit();

		// For a sample to be contained both the instrument's drumkit path must
		// match the selected one and the instrument has to contain at least one
		// sample with a non-empty, relative path.
		bool bSampleContained = false;
		sTargetPath = Filesystem::drumkitDirFromPath( it->second->getPath() );
		for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
			if ( ppInstrument != nullptr &&
				 ppInstrument->getDrumkitPath() == it->second->getPath() ) {
				for ( const auto& ppComponent :
					  *ppInstrument->getComponents() ) {
					if ( ppComponent != nullptr ) {
						for ( const auto& ppLayer : ppComponent->getLayers() ) {
							if ( ppLayer != nullptr &&
								 ppLayer->getSample() != nullptr &&
								 !ppLayer->getSample()->getFilePath().isEmpty(
								 ) &&
								 ppLayer->getSample()->getFilePath().contains(
									 sTargetPath
								 ) ) {
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
			QMessageBox::critical(
				this, "Hydrogen",
				tr( "It is not possible to delete drumkit: \n  [%1]\nIt "
					"contains "
					"samples used and loaded in the current song kit." )
					.arg( it->second->getName() )
			);
			return;
		}
	}

	if ( QMessageBox::warning(
			 this, "Hydrogen",
			 tr( "Warning, \"%1\" [%2] will be deleted from disk.\nAre "
				 "you sure?" )
				 .arg( it->second->getName() )
				 .arg( sTargetPath ),
			 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
		 ) == QMessageBox::Cancel ) {
		return;
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	INFOLOG( QString( "Removing %1 [%2] at [%3]" )
				 .arg( SoundLibraryInfo::TypeToQString( it->second->getType() )
				 )
				 .arg( it->second->getName() )
				 .arg( sTargetPath ) );
	const bool bOk = Filesystem::rm( sTargetPath, true );
	if ( !bOk ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(
			this, "Hydrogen", tr( "Drumkit deletion failed." )
		);
		return;
	}

	switch ( m_type ) {
		case SoundLibraryInfo::Type::Drumkit:
			Hydrogen::get_instance()->getSoundLibraryDatabase()->updateDrumkits(
				Event::Trigger::Default
			);
			break;
		case SoundLibraryInfo::Type::Pattern:
			Hydrogen::get_instance()->getSoundLibraryDatabase()->updatePatterns(
				Event::Trigger::Default
			);
			break;
		default:
			Hydrogen::get_instance()->getSoundLibraryDatabase()->updateSongs(
				Event::Trigger::Default
			);
	}
	QApplication::restoreOverrideCursor();
}

void SoundLibraryTree::actionExport()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getName() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// Pass a copy of the kit since we do not want to alter the settings of
		// the original one.
		MainForm::exportDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
	}
	else {
		INFOLOG( "not implemented" );
	}
}
void SoundLibraryTree::actionImport()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}
	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		HydrogenApp::get_instance()->getMainForm()->action_drumkit_import( false
		);
	}
	else {
		INFOLOG( "not implemented" );
	}
}
void SoundLibraryTree::actionOnlineImport()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	HydrogenApp::get_instance()->getMainForm()->action_drumkit_onlineImport();
}

void SoundLibraryTree::recursivelyUpdateFont( QTreeWidgetItem* pItem )
{
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();
	const QFont font(
		pFontTheme->m_sLevel2FontFamily, getPointSize( pFontTheme->m_fontSize )
	);
	QFont dirFont(
		pFontTheme->m_sApplicationFontFamily,
		getPointSize( pFontTheme->m_fontSize )
	);
	dirFont.setItalic( true );
	for ( int ii = 0; ii < pItem->childCount(); ++ii ) {
		auto pChildNode = pItem->child( ii );
		if ( pChildNode == nullptr ) {
			continue;
		}
		bool bIsDir;
		if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
			// In the drumkit tree it is rather hard to distinguish between
			// general folders and those containing only a single drumkit.
			bIsDir = m_registry.find( pChildNode ) == m_registry.end();
		}
		else {
			bIsDir = pChildNode->childCount() > 0;
		}

		if ( bIsDir ) {
			pChildNode->setFont( 0, dirFont );
			recursivelyUpdateFont( pChildNode );
		}
		else {
			pChildNode->setFont( 0, font );
		}
	}
}

void SoundLibraryTree::mousePressEvent( QMouseEvent* event )
{
	QTreeWidget::mousePressEvent( event );

	auto pEv = static_cast<MouseEvent*>( event );

	if ( event->button() == Qt::RightButton && ! m_bStandAlone &&
		 currentItem() != nullptr ) {
		// Show popup menu
		auto it = m_registry.find( currentItem() );
		if ( it != m_registry.end() && it->second != nullptr ) {
			QMenu* pMenu;
			if ( it->second->getContext() == Filesystem::Context::System ||
				 it->second->getContext() ==
					 Filesystem::Context::SessionReadOnly ) {
				pMenu = m_pPopupMenuReadOnly;
			}
			else {
				pMenu = m_pPopupMenu;
			}

			pMenu->popup( pEv->globalPosition().toPoint() );
		}
	}
	else if ( event->button() == Qt::LeftButton ) {
		m_dragStartPosition = pEv->globalPosition().toPoint();

        // Preview the clicked instrument
		if ( m_type == SoundLibraryInfo::Type::Drumkit &&
			 currentItem() != nullptr ) {
			auto it = m_registry.find( currentItem() );
			if ( it != m_registry.end() && it->second != nullptr &&
				 it->second->getType() == SoundLibraryInfo::Type::Instrument ) {
				auto pHydrogen = Hydrogen::get_instance();
				auto pInstrumentInfo =
					std::dynamic_pointer_cast<InstrumentInfo>( it->second );
				if ( pInstrumentInfo == nullptr ) {
					return;
				}
				auto pDrumkit =
					pHydrogen->getSoundLibraryDatabase()->getDrumkit(
						pInstrumentInfo->getPath()
					);
				if ( pDrumkit == nullptr ) {
					ERRORLOG(
						QString(
							"Unable to retrieve kit [%1] for instrument [%2]"
						)
							.arg( pInstrumentInfo->getPath() )
							.arg( pInstrumentInfo->getName() )
					);
					return;
				}
				const auto pTargetInstrument =
					pDrumkit->getInstruments()->find( pInstrumentInfo->getId() );
				if ( pTargetInstrument == nullptr ) {
					ERRORLOG(
						QString(
							"Unable to retrieve instrument [%1](%2) from kit [%3]"
						)
							.arg( pInstrumentInfo->getName() )
							.arg( static_cast<int>(pInstrumentInfo->getId()) )
							.arg( pInstrumentInfo->getPath() )
					);
					return;
				}

				auto pPreviewInstrument =
					std::make_shared<Instrument>( pTargetInstrument );
				pPreviewInstrument->loadSamples(
					pHydrogen->getAudioEngine()->getPlayhead()->getBpm()
				);
				pPreviewInstrument->setIsPreviewInstrument( true );
				pPreviewInstrument->setId( Instrument::EmptyId );

				INFOLOG( QString( "Loading instrument [%1] from drumkit [%2]" )
							 .arg( pInstrumentInfo->getName() )
							 .arg( pInstrumentInfo->getPath() ) );

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

	if ( ( pEvent->pos() - m_dragStartPosition ).manhattanLength() <
		 QApplication::startDragDistance() ) {
		return;
	}

	if ( currentItem() == nullptr ) {
		return;
	}

	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	QString sMimeText;
	switch ( m_type ) {
		case SoundLibraryInfo::Type::Drumkit:
			if ( it->second->getType() == SoundLibraryInfo::Type::Instrument ) {
				sMimeText = "importInstrument:" + it->second->getPath() +
							"::" + it->second->getName();
			}
			else {
				WARNINGLOG( "not implemented" );
			}
			break;
		case SoundLibraryInfo::Type::Pattern:
			sMimeText = QString( "drag %1::%2" )
							.arg( SoundLibraryInfo::TypeToQString( m_type ) )
							.arg( it->second->getPath() );
			break;
		default:
			WARNINGLOG( "no implemented" );
	}

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
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

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
				m_type == SoundLibraryInfo::Type::Drumkit ? 2 : 1;
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
		pDirItem->setIcon( 0, QIcon( sIconPath + "folder.svg" ) );
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
		m_registry[pFileItem] = ppInfo;

		if ( ppInfo->getType() == SoundLibraryInfo::Type::Drumkit ) {
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
					pInstrumentItem->setIcon(
						0, QIcon( sIconPath + "speaker.svg" )
					);
					m_registry[pInstrumentItem] = ppInstrumentInfo;
				}
			}
		}
	}
}
