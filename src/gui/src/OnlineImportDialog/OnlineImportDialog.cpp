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

#include "OnlineImportDialog.h"
#include "OnlineImportSourcesDialog.h"
#include "HydrogenApp.h"
#include "CommonStrings.h"
#include "Skin.h"

#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QTextDocumentFragment>
#include <QToolButton>
#include <QVBoxLayout>

using namespace H2Core;

// ═══════════════════════════════════════════════════════════════════════════════
// OnlineArtifactTableModel
// ═══════════════════════════════════════════════════════════════════════════════

OnlineArtifactTableModel::OnlineArtifactTableModel( QObject* pParent )
	: QAbstractTableModel( pParent )
{
}

OnlineArtifactTableModel::~OnlineArtifactTableModel()
{
}

void OnlineArtifactTableModel::setArtifacts(
	const QVector<OnlineArtifact>& artifacts
)
{
	beginResetModel();
	m_artifacts = artifacts;
	endResetModel();
}

const OnlineArtifact* OnlineArtifactTableModel::artifactAt( int nRow ) const
{
	if ( nRow < 0 || nRow >= m_artifacts.size() ) {
		return nullptr;
	}
	return &m_artifacts[nRow];
}

void OnlineArtifactTableModel::refreshStatuses()
{
	if ( m_artifacts.isEmpty() ) {
		return;
	}
	emit dataChanged(
		index( 0, Column::Status ),
		index( m_artifacts.size() - 1, Column::Status ), { Qt::DisplayRole }
	);
}

int OnlineArtifactTableModel::rowCount( const QModelIndex& parent ) const
{
	if ( parent.isValid() ) {
		return 0;
	}
	return m_artifacts.size();
}

int OnlineArtifactTableModel::columnCount( const QModelIndex& parent ) const
{
	if ( parent.isValid() ) {
		return 0;
	}
	return Column::ColumnCount;
}

QVariant OnlineArtifactTableModel::data( const QModelIndex& index, int role )
	const
{
	if ( !index.isValid() || index.row() >= m_artifacts.size() ) {
		return QVariant();
	}

	const auto& artifact = m_artifacts[index.row()];

	if ( role == Qt::DisplayRole ) {
		switch ( index.column() ) {
			case Column::Name:
				return artifact.sName;
			case Column::Author:
				return artifact.sAuthor;
			case Column::Tags:
				return artifact.tags.join( ", " );
			case Column::Size:
				return formatSize( artifact.size );
			case Column::Status:
				return localStatusToString( artifact.localStatus );
			default:
				return QVariant();
		}
	}

	if ( role == Qt::ToolTipRole && index.column() == Column::Name ) {
		return artifact.sDescription;
	}

	return QVariant();
}

QVariant OnlineArtifactTableModel::headerData(
	int section,
	Qt::Orientation orientation,
	int role
) const
{
	if ( orientation != Qt::Horizontal || role != Qt::DisplayRole ) {
		return QVariant();
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	switch ( section ) {
		case Column::Name:
			return pCommonStrings->getNameDialog();
		case Column::Author:
			return pCommonStrings->getAuthorDialog();
		case Column::Tags:
			return pCommonStrings->getTagsLabel();
		case Column::Size:
			return pCommonStrings->getSizeLabel();
		case Column::Status:
			return pCommonStrings->getStatusLabel();
		default:
			return QVariant();
	}
}

QString OnlineArtifactTableModel::localStatusToString(
	OnlineArtifact::LocalStatus status
)
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	switch ( status ) {
		case OnlineArtifact::LocalStatus::NotInstalled:
			return pCommonStrings->getMenuActionNew();
		case OnlineArtifact::LocalStatus::Installed:
			return pCommonStrings->getOnlineInstalledStatus();
		case OnlineArtifact::LocalStatus::Modified:
			return pCommonStrings->getOnlineModifiedStatus();
		case OnlineArtifact::LocalStatus::UpdateAvailable:
			return pCommonStrings->getOnlineUpdateStatus();
		default:
			return QString();
	}
}

