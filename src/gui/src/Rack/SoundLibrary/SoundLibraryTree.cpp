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
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include "SoundLibraryPanel.h"
#include "../Rack.h"
#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../DrumkitPropertiesDialog.h"
#include "../../HydrogenApp.h"
#include "../../OnlineImportDialog.h"
#include "../../PatternPropertiesDialog.h"
#include "../../Skin.h"
#include "../../SongPropertiesDialog.h"
#include "../../UndoActions.h"
#include "../../Widgets/FileDialog.h"

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
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	headerItem()->setHidden( true );

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Menus working on artifacts
	auto addArtifactActions = [&]( QMenu* pMenu, bool bWritable, bool bAdd ) {
		if ( m_bStandAlone ) {
			return;
		}
		if ( bAdd ) {
			m_multiSelectActions.insert( pMenu->addAction(
				pCommonStrings->getMenuActionAdd(), this, SLOT( actionAdd() )
			) );
		}
		else {
			pMenu->addAction(
				pCommonStrings->getMenuActionLoad(), this, SLOT( actionLoad() )
			);
		}
		if ( m_type == SoundLibraryInfo::Type::Drumkit && bAdd ) {
			// The popup menu for the instrument node does solely allow to add
			// instruments.
			return;
		}
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
		m_multiSelectActions.insert( pDeleteAction );
		if ( !bWritable ) {
			pDeleteAction->setEnabled( false );
		}
		if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
			// Only for drumkits we support the notion of import/export in the
			// Sound Library to convert between their bundled and extracted
			// versions. For songs "export" is already in use for rendering to
			// audio and both songs and patterns can be easily moved using a
			// file browser.
			pMenu->addAction(
				pCommonStrings->getMenuActionExport(), this,
				SLOT( actionExport() )
			);
			pMenu->addSeparator();
			pMenu->addAction(
				pCommonStrings->getMenuActionImport(), this,
				SLOT( actionImport() )
			);
		}
		else {
			pMenu->addSeparator();
		}
		pMenu->addAction(
			pCommonStrings->getMenuActionOnlineImport(), this,
			SLOT( actionOnlineImport() )
		);
	};

	m_pPopupMenu = new QMenu( this );
	addArtifactActions( m_pPopupMenu, true, false );
	m_pPopupMenuReadOnly = new QMenu( this );
	addArtifactActions( m_pPopupMenuReadOnly, false, false );

	m_pPopupMenuAdd = new QMenu( this );
	addArtifactActions( m_pPopupMenuAdd, true, true );
	m_pPopupMenuAddReadOnly = new QMenu( this );
	addArtifactActions( m_pPopupMenuAddReadOnly, false, true );

	// Menus working on top-level folders
	auto addDirActions = [&]( QMenu* pMenu, bool bWritable ) {
		if ( m_bStandAlone ) {
			return;
		}
		auto pDeleteAction = pMenu->addAction(
			pCommonStrings->getMenuActionRemoveDirFromSoundLibrary(), this, SLOT( actionRemoveFolder() )
		);
		if ( !bWritable ) {
			pDeleteAction->setEnabled( false );
		}
		pMenu->addAction(
			pCommonStrings->getMenuActionAddDirToSoundLibrary(), this,
			SLOT( actionAddFolder() )
		);
	};

	m_pPopupMenuDir = new QMenu( this );
	addDirActions( m_pPopupMenuDir, true );
	m_pPopupMenuDirReadOnly = new QMenu( this );
	addDirActions( m_pPopupMenuDirReadOnly, false );

	// Select the expanded node (in case it is a drumkit). Else selecting an
	// instrument would cause preview sounds of that instrument on each
	// subsequent drumkit expanding.
	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto selectItem = [&]( QTreeWidgetItem* pItem ) {
			if ( pItem == nullptr ) {
				return;
			}
			auto it = m_registry.find( pItem );
			if ( it != m_registry.end() && it->second != nullptr &&
				 it->second->getType() == SoundLibraryInfo::Type::Drumkit ) {
				setCurrentItem( pItem );
			}
		};

		connect( this, &QTreeWidget::itemCollapsed, selectItem );
		connect( this, &QTreeWidget::itemExpanded, selectItem );
	}

	connect( this, &QTreeWidget::currentItemChanged, [&]() { updateInfo(); } );
}

