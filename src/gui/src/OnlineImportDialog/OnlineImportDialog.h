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

#ifndef ONLINE_IMPORT_DIALOG_H
#define ONLINE_IMPORT_DIALOG_H

#include "EventListener.h"

#include <core/Object.h>
#include <core/OnlineImporter.h>

#include <QAbstractTableModel>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QMenu;
class QProgressBar;
class QPushButton;
class QTableView;
class QToolButton;
class QWidget;

// ─────────────────────────────────────────────────────────────────────────────
// OnlineArtifactTableModel
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Table model exposing a flat list of OnlineArtifact entries with columns:
 * Name, Author, Tags, Size, Status.
 *
 * \ingroup docGUI
 */
class OnlineArtifactTableModel : public QAbstractTableModel,
                                 public H2Core::Object<OnlineArtifactTableModel>
{
	H2_OBJECT(OnlineArtifactTableModel)
	Q_OBJECT

public:
	enum Column {
		Name = 0,
		Author,
		Tags,
		Size,
		Status,
		ColumnCount
	};

	explicit OnlineArtifactTableModel( QObject* pParent = nullptr );
	~OnlineArtifactTableModel() override;

	/** Replace the model's backing data with \a artifacts. */
	void setArtifacts( const QVector<H2Core::OnlineArtifact>& artifacts );

	/** Returns the artifact at the given model \a row, or nullptr if invalid. */
	const H2Core::OnlineArtifact* artifactAt( int nRow ) const;

	/** Refreshes the status column for all rows. */
	void refreshStatuses();

	// QAbstractTableModel interface
	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
	QVariant data( const QModelIndex& index,
	               int role = Qt::DisplayRole ) const override;
	QVariant headerData( int section, Qt::Orientation orientation,
	                     int role = Qt::DisplayRole ) const override;

private:
	QVector<H2Core::OnlineArtifact> m_artifacts;

	static QString localStatusToString( H2Core::OnlineArtifact::LocalStatus status );
	static QString formatSize( qint64 nBytes );
};

// ─────────────────────────────────────────────────────────────────────────────
// OnlineArtifactFilterProxy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Proxy model providing combined text + source URL filtering on top of
 * OnlineArtifactTableModel.
 *
 * Text filtering matches against Name and Tags columns.
 * Source filtering hides artifacts whose sourceUrl is in the disabled set.
 *
 * \ingroup docGUI
 */
class OnlineArtifactFilterProxy : public QSortFilterProxyModel,
                                  public H2Core::Object<OnlineArtifactFilterProxy>
{
	H2_OBJECT(OnlineArtifactFilterProxy)
	Q_OBJECT

public:
	explicit OnlineArtifactFilterProxy( QObject* pParent = nullptr );
	~OnlineArtifactFilterProxy() override;

	/** Set the text filter string (matches name and tags). */
	void setTextFilter( const QString& sText );

	/** Set URLs to exclude from results. */
	void setDisabledSources( const QSet<QUrl>& disabledUrls );

	/** The full artifact list including source URLs for filtering. */
	void setFullArtifactList( const QVector<H2Core::OnlineArtifact>* pArtifacts );

protected:
	bool filterAcceptsRow( int nSourceRow,
	                       const QModelIndex& sourceParent ) const override;

private:
	QString m_sTextFilter;
	QSet<QUrl> m_disabledSources;
	const QVector<H2Core::OnlineArtifact>* m_pArtifacts;
};

// ─────────────────────────────────────────────────────────────────────────────
// OnlineImportDialog
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Modern online import dialog providing table-based browsing of remote
 * sound library artifacts with search filtering, source toggling,
 * multi-select, and batch downloading with progress display.
 *
 * Replaces the legacy SoundLibraryOnlineImportDialog.
 *
 * \ingroup docGUI
 */
class OnlineImportDialog : public QDialog,
                           public EventListener,
                           public H2Core::Object<OnlineImportDialog>
{
	H2_OBJECT(OnlineImportDialog)
	Q_OBJECT

public:
	static constexpr int nDetailSectionLineHeight = 24;
	static constexpr int nMinimumHeight = 800;
	static constexpr int nMinimumWidth = 900;

	/**
	 * \param type Initial artifact type to show in the combo box.
	 */
	explicit OnlineImportDialog( QWidget* pParent,
	                             H2Core::OnlineArtifact::Type type =
	                                 H2Core::OnlineArtifact::Type::Pattern );
	~OnlineImportDialog() override;

	// EventListener overrides
	void onlineImportProgressEvent( int nValue ) override;

private slots:
	void onTypeChanged( int nIndex );
	void onSearchTextChanged( const QString& sText );
	void onSelectionChanged();
	void onDownloadClicked();
	void onCancelClicked();
	void onSourceMenuTriggered( QAction* pAction );
	void onEditSources();

private:
	void buildLayout();
	void populateSourceMenu();
	void loadIndices();
	void updateTableForCurrentType();
	void updateDetailPanel( const H2Core::OnlineArtifact* pArtifact );
	void updateDetailStyleSheet();
	void updateDownloadButton();
	void setDownloadingState( bool bDownloading );

	// Merged index data from all repos
	QVector<H2Core::OnlineArtifact> m_allPatterns;
	QVector<H2Core::OnlineArtifact> m_allSongs;
	QVector<H2Core::OnlineArtifact> m_allDrumkits;

	// Widgets
	QComboBox* m_pTypeCombo;
	QLineEdit* m_pSearchLine;
	QToolButton* m_pSourceButton;
	QMenu* m_pSourceMenu;
	QTableView* m_pTableView;

	// Detail panel (InfoView-style QGridLayout)
	QWidget* m_pDetailWidget;
	// Common label pairs (always visible): [label, value]
	QLabel* m_pDetailNameLabel;
	QLabel* m_pDetailNameText;
	QLabel* m_pDetailAuthorLabel;
	QLabel* m_pDetailAuthorText;
	QLabel* m_pDetailDescriptionLabel;
	QLabel* m_pDetailDescriptionText;
	QLabel* m_pDetailLicenseLabel;
	QLabel* m_pDetailLicenseText;
	QLabel* m_pDetailTagsLabel;
	QLabel* m_pDetailTagsText;
	QLabel* m_pDetailVersionLabel;
	QLabel* m_pDetailVersionText;
	// Pattern-specific rows (hidden for Song/Drumkit)
	QLabel* m_pDetailNotesLabel;
	QLabel* m_pDetailNotesText;
	// Song-specific rows (hidden for Pattern/Drumkit)
	QLabel* m_pDetailPatternCountLabel;
	QLabel* m_pDetailPatternCountText;
	// Drumkit-specific rows (hidden for Pattern/Song)
	QLabel* m_pDetailInstrumentsLabel;
	QLabel* m_pDetailInstrumentsText;
	QLabel* m_pDetailComponentsLabel;
	QLabel* m_pDetailComponentsText;
	QLabel* m_pDetailSamplesLabel;
	QLabel* m_pDetailSamplesText;

	// Bottom bar
	QProgressBar* m_pProgressBar;
	QPushButton* m_pCancelButton;
	QPushButton* m_pDownloadButton;

	// Model / proxy
	OnlineArtifactTableModel* m_pModel;
	OnlineArtifactFilterProxy* m_pProxy;

	// Core importer
	H2Core::OnlineImporter* m_pImporter;
	bool m_bDownloading;
};

#endif // ONLINE_IMPORT_DIALOG_H