QString OnlineArtifactTableModel::formatSize( qint64 nBytes )
{
	if ( nBytes < 0 ) {
		return QString( "?" );
	}
	if ( nBytes < 1024 ) {
		return QString( "%1 B" ).arg( nBytes );
	}
	if ( nBytes < 1024 * 1024 ) {
		return QString( "%1 KB" ).arg( nBytes / 1024 );
	}
	return QString( "%1 MB" ).arg( nBytes / ( 1024 * 1024 ) );
}

// ═══════════════════════════════════════════════════════════════════════════════
// OnlineArtifactFilterProxy
// ═══════════════════════════════════════════════════════════════════════════════

OnlineArtifactFilterProxy::OnlineArtifactFilterProxy( QObject* pParent )
	: QSortFilterProxyModel( pParent ), m_pArtifacts( nullptr )
{
}

OnlineArtifactFilterProxy::~OnlineArtifactFilterProxy()
{
}

void OnlineArtifactFilterProxy::setTextFilter( const QString& sText )
{
	m_sTextFilter = sText;
	invalidateFilter();
}

void OnlineArtifactFilterProxy::setDisabledSources(
	const QSet<QUrl>& disabledUrls
)
{
	m_disabledSources = disabledUrls;
	invalidateFilter();
}

void OnlineArtifactFilterProxy::setFullArtifactList(
	const QVector<OnlineArtifact>* pArtifacts
)
{
	m_pArtifacts = pArtifacts;
}