QItemSelectionModel::SelectionFlags SoundLibraryTree::selectionCommand(
	const QModelIndex& index,
	const QEvent* event
) const
{
	QTreeWidgetItem* pItem = itemFromIndex( index );
	if ( pItem == nullptr ) {
		return QTreeWidget::selectionCommand( index, event );
	}

	auto it = m_registry.find( pItem );
	if ( it == m_registry.end() ) {
		// Top-level or subfolder node — do not alter selection
		return QItemSelectionModel::NoUpdate;
	}

	if ( it->second != nullptr ) {
		const auto infoType = it->second->getType();
		if ( infoType == SoundLibraryInfo::Type::Pattern ||
			 infoType == SoundLibraryInfo::Type::Instrument ) {
			// Selectable — delegate to default ExtendedSelection behavior
			return QTreeWidget::selectionCommand( index, event );
		}
	}

	// Drumkit, Song, or null info — clear selection
	return QItemSelectionModel::Clear;
}

void SoundLibraryTree::updateFont()
{
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
	if ( m_customDirs.size() > 0 ) {
		for ( auto& ppItem : m_customDirs ) {
			if ( ppItem != nullptr ) {
				ppItem->setFont( 0, boldFont );
				recursivelyUpdateFont( ppItem );
			}
		}
	}
}

void SoundLibraryTree::updateInfo()
{
	if ( currentItem() != nullptr ) {
		auto it = m_registry.find( currentItem() );
		if ( it != m_registry.end() && it->second != nullptr ) {
			m_pSoundLibraryPanel->updateInfoView( it->second );
			if ( m_bStandAlone ) {
				emit itemChanged( true );
			}
			return;
		}
	}
	m_pSoundLibraryPanel->updateInfoView( nullptr );
	if ( m_bStandAlone ) {
		emit itemChanged( false );
	}
}

void SoundLibraryTree::updateRegistry()
{
	clear();
	m_registry.clear();
	m_internalDirs.clear();
	m_customDirs.clear();
	m_pSessionItem = nullptr;
	m_pSystemItem = nullptr;
	m_pUserItem = nullptr;

	const auto pPref = Preferences::get_instance();
	const auto pFontTheme = pPref->getFontTheme();
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
	std::map<QString, std::vector<std::shared_ptr<SoundLibraryInfo>>> customInfos;

	// Separate artifacts by context
	const auto customDirs = pPref->getCustomSoundLibraryDirs();
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
		else if ( ppInfo->getContext() == H2Core::Filesystem::Context::Custom ) {
			for ( const auto& ssDir : customDirs ) {
				if ( ppInfo->getPath().contains( ssDir ) ) {
					if ( customInfos.find( ssDir ) != customInfos.end() ) {
						customInfos[ ssDir ].push_back( ppInfo );
					}
					else {
						std::vector<std::shared_ptr<SoundLibraryInfo>> infos;
						infos.push_back( ppInfo );
						customInfos[ ssDir ] = std::move( infos );
					}
				}
			}
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
		m_pSessionItem->setFlags(
			m_pSessionItem->flags() & ~Qt::ItemIsSelectable
		);
		m_internalDirs.push_back( m_pSessionItem );
		addNodes( m_pSessionItem, sessionInfos, "" );
	}
	if ( customInfos.size() > 0 ) {
		for ( const auto& [ssLabel, iinfos] : customInfos ) {
			auto pItem = new QTreeWidgetItem( this );
			pItem->setText(
				0, Skin::trimPathToFitWidth(
					   ssLabel, boldFont, Rack::nWidth, QMargins( 15, 0, 0, 0 )
				   )
			);
			pItem->setFont( 0, boldFont );
			pItem->setExpanded( true );
			pItem->setToolTip( 0, ssLabel );
			pItem->setFlags(
				pItem->flags() & ~Qt::ItemIsSelectable
			);
			m_customDirs.push_back( pItem );
			addNodes( pItem, iinfos, ssLabel );
		}
	}
	if ( userInfos.size() > 0 ) {
		m_pUserItem = new QTreeWidgetItem( this );
		m_pUserItem->setText( 0, pCommonStrings->getSoundLibraryUser() );
		m_pUserItem->setFont( 0, boldFont );
		m_pUserItem->setExpanded( true );
		switch ( m_type ) {
			case SoundLibraryInfo::Type::Drumkit:
			case SoundLibraryInfo::Type::Instrument:
				m_pUserItem->setToolTip( 0, Filesystem::userDrumkitsDir() );
				break;
			case SoundLibraryInfo::Type::Pattern:
				m_pUserItem->setToolTip( 0, Filesystem::userPatternsDir() );
				break;
			case SoundLibraryInfo::Type::Song:
				m_pUserItem->setToolTip( 0, Filesystem::userSongsDir() );
				break;
		}
		m_pUserItem->setFlags( m_pUserItem->flags() & ~Qt::ItemIsSelectable );
		m_internalDirs.push_back( m_pUserItem );
		addNodes( m_pUserItem, userInfos, "" );
	}
	if ( systemInfos.size() > 0 ) {
		m_pSystemItem = new QTreeWidgetItem( this );
		m_pSystemItem->setText( 0, pCommonStrings->getSoundLibrarySystem() );
		m_pSystemItem->setFont( 0, boldFont );
		m_pSystemItem->setExpanded( true );
		switch ( m_type ) {
			case SoundLibraryInfo::Type::Drumkit:
			case SoundLibraryInfo::Type::Instrument:
				m_pSystemItem->setToolTip( 0, Filesystem::systemDrumkitsDir() );
				break;
			case SoundLibraryInfo::Type::Pattern:
				m_pSystemItem->setToolTip( 0, Filesystem::systemPatternsDir() );
				break;
			case SoundLibraryInfo::Type::Song:
				m_pSystemItem->setToolTip( 0, Filesystem::systemSongsDir() );
				break;
		}
		m_pSystemItem->setFlags(
			m_pSystemItem->flags() & ~Qt::ItemIsSelectable
		);
		m_internalDirs.push_back( m_pSystemItem );
		addNodes( m_pSystemItem, systemInfos, "" );
	}
}

