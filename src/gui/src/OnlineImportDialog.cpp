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
#include "HydrogenApp.h"
#include "CommonStrings.h"

#include <core/Basics/Event.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
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

	// Source URL filter
	if ( !m_disabledSources.isEmpty() &&
		 m_disabledSources.contains( artifact.url ) ) {
		// We filter by sourceUrl. Since we don't store sourceUrl per artifact
		// in OnlineArtifact, we use a match on QUrl from the parent index.
		// For now, this placeholder always passes source filtering unless
		// we add sourceUrl to OnlineArtifact.
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
	  m_pDetailGroup( nullptr ),
	  m_pDetailName( nullptr ),
	  m_pDetailAuthor( nullptr ),
	  m_pDetailDescription( nullptr ),
	  m_pDetailLicense( nullptr ),
	  m_pDetailTags( nullptr ),
	  m_pDetailVersion( nullptr ),
	  m_pDetailExtra( nullptr ),
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

	m_pImporter = new OnlineImporter( this );

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

	HydrogenApp::get_instance()->addEventListener( this );

	loadIndices();
}

OnlineImportDialog::~OnlineImportDialog()
{
	if ( auto pH2App = HydrogenApp::get_instance() ) {
		pH2App->removeEventListener( this );
	}
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

	// --- Detail panel ---
	m_pDetailGroup = new QGroupBox( pCommonStrings->getDetailsLabel(), this );
	auto* pDetailLayout = new QVBoxLayout( m_pDetailGroup );
	pDetailLayout->setSpacing( 2 );

	m_pDetailName = new QLabel( m_pDetailGroup );
	m_pDetailName->setWordWrap( true );
	m_pDetailAuthor = new QLabel( m_pDetailGroup );
	m_pDetailDescription = new QLabel( m_pDetailGroup );
	m_pDetailDescription->setWordWrap( true );
	m_pDetailLicense = new QLabel( m_pDetailGroup );
	m_pDetailTags = new QLabel( m_pDetailGroup );
	m_pDetailTags->setWordWrap( true );
	m_pDetailVersion = new QLabel( m_pDetailGroup );
	m_pDetailExtra = new QLabel( m_pDetailGroup );

	pDetailLayout->addWidget( m_pDetailName );
	pDetailLayout->addWidget( m_pDetailAuthor );
	pDetailLayout->addWidget( m_pDetailDescription );
	pDetailLayout->addWidget( m_pDetailLicense );
	pDetailLayout->addWidget( m_pDetailTags );
	pDetailLayout->addWidget( m_pDetailVersion );
	pDetailLayout->addWidget( m_pDetailExtra );

	m_pDetailGroup->setMaximumHeight( 160 );
	pMainLayout->addWidget( m_pDetailGroup );

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
}

// ─────────────────────────────────────────────────────────────────────────────
// Source menu
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::populateSourceMenu()
{
	m_pSourceMenu->clear();

	const auto repos = Preferences::get_instance()->getOnlineRepos();
	for ( const auto& sUrl : repos ) {
		QAction* pAction = m_pSourceMenu->addAction( sUrl );
		pAction->setCheckable( true );
		pAction->setChecked( true );
		pAction->setData( sUrl );
	}
}

void OnlineImportDialog::onSourceMenuTriggered( QAction* pAction )
{
	Q_UNUSED( pAction );

	// Rebuild disabled sources set from unchecked actions
	QSet<QUrl> disabledSources;
	for ( const auto* pAct : m_pSourceMenu->actions() ) {
		if ( !pAct->isChecked() ) {
			disabledSources.insert( QUrl( pAct->data().toString() ) );
		}
	}
	m_pProxy->setDisabledSources( disabledSources );
}