bool OnlineArtifactFilterProxy::filterAcceptsRow(
	int nSourceRow,
	const QModelIndex& sourceParent
) const
{
	Q_UNUSED( sourceParent );

	if ( m_pArtifacts == nullptr || nSourceRow >= m_pArtifacts->size() ) {
		return false;
	}

	const auto& artifact = ( *m_pArtifacts )[nSourceRow];

	// Source URL filter — hide artifacts from disabled sources
	if ( !m_disabledSources.isEmpty() &&
		 m_disabledSources.contains( artifact.sourceUrl ) ) {
		return false;
	}

	// Text filter
	if ( !m_sTextFilter.isEmpty() ) {
		const bool bNameMatch =
			artifact.sName.contains( m_sTextFilter, Qt::CaseInsensitive );
		const bool bTagMatch = artifact.tags.join( " " ).contains(
			m_sTextFilter, Qt::CaseInsensitive
		);
		const bool bAuthorMatch =
			artifact.sAuthor.contains( m_sTextFilter, Qt::CaseInsensitive );
		if ( !bNameMatch && !bTagMatch && !bAuthorMatch ) {
			return false;
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// OnlineImportDialog
// ═══════════════════════════════════════════════════════════════════════════════

OnlineImportDialog::OnlineImportDialog(
	QWidget* pParent,
	OnlineArtifact::Type type
)
	: QDialog( pParent ),
	  m_pTypeCombo( nullptr ),
	  m_pSearchLine( nullptr ),
	  m_pSourceButton( nullptr ),
	  m_pSourceMenu( nullptr ),
	  m_pTableView( nullptr ),
	  m_pDetailWidget( nullptr ),
	  m_pDetailNameLabel( nullptr ),
	  m_pDetailNameText( nullptr ),
	  m_pDetailAuthorLabel( nullptr ),
	  m_pDetailAuthorText( nullptr ),
	  m_pDetailDescriptionLabel( nullptr ),
	  m_pDetailDescriptionText( nullptr ),
	  m_pDetailLicenseLabel( nullptr ),
	  m_pDetailLicenseText( nullptr ),
	  m_pDetailTagsLabel( nullptr ),
	  m_pDetailTagsText( nullptr ),
	  m_pDetailVersionLabel( nullptr ),
	  m_pDetailVersionText( nullptr ),
	  m_pDetailNotesLabel( nullptr ),
	  m_pDetailNotesText( nullptr ),
	  m_pDetailPatternCountLabel( nullptr ),
	  m_pDetailPatternCountText( nullptr ),
	  m_pDetailInstrumentsLabel( nullptr ),
	  m_pDetailInstrumentsText( nullptr ),
	  m_pDetailComponentsLabel( nullptr ),
	  m_pDetailComponentsText( nullptr ),
	  m_pDetailSamplesLabel( nullptr ),
	  m_pDetailSamplesText( nullptr ),
	  m_pProgressBar( nullptr ),
	  m_pCancelButton( nullptr ),
	  m_pDownloadButton( nullptr ),
	  m_pModel( nullptr ),
	  m_pProxy( nullptr ),
	  m_pImporter( nullptr ),
	  m_bDownloading( false )
{
	setWindowFlags(
		windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint
	);
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	setWindowTitle( pCommonStrings->getMenuActionOnlineImport() );
	setMinimumSize(
		OnlineImportDialog::nMinimumWidth, OnlineImportDialog::nMinimumHeight
	);

	m_pImporter = new OnlineImporter( HydrogenApp::pHydrogen(), this );
	connect( m_pImporter, &OnlineImporter::batchFinished, [&]() {
		setDownloadingState( false );

		auto pDB = HydrogenApp::pHydrogen()->getSoundLibraryDatabase();

		// Re-resolve statuses to reflect newly installed items
		switch ( m_pTypeCombo->currentIndex() ) {
			case 0:
				pDB->updatePatterns( Event::Trigger::Default );
				for ( auto& a : m_allPatterns ) {
					m_pImporter->resolveLocalStatus( a );
				}
				break;
			case 1:
				pDB->updateSongs( Event::Trigger::Default );
				for ( auto& a : m_allSongs ) {
					m_pImporter->resolveLocalStatus( a );
				}
				break;
			case 2:
				pDB->updateDrumkits( Event::Trigger::Default );
				for ( auto& a : m_allDrumkits ) {
					m_pImporter->resolveLocalStatus( a );
				}
				break;
		}

		updateTableForCurrentType();
	} );
	connect(
		m_pImporter, &OnlineImporter::downloadProgress,
		[&]( qint64 nDone, qint64 nTotal ) {
			m_pProgressBar->setValue(
				static_cast<int>( ( ( nDone + 1 ) * 100 ) / nTotal )
			);
		}
	);
	connect(
		m_pImporter, &OnlineImporter::downloadFinished,
		[&]( const QString&, bool, const QString& sError ) {
			if ( sError.isEmpty() ) {
				return;
			}
			const auto pCommonStrings =
				HydrogenApp::get_instance()->getCommonStrings();
			QMessageBox::warning(
				this,
				pCommonStrings->getDownloadError(),
				QString( "%1\n\n%2" )
					.arg( pCommonStrings->getDownloadFailed() )
					.arg( sError )
			);
		}
	);

	buildLayout();
	populateSourceMenu();

	// Set the initial type selection before loading indices so
	// updateTableForCurrentType() shows the correct category.
	int nInitialIndex = 0;
	switch ( type ) {
		case OnlineArtifact::Type::Pattern:
			nInitialIndex = 0;
			break;
		case OnlineArtifact::Type::Song:
			nInitialIndex = 1;
			break;
		case OnlineArtifact::Type::Drumkit:
			nInitialIndex = 2;
			break;
	}
	m_pTypeCombo->setCurrentIndex( nInitialIndex );

	loadIndices();
}

OnlineImportDialog::~OnlineImportDialog()
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout construction
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::buildLayout()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pOverallLayout = new QVBoxLayout( this );
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pOverallLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setWidgetResizable( true );
	pOverallLayout->addWidget( pScrollArea );

	auto pScrollAreaContent = new QWidget( pScrollArea );
	pScrollAreaContent->setMinimumSize(
		OnlineImportDialog::nMinimumWidth - 5,
		OnlineImportDialog::nMinimumHeight - 5
	);
	pScrollArea->setWidget( pScrollAreaContent );
	auto* pMainLayout = new QVBoxLayout( pScrollAreaContent );

	// --- Top bar: type combo, search, source button ---
	auto* pTopBar = new QHBoxLayout();

	m_pTypeCombo = new QComboBox( pScrollAreaContent );
	m_pTypeCombo->addItem( pCommonStrings->getPatternsLabel() );
	m_pTypeCombo->addItem( pCommonStrings->getSongsLabel() );
	m_pTypeCombo->addItem( pCommonStrings->getDrumkitsLabel() );
	pTopBar->addWidget( m_pTypeCombo );

	m_pSearchLine = new QLineEdit( pScrollAreaContent );
	m_pSearchLine->setPlaceholderText( pCommonStrings->getSearchPlaceholder() );
	m_pSearchLine->setClearButtonEnabled( true );
	m_pSearchLine->setObjectName( "OnlineImportDialogSearchLine" );
	pTopBar->addWidget( m_pSearchLine, 1 );

	m_pSourceButton = new QToolButton( pScrollAreaContent );
	m_pSourceButton->setContentsMargins( 0, 0, 0, 10 );
	m_pSourceButton->setText( pCommonStrings->getSourcesLabel() );
	m_pSourceButton->setPopupMode( QToolButton::InstantPopup );
	m_pSourceMenu = new QMenu( m_pSourceButton );
	m_pSourceButton->setMenu( m_pSourceMenu );
	pTopBar->addWidget( m_pSourceButton );

	pMainLayout->addLayout( pTopBar );

	// --- Table view ---
	m_pModel = new OnlineArtifactTableModel( pScrollAreaContent );
	m_pProxy = new OnlineArtifactFilterProxy( pScrollAreaContent );
	m_pProxy->setSourceModel( m_pModel );

	m_pTableView = new QTableView( pScrollAreaContent );
	m_pTableView->setModel( m_pProxy );
	m_pTableView->setSelectionMode( QAbstractItemView::ExtendedSelection );
	m_pTableView->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_pTableView->setSortingEnabled( true );
	m_pTableView->setAlternatingRowColors( true );
	m_pTableView->verticalHeader()->hide();
	m_pTableView->horizontalHeader()->setStretchLastSection( true );
	m_pTableView->horizontalHeader()->setSectionResizeMode(
		QHeaderView::ResizeToContents
	);

	pMainLayout->addWidget( m_pTableView, 1 );

	// --- Detail panel (InfoView-style QGridLayout) ---
	m_pDetailWidget = new QWidget( pScrollAreaContent );
	m_pDetailWidget->setObjectName( "OnlineImportDetailWidget" );

	auto* pDetailGridLayout = new QGridLayout( m_pDetailWidget );
	pDetailGridLayout->setSpacing( 0 );
	pDetailGridLayout->setContentsMargins( 3, 0, 3, 0 );
	pDetailGridLayout->setColumnStretch( 0, 0 );
	pDetailGridLayout->setColumnStretch( 1, 0 );
	pDetailGridLayout->setColumnStretch( 2, 1 );

	// Helper lambda to add a label/value row to the grid.
	auto addDetailRow = [pDetailGridLayout]( const QString& sLabel,
											 QLabel** ppLabel,
											 QLabel** ppText ) {
		const int nRow = pDetailGridLayout->rowCount();

		auto* pLabel = new QLabel( sLabel );
		pLabel->setFixedHeight( OnlineImportDialog::nDetailSectionLineHeight );
		pLabel->setObjectName( "OnlineImportDetailLabel" );
		pDetailGridLayout->addWidget( pLabel, nRow, 0 );
		*ppLabel = pLabel;

		auto* pText = new QLabel();
		pText->setFixedHeight( OnlineImportDialog::nDetailSectionLineHeight );
		pText->setObjectName( "OnlineImportDetailText" );
		pText->setWordWrap( true );
		pDetailGridLayout->addWidget( pText, nRow, 2 );
		*ppText = pText;
	};

	addDetailRow( pCommonStrings->getNameDialog(),
				  &m_pDetailNameLabel, &m_pDetailNameText );
	addDetailRow( pCommonStrings->getAuthorDialog(),
				  &m_pDetailAuthorLabel, &m_pDetailAuthorText );
	addDetailRow( pCommonStrings->getDescriptionLabel(),
				  &m_pDetailDescriptionLabel, &m_pDetailDescriptionText );
	addDetailRow( pCommonStrings->getLicenseDialog(),
				  &m_pDetailLicenseLabel, &m_pDetailLicenseText );
	addDetailRow( pCommonStrings->getTagsLabel(),
				  &m_pDetailTagsLabel, &m_pDetailTagsText );
	addDetailRow( pCommonStrings->getVersionDialog(),
				  &m_pDetailVersionLabel, &m_pDetailVersionText );

	// Pattern-specific rows
	addDetailRow( pCommonStrings->getNoteCountLabel(),
				  &m_pDetailNotesLabel, &m_pDetailNotesText );

	// Song-specific rows
	addDetailRow( pCommonStrings->getPatternsLabel(),
				  &m_pDetailPatternCountLabel, &m_pDetailPatternCountText );

	// Drumkit-specific rows
	addDetailRow( pCommonStrings->getInstrumentsLabel(),
				  &m_pDetailInstrumentsLabel, &m_pDetailInstrumentsText );
	addDetailRow( pCommonStrings->getComponentsLabel(),
				  &m_pDetailComponentsLabel, &m_pDetailComponentsText );
	addDetailRow( pCommonStrings->getSamplesLabel(),
				  &m_pDetailSamplesLabel, &m_pDetailSamplesText );

	auto pSeparator = new QFrame( pScrollAreaContent );
	pSeparator->setFixedWidth( 1 );
	pSeparator->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	pDetailGridLayout->addWidget(
		pSeparator, 0, 1, pDetailGridLayout->rowCount(), 1
	);

	pMainLayout->addWidget( m_pDetailWidget );

	updateDetailPanel( nullptr );

	// --- Bottom bar: progress, cancel, download ---
	auto* pBottomBar = new QHBoxLayout();

	m_pProgressBar = new QProgressBar( pScrollAreaContent );
	m_pProgressBar->setRange( 0, 100 );
	m_pProgressBar->setValue( 0 );
	m_pProgressBar->setVisible( false );
	pBottomBar->addWidget( m_pProgressBar, 1 );

	m_pCancelButton = new QPushButton( pCommonStrings->getButtonClose(), this );
	pBottomBar->addWidget( m_pCancelButton );

	m_pDownloadButton =
		new QPushButton( pCommonStrings->getButtonDownload(), this );
	m_pDownloadButton->setEnabled( false );
	pBottomBar->addWidget( m_pDownloadButton );

	pMainLayout->addLayout( pBottomBar );

	// --- Connections ---
	connect(
		m_pTypeCombo, QOverload<int>::of( &QComboBox::currentIndexChanged ),
		this, &OnlineImportDialog::onTypeChanged
	);
	connect(
		m_pSearchLine, &QLineEdit::textChanged, this,
		&OnlineImportDialog::onSearchTextChanged
	);
	connect(
		m_pSourceMenu, &QMenu::triggered, this,
		&OnlineImportDialog::onSourceMenuTriggered
	);
	connect(
		m_pDownloadButton, &QPushButton::clicked, this,
		&OnlineImportDialog::onDownloadClicked
	);
	connect(
		m_pCancelButton, &QPushButton::clicked, this,
		&OnlineImportDialog::onCancelClicked
	);
	connect(
		m_pTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &OnlineImportDialog::onSelectionChanged
	);

	updateStyleSheet();
}

// ─────────────────────────────────────────────────────────────────────────────
// Source menu
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::populateSourceMenu()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_pSourceMenu->clear();

	const auto repos = HydrogenApp::pPreferences()->getOnlineRepos();
	for ( const auto& sUrl : repos ) {
		QAction* pAction = m_pSourceMenu->addAction( sUrl );
		pAction->setCheckable( true );
		pAction->setChecked( true );
		pAction->setData( sUrl );
	}

	// Separator + Edit action (always present)
	m_pSourceMenu->addSeparator();
	auto pEditAction =
		m_pSourceMenu->addAction( pCommonStrings->getEditButton() );
	connect(
		pEditAction, &QAction::triggered, this,
		&OnlineImportDialog::onEditSources
	);
}

void OnlineImportDialog::onSourceMenuTriggered( QAction* pAction )
{
	Q_UNUSED( pAction );

	// Rebuild disabled sources set from unchecked actions
	QSet<QUrl> disabledSources;
	for ( const auto* pAct : m_pSourceMenu->actions() ) {
		if ( !pAct->isCheckable() ) {
			continue;
		}
		if ( !pAct->isChecked() ) {
			disabledSources.insert( QUrl( pAct->data().toString() ) );
		}
	}
	m_pProxy->setDisabledSources( disabledSources );
}

void OnlineImportDialog::onEditSources()
{
	OnlineImportSourcesDialog dialog( this );
	if ( dialog.exec() != QDialog::Accepted ) {
		return;
	}

	const QStringList repos = dialog.getSources();
	HydrogenApp::pPreferences()->setOnlineRepos( repos );

	// Reload indices and refresh source menu
	loadIndices();
	populateSourceMenu();
}

// ─────────────────────────────────────────────────────────────────────────────
// Index loading
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::loadIndices()
{
	m_allPatterns.clear();
	m_allSongs.clear();
	m_allDrumkits.clear();

	const auto repos = HydrogenApp::pPreferences()->getOnlineRepos();
	auto indices = m_pImporter->fetchAllIndices( repos );

	for ( auto& index : indices ) {
		m_pImporter->resolveAllLocalStatuses( index );
		m_allPatterns.append( index.patterns );
		m_allSongs.append( index.songs );
		m_allDrumkits.append( index.drumkits );
	}

	updateTableForCurrentType();
}

// ─────────────────────────────────────────────────────────────────────────────
// Type switching
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::onTypeChanged( int nIndex )
{
	Q_UNUSED( nIndex );
	updateTableForCurrentType();
}

void OnlineImportDialog::updateTableForCurrentType()
{
	const QVector<OnlineArtifact>* pData = nullptr;

	switch ( m_pTypeCombo->currentIndex() ) {
		case 0:
			pData = &m_allPatterns;
			break;
		case 1:
			pData = &m_allSongs;
			break;
		case 2:
			pData = &m_allDrumkits;
			break;
		default:
			pData = &m_allPatterns;
			break;
	}

	m_pModel->setArtifacts( *pData );
	m_pProxy->setFullArtifactList( pData );
	m_pProxy->invalidate();

	updateDetailPanel( nullptr );
	updateDownloadButton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Search filtering
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::onSearchTextChanged( const QString& sText )
{
	m_pProxy->setTextFilter( sText );
}

// ─────────────────────────────────────────────────────────────────────────────
// Selection / detail panel
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::onSelectionChanged()
{
	const auto current = m_pTableView->selectionModel()->currentIndex();
	const OnlineArtifact* pArtifact = nullptr;

	if ( current.isValid() ) {
		// Map proxy row to source row
		const auto sourceIndex = m_pProxy->mapToSource( current );
		pArtifact = m_pModel->artifactAt( sourceIndex.row() );
	}

	updateDetailPanel( pArtifact );
	updateDownloadButton();
}

void OnlineImportDialog::updateDetailPanel( const OnlineArtifact* pArtifact )
{
	auto setText = [&]( QLabel* pLabel, const QString& sText ) {
		pLabel->setToolTip( sText.simplified() );
		pLabel->setText( Skin::trimTextToFitWidth(
			sText.simplified(), pLabel->font(), pLabel->width(),
			QMargins( 10, 0, 0, 0 )
		) );
	};

	auto setRow = [&]( QLabel* pLabel, QLabel* pText, const QString& sText ) {
		if ( sText.isEmpty() ) {
			pLabel->hide();
			pText->hide();
			pText->clear();
			pText->setToolTip( "" );
		}
		else {
			pLabel->show();
			pText->show();
			setText( pText, sText );
		}
	};

	int nRows = 0;
	// Update visibility of type-specific rows first.
	switch (  m_pTypeCombo->currentIndex() ) {
		case 1:
			// Song
			m_pDetailNotesLabel->hide();
			m_pDetailNotesText->hide();
			m_pDetailPatternCountLabel->show();
			m_pDetailPatternCountText->show();
			m_pDetailInstrumentsLabel->hide();
			m_pDetailInstrumentsText->hide();
			m_pDetailComponentsLabel->hide();
			m_pDetailComponentsText->hide();
			m_pDetailSamplesLabel->hide();
			m_pDetailSamplesText->hide();
			nRows = 7;
			break;
		case 2:
			// Drumkit
			m_pDetailNotesLabel->hide();
			m_pDetailNotesText->hide();
			m_pDetailPatternCountLabel->hide();
			m_pDetailPatternCountText->hide();
			m_pDetailInstrumentsLabel->show();
			m_pDetailInstrumentsText->show();
			m_pDetailComponentsLabel->show();
			m_pDetailComponentsText->show();
			m_pDetailSamplesLabel->show();
			m_pDetailSamplesText->show();
			nRows = 10;
			break;
		case 0:
		default:
			// Pattern
			m_pDetailNotesLabel->show();
			m_pDetailNotesText->show();
			m_pDetailPatternCountLabel->hide();
			m_pDetailPatternCountText->hide();
			m_pDetailInstrumentsLabel->show();
			m_pDetailInstrumentsText->show();
			m_pDetailComponentsLabel->hide();
			m_pDetailComponentsText->hide();
			m_pDetailSamplesLabel->hide();
			m_pDetailSamplesText->hide();
			nRows = 8;
			break;
	};
	// m_pDetailWidget->setMinimumHeight(
	// 	nRows * OnlineImportDialog::nDetailSectionLineHeight
	// );

	if ( pArtifact == nullptr ) {
		m_pDetailNameText->clear();
		m_pDetailNameText->setToolTip( "" );
		m_pDetailAuthorText->clear();
		m_pDetailAuthorText->setToolTip( "" );
		m_pDetailDescriptionText->clear();
		m_pDetailDescriptionText->setToolTip( "" );
		m_pDetailLicenseText->clear();
		m_pDetailLicenseText->setToolTip( "" );
		m_pDetailTagsText->clear();
		m_pDetailTagsText->setToolTip( "" );
		m_pDetailVersionText->clear();
		m_pDetailVersionText->setToolTip( "" );
		m_pDetailNotesText->clear();
		m_pDetailNotesText->setToolTip( "" );
		m_pDetailPatternCountText->clear();
		m_pDetailPatternCountText->setToolTip( "" );
		m_pDetailInstrumentsText->clear();
		m_pDetailInstrumentsText->setToolTip( "" );
		m_pDetailComponentsText->clear();
		m_pDetailComponentsText->setToolTip( "" );
		m_pDetailSamplesText->clear();
		m_pDetailSamplesText->setToolTip( "" );
		return;
	}

	// Some drumkits feature a HTML-based description.
	const QString sDescriptionCleaned =
		QTextDocumentFragment::fromHtml( pArtifact->sDescription ).toPlainText();

	setText( m_pDetailNameText, pArtifact->sName );
	setText( m_pDetailAuthorText, pArtifact->sAuthor );
	setText( m_pDetailDescriptionText, sDescriptionCleaned );
	setText( m_pDetailLicenseText, pArtifact->sLicense );
	setText( m_pDetailTagsText, pArtifact->tags.join( ", " ) );
	setText( m_pDetailVersionText, QString::number( pArtifact->nVersion ) );

	// Show type-specific rows.
	switch ( pArtifact->type ) {
		case OnlineArtifact::Type::Pattern:
			setRow(
				m_pDetailNotesLabel, m_pDetailNotesText,
				pArtifact->nNotes >= 0 ? QString::number( pArtifact->nNotes )
									   : QString()
			);
			setRow(
				m_pDetailInstrumentsLabel, m_pDetailInstrumentsText,
				pArtifact->instrumentTypes.join( ", " )
			);
			break;
		case OnlineArtifact::Type::Song:
			setRow(
				m_pDetailPatternCountLabel, m_pDetailPatternCountText,
				pArtifact->nPatternCount >= 0
					? QString::number( pArtifact->nPatternCount )
					: QString()
			);
			break;
		case OnlineArtifact::Type::Drumkit:
			auto sInstrument = pArtifact->nInstruments >= 0
								   ? QString::number( pArtifact->nInstruments )
								   : QString();
			if ( pArtifact->instrumentTypes.size() > 0 ) {
				sInstrument.append( QString( ": %1" ).arg(
					pArtifact->instrumentTypes.join( ", " )
				) );
			}
			setRow(
				m_pDetailInstrumentsLabel, m_pDetailInstrumentsText, sInstrument
			);
			setRow(
				m_pDetailComponentsLabel, m_pDetailComponentsText,
				pArtifact->nComponents >= 0
					? QString::number( pArtifact->nComponents )
					: QString()
			);
			setRow(
				m_pDetailSamplesLabel, m_pDetailSamplesText,
				pArtifact->nSamples >= 0
					? QString::number( pArtifact->nSamples )
					: QString()
			);
			break;
	}
}

void OnlineImportDialog::updateStyleSheet()
{
	const auto pColorTheme = HydrogenApp::pPreferences()->getColorTheme();

	const auto borderColor = pColorTheme->m_windowColor.darker( 140 );
	const auto separatorColor = pColorTheme->m_windowColor;

	const auto backgroundColor = pColorTheme->m_baseColor;
	const QColor textColor = Skin::moreBlackThanWhite( backgroundColor )
							   ? Qt::white
							   : Qt::black;

	m_pDetailWidget->setStyleSheet( QString( "            \
QWidget#OnlineImportDetailWidget {				\
    background-color: %1;						\
    border-left: 1px solid %3;					\
    border-right: 1px solid %3;					\
    border-bottom: 1px solid %3;				\
}												\
QFrame {										\
    background-color: %4;						\
}												\
QLabel#OnlineImportDetailLabel {				\
    background-color: %1;						\
    color: %2;									\
    border-bottom: 1px solid %4;				\
    padding: 4px;								\
}												\
QLabel#OnlineImportDetailText {					\
    background-color: %1;						\
    color: %2;									\
    border-bottom: 1px solid %4;				\
    padding: 4px;								\
}												\
" )
										.arg( backgroundColor.name() )
										.arg( textColor.name() )
										.arg( borderColor.name() )
										.arg( separatorColor.name() )
	);
	setStyleSheet( QString( "                   \
QLineEdit#OnlineImportDialogSearchLine {		\
    background-color: %1;						\
    color: %2;									\
}												\
" )
					   .arg( pColorTheme->m_spinBoxColor.name() )
					   .arg( pColorTheme->m_spinBoxTextColor.name() ) );
}

// ─────────────────────────────────────────────────────────────────────────────
// Download action
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::updateDownloadButton()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto selection = m_pTableView->selectionModel()->selectedRows();
	const int nCount = selection.size();

	if ( nCount == 0 ) {
		m_pDownloadButton->setText( pCommonStrings->getButtonDownload() );
		m_pDownloadButton->setEnabled( false );
	}
	else {
		m_pDownloadButton->setText(
			pCommonStrings->getButtonDownloadN().arg( nCount )
		);
		m_pDownloadButton->setEnabled( !m_bDownloading );
	}
}

void OnlineImportDialog::onDownloadClicked()
{
	const auto selectedRows = m_pTableView->selectionModel()->selectedRows();
	if ( selectedRows.isEmpty() ) {
		return;
	}

	QVector<OnlineArtifact> artifacts;
	artifacts.reserve( selectedRows.size() );

	for ( const auto& proxyIndex : selectedRows ) {
		const auto sourceIndex = m_pProxy->mapToSource( proxyIndex );
		const auto* pArtifact = m_pModel->artifactAt( sourceIndex.row() );
		if ( pArtifact != nullptr ) {
			artifacts.append( *pArtifact );
		}
	}

	if ( artifacts.isEmpty() ) {
		return;
	}

	setDownloadingState( true );
	m_pImporter->downloadArtifactsAsync( artifacts );
}

void OnlineImportDialog::onCancelClicked()
{
	if ( m_bDownloading ) {
		m_pImporter->abort();
		setDownloadingState( false );
	}
	else {
		accept();
	}
}

void OnlineImportDialog::setDownloadingState( bool bDownloading )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	m_bDownloading = bDownloading;

	m_pTableView->setEnabled( !bDownloading );
	m_pTypeCombo->setEnabled( !bDownloading );
	m_pSearchLine->setEnabled( !bDownloading );
	m_pSourceButton->setEnabled( !bDownloading );
	m_pProgressBar->setVisible( bDownloading );
	m_pProgressBar->setValue( 0 );

	if ( bDownloading ) {
		m_pCancelButton->setText( pCommonStrings->getButtonCancel() );
		m_pDownloadButton->setEnabled( false );
	}
	else {
		m_pCancelButton->setText( pCommonStrings->getButtonClose() );
		updateDownloadButton();
	}
}