void SoundLibraryTree::addDirToLibrary( const QString& sDirPath )
{
	if ( sDirPath.isEmpty() ) {
		return;
	}
	auto pPref = Preferences::get_instance();

	auto customDirs = pPref->getCustomSoundLibraryDirs();
	customDirs << sDirPath;
	pPref->setCustomSoundLibraryDirs( customDirs );

	Hydrogen::get_instance()->getSoundLibraryDatabase()->update();
}
void SoundLibraryTree::removeDirFromLibrary( const QString& sDirPath )
{
	if ( sDirPath.isEmpty() ) {
		return;
	}
	auto pPref = Preferences::get_instance();

	auto customDirs = pPref->getCustomSoundLibraryDirs();
	customDirs.removeAll( sDirPath );
	pPref->setCustomSoundLibraryDirs( customDirs );

	Hydrogen::get_instance()->getSoundLibraryDatabase()->update();
}

void SoundLibraryTree::actionAdd()
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return;
	}

	for ( auto* pItem : selectedItems() ) {
		auto it = m_registry.find( pItem );
		if ( it == m_registry.end() || it->second == nullptr ) {
			continue;
		}

		if ( m_type == SoundLibraryInfo::Type::Drumkit &&
			 it->second->getType() == SoundLibraryInfo::Type::Instrument ) {
			pHydrogenApp->getPatternEditorPanel()->addInstrument(
				it->second->getPath(), it->second->getName(), -1
			);
		}
		else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
			const auto pCommonStrings = pHydrogenApp->getCommonStrings();
			const auto pPattern =
				H2Core::CoreActionController::loadPattern( it->second->getPath() );
			if ( pPattern == nullptr ) {
				QMessageBox::critical(
					this, "Hydrogen", pCommonStrings->getPatternLoadError()
				);
				continue;
			}

			pHydrogenApp->pushUndoCommand(
				new SE_insertPatternAction(
					SE_insertPatternAction::Type::Insert,
					pHydrogen->getSong()->getPatternList()->size(), pPattern,
					nullptr
				),
				"SoundLibraryTree::insertPatternAction"
			);
		}
		else {
			ERRORLOG( QString( "Invalid type [%1]" )
						  .arg( SoundLibraryInfo::TypeToQString( m_type ) ) );
		}
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
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// Pass a copy of the kit since we do not want to alter the settings of
		// the original one.
		MainForm::switchDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
	}
	else if ( m_type == SoundLibraryInfo::Type::Song ) {
		// Error handling and dialog is handled within openFile.
		HydrogenApp::get_instance()->openFile(
			Filesystem::Artifact::Song, it->second->getPath()
		);
	}
	else {
		ERRORLOG( QString( "Invalid type [%1]" )
					  .arg( SoundLibraryInfo::TypeToQString( m_type ) ) );
	}
}