// ─────────────────────────────────────────────────────────────────────────────
// Index loading
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::loadIndices()
{
	m_allPatterns.clear();
	m_allSongs.clear();
	m_allDrumkits.clear();

	const auto repos = Preferences::get_instance()->getOnlineRepos();
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
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	if ( pArtifact == nullptr ) {
		m_pDetailName->setText( pCommonStrings->getNameDialog() + ": —" );
		m_pDetailAuthor->setText( pCommonStrings->getAuthorDialog() + ": —" );
		m_pDetailDescription->setText(
			pCommonStrings->getDescriptionLabel() + ": —"
		);
		m_pDetailDescription->setToolTip( "" );
		m_pDetailLicense->setText( pCommonStrings->getLicenseDialog() + ": —" );
		m_pDetailTags->setText( pCommonStrings->getTagsLabel() + ": —" );
		m_pDetailVersion->setText( pCommonStrings->getVersionDialog() + ": —" );
		m_pDetailExtra->setText( "" );
		m_pDetailExtra->hide();
		return;
	}

	m_pDetailName->setText(
		pCommonStrings->getNameDialog() + ": " + pArtifact->sName
	);
	m_pDetailAuthor->setText(
		pCommonStrings->getAuthorDialog() + ": " + pArtifact->sAuthor
	);
	// Some drumkits feature a HTML-based description.
	const QString sDescriptionCleaned =
		QTextDocumentFragment::fromHtml( pArtifact->sDescription ).toPlainText();
	m_pDetailDescription->setText(
		pCommonStrings->getDescriptionLabel() + ": " + sDescriptionCleaned
	);
	m_pDetailDescription->setToolTip( sDescriptionCleaned );
	m_pDetailLicense->setText(
		pCommonStrings->getLicenseDialog() + ": " + pArtifact->sLicense
	);
	m_pDetailTags->setText(
		pCommonStrings->getTagsLabel() + ": " + pArtifact->tags.join( ", " )
	);
	m_pDetailVersion->setText(
		pCommonStrings->getVersionDialog() + ": " +
		QString::number( pArtifact->nVersion )
	);

	// Type-specific extra info
	QString sExtra;
	switch ( pArtifact->type ) {
		case OnlineArtifact::Type::Pattern:
			if ( pArtifact->nNotes >= 0 ) {
				sExtra = pCommonStrings->getNoteCountLabel() + ": " +
						 QString::number( pArtifact->nNotes );
			}
			if ( !pArtifact->instrumentTypes.isEmpty() ) {
				sExtra += "  |  " + pCommonStrings->getInstrumentsLabel() +
						  ": " + pArtifact->instrumentTypes.join( ", " );
			}
			break;
		case OnlineArtifact::Type::Song:
			if ( pArtifact->nPatternCount >= 0 ) {
				sExtra = pCommonStrings->getPatternsLabel() + ": " +
						 QString::number( pArtifact->nPatternCount );
			}
			break;
		case OnlineArtifact::Type::Drumkit:
			if ( pArtifact->nInstruments >= 0 ) {
				sExtra = pCommonStrings->getInstrumentsLabel() + ": " +
						 QString::number( pArtifact->nInstruments );
			}
			if ( pArtifact->nComponents >= 0 ) {
				sExtra += "  |  " + pCommonStrings->getComponentsLabel() +
						  ": " + QString::number( pArtifact->nComponents );
			}
			if ( pArtifact->nSamples >= 0 ) {
				sExtra += "  |  " + pCommonStrings->getSamplesLabel() + ": " +
						  QString::number( pArtifact->nSamples );
			}
			break;
	}
	if ( ! sExtra.isEmpty() ) {
		m_pDetailExtra->show();
		m_pDetailExtra->setText( sExtra );
	}
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

// ─────────────────────────────────────────────────────────────────────────────
// Progress event
// ─────────────────────────────────────────────────────────────────────────────

void OnlineImportDialog::onlineImportProgressEvent( int nValue )
{
	if ( nValue == OnlineImporter::nProgressError ) {
		setDownloadingState( false );
		const auto pCommonStrings =
			HydrogenApp::get_instance()->getCommonStrings();
		QMessageBox::warning(
			this, pCommonStrings->getDownloadError(),
			pCommonStrings->getDownloadFailed()
		);
	}
	else if ( nValue == OnlineImporter::nProgressComplete ) {
		setDownloadingState( false );

		auto pDB = Hydrogen::get_instance()->getSoundLibraryDatabase();

		// Re-resolve statuses to reflect newly installed items
		switch ( m_pTypeCombo->currentIndex() ) {
			case 0:
				for ( auto& a : m_allPatterns ) {
					m_pImporter->resolveLocalStatus( a );
				}
				pDB->updatePatterns( Event::Trigger::Default );
				break;
			case 1:
				for ( auto& a : m_allSongs ) {
					m_pImporter->resolveLocalStatus( a );
				}
				pDB->updateSongs( Event::Trigger::Default );
				break;
			case 2:
				for ( auto& a : m_allDrumkits ) {
					m_pImporter->resolveLocalStatus( a );
				}
				pDB->updateDrumkits( Event::Trigger::Default );
				break;
		}

		updateTableForCurrentType();
	}
	else if ( nValue >= 0 && nValue <= 100 ) {
		m_pProgressBar->setValue( nValue );
	}
}