void SoundLibraryTree::actionProperties()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	auto pDB = pHydrogen->getSoundLibraryDatabase();

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// We provide a copy of the recent drumkit to ensure the drumkit
		// is not getting dirty upon saving (in case new properties are
		// stored in the kit but writing it to disk fails).
		auto pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
		DrumkitPropertiesDialog dialog(
			this, pNewDrumkit, DrumkitPropertiesDialog::Action::None,
			pNewDrumkit->getPath()
		);
		dialog.exec();
	}
	else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
		auto pPattern = Pattern::from( it->second );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve pattern [%1] at [%2]" )
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}

		PatternPropertiesDialog dialog(
			this, pPattern, -1, PatternPropertiesDialog::Action::None
		);
		if ( dialog.exec() == QDialog::Accepted ) {
			pPattern->save( pPattern->getPath() );
			pDB->updatePatterns( Event::Trigger::Default );
		}
	}
	else {
		auto pSong = Song::from( it->second );
		if ( pSong == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve song [%1] at [%2]" )
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}

		SongPropertiesDialog dialog(
			this, pSong, SongPropertiesDialog::Action::None
		);
		if ( dialog.exec() == QDialog::Accepted ) {
			pSong->save( pSong->getPath(), true, true );
			pDB->updateSongs( Event::Trigger::Default );
		}
	}
}
void SoundLibraryTree::actionDuplicate()
{
	auto pHydrogen = Hydrogen::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	auto pDB = pHydrogen->getSoundLibraryDatabase();

	if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
		auto pDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit(
			it->second->getPath()
		);
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to find drumkit [%1] at [%2]" )
						  .arg( it->second->getLabel() )
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

		DrumkitPropertiesDialog dialog(
			this, pNewDrumkit,
			static_cast<DrumkitPropertiesDialog::Action>(
				DrumkitPropertiesDialog::Action::Duplicate |
				DrumkitPropertiesDialog::Action::SaveAs
			),
			Filesystem::drumkitPathFromDir(
				H2Core::Filesystem::userDrumkitsDir() + pNewDrumkit->getName()
			)
		);
		dialog.exec();
	}
	else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
		auto pPattern = Pattern::from( it->second );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve pattern [%1] at [%2]" )
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}

		PatternPropertiesDialog dialog(
			this, pPattern, -1,
			static_cast<PatternPropertiesDialog::Action>(
				PatternPropertiesDialog::Action::Duplicate |
				PatternPropertiesDialog::Action::SaveAs
			)
		);
		if ( dialog.exec() == QDialog::Accepted ) {
			if ( pPattern->save( pPattern->getPath() ) ) {
				pDB->updatePatterns( Event::Trigger::Default );
			}
			else {
				QMessageBox::warning(
					this, "Hydrogen", pCommonStrings->getErrorPatternSaved()
				);
			}
		}
	}
	else {
		auto pSong = Song::from( it->second );
		if ( pSong == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve song [%1] at [%2]" )
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}

		SongPropertiesDialog dialog(
			this, pSong,
			static_cast<SongPropertiesDialog::Action>(
				SongPropertiesDialog::Action::Duplicate |
				SongPropertiesDialog::Action::SaveAs
			)
		);
		if ( dialog.exec() == QDialog::Accepted ) {
			pSong->save( pSong->getPath(), true, true );
			pDB->updateSongs( Event::Trigger::Default );
		}
	}
}

void SoundLibraryTree::actionDelete()
{
	auto pHydrogen = Hydrogen::get_instance();

	// Collect all deletable items from the selection.
	struct DeleteCandidate {
		std::shared_ptr<SoundLibraryInfo> pInfo;
		QString sTargetPath;
	};
	std::vector<DeleteCandidate> candidates;

	auto items = selectedItems();
	bool bCurrentItemContained = false;
	for ( const auto& pItem : items ) {
		if ( currentItem() == pItem ) {
			bCurrentItemContained = true;
			break;
		}
	}
	if ( ! bCurrentItemContained ) {
		items << currentItem();
	}

	for ( auto* pItem : items ) {
		auto it = m_registry.find( pItem );
		if ( it == m_registry.end() || it->second == nullptr ) {
			continue;
		}

		if ( it->second->getContext() == Filesystem::Context::System ||
			 it->second->getContext() == Filesystem::Context::SessionReadOnly ) {
			QMessageBox::warning(
				this, "Hydrogen",
				QString( "%1 [%2] " )
					.arg( it->second->getLabel() )
					.arg( it->second->getPath() )
					.append( tr( "is a read-only and can't be deleted." ) )
			);
			continue;
		}

		QString sTargetPath = it->second->getPath();

		if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
			// If we delete a kit containing samples used and loaded in the
			// current song's drumkit, we get into trouble.
			if ( pHydrogen->getSong() == nullptr ||
				 pHydrogen->getSong()->getDrumkit() == nullptr ) {
				continue;
			}
			auto pDrumkit = pHydrogen->getSong()->getDrumkit();

			// For a sample to be contained both the instrument's drumkit path
			// must match the selected one and the instrument has to contain at
			// least one sample with a non-empty, relative path.
			bool bSampleContained = false;
			sTargetPath =
				Filesystem::drumkitDirFromPath( it->second->getPath() );
			for ( const auto& ppInstrument : *pDrumkit->getInstruments() ) {
				if ( ppInstrument == nullptr ||
					 ppInstrument->getDrumkitPath() != it->second->getPath() ) {
					continue;
				}
				for ( const auto& ppComponent :
					  *ppInstrument->getComponents() ) {
					if ( ppComponent == nullptr ) {
						continue;
					}
					for ( const auto& ppLayer : ppComponent->getLayers() ) {
						if ( ppLayer != nullptr &&
							 ppLayer->getSample() != nullptr &&
							 !ppLayer->getSample()->getFilePath().isEmpty() &&
							 ppLayer->getSample()->getFilePath().contains(
								 sTargetPath
							 ) ) {
							bSampleContained = true;
							break;
						}
					}

					if ( bSampleContained ) {
						break;
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
						.arg( it->second->getLabel() )
				);
				continue;
			}
		}

		candidates.push_back( { it->second, sTargetPath } );
	}

	if ( candidates.empty() ) {
		return;
	}

	// Build a single confirmation message listing all items.
	QStringList itemLabels;
	for ( const auto& candidate : candidates ) {
		itemLabels << QString( "\"%1\" [%2]" )
						  .arg( candidate.pInfo->getLabel() )
						  .arg( candidate.sTargetPath );
	}

	if ( QMessageBox::warning(
			 this, "Hydrogen",
			 tr( "Warning, the following will be deleted from "
				 "disk.\nAre you sure?\n\n%1" )
				 .arg( itemLabels.join( "\n" ) ),
			 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel
		 ) == QMessageBox::Cancel ) {
		return;
	}

	QApplication::setOverrideCursor( Qt::WaitCursor );

	bool bDeleteFailed = false;
	for ( const auto& candidate : candidates ) {
		INFOLOG( QString( "Removing %1 [%2] at [%3]" )
					 .arg( SoundLibraryInfo::TypeToQString(
						 candidate.pInfo->getType() ) )
					 .arg( candidate.pInfo->getLabel() )
					 .arg( candidate.sTargetPath ) );
		const bool bOk = Filesystem::rm( candidate.sTargetPath, true );
		if ( !bOk ) {
			bDeleteFailed = true;
		}
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

	if ( bDeleteFailed ) {
		QMessageBox::warning(
			this, "Hydrogen", tr( "Deletion failed." )
		);
	}
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
						  .arg( it->second->getLabel() )
						  .arg( it->second->getPath() ) );
			return;
		}
		// Pass a copy of the kit since we do not want to alter the settings of
		// the original one.
		MainForm::exportDrumkit( std::make_shared<Drumkit>( pDrumkit ) );
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
}
void SoundLibraryTree::actionOnlineImport()
{
	OnlineArtifact::Type type = OnlineArtifact::Type::Pattern;
	switch ( m_type ) {
	case SoundLibraryInfo::Type::Pattern:
		type = OnlineArtifact::Type::Pattern;
		break;
	case SoundLibraryInfo::Type::Song:
		type = OnlineArtifact::Type::Song;
		break;
	case SoundLibraryInfo::Type::Drumkit:
		type = OnlineArtifact::Type::Drumkit;
		break;
	default:
		type = OnlineArtifact::Type::Pattern;
		break;
	}

	OnlineImportDialog dialog( this, type );
	dialog.exec();
}

void SoundLibraryTree::actionAddFolder()
{
	auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	FileDialog fd( this );

	fd.setFileMode( QFileDialog::Directory );
	fd.setDirectory( QDir::home() );
	fd.setWindowTitle( pCommonStrings->getMenuActionAddDirToSoundLibrary() );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QString sDirPath = fd.selectedFiles().first();
	if ( sDirPath.isEmpty() ) {
		return;
	}

	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_modifyCustomLibraryDirsAction(
			sDirPath, SE_modifyCustomLibraryDirsAction::Action::Add
		)
	);
}
void SoundLibraryTree::actionRemoveFolder()
{
	HydrogenApp::get_instance()->pushUndoCommand(
		new SE_modifyCustomLibraryDirsAction(
			currentItem()->toolTip( 0 ),
			SE_modifyCustomLibraryDirsAction::Action::Remove
		)
	);
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

	if ( event->button() == Qt::RightButton && !m_bStandAlone &&
		 currentItem() != nullptr ) {
		QMenu* pMenu = nullptr;
		auto it = m_registry.find( currentItem() );
		if ( it != m_registry.end() && it->second != nullptr ) {
			// Show popup menu for artifacts
			if ( it->second->getContext() == Filesystem::Context::System ||
				 it->second->getContext() ==
					 Filesystem::Context::SessionReadOnly ) {
				if ( it->second->getType() == SoundLibraryInfo::Type::Drumkit ||
					 it->second->getType() == SoundLibraryInfo::Type::Song ) {
					pMenu = m_pPopupMenuReadOnly;
				}
				else {
					pMenu = m_pPopupMenuAddReadOnly;
				}
			}
			else {
				if ( it->second->getType() == SoundLibraryInfo::Type::Drumkit ||
					 it->second->getType() == SoundLibraryInfo::Type::Song ) {
					pMenu = m_pPopupMenu;
				}
				else {
					pMenu = m_pPopupMenuAdd;
				}
			}

			const bool bMultiSelect = selectedItems().size() > 1;
			for ( auto* pAction : pMenu->actions() ) {
				if ( pAction->isSeparator() ) {
					continue;
				}
				if ( !m_multiSelectActions.contains( pAction ) ) {
					pAction->setEnabled( !bMultiSelect );
				}
			}

		}
		else {
			// Popup menu for top-level folders
			for ( const auto& ppItem : m_internalDirs ) {
				if ( ppItem == currentItem() ) {
					pMenu = m_pPopupMenuDirReadOnly;
					break;
				}
			}
			for ( const auto& ppItem : m_customDirs ) {
				if ( ppItem == currentItem() ) {
					pMenu = m_pPopupMenuDir;
					break;
				}
			}
		}

		if ( pMenu != nullptr ) {
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
							.arg( pInstrumentInfo->getLabel() )
					);
					return;
				}
				const auto pTargetInstrument =
					pDrumkit->getInstruments()->find( pInstrumentInfo->getId()
					);
				if ( pTargetInstrument == nullptr ) {
					ERRORLOG(
						QString( "Unable to retrieve instrument [%1](%2) from "
								 "kit [%3]" )
							.arg( pInstrumentInfo->getLabel() )
							.arg( static_cast<int>( pInstrumentInfo->getId() ) )
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
							 .arg( pInstrumentInfo->getLabel() )
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

	const auto items = selectedItems();
	if ( items.isEmpty() ) {
		return;
	}

	QString sMimeText;
	switch ( m_type ) {
		case SoundLibraryInfo::Type::Drumkit: {
			QStringList parts;
			for ( auto* pItem : items ) {
				auto it = m_registry.find( pItem );
				if ( it == m_registry.end() || it->second == nullptr ||
					 it->second->getType() !=
						 SoundLibraryInfo::Type::Instrument ) {
					continue;
				}
				parts << QString( "%1%2%3" )
							 .arg( it->second->getPath() )
							 .arg( HydrogenApp::sMimeSubSeparator )
							 .arg( it->second->getName() );
			}
			if ( parts.isEmpty() ) {
				return;
			}
			sMimeText = HydrogenApp::sMimeDragInstrument +
						HydrogenApp::sMimeSeparator +
						parts.join( HydrogenApp::sMimeSeparator );
			break;
		}
		case SoundLibraryInfo::Type::Pattern: {
			QStringList paths;
			for ( auto* pItem : items ) {
				auto it = m_registry.find( pItem );
				if ( it == m_registry.end() || it->second == nullptr ) {
					continue;
				}
				paths << it->second->getPath();
			}
			if ( paths.isEmpty() ) {
				return;
			}
			sMimeText = HydrogenApp::sMimeDragPattern +
						HydrogenApp::sMimeSeparator +
						paths.join( HydrogenApp::sMimeSeparator );
			break;
		}
		default:
			return;
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
	}
	else {
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
			sPath.remove( 0, 1 );
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
		pDirItem->setFlags( pDirItem->flags() & ~Qt::ItemIsSelectable );
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
		QString sDisplayLabel = ppInfo->getLabel();
		if ( sDisplayLabel.isEmpty() ) {
			// Fallback to filename without extension
			QFileInfo fi( ppInfo->getPath() );
			sDisplayLabel = fi.completeBaseName();
		}
		pFileItem->setText( 0, sDisplayLabel );
		if ( m_type == SoundLibraryInfo::Type::Drumkit ) {
			pFileItem->setIcon( 0, QIcon( sIconPath + "drum.svg" ) );
		}
		else if ( m_type == SoundLibraryInfo::Type::Pattern ) {
			pFileItem->setIcon( 0, QIcon( sIconPath + "pattern-editor.svg" ) );
		}
		else if ( m_type == SoundLibraryInfo::Type::Song ) {
			pFileItem->setIcon( 0, QIcon( sIconPath + "song-editor.svg" ) );
		}
		m_registry[pFileItem] = ppInfo;

		if ( ppInfo->getType() != SoundLibraryInfo::Type::Pattern &&
			 ppInfo->getType() != SoundLibraryInfo::Type::Instrument ) {
			pFileItem->setFlags( pFileItem->flags() & ~Qt::ItemIsSelectable );
		}

		if ( ppInfo->getType() == SoundLibraryInfo::Type::Drumkit ) {
			auto pDrumkitInfo =
				std::dynamic_pointer_cast<DrumkitInfo>( ppInfo );
			if ( pDrumkitInfo != nullptr ) {
				for ( const auto& ppInstrumentInfo :
					  pDrumkitInfo->getInstrumentInfos() ) {
					auto pInstrumentItem = new QTreeWidgetItem( pFileItem );
					QString sInstrumentLabel;
					if ( !ppInstrumentInfo->getName().isEmpty() ) {
						sInstrumentLabel =
							QString( "%1 [%2]" )
								.arg( ppInstrumentInfo->getName() )
								.arg( ppInstrumentInfo->getType() );
					}
					else {
						sInstrumentLabel = ppInstrumentInfo->getType();
					}
					pInstrumentItem->setText( 0, sInstrumentLabel );
					pInstrumentItem->setToolTip( 0, sInstrumentLabel );
					pInstrumentItem->setIcon(
						0, QIcon( sIconPath + "speaker.svg" )
					);
					m_registry[pInstrumentItem] = ppInstrumentInfo;
				}
			}
		}
	}
}
